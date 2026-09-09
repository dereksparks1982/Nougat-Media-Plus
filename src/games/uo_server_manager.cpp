#include "games/uo_server_manager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace nougat::games::uo {

namespace {

constexpr const char* kOwnerEnvironment = "NOUGAT_UO_SERVER_OWNER";

std::string getenv_string(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string(value);
}

std::string read_text_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    std::ostringstream stream;
    stream << input.rdbuf();
    return stream.str();
}

std::string canonical_path(const std::string& path) {
    std::error_code error;
    const auto value = std::filesystem::weakly_canonical(path, error);
    return error ? path : value.string();
}

bool regular_executable(const std::string& path) {
    struct stat info {};
    return !path.empty() && ::stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) && ::access(path.c_str(), X_OK) == 0;
}

bool directory_exists(const std::string& path) {
    struct stat info {};
    return !path.empty() && ::stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

bool process_exists(long pid) {
    if (pid <= 1) return false;
    if (::kill(static_cast<pid_t>(pid), 0) == 0) return true;
    return errno == EPERM;
}

bool proc_uid_matches(long pid) {
    struct stat info {};
    const std::string proc = "/proc/" + std::to_string(pid);
    return ::stat(proc.c_str(), &info) == 0 && info.st_uid == ::geteuid();
}

bool proc_token_matches(long pid, const std::string& token) {
    if (token.empty()) return false;
    const std::string environment = read_text_file("/proc/" + std::to_string(pid) + "/environ");
    const std::string expected = std::string(kOwnerEnvironment) + "=" + token;
    std::size_t start = 0;
    while (start < environment.size()) {
        const auto end = environment.find('\0', start);
        const std::string entry = environment.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (entry == expected) return true;
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return false;
}

bool proc_executable_matches(long pid, const std::string& executable) {
    if (executable.empty()) return false;
    std::array<char, 4096> buffer{};
    const std::string proc_exe = "/proc/" + std::to_string(pid) + "/exe";
    const ssize_t length = ::readlink(proc_exe.c_str(), buffer.data(), buffer.size() - 1U);
    if (length <= 0) return false;
    buffer[static_cast<std::size_t>(length)] = '\0';
    return canonical_path(buffer.data()) == canonical_path(executable);
}

bool write_all(int fd, const void* data, std::size_t bytes) {
    const auto* p = static_cast<const unsigned char*>(data);
    std::size_t done = 0;
    while (done < bytes) {
        const ssize_t amount = ::write(fd, p + done, bytes - done);
        if (amount < 0 && errno == EINTR) continue;
        if (amount <= 0) return false;
        done += static_cast<std::size_t>(amount);
    }
    return true;
}

bool read_all(int fd, void* data, std::size_t bytes) {
    auto* p = static_cast<unsigned char*>(data);
    std::size_t done = 0;
    while (done < bytes) {
        const ssize_t amount = ::read(fd, p + done, bytes - done);
        if (amount < 0 && errno == EINTR) continue;
        if (amount <= 0) return false;
        done += static_cast<std::size_t>(amount);
    }
    return true;
}

std::string owner_token() {
    std::array<unsigned char, 24> bytes{};
    const int fd = ::open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        const ssize_t got = ::read(fd, bytes.data(), bytes.size());
        (void)::close(fd);
        if (got == static_cast<ssize_t>(bytes.size())) {
            std::ostringstream out;
            out << std::hex << std::setfill('0');
            for (unsigned char byte : bytes) out << std::setw(2) << static_cast<unsigned int>(byte);
            return out.str();
        }
    }
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::to_string(static_cast<long long>(::getpid())) + "-" + std::to_string(static_cast<long long>(ticks));
}

bool spawn_detached(const ServerLaunchConfig& config, const std::string& token, long& pid_out, std::string& error) {
    int pid_pipe[2] {-1, -1};
#if defined(__linux__)
    if (::pipe2(pid_pipe, O_CLOEXEC) != 0) {
        error = "Could not create Sphere launch pipe.";
        return false;
    }
#else
    if (::pipe(pid_pipe) != 0) {
        error = "Could not create Sphere launch pipe.";
        return false;
    }
#endif

    const pid_t first = ::fork();
    if (first < 0) {
        (void)::close(pid_pipe[0]);
        (void)::close(pid_pipe[1]);
        error = "Could not fork Sphere launcher.";
        return false;
    }

    if (first == 0) {
        (void)::close(pid_pipe[0]);
        if (::setsid() < 0) _exit(120);

        const pid_t second = ::fork();
        if (second < 0) _exit(121);
        if (second > 0) {
            const long child_pid = static_cast<long>(second);
            (void)write_all(pid_pipe[1], &child_pid, sizeof(child_pid));
            _exit(0);
        }

        (void)::close(pid_pipe[1]);
        if (!config.working_directory.empty() && ::chdir(config.working_directory.c_str()) != 0) _exit(122);
        if (::setenv(kOwnerEnvironment, token.c_str(), 1) != 0) _exit(123);

        const int null_fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        if (null_fd >= 0) {
            (void)::dup2(null_fd, STDIN_FILENO);
            if (null_fd != STDIN_FILENO) (void)::close(null_fd);
        }

        if (!config.log_file.empty()) {
            const int log_fd = ::open(config.log_file.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0600);
            if (log_fd < 0) _exit(124);
            (void)::dup2(log_fd, STDOUT_FILENO);
            (void)::dup2(log_fd, STDERR_FILENO);
            if (log_fd != STDOUT_FILENO && log_fd != STDERR_FILENO) (void)::close(log_fd);
        }

        std::vector<std::string> storage;
        storage.reserve(config.arguments.size() + 1U);
        storage.push_back(config.executable);
        storage.insert(storage.end(), config.arguments.begin(), config.arguments.end());
        std::vector<char*> argv;
        argv.reserve(storage.size() + 1U);
        for (std::string& value : storage) argv.push_back(value.data());
        argv.push_back(nullptr);
        ::execv(config.executable.c_str(), argv.data());
        _exit(127);
    }

    (void)::close(pid_pipe[1]);
    long launched_pid = -1;
    const bool got_pid = read_all(pid_pipe[0], &launched_pid, sizeof(launched_pid));
    (void)::close(pid_pipe[0]);

    int status = 0;
    while (::waitpid(first, &status, 0) < 0 && errno == EINTR) {}
    if (!got_pid || launched_pid <= 1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        error = "Sphere detached launcher did not produce a valid server process.";
        return false;
    }
    pid_out = launched_pid;
    return true;
}

}  // namespace

ServerManager::ServerManager(std::uint16_t port) : port_(port) {}

std::string ServerManager::state_directory() const {
    std::string base = getenv_string("XDG_DATA_HOME");
    if (base.empty()) {
        const std::string home = getenv_string("HOME");
        base = (home.empty() ? std::string{"."} : home) + "/.local/share";
    }
    return base + "/nougat-media-plus/servers/ultima-online";
}

std::string ServerManager::state_file() const { return state_directory() + "/owner.state"; }
std::string ServerManager::default_server_root() const { return state_directory() + "/shard"; }

std::string ServerManager::resolve_server_executable() const {
    const std::string override_path = getenv_string("NOUGAT_SPHERE_SERVER");
    if (regular_executable(override_path)) return override_path;
    const std::string managed = default_server_root() + "/SphereSvrX64_nightly";
    if (regular_executable(managed)) return managed;
    return {};
}

bool ServerManager::ready() const {
    const int socket_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (socket_fd < 0) return false;
    const int flags = ::fcntl(socket_fd, F_GETFL, 0);
    if (flags >= 0) (void)::fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    const int result = ::connect(socket_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    if (result == 0) {
        (void)::close(socket_fd);
        return true;
    }
    if (errno != EINPROGRESS) {
        (void)::close(socket_fd);
        return false;
    }

    fd_set write_set;
    FD_ZERO(&write_set);
    FD_SET(socket_fd, &write_set);
    timeval timeout{};
    timeout.tv_usec = 150000;
    const int selected = ::select(socket_fd + 1, nullptr, &write_set, nullptr, &timeout);
    if (selected <= 0) {
        (void)::close(socket_fd);
        return false;
    }
    int socket_error = 0;
    socklen_t length = sizeof(socket_error);
    const bool ok = ::getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &length) == 0 && socket_error == 0;
    (void)::close(socket_fd);
    return ok;
}

bool ServerManager::owned_process_matches(const ServerState& state) const {
    if (state.pid <= 1 || state.token.empty() || state.executable.empty()) return false;
    return process_exists(state.pid) && proc_uid_matches(state.pid) && proc_token_matches(state.pid, state.token) &&
           proc_executable_matches(state.pid, state.executable);
}

bool ServerManager::load_state(ServerState& state) const {
    std::ifstream input(state_file());
    if (!input) return false;
    ServerState loaded;
    if (!std::getline(input, loaded.token) || !std::getline(input, loaded.executable)) return false;
    std::string pid_line;
    if (!std::getline(input, pid_line)) return false;
    try {
        loaded.pid = std::stol(pid_line);
    } catch (...) {
        return false;
    }
    state = loaded;
    return true;
}

bool ServerManager::save_state(const ServerState& state) const {
    std::error_code error;
    std::filesystem::create_directories(state_directory(), error);
    if (error) return false;
    const std::string temporary = state_file() + ".tmp";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) return false;
        output << state.token << '\n' << canonical_path(state.executable) << '\n' << state.pid << '\n';
        if (!output) return false;
    }
    (void)::chmod(temporary.c_str(), 0600);
    std::filesystem::rename(temporary, state_file(), error);
    if (error) {
        std::filesystem::remove(temporary);
        return false;
    }
    return true;
}

bool ServerManager::clear_stale_state() const {
    ServerState state;
    if (!load_state(state)) return true;
    if (owned_process_matches(state)) return false;
    std::error_code error;
    std::filesystem::remove(state_file(), error);
    return !error;
}

ServerStartResult ServerManager::start_or_reuse(const ServerLaunchConfig& input) const {
    ServerStartResult result;
    ServerLaunchConfig config = input;
    if (config.executable.empty()) config.executable = resolve_server_executable();
    if (config.executable.empty() || !regular_executable(config.executable)) {
        result.error = "Managed Sphere executable is unavailable.";
        return result;
    }
    config.executable = canonical_path(config.executable);
    if (config.working_directory.empty()) config.working_directory = std::filesystem::path(config.executable).parent_path().string();
    if (!directory_exists(config.working_directory)) {
        result.error = "Managed Sphere working directory is unavailable.";
        return result;
    }
    if (config.log_file.empty()) config.log_file = state_directory() + "/sphere.log";
    std::error_code fs_error;
    std::filesystem::create_directories(std::filesystem::path(config.log_file).parent_path(), fs_error);
    if (fs_error) {
        result.error = "Could not create Sphere log directory.";
        return result;
    }

    ServerState existing;
    if (load_state(existing)) {
        if (owned_process_matches(existing)) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(0, config.readiness_timeout_ms));
            while (std::chrono::steady_clock::now() <= deadline) {
                if (ready()) {
                    result.ok = true;
                    result.reused = true;
                    result.pid = existing.pid;
                    return result;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            result.error = "Nougat-owned Sphere process exists but did not become ready.";
            return result;
        }
        (void)clear_stale_state();
    }

    if (ready()) {
        result.error = "Port " + std::to_string(port_) + " is already in use by a process Nougat does not own.";
        return result;
    }

    const std::string token = owner_token();
    long pid = -1;
    if (!spawn_detached(config, token, pid, result.error)) return result;

    const ServerState launched{pid, token, config.executable};
    if (!save_state(launched)) {
        if (owned_process_matches(launched)) (void)::kill(static_cast<pid_t>(pid), SIGTERM);
        result.error = "Could not persist Sphere ownership record.";
        return result;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(0, config.readiness_timeout_ms));
    while (std::chrono::steady_clock::now() <= deadline) {
        if (owned_process_matches(launched) && ready()) {
            result.ok = true;
            result.pid = pid;
            return result;
        }
        if (!process_exists(pid)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (owned_process_matches(launched)) (void)::kill(static_cast<pid_t>(pid), SIGTERM);
    std::error_code remove_error;
    std::filesystem::remove(state_file(), remove_error);
    result.error = "Sphere failed to become ready on localhost port " + std::to_string(port_) + ".";
    return result;
}

bool ServerManager::stop_owned(std::string& error, int timeout_ms) const {
    ServerState state;
    if (!load_state(state)) return true;
    if (!owned_process_matches(state)) {
        (void)clear_stale_state();
        error = "Sphere ownership record was stale; no process was killed.";
        return true;
    }
    if (::kill(static_cast<pid_t>(state.pid), SIGTERM) != 0) {
        error = "Could not signal the verified Nougat-owned Sphere process.";
        return false;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(0, timeout_ms));
    while (std::chrono::steady_clock::now() <= deadline) {
        if (!process_exists(state.pid)) {
            std::error_code remove_error;
            std::filesystem::remove(state_file(), remove_error);
            return !remove_error;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    error = "Verified Nougat-owned Sphere did not exit after SIGTERM.";
    return false;
}

}  // namespace nougat::games::uo
