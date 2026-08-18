#include "media_server_manager.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace reddmedia {
namespace {

long long monotonic_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

bool regular_executable(const std::string& path) {
    struct stat info {};
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) && access(path.c_str(), X_OK) == 0;
}

void ensure_directory(const std::string& path) {
    if (path.empty()) return;
    std::string current;
    if (path.front() == '/') current = "/";
    std::size_t start = path.front() == '/' ? 1U : 0U;
    while (start <= path.size()) {
        const std::size_t slash = path.find('/', start);
        const std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (!part.empty()) {
            if (current.size() > 1 && current.back() != '/') current += '/';
            current += part;
            if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) return;
        }
        if (slash == std::string::npos) break;
        start = slash + 1;
    }
}

std::string parent_directory(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? "." : path.substr(0, slash);
}

} // namespace

MediaServerManager::MediaServerManager() {
    resolve_paths();
}

void MediaServerManager::resolve_paths() {
    char executable[PATH_MAX] {};
    const ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
    application_dir_ = length > 0 ? parent_directory(std::string(executable, static_cast<std::size_t>(length))) : ".";
    runtime_path_ = application_dir_ + "/components/jellyfin/runtime/jellyfin/jellyfin";

    const char* home = std::getenv("HOME");
    const std::string home_path = home ? home : ".";
    data_path_ = home_path + "/.local/share/reddmedia/server/data";
    config_path_ = home_path + "/.config/reddmedia/server";
    cache_path_ = home_path + "/.cache/reddmedia/server";
    log_path_ = home_path + "/.local/share/reddmedia/server/log";
}

bool MediaServerManager::health_ready() const {
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) return false;

    const int original_flags = fcntl(socket_fd, F_GETFL, 0);
    if (original_flags < 0 || fcntl(socket_fd, F_SETFL, original_flags | O_NONBLOCK) != 0) {
        close(socket_fd);
        return false;
    }

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(8096);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    int result = connect(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (result != 0 && errno == EINPROGRESS) {
        fd_set write_set;
        FD_ZERO(&write_set);
        FD_SET(socket_fd, &write_set);
        timeval timeout {0, 150000};
        result = select(socket_fd + 1, nullptr, &write_set, nullptr, &timeout);
        int socket_error = 0;
        socklen_t error_length = sizeof(socket_error);
        if (result <= 0 || getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &error_length) != 0 || socket_error != 0) {
            close(socket_fd);
            return false;
        }
    } else if (result != 0) {
        close(socket_fd);
        return false;
    }

    fcntl(socket_fd, F_SETFL, original_flags);
    const char request[] = "GET /health HTTP/1.1\r\nHost: 127.0.0.1:8096\r\nConnection: close\r\n\r\n";
    if (send(socket_fd, request, sizeof(request) - 1, MSG_NOSIGNAL) < 0) {
        close(socket_fd);
        return false;
    }

    timeval timeout {0, 250000};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    char response[160] {};
    const ssize_t received = recv(socket_fd, response, sizeof(response) - 1, 0);
    close(socket_fd);
    if (received <= 0) return false;
    const std::string status(response, static_cast<std::size_t>(received));
    return status.find("HTTP/1.1 200") != std::string::npos || status.find("HTTP/1.0 200") != std::string::npos;
}

bool MediaServerManager::launch_runtime() {
    if (!regular_executable(runtime_path_)) {
        state_ = MediaServerState::RuntimeMissing;
        return false;
    }

    ensure_directory(data_path_);
    ensure_directory(config_path_);
    ensure_directory(cache_path_);
    ensure_directory(log_path_);

    const pid_t child = fork();
    if (child < 0) {
        state_ = MediaServerState::Fault;
        return false;
    }
    if (child == 0) {
        setsid();
        const std::string log_file = log_path_ + "/jellyfin.log";
        const int log_fd = open(log_file.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
        const int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd >= 0) dup2(null_fd, STDIN_FILENO);
        if (log_fd >= 0) {
            dup2(log_fd, STDOUT_FILENO);
            dup2(log_fd, STDERR_FILENO);
        }
        execl(runtime_path_.c_str(), runtime_path_.c_str(),
              "--datadir", data_path_.c_str(),
              "--configdir", config_path_.c_str(),
              "--cachedir", cache_path_.c_str(),
              "--logdir", log_path_.c_str(),
              "--nowebclient",
              "--ffmpeg", "/usr/bin/ffmpeg",
              "--service",
              "--package-name", "ReddMedia integrated Jellyfin",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    owned_pid_ = child;
    state_ = MediaServerState::Starting;
    return true;
}

void MediaServerManager::start() {
    if (health_ready()) {
        state_ = MediaServerState::Ready;
        return;
    }
    launch_runtime();
}

bool MediaServerManager::poll() {
    const long long now = monotonic_ms();
    if (now - last_poll_ms_ < 2000) return false;
    last_poll_ms_ = now;
    const MediaServerState previous = state_;

    if (health_ready()) {
        state_ = MediaServerState::Ready;
        return state_ != previous;
    }

    if (owned_pid_ > 0) {
        int status = 0;
        const pid_t result = waitpid(owned_pid_, &status, WNOHANG);
        if (result == 0) {
            state_ = MediaServerState::Starting;
            return state_ != previous;
        }
        owned_pid_ = -1;
        state_ = MediaServerState::Fault;
        next_restart_ms_ = now + 5000;
    }

    if (state_ == MediaServerState::RuntimeMissing) return state_ != previous;
    if (owned_pid_ < 0 && now >= next_restart_ms_) launch_runtime();
    return state_ != previous;
}

std::string MediaServerManager::status_label() const {
    switch (state_) {
    case MediaServerState::Starting: return "Server: Starting";
    case MediaServerState::Ready: return "Server: Ready";
    case MediaServerState::Fault: return "Server: Fault";
    case MediaServerState::RuntimeMissing: return "Server: Runtime Missing";
    case MediaServerState::Stopped: return "Server: Stopped";
    }
    return "Server: Fault";
}

MediaServerState MediaServerManager::state() const {
    return state_;
}

} // namespace reddmedia
