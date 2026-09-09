#include "games/uo_playable_runtime.hpp"

#include "games/uo_client.hpp"
#include "games/uo_server_manager.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace nougat::games::uo {

namespace {

std::string home_dir() {
    const char* value = std::getenv("HOME");
    return value == nullptr || *value == '\0' ? std::string{"."} : std::string(value);
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

bool spawn_t2a_client_detached(const std::vector<std::string>& command,
                               const std::string& working_directory,
                               const std::string& wine_prefix,
                               const std::string& log_file,
                               long& pid_out,
                               std::string& error) {
    if (command.size() != 2U || command.front().empty() || command[1].empty()) {
        error = "T2A Wine launch command is invalid.";
        return false;
    }

    int pid_pipe[2] {-1, -1};
#if defined(__linux__)
    if (::pipe2(pid_pipe, O_CLOEXEC) != 0) {
        error = "Could not create T2A client launch pipe.";
        return false;
    }
#else
    if (::pipe(pid_pipe) != 0) {
        error = "Could not create T2A client launch pipe.";
        return false;
    }
#endif

    const pid_t first = ::fork();
    if (first < 0) {
        (void)::close(pid_pipe[0]);
        (void)::close(pid_pipe[1]);
        error = "Could not fork T2A client launcher.";
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
        if (!working_directory.empty() && ::chdir(working_directory.c_str()) != 0) _exit(122);
        if (::setenv("WINEPREFIX", wine_prefix.c_str(), 1) != 0) _exit(123);
        if (::setenv("WINEARCH", "win32", 1) != 0) _exit(124);
        if (::setenv("WINEDEBUG", "-all", 1) != 0) _exit(125);
        if (::setenv("WINEDLLOVERRIDES", "mscoree,mshtml=", 1) != 0) _exit(126);

        const int null_fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        if (null_fd >= 0) {
            (void)::dup2(null_fd, STDIN_FILENO);
            if (null_fd != STDIN_FILENO) (void)::close(null_fd);
        }

        if (!log_file.empty()) {
            const int log_fd = ::open(log_file.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0600);
            if (log_fd < 0) _exit(127);
            (void)::dup2(log_fd, STDOUT_FILENO);
            (void)::dup2(log_fd, STDERR_FILENO);
            if (log_fd != STDOUT_FILENO && log_fd != STDERR_FILENO) (void)::close(log_fd);
        }

        std::vector<std::string> storage = command;
        std::vector<char*> argv;
        argv.reserve(storage.size() + 1U);
        for (std::string& value : storage) argv.push_back(value.data());
        argv.push_back(nullptr);
        ::execv(storage.front().c_str(), argv.data());
        _exit(128);
    }

    (void)::close(pid_pipe[1]);
    long launched_pid = -1;
    const bool got_pid = read_all(pid_pipe[0], &launched_pid, sizeof(launched_pid));
    (void)::close(pid_pipe[0]);

    int status = 0;
    while (::waitpid(first, &status, 0) < 0 && errno == EINTR) {}
    if (!got_pid || launched_pid <= 1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        error = "T2A detached Wine launcher did not produce a valid client process.";
        return false;
    }

    pid_out = launched_pid;
    return true;
}

}  // namespace

PlayableLaunchResult launch_t2a_playable() {
    PlayableLaunchResult result;

    const std::string data_path = resolve_uo_data_path();
    std::string validation_error;
    if (!validate_t2a_data_path(data_path, validation_error)) {
        result.error = validation_error;
        return result;
    }

    const std::string wine = resolve_wine_executable();
    if (wine.empty()) {
        result.error = "Wine is unavailable. Install the Nougat T2A Wine runtime repair first.";
        return result;
    }
    struct stat wine_info {};
    if (::stat(wine.c_str(), &wine_info) != 0 || !S_ISREG(wine_info.st_mode) || ::access(wine.c_str(), X_OK) != 0) {
        result.error = "Resolved Wine launcher is not executable.";
        return result;
    }

    const std::string client = resolve_t2a_client_executable();
    struct stat client_info {};
    if (::stat(client.c_str(), &client_info) != 0 || !S_ISREG(client_info.st_mode)) {
        result.error = "The actual T2A 1.25.35 client.exe is unavailable.";
        return result;
    }

    std::string login_error;
    if (!prepare_t2a_login_config(data_path, "127.0.0.1", 2593, login_error)) {
        result.error = login_error;
        return result;
    }

    ServerManager server(2593);
    const std::string sphere = server.resolve_server_executable();
    if (sphere.empty()) {
        result.error = "Managed SphereServer X runtime is unavailable.";
        return result;
    }

    ServerLaunchConfig server_config;
    server_config.executable = sphere;
    server_config.working_directory = server.default_server_root();
    server_config.log_file = server.state_directory() + "/sphere.log";
    server_config.readiness_timeout_ms = 15000;

    const ServerStartResult started = server.start_or_reuse(server_config);
    if (!started.ok) {
        result.error = started.error.empty() ? "SphereServer X did not become ready." : started.error;
        return result;
    }

    result.server_reused = started.reused;
    result.server_pid = started.pid;

    const std::filesystem::path state_root =
        std::filesystem::path(home_dir()) / ".local/share/nougat-play-portal/games/ultima-online";
    std::error_code fs_error;
    std::filesystem::create_directories(state_root, fs_error);
    if (fs_error) {
        if (!started.reused) {
            std::string stop_error;
            (void)server.stop_owned(stop_error);
        }
        result.error = "Could not create the managed Ultima Online client state directory.";
        return result;
    }

    ClientLaunchConfig client_config;
    client_config.wine_executable = wine;
    client_config.client_executable = client;
    client_config.uo_data_path = data_path;
    client_config.wine_prefix = resolve_t2a_wine_prefix();
    client_config.host = "127.0.0.1";
    client_config.port = 2593;

    std::vector<std::string> command;
    try {
        command = build_t2a_wine_command(client_config);
    } catch (const std::exception& ex) {
        if (!started.reused) {
            std::string stop_error;
            (void)server.stop_owned(stop_error);
        }
        result.error = std::string("T2A client command preparation failed: ") + ex.what();
        return result;
    }

    const std::string log_file = (state_root / "t2a-wine-launch.log").string();
    std::string client_error;
    long client_pid = -1;
    if (!spawn_t2a_client_detached(command, data_path, client_config.wine_prefix,
                                   log_file, client_pid, client_error)) {
        if (!started.reused) {
            std::string stop_error;
            (void)server.stop_owned(stop_error);
        }
        result.error = client_error;
        return result;
    }

    result.client_pid = client_pid;
    result.ok = true;
    return result;
}

}  // namespace nougat::games::uo
