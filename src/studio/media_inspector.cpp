#include "media_inspector.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

namespace nougat::studio {
namespace {
bool regular_readable(const std::filesystem::path& file) {
    struct stat st{};
    return file.is_absolute() && stat(file.c_str(), &st) == 0 &&
           S_ISREG(st.st_mode) && access(file.c_str(), R_OK) == 0;
}
std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \r\n\t");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \r\n\t");
    return value.substr(first, last - first + 1U);
}
bool decimal_number(const std::string& value, double& number) {
    if (value.empty() || value.size() > 40U) return false;
    char* end = nullptr;
    errno = 0;
    number = std::strtod(value.c_str(), &end);
    return errno == 0 && end && *end == '\0' && std::isfinite(number) && number >= 0.0;
}
bool positive_dimension(const std::string& input, int& out) {
    if (input.empty() || input.size() > 8U ||
        !std::all_of(input.begin(), input.end(), [](unsigned char c) { return c >= '0' && c <= '9'; }))
        return false;
    try {
        const auto n = std::stoul(input);
        if (n > 0 && n <= 32768UL) { out = static_cast<int>(n); return true; }
    } catch (...) {}
    return false;
}
std::vector<std::pair<std::string,std::string>> fields(const std::string& line) {
    std::vector<std::pair<std::string,std::string>> result;
    std::size_t at = 0;
    while (at <= line.size()) {
        const auto next = line.find('|', at);
        const std::string field = line.substr(at,
            next == std::string::npos ? std::string::npos : next - at);
        const auto equals = field.find('=');
        if (equals != std::string::npos)
            result.emplace_back(field.substr(0, equals), field.substr(equals + 1U));
        if (next == std::string::npos) break;
        at = next + 1U;
    }
    return result;
}

bool run_probe(const std::string& executable, const std::filesystem::path& input,
               std::chrono::milliseconds timeout, std::string& output, std::string& error) {
    struct stat st{};
    if (executable.empty() || executable.front() != '/' ||
        stat(executable.c_str(), &st) != 0 || !S_ISREG(st.st_mode) ||
        access(executable.c_str(), X_OK) != 0) {
        error = "FFprobe executable is unavailable.";
        return false;
    }
    int fd[2]{};
    if (pipe(fd) != 0) { error = "Cannot open the FFprobe output pipe."; return false; }
    const std::string name = input.string();
    // No heap allocation between fork and exec. stdout only; stderr is discarded.
    const std::array<const char*, 13> args{{
        executable.c_str(), "-v", "error",
        "-show_entries", "format=duration:stream=codec_type,codec_name,width,height",
        "-of", "compact=p=0:nk=0", "-i", name.c_str(), nullptr, nullptr, nullptr
    }};
    const pid_t child = fork();
    if (child < 0) { close(fd[0]); close(fd[1]); error = "Cannot create FFprobe process."; return false; }
    if (child == 0) {
        (void)setpgid(0, 0);
        close(fd[0]);
        (void)dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        const int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            (void)dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execv(executable.c_str(), const_cast<char* const*>(args.data()));
        _exit(127);
    }
    (void)setpgid(child, child);
    close(fd[1]);
    bool pipe_open = true;
    bool reaped = false;
    bool timed_out = false;
    bool oversized = false;
    int status = -1;
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (pipe_open || !reaped) {
        if (std::chrono::steady_clock::now() >= deadline) {
            timed_out = true;
            if (!reaped) {
                (void)kill(-child, SIGKILL);
                (void)kill(child, SIGKILL);
            }
            break;
        }
        if (pipe_open) {
            pollfd pfd{};
            pfd.fd = fd[0];
            pfd.events = POLLIN | POLLHUP | POLLERR;
            const int polled = poll(&pfd, 1, 100);
            if (polled > 0) {
                char buffer[4096];
                const ssize_t n = read(fd[0], buffer, sizeof(buffer));
                if (n == 0) { close(fd[0]); pipe_open = false; }
                else if (n > 0) {
                    if (output.size() + static_cast<std::size_t>(n) > 65536U) {
                        oversized = true;
                        if (!reaped) {
                            (void)kill(-child, SIGKILL);
                            (void)kill(child, SIGKILL);
                        }
                        break;
                    }
                    output.append(buffer, static_cast<std::size_t>(n));
                } else if (errno != EINTR) { close(fd[0]); pipe_open = false; }
            } else if (polled < 0 && errno != EINTR) { close(fd[0]); pipe_open = false; }
        }
        if (!reaped) {
            const pid_t waited = waitpid(child, &status, WNOHANG);
            if (waited == child) reaped = true;
            else if (waited < 0 && errno == ECHILD) reaped = true;
        }
    }
    if (pipe_open) close(fd[0]);
    if (!reaped) {
        int waited_status = -1;
        // Only after terminating a timed-out/oversized process. No endless
        // blocking wait on a child which is still doing legitimate work.
        while (waitpid(child, &waited_status, 0) < 0 && errno == EINTR) {}
        status = waited_status;
    }
    if (timed_out) { error = "FFprobe exceeded its inspection deadline."; return false; }
    if (oversized) { error = "FFprobe returned more data than the inspection limit."; return false; }
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        error = "FFprobe could not inspect the media file."; return false;
    }
    return true;
}
} // namespace

MediaInspector::MediaInspector(std::string ffprobe, std::chrono::milliseconds timeout)
    : ffprobe_(std::move(ffprobe)), timeout_(std::clamp(timeout,
      std::chrono::milliseconds(100), std::chrono::milliseconds(30000))) {}

MediaProbe MediaInspector::inspect(const std::filesystem::path& input) const {
    MediaProbe result;
    result.input = input;
    if (!regular_readable(input)) {
        result.error = "Media inspection requires an absolute readable regular file path.";
        return result;
    }
    std::string raw;
    if (!run_probe(ffprobe_, input, timeout_, raw, result.error)) return result;
    std::size_t offset = 0;
    while (offset < raw.size()) {
        const auto next = raw.find('\n', offset);
        const std::string line = trim(raw.substr(offset,
            next == std::string::npos ? std::string::npos : next - offset));
        offset = next == std::string::npos ? raw.size() : next + 1U;
        if (line.empty()) continue;
        StreamInfo stream;
        for (const auto& pair : fields(line)) {
            if (pair.first == "codec_type") stream.kind = pair.second;
            else if (pair.first == "codec_name") stream.codec = pair.second;
            else if (pair.first == "width") (void)positive_dimension(pair.second, stream.width);
            else if (pair.first == "height") (void)positive_dimension(pair.second, stream.height);
            else if (pair.first == "duration") {
                double seconds = 0;
                if (decimal_number(pair.second, seconds) &&
                    seconds < 9223372036854.0) {
                    result.duration_ms = static_cast<std::uint64_t>(seconds * 1000.0);
                }
            }
        }
        if (!stream.kind.empty()) {
            if (result.streams.size() >= 128U) {
                result.error = "Media contains too many tracks for safe inspection.";
                result.streams.clear();
                return result;
            }
            result.streams.push_back(std::move(stream));
        }
    }
    if (result.streams.empty()) {
        result.error = "FFprobe returned no recognized media streams.";
        return result;
    }
    result.ok = true;
    return result;
}

CopyAdvice MediaInspector::assess_copy(const MediaProbe& probe, Preset preset) const {
    if (!probe.ok) return {CopyAssessment::Unknown, "Media must be inspected successfully first."};
    if (preset != Preset::RemuxMp4 && preset != Preset::RemuxMkv)
        return {CopyAssessment::ReencodeRecommended, "Selected preset performs encoding rather than stream copy."};
    if (preset == Preset::RemuxMkv)
        return {CopyAssessment::CompatibleCandidate,
                "Matroska stream copy looks plausible; only a real mux attempt confirms compatibility."};
    const auto allowed_video = [](const std::string& codec) {
        return codec == "h264" || codec == "hevc" || codec == "av1" ||
               codec == "mpeg4" || codec == "mjpeg";
    };
    const auto allowed_audio = [](const std::string& codec) {
        return codec == "aac" || codec == "mp3" || codec == "alac" ||
               codec == "ac3" || codec == "eac3";
    };
    for (const StreamInfo& s : probe.streams) {
        if (s.kind == "video" && !allowed_video(s.codec))
            return {CopyAssessment::ReencodeRecommended, "The video codec may not fit MP4 stream copy."};
        if (s.kind == "audio" && !allowed_audio(s.codec))
            return {CopyAssessment::ReencodeRecommended, "The audio codec may not fit MP4 stream copy."};
        if (s.kind != "video" && s.kind != "audio")
            return {CopyAssessment::Unknown, "Other tracks require an actual muxing compatibility check."};
    }
    return {CopyAssessment::CompatibleCandidate,
            "Stream-copy candidate only: container compatibility is not guaranteed."};
}

} // namespace nougat::studio
