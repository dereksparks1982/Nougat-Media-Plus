#include "world_tv_service.hpp"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <utility>
#include <vector>

namespace reddmedia {
namespace {

std::string trim_copy(std::string value) {
    while (!value.empty() &&
           (value.back() == '\n' || value.back() == '\r' ||
            value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    std::size_t first = 0;
    while (first < value.size() &&
           (value[first] == ' ' || value[first] == '\t' ||
            value[first] == '\n' || value[first] == '\r')) {
        ++first;
    }
    return value.substr(first);
}

} // namespace

WorldTvService::WorldTvService(std::string worker_path)
    : worker_path_(std::move(worker_path)) {}

std::string WorldTvService::run_worker(const std::vector<std::string>& args,
                                       int& exit_code) const {
    exit_code = -1;
    if (worker_path_.empty()) return {};

    int pipe_fd[2];
    if (::pipe(pipe_fd) != 0) return {};

    const pid_t child = ::fork();
    if (child == 0) {
        // The worker owns a process group so ffprobe/ffmpeg descendants can be
        // terminated together when a broadcaster stalls during resolution.
        ::setpgid(0, 0);
        ::dup2(pipe_fd[1], STDOUT_FILENO);
        const int null_fd = ::open("/dev/null", O_WRONLY);
        if (null_fd >= 0) {
            ::dup2(null_fd, STDERR_FILENO);
            if (null_fd > STDERR_FILENO) ::close(null_fd);
        }
        ::close(pipe_fd[0]);
        ::close(pipe_fd[1]);

        std::vector<std::string> storage;
        storage.reserve(args.size() + 2U);
        storage.push_back("python3");
        storage.push_back(worker_path_);
        for (const auto& arg : args) storage.push_back(arg);

        std::vector<char*> argv;
        argv.reserve(storage.size() + 1U);
        for (auto& item : storage) argv.push_back(item.data());
        argv.push_back(nullptr);

        ::execvp("python3", argv.data());
        _exit(127);
    }

    ::close(pipe_fd[1]);
    if (child < 0) {
        ::close(pipe_fd[0]);
        return {};
    }

    // Race-safe best effort: the child also calls setpgid itself.
    if (::setpgid(child, child) != 0 && errno != EACCES && errno != ESRCH) {
        // Continue; kill(child, ...) remains a safe fallback below.
    }
    const int old_flags = ::fcntl(pipe_fd[0], F_GETFL, 0);
    if (old_flags >= 0) ::fcntl(pipe_fd[0], F_SETFL, old_flags | O_NONBLOCK);

    std::string output;
    output.reserve(4096);
    int status = 0;
    bool reaped = false;
    bool timed_out = false;
    constexpr std::size_t kMaxOutput = 1024U * 1024U;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(70);

    const auto drain = [&]() {
        char buffer[4096];
        for (;;) {
            const ssize_t amount = ::read(pipe_fd[0], buffer, sizeof(buffer));
            if (amount > 0) {
                const std::size_t room = output.size() < kMaxOutput ? kMaxOutput - output.size() : 0U;
                if (room > 0U) output.append(buffer, std::min<std::size_t>(room, static_cast<std::size_t>(amount)));
                continue;
            }
            if (amount < 0 && errno == EINTR) continue;
            break;
        }
    };

    while (std::chrono::steady_clock::now() < deadline) {
        drain();
        const pid_t result = ::waitpid(child, &status, WNOHANG);
        if (result == child || result == -1) {
            reaped = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (!reaped) {
        timed_out = true;
        // Never let a dead/hostile CDN hold Nougat shutdown indefinitely.
        if (::kill(-child, SIGTERM) != 0) ::kill(child, SIGTERM);
        const auto term_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        while (std::chrono::steady_clock::now() < term_deadline) {
            drain();
            const pid_t result = ::waitpid(child, &status, WNOHANG);
            if (result == child || result == -1) {
                reaped = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    if (!reaped) {
        if (::kill(-child, SIGKILL) != 0) ::kill(child, SIGKILL);
        while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
        reaped = true;
    }

    drain();
    ::close(pipe_fd[0]);
    if (reaped && WIFEXITED(status)) exit_code = WEXITSTATUS(status);
    if (timed_out && output.find("ERROR_CLASS=") == std::string::npos) {
        output += "ERROR_CLASS=startup_timeout\nERROR=World TV startup exceeded the 70-second worker limit.\n";
    }
    return output;
}

std::string WorldTvService::value_for(const std::string& output,
                                      const std::string& key) {
    const std::string prefix = key + "=";
    std::size_t pos = 0;
    while (pos < output.size()) {
        const std::size_t end = output.find('\n', pos);
        const std::size_t length =
            (end == std::string::npos ? output.size() : end) - pos;
        const std::string line = output.substr(pos, length);
        if (line.rfind(prefix, 0) == 0) {
            return trim_copy(line.substr(prefix.size()));
        }
        if (end == std::string::npos) break;
        pos = end + 1U;
    }
    return {};
}

long long WorldTvService::number_for(const std::string& output,
                                     const std::string& key) {
    const std::string value = value_for(output, key);
    if (value.empty()) return 0;
    char* end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str()) return 0;
    return parsed;
}

WorldTvResolveResult WorldTvService::resolve(const std::string& channel_id,
                                             const std::string& feed_id,
                                             const std::string& preferred_url,
                                             const std::string& resolver,
                                             int max_height,
                                             const std::string& exclude_url) const {
    WorldTvResolveResult result;
    int exit_code = -1;
    const std::string output = run_worker({
        "resolve",
        channel_id,
        feed_id,
        preferred_url,
        resolver,
        std::to_string(max_height),
        exclude_url,
    }, exit_code);

    result.ok = exit_code == 0 && value_for(output, "OK") == "1";
    result.url = value_for(output, "URL");
    result.referrer = value_for(output, "REFERRER");
    result.user_agent = value_for(output, "USER_AGENT");
    result.error_class = value_for(output, "ERROR_CLASS");
    result.error = value_for(output, "ERROR");
    if (!result.ok && result.error_class.empty()) {
        result.error_class = exit_code == 127 ? "dependency" : "stream";
    }
    if (!result.ok && result.error.empty()) {
        result.error = "No playable direct World TV source was verified.";
    }
    if (!result.ok && !result.error_class.empty()) {
        result.error = "World TV " + result.error_class + " failure: " + result.error;
    }
    return result;
}

bool WorldTvService::refresh_artwork(const std::string& channel_id,
                                     const std::string& feed_id,
                                     const std::string& output_ppm,
                                     std::string& error) const {
    int exit_code = -1;
    const std::string output = run_worker({
        "artwork",
        channel_id,
        feed_id,
        output_ppm,
    }, exit_code);
    const bool ok = exit_code == 0 && value_for(output, "OK") == "1";
    error = value_for(output, "ERROR");
    if (!ok && error.empty()) error = "Station artwork could not be resolved.";
    return ok;
}

WorldTvGuideInfo WorldTvService::guide(const std::string& channel_id,
                                       const std::string& feed_id) const {
    WorldTvGuideInfo result;
    int exit_code = -1;
    const std::string output = run_worker({
        "guide",
        channel_id,
        feed_id,
    }, exit_code);

    result.available = exit_code == 0 && value_for(output, "OK") == "1";
    result.current_title = value_for(output, "CURRENT_TITLE");
    result.next_title = value_for(output, "NEXT_TITLE");
    result.current_start = number_for(output, "CURRENT_START");
    result.current_end = number_for(output, "CURRENT_END");
    result.next_start = number_for(output, "NEXT_START");
    result.next_end = number_for(output, "NEXT_END");
    result.source = value_for(output, "SOURCE");
    return result;
}

} // namespace reddmedia
