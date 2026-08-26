#include "plugin_process_host.hpp"

#include "platform/nougat_paths.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

namespace nougat::plugins {
namespace {

bool ensure_directory(const fs::path& path, std::string& error) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        error = "Unable to create plugin runtime directory " + path.string() + ": " + ec.message();
        return false;
    }
    return true;
}

void set_child_environment(const PluginManifest& plugin,
                           unsigned long parent_xid,
                           int width,
                           int height,
                           const fs::path& data_dir,
                           const fs::path& config_dir,
                           const fs::path& cache_dir,
                           const fs::path& state_dir) {
    const std::string parent = std::to_string(parent_xid);
    const std::string width_text = std::to_string(width);
    const std::string height_text = std::to_string(height);
    (void)::setenv("NOUGAT_PLUGIN_API", "1", 1);
    (void)::setenv("NOUGAT_PLUGIN_ID", plugin.id.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_ROOT", plugin.root.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_DATA_DIR", data_dir.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_CONFIG_DIR", config_dir.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_CACHE_DIR", cache_dir.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_STATE_DIR", state_dir.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_PARENT_XID", parent.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_WIDTH", width_text.c_str(), 1);
    (void)::setenv("NOUGAT_PLUGIN_HEIGHT", height_text.c_str(), 1);
}

void signal_plugin(pid_t process_group, pid_t child, int signal_value) {
    if (process_group > 1) {
        if (::kill(-process_group, signal_value) == 0 || errno == ESRCH) return;
    }
    if (child > 1) (void)::kill(child, signal_value);
}

} // namespace

PluginProcessHost::~PluginProcessHost() {
    stop();
}

bool PluginProcessHost::start(const PluginManifest& plugin,
                              unsigned long parent_xid,
                              int width,
                              int height,
                              std::string& error) {
    stop();
    error.clear();

    if (plugin.runtime_kind != "x11-process") {
        error = "Plugin runtime is not supported by this host: " + plugin.runtime_kind;
        return false;
    }
    if (parent_xid == 0UL || width <= 0 || height <= 0) {
        error = "Plugin host surface is not ready";
        return false;
    }

    std::error_code ec;
    if (!fs::is_regular_file(plugin.entrypoint, ec) || ec || ::access(plugin.entrypoint.c_str(), X_OK) != 0) {
        error = "Plugin entrypoint is unavailable: " + plugin.entrypoint.string();
        return false;
    }

    const auto& layout = nougat::paths::layout();
    const fs::path data_dir = layout.data / "plugin-data" / plugin.id;
    const fs::path config_dir = layout.config / "plugins" / plugin.id;
    const fs::path cache_dir = layout.cache / "plugins" / plugin.id;
    const fs::path state_dir = layout.state / "plugins" / plugin.id;
    if (!ensure_directory(data_dir, error) ||
        !ensure_directory(config_dir, error) ||
        !ensure_directory(cache_dir, error) ||
        !ensure_directory(state_dir, error)) {
        return false;
    }

    const pid_t child = ::fork();
    if (child < 0) {
        error = "Unable to launch plugin " + plugin.id + ": fork failed";
        return false;
    }

    if (child == 0) {
        (void)::setpgid(0, 0);
        const fs::path log_path = state_dir / "plugin.log";
        const int log_fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (log_fd >= 0) {
            (void)::dup2(log_fd, STDOUT_FILENO);
            (void)::dup2(log_fd, STDERR_FILENO);
            if (log_fd > STDERR_FILENO) (void)::close(log_fd);
        }
        set_child_environment(plugin, parent_xid, width, height,
                              data_dir, config_dir, cache_dir, state_dir);
        const std::string executable = plugin.entrypoint.string();
        ::execl(executable.c_str(), executable.c_str(), "--nougat-plugin", static_cast<char*>(nullptr));
        _exit(127);
    }

    child_ = child;
    process_group_ = child;
    plugin_id_ = plugin.id;
    if (::setpgid(child_, child_) != 0 && errno != EACCES && errno != ESRCH) {
        process_group_ = -1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    std::string status;
    if (!poll(&status)) {
        error = status.empty() ? "Plugin exited immediately: " + plugin.id : status;
        return false;
    }
    return true;
}

bool PluginProcessHost::poll(std::string* status) {
    if (child_ <= 1) return false;
    int wait_status = 0;
    const pid_t result = ::waitpid(child_, &wait_status, WNOHANG);
    if (result == 0) return true;
    if (result < 0) {
        if (errno == EINTR) return true;
        if (status != nullptr) *status = "Plugin process could not be queried: " + plugin_id_;
    } else if (status != nullptr) {
        if (WIFEXITED(wait_status)) {
            *status = "Plugin " + plugin_id_ + " exited with status " +
                      std::to_string(WEXITSTATUS(wait_status));
        } else if (WIFSIGNALED(wait_status)) {
            *status = "Plugin " + plugin_id_ + " stopped by signal " +
                      std::to_string(WTERMSIG(wait_status));
        } else {
            *status = "Plugin " + plugin_id_ + " stopped";
        }
    }
    child_ = -1;
    process_group_ = -1;
    plugin_id_.clear();
    return false;
}

void PluginProcessHost::stop() {
    if (child_ <= 1) {
        child_ = -1;
        process_group_ = -1;
        plugin_id_.clear();
        return;
    }

    signal_plugin(process_group_, child_, SIGTERM);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        int status = 0;
        const pid_t result = ::waitpid(child_, &status, WNOHANG);
        if (result == child_ || (result < 0 && errno == ECHILD)) {
            child_ = -1;
            process_group_ = -1;
            plugin_id_.clear();
            return;
        }
        if (result < 0 && errno != EINTR) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    signal_plugin(process_group_, child_, SIGKILL);
    int status = 0;
    while (::waitpid(child_, &status, 0) < 0 && errno == EINTR) {}
    child_ = -1;
    process_group_ = -1;
    plugin_id_.clear();
}

} // namespace nougat::plugins
