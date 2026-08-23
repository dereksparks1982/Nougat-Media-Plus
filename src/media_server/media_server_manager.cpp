#include "media_server_manager.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <limits.h>
#include <limits>
#include <netinet/in.h>
#include <signal.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace reddmedia {
namespace {

constexpr const char* kOwnerEnvironment = "NOUGAT_MEDIA_SERVER_OWNER";

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

std::string read_link(const std::string& path) {
    char buffer[PATH_MAX] {};
    const ssize_t length = readlink(path.c_str(), buffer, sizeof(buffer) - 1);
    return length > 0 ? std::string(buffer, static_cast<std::size_t>(length)) : std::string();
}

std::string read_binary_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

bool numeric_name(const char* text) {
    if (!text || !*text) return false;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        if (!std::isdigit(*p)) return false;
    }
    return true;
}

bool null_list_contains(const std::string& blob, const std::string& exact) {
    std::size_t begin = 0;
    while (begin < blob.size()) {
        std::size_t end = blob.find('\0', begin);
        if (end == std::string::npos) end = blob.size();
        if (blob.compare(begin, end - begin, exact) == 0) return true;
        begin = end + 1;
    }
    return false;
}

bool null_list_contains_value(const std::string& blob, const std::string& value) {
    std::size_t begin = 0;
    while (begin < blob.size()) {
        std::size_t end = blob.find('\0', begin);
        if (end == std::string::npos) end = blob.size();
        if (blob.compare(begin, end - begin, value) == 0) return true;
        begin = end + 1;
    }
    return false;
}

bool process_is_zombie(pid_t pid) {
    const std::string stat_line = read_binary_file("/proc/" + std::to_string(pid) + "/stat");
    if (stat_line.empty()) return false;
    const std::size_t close = stat_line.rfind(')');
    if (close == std::string::npos || close + 2 >= stat_line.size()) return false;
    return stat_line[close + 2] == 'Z';
}

} // namespace

MediaServerManager::MediaServerManager() {
    resolve_paths();
    persistent_enabled_ = load_enabled_state();
    refresh();
}

// Deliberately does NOT call stop(). The server is a persistent Nougat-owned
// background service once the owner presses Start Server.
MediaServerManager::~MediaServerManager() = default;

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
    ownership_path_ = home_path + "/.local/share/reddmedia/server/nougat-owned.pid";
    enabled_path_ = home_path + "/.config/reddmedia/server/persistent-enabled";
}

bool MediaServerManager::same_user_process(pid_t pid) const {
    if (pid <= 1) return false;
    struct stat info {};
    const std::string proc_path = "/proc/" + std::to_string(pid);
    return stat(proc_path.c_str(), &info) == 0 && info.st_uid == geteuid();
}

bool MediaServerManager::process_alive(pid_t pid) const {
    if (pid <= 1 || process_is_zombie(pid)) return false;
    if (kill(pid, 0) == 0) return true;
    return errno == EPERM;
}

bool MediaServerManager::process_matches_runtime(pid_t pid) const {
    if (!process_alive(pid) || !same_user_process(pid)) return false;
    const std::string exe = read_link("/proc/" + std::to_string(pid) + "/exe");
    if (!exe.empty() && exe == runtime_path_) return true;
    const std::string cmdline = read_binary_file("/proc/" + std::to_string(pid) + "/cmdline");
    return !cmdline.empty() && null_list_contains_value(cmdline, runtime_path_);
}

bool MediaServerManager::process_matches_nougat_signature(pid_t pid) const {
    if (!process_alive(pid) || !same_user_process(pid)) return false;
    const std::string cmdline = read_binary_file("/proc/" + std::to_string(pid) + "/cmdline");
    if (cmdline.empty()) return false;
    const bool runtime = process_matches_runtime(pid);
    const bool data = null_list_contains_value(cmdline, data_path_);
    const bool config = null_list_contains_value(cmdline, config_path_);
    const bool package = null_list_contains_value(cmdline, "Nougat Media Suite integrated Jellyfin");
    return runtime && data && config && package;
}

bool MediaServerManager::process_has_owner_token(pid_t pid, const std::string& token) const {
    if (token.empty() || !process_alive(pid) || !same_user_process(pid)) return false;
    const std::string environment = read_binary_file("/proc/" + std::to_string(pid) + "/environ");
    if (environment.empty()) return false;
    return null_list_contains(environment, std::string(kOwnerEnvironment) + "=" + token);
}

std::vector<pid_t> MediaServerManager::owned_processes(const std::string& token, pid_t recorded_pid) const {
    std::vector<pid_t> result;
    DIR* proc = opendir("/proc");
    if (!proc) {
        if (recorded_pid > 1 && process_matches_runtime(recorded_pid)) result.push_back(recorded_pid);
        return result;
    }

    while (dirent* entry = readdir(proc)) {
        if (!numeric_name(entry->d_name)) continue;
        errno = 0;
        char* end = nullptr;
        const long value = std::strtol(entry->d_name, &end, 10);
        if (errno != 0 || !end || *end != '\0' || value <= 1 ||
            value > static_cast<long>(std::numeric_limits<pid_t>::max())) continue;
        const pid_t pid = static_cast<pid_t>(value);
        const bool token_match = process_has_owner_token(pid, token);
        const bool signature_match = process_matches_nougat_signature(pid);
        const bool recorded_runtime = pid == recorded_pid && process_matches_runtime(pid);
        if (token_match || signature_match || recorded_runtime) result.push_back(pid);
    }
    closedir(proc);
    return result;
}

std::string MediaServerManager::make_owner_token() const {
    unsigned char bytes[16] {};
    const int random_fd = open("/dev/urandom", O_RDONLY);
    ssize_t got = -1;
    if (random_fd >= 0) {
        got = read(random_fd, bytes, sizeof(bytes));
        close(random_fd);
    }
    if (got == static_cast<ssize_t>(sizeof(bytes))) {
        static constexpr char hex[] = "0123456789abcdef";
        std::string token;
        token.reserve(sizeof(bytes) * 2);
        for (unsigned char byte : bytes) {
            token.push_back(hex[(byte >> 4) & 0x0f]);
            token.push_back(hex[byte & 0x0f]);
        }
        return token;
    }
    return std::to_string(static_cast<long long>(getpid())) + "-" + std::to_string(monotonic_ms());
}

void MediaServerManager::persist_owned_record(pid_t pid, const std::string& token) const {
    ensure_directory(parent_directory(ownership_path_));
    const std::string temporary = ownership_path_ + ".tmp";
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) return;
    out << pid << '\n' << runtime_path_ << '\n' << token << '\n';
    out.close();
    chmod(temporary.c_str(), 0600);
    rename(temporary.c_str(), ownership_path_.c_str());
}

void MediaServerManager::clear_owned_pid() const {
    unlink(ownership_path_.c_str());
}

bool MediaServerManager::load_owned_record(pid_t& pid, std::string& token) const {
    pid = -1;
    token.clear();
    std::ifstream in(ownership_path_);
    long long value = -1;
    std::string recorded_runtime;
    if (!(in >> value)) return false;
    in.ignore(4096, '\n');
    std::getline(in, recorded_runtime);
    std::getline(in, token); // empty for legacy v0.0.33 ownership files
    if (value <= 1 || value > static_cast<long long>(std::numeric_limits<pid_t>::max())) return false;
    if (!recorded_runtime.empty() && recorded_runtime != runtime_path_) return false;
    pid = static_cast<pid_t>(value);
    return true;
}

void MediaServerManager::persist_enabled(bool enabled) const {
    ensure_directory(parent_directory(enabled_path_));
    const std::string temporary = enabled_path_ + ".tmp";
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) return;
    out << (enabled ? "1\n" : "0\n");
    out.close();
    chmod(temporary.c_str(), 0600);
    rename(temporary.c_str(), enabled_path_.c_str());
}

bool MediaServerManager::load_enabled_state() const {
    std::ifstream in(enabled_path_);
    int enabled = 1; // upgrade compatibility: v0.0.32 automatically ran the server.
    if (in >> enabled) return enabled != 0;
    return true;
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

bool MediaServerManager::adopt_owned_server() {
    pid_t recorded_pid = -1;
    std::string token;
    if (!load_owned_record(recorded_pid, token)) return false;

    const std::vector<pid_t> processes = owned_processes(token, recorded_pid);
    if (processes.empty()) {
        clear_owned_pid();
        owned_pid_ = -1;
        owned_token_.clear();
        return false;
    }

    owned_pid_ = process_alive(recorded_pid) ? recorded_pid : processes.front();
    owned_token_ = token;
    state_ = health_ready() ? MediaServerState::Ready : MediaServerState::Starting;
    return true;
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

    const std::string token = make_owner_token();
    const pid_t child = fork();
    if (child < 0) {
        state_ = MediaServerState::Fault;
        return false;
    }
    if (child == 0) {
        if (setsid() < 0) _exit(126);
        setenv(kOwnerEnvironment, token.c_str(), 1);
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
              "--package-name", "Nougat Media Suite integrated Jellyfin",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    owned_pid_ = child;
    owned_token_ = token;
    persist_owned_record(child, token);
    state_ = MediaServerState::Starting;
    return true;
}

void MediaServerManager::start() {
    persistent_enabled_ = true;
    persist_enabled(true);

    if (adopt_owned_server()) return;

    // A healthy server not carrying Nougat ownership metadata is deliberately
    // not claimed or killed. This preserves independently started Jellyfin.
    if (health_ready()) {
        state_ = MediaServerState::Ready;
        owned_pid_ = -1;
        owned_token_.clear();
        return;
    }
    launch_runtime();
}

void MediaServerManager::stop() {
    persistent_enabled_ = false;
    persist_enabled(false);

    pid_t recorded_pid = owned_pid_;
    std::string token = owned_token_;
    pid_t file_pid = -1;
    std::string file_token;
    if (load_owned_record(file_pid, file_token)) {
        if (recorded_pid <= 1) recorded_pid = file_pid;
        if (token.empty()) token = file_token;
    }

    std::vector<pid_t> processes = owned_processes(token, recorded_pid);
    if (processes.empty()) {
        clear_owned_pid();
        owned_pid_ = -1;
        owned_token_.clear();
        state_ = health_ready() ? MediaServerState::Ready : MediaServerState::Stopped;
        return;
    }

    // Every process selected here either inherited Nougat's unguessable owner
    // token or matches Nougat's exact runtime + data/config/package signature.
    // Never kill by executable name alone.
    for (pid_t pid : processes) kill(pid, SIGTERM);

    bool stopped = false;
    for (int attempt = 0; attempt < 120; ++attempt) {
        processes = owned_processes(token, recorded_pid);
        if (processes.empty() && !health_ready()) {
            stopped = true;
            break;
        }
        if (attempt > 0 && attempt % 20 == 0) {
            for (pid_t pid : processes) kill(pid, SIGTERM);
        }
        usleep(50000);
    }

    if (!stopped) {
        processes = owned_processes(token, recorded_pid);
        for (pid_t pid : processes) kill(pid, SIGKILL);
        for (int attempt = 0; attempt < 100; ++attempt) {
            processes = owned_processes(token, recorded_pid);
            if (processes.empty() && !health_ready()) {
                stopped = true;
                break;
            }
            if (attempt > 0 && attempt % 20 == 0) {
                for (pid_t pid : processes) kill(pid, SIGKILL);
            }
            usleep(50000);
        }
    }

    clear_owned_pid();
    owned_pid_ = -1;
    owned_token_.clear();
    if (stopped) state_ = MediaServerState::Stopped;
    else if (owned_processes(token, recorded_pid).empty() && health_ready()) state_ = MediaServerState::Ready;
    else state_ = MediaServerState::Fault;
}

void MediaServerManager::refresh() {
    if (adopt_owned_server()) return;

    const bool healthy = health_ready();
    if (healthy) {
        state_ = MediaServerState::Ready;
        owned_pid_ = -1;
        owned_token_.clear();
        return;
    }

    owned_pid_ = -1;
    owned_token_.clear();
    if (!regular_executable(runtime_path_)) state_ = MediaServerState::RuntimeMissing;
    else state_ = MediaServerState::Stopped;
}

bool MediaServerManager::poll() {
    const long long now = monotonic_ms();
    if (now - last_poll_ms_ < 2000) return false;
    last_poll_ms_ = now;
    const MediaServerState previous = state_;

    if (!adopt_owned_server()) {
        if (health_ready()) state_ = MediaServerState::Ready; // independent server, not owned
        else if (!regular_executable(runtime_path_)) state_ = MediaServerState::RuntimeMissing;
        else state_ = MediaServerState::Stopped;
    }
    return state_ != previous;
}

std::string MediaServerManager::status_label() const {
    switch (state_) {
    case MediaServerState::Starting: return owns_server() ? "Server: Starting (background)" : "Server: Starting";
    case MediaServerState::Ready: return owns_server() ? "Server: Ready (background)" : "Server: Ready (external)";
    case MediaServerState::Fault: return "Server: Fault";
    case MediaServerState::RuntimeMissing: return "Server: Runtime Missing";
    case MediaServerState::Stopped: return "Server: Stopped";
    }
    return "Server: Fault";
}

MediaServerState MediaServerManager::state() const { return state_; }
bool MediaServerManager::owns_server() const {
    if (owned_pid_ <= 1 && owned_token_.empty()) return false;
    return !owned_processes(owned_token_, owned_pid_).empty();
}
bool MediaServerManager::probe_health() const { return health_ready(); }
bool MediaServerManager::persistent_enabled() const { return persistent_enabled_; }
const std::string& MediaServerManager::runtime_path() const { return runtime_path_; }
const std::string& MediaServerManager::data_path() const { return data_path_; }
const std::string& MediaServerManager::config_path() const { return config_path_; }
const std::string& MediaServerManager::cache_path() const { return cache_path_; }
const std::string& MediaServerManager::log_path() const { return log_path_; }

} // namespace reddmedia
