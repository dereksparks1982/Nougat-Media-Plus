#include "media_job_engine.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <poll.h>
#include <set>
#include <signal.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

namespace nougat::studio {
namespace fs = std::filesystem;
namespace {

struct Format {
    const char* extension;
    const char* muxer;
    std::vector<std::string> options;
};
Format format_for(Preset preset) {
    switch (preset) {
    case Preset::RemuxMp4: return {".mp4", "mp4", {"-map","0","-c","copy","-movflags","+faststart"}};
    case Preset::RemuxMkv: return {".mkv", "matroska", {"-map","0","-c","copy"}};
    case Preset::VideoMp4H264:
        return {".mp4", "mp4", {"-map","0:v:0","-map","0:a:0?","-c:v","libx264",
            "-preset","medium","-crf","23","-pix_fmt","yuv420p","-c:a","aac",
            "-b:a","160k","-movflags","+faststart"}};
    case Preset::VideoWebmVp9:
        return {".webm", "webm", {"-map","0:v:0","-map","0:a:0?","-c:v","libvpx-vp9",
            "-crf","32","-b:v","0","-c:a","libopus","-b:a","128k"}};
    case Preset::AudioMp3:
        return {".mp3", "mp3", {"-map","0:a:0","-vn","-c:a","libmp3lame","-q:a","3"}};
    case Preset::AudioFlac:
        return {".flac", "flac", {"-map","0:a:0","-vn","-c:a","flac"}};
    case Preset::AudioWav:
        return {".wav", "wav", {"-map","0:a:0","-vn","-c:a","pcm_s16le"}};
    case Preset::AudioOpus:
        return {".opus", "opus", {"-map","0:a:0","-vn","-c:a","libopus","-b:a","128k"}};
    }
    return {"", "", {}};
}

std::string hex_encode(const std::string& text) {
    constexpr char symbols[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(text.size() * 2U);
    for (unsigned char c : text) { out += symbols[c >> 4]; out += symbols[c & 15U]; }
    return out;
}
bool hex_decode(const std::string& input, std::string& output) {
    if (input.size() % 2U != 0U || input.size() > 65536U) return false;
    auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    output.clear();
    for (std::size_t i = 0; i < input.size(); i += 2U) {
        const int a = digit(input[i]);
        const int b = digit(input[i + 1U]);
        if (a < 0 || b < 0) return false;
        output.push_back(static_cast<char>(a * 16 + b));
    }
    return true;
}
bool parse_uint64(const std::string& text, std::uint64_t& value) {
    if (text.empty() || !std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return c >= '0' && c <= '9'; })) return false;
    try {
        std::size_t used = 0;
        value = std::stoull(text, &used);
        return used == text.size();
    } catch (...) { return false; }
}
std::vector<std::string> split_tabs(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    for (;;) {
        const std::size_t stop = line.find('\t', start);
        fields.push_back(line.substr(start, stop == std::string::npos ? stop : stop - start));
        if (stop == std::string::npos) return fields;
        start = stop + 1U;
    }
}
bool destination_free(const fs::path& path) {
    struct stat info{};
    if (lstat(path.c_str(), &info) == 0) return false;
    return errno == ENOENT;
}
bool validate_files(const fs::path& input, const fs::path& output, Preset preset,
                    std::string& error) {
    if (!input.is_absolute() || !output.is_absolute() || input.empty() || output.empty()) {
        error = "Specify absolute input and output file paths."; return false;
    }
    if (input == output) { error = "Source and destination must differ."; return false; }
    struct stat input_info{};
    if (stat(input.c_str(), &input_info) != 0 || !S_ISREG(input_info.st_mode)) {
        error = "Input must be a readable regular file."; return false;
    }
    if (access(input.c_str(), R_OK) != 0) {
        error = "Input cannot be read."; return false;
    }
    const Format target = format_for(preset);
    if (target.extension[0] == '\0' || output.extension() != target.extension) {
        error = "Output extension must match the selected preset."; return false;
    }
    if (!destination_free(output)) {
        error = "Output already exists; Nougat will not overwrite it."; return false;
    }
    const fs::path parent = output.parent_path();
    std::error_code ec;
    if (!fs::is_directory(parent, ec) || ec) {
        error = "Output directory does not exist."; return false;
    }
    // Also reject different spellings of the same input file.
    if (fs::equivalent(input, output, ec) && !ec) {
        error = "Output cannot overwrite the input."; return false;
    }
    return true;
}
std::string temporary_pattern(const fs::path& output) {
    return (output.parent_path() / ("." + output.filename().string() + ".nougat-XXXXXX")).string();
}
bool write_all(int fd, const std::string& value) {
    std::size_t done = 0;
    while (done < value.size()) {
        const ssize_t n = ::write(fd, value.data() + done, value.size() - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}
bool is_active(JobState state) {
    return state == JobState::Queued || state == JobState::Running;
}
} // namespace

const char* preset_name(Preset preset) {
    switch (preset) {
    case Preset::RemuxMp4: return "remux-mp4";
    case Preset::RemuxMkv: return "remux-mkv";
    case Preset::VideoMp4H264: return "video-mp4-h264";
    case Preset::VideoWebmVp9: return "video-webm-vp9";
    case Preset::AudioMp3: return "audio-mp3";
    case Preset::AudioFlac: return "audio-flac";
    case Preset::AudioWav: return "audio-wav";
    case Preset::AudioOpus: return "audio-opus";
    }
    return "unknown";
}
const char* state_name(JobState state) {
    switch (state) {
    case JobState::Queued: return "queued";
    case JobState::Running: return "running";
    case JobState::Completed: return "completed";
    case JobState::Failed: return "failed";
    case JobState::Cancelled: return "cancelled";
    case JobState::Interrupted: return "interrupted";
    }
    return "unknown";
}
fs::path default_queue_path() {
    const char* xdg = std::getenv("XDG_STATE_HOME");
    if (xdg && *xdg) return fs::path(xdg) / "nougat-media-plus" / "studio-queue.v1";
    const char* home = std::getenv("HOME");
    return fs::path(home && *home ? home : ".") / ".local/state/nougat-media-plus/studio-queue.v1";
}

Engine::Engine(fs::path queue_path, std::string ffmpeg)
    : queue_path_(std::move(queue_path)), ffmpeg_(std::move(ffmpeg)) {
    load();
    worker_ = std::thread([this] { worker_loop(); });
}
Engine::~Engine() {
    closing_.store(true);
    cancel_running_.store(true);
    interrupt_child();
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

bool Engine::save_locked() {
    std::error_code ec;
    const fs::path parent = queue_path_.parent_path();
    if (!fs::is_directory(parent, ec)) fs::create_directories(parent, ec);
    if (ec || parent.empty()) {
        persistence_ok_ = false;
        persistence_error_ = "Cannot create Studio queue state directory.";
        return false;
    }
    std::string pattern = (parent / ".studio-queue-XXXXXX").string();
    std::vector<char> scratch(pattern.begin(), pattern.end());
    scratch.push_back('\0');
    const int fd = mkstemp(scratch.data());
    if (fd < 0) {
        persistence_ok_ = false;
        persistence_error_ = "Cannot create temporary Studio queue state.";
        return false;
    }
    std::string serialized = "NOUGAT_STUDIO_QUEUE_V1\n";
    for (const Job& job : jobs_) {
        serialized += std::to_string(job.id) + "\t" +
          std::to_string(static_cast<int>(job.preset)) + "\t" +
          std::to_string(static_cast<int>(job.state)) + "\t" +
          std::to_string(job.duration_ms) + "\t" +
          hex_encode(job.input.string()) + "\t" +
          hex_encode(job.output.string()) + "\t" +
          hex_encode(job.error) + "\n";
    }
    const bool ok = write_all(fd, serialized) && fsync(fd) == 0;
    close(fd);
    if (!ok || rename(scratch.data(), queue_path_.c_str()) != 0) {
        unlink(scratch.data());
        persistence_ok_ = false;
        persistence_error_ = "Could not atomically save Studio queue state.";
        return false;
    }
    const int directory = open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory >= 0) { (void)fsync(directory); close(directory); }
    persistence_ok_ = true;
    persistence_error_.clear();
    return true;
}

void Engine::load() {
    std::ifstream in(queue_path_);
    if (!in) {
        std::error_code ec;
        if (fs::exists(queue_path_, ec) || ec) {
            persistence_ok_ = false;
            persistence_error_ = "Existing Studio queue could not be opened.";
        }
        return;
    }
    std::string line;
    if (!std::getline(in, line) || line != "NOUGAT_STUDIO_QUEUE_V1") {
        persistence_ok_ = false;
        persistence_error_ = "Unrecognized Studio queue format.";
        return;
    }
    std::vector<Job> restored;
    while (std::getline(in, line)) {
        if (line.size() > 150000U || restored.size() >= 10000U) {
            persistence_ok_ = false; persistence_error_ = "Studio queue exceeds safe limits."; return;
        }
        const auto fields = split_tabs(line);
        if (fields.size() != 7U) {
            persistence_ok_ = false; persistence_error_ = "Studio queue record is malformed."; return;
        }
        std::uint64_t id=0, preset=0, status=0, duration=0;
        std::string input, output, err;
        if (!parse_uint64(fields[0], id) || id == 0 || id == UINT64_MAX ||
            !parse_uint64(fields[1], preset) || preset > 7 ||
            !parse_uint64(fields[2], status) || status > 5 ||
            !parse_uint64(fields[3], duration) ||
            !hex_decode(fields[4], input) || !hex_decode(fields[5], output) ||
            !hex_decode(fields[6], err)) {
            persistence_ok_ = false; persistence_error_ = "Studio queue contains invalid data."; return;
        }
        if (std::any_of(restored.begin(), restored.end(), [id](const Job& j) { return j.id == id; })) {
            persistence_ok_ = false; persistence_error_ = "Duplicate Studio job ID."; return;
        }
        Job job;
        job.id = id;
        job.input = input;
        job.output = output;
        job.preset = static_cast<Preset>(preset);
        job.state = static_cast<JobState>(status);
        job.duration_ms = duration;
        job.error = err;
        if (is_active(job.state)) {
            job.state = JobState::Interrupted;
            job.error = "Job interrupted by a previous application exit. Retry explicitly.";
        }
        restored.push_back(std::move(job));
        next_id_ = std::max(next_id_, id + 1U);
    }
    if (in.bad()) {
        persistence_ok_ = false; persistence_error_ = "Cannot read the Studio queue."; return;
    }
    jobs_ = std::move(restored);
    // A former running/queued job is never launched blindly on restart.
    (void)save_locked();
}

EnqueueResult Engine::enqueue(const fs::path& input, const fs::path& output,
                              Preset preset, std::uint64_t duration_ms) {
    std::string error;
    if (!validate_files(input, output, preset, error)) return {false, 0, error};
    std::lock_guard<std::mutex> lock(mutex_);
    if (!persistence_ok_) return {false, 0, persistence_error_};
    if (jobs_.size() >= 10000U || next_id_ == UINT64_MAX)
        return {false, 0, "Studio queue is full."};
    // Do not queue multiple writers for one output in the same session.
    for (const Job& existing : jobs_) {
        if (is_active(existing.state) && existing.output == output)
            return {false, 0, "Another queued job already targets this output."};
    }
    Job job;
    job.id = next_id_;
    job.input = input;
    job.output = output;
    job.preset = preset;
    job.duration_ms = duration_ms;
    jobs_.push_back(job);
    if (!save_locked()) {
        jobs_.pop_back();
        return {false, 0, persistence_error_};
    }
    ++next_id_;
    cv_.notify_one();
    return {true, job.id, {}};
}

BatchResult Engine::enqueue_batch(const std::vector<BatchItem>& items, Preset preset) {
    if (items.empty() || items.size() > 10000U)
        return {false, {}, "Batch must contain between 1 and 10000 items."};
    std::set<fs::path> target_paths;
    for (const BatchItem& item : items) {
        std::string error;
        if (!validate_files(item.input, item.output, preset, error))
            return {false, {}, error};
        if (!target_paths.insert(item.output.lexically_normal()).second)
            return {false, {}, "Batch contains duplicate output paths."};
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!persistence_ok_) return {false, {}, persistence_error_};
    if (jobs_.size() + items.size() > 10000U ||
        items.size() > UINT64_MAX - next_id_) {
        return {false, {}, "Batch exceeds the durable queue capacity."};
    }
    for (const Job& prior : jobs_) {
        if (is_active(prior.state) &&
            target_paths.count(prior.output.lexically_normal()) != 0U)
            return {false, {}, "An existing job already targets a batch output."};
    }
    const std::size_t old_size = jobs_.size();
    std::uint64_t next = next_id_;
    std::vector<std::uint64_t> ids;
    ids.reserve(items.size());
    for (const BatchItem& item : items) {
        Job job;
        job.id = next++;
        job.input = item.input;
        job.output = item.output;
        job.preset = preset;
        job.duration_ms = item.estimated_duration_ms;
        jobs_.push_back(std::move(job));
        ids.push_back(next - 1U);
    }
    if (!save_locked()) {
        jobs_.resize(old_size); // Roll back the entire batch in memory.
        return {false, {}, persistence_error_};
    }
    next_id_ = next;
    cv_.notify_one();
    return {true, std::move(ids), {}};
}

bool Engine::cancel(std::uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (Job& job : jobs_) {
        if (job.id != id) continue;
        if (job.state == JobState::Queued) {
            const Job previous = job;
            job.state = JobState::Cancelled;
            job.error = "Cancelled before processing.";
            if (!save_locked()) { job = previous; return false; }
            return true;
        }
        if (job.state == JobState::Running) {
            cancel_running_.store(true);
            interrupt_child();
            return true;
        }
        return false;
    }
    return false;
}
bool Engine::retry(std::uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!persistence_ok_) return false;
    for (Job& job : jobs_) {
        if (job.id != id || is_active(job.state) || job.state == JobState::Completed) continue;
        std::string error;
        if (!validate_files(job.input, job.output, job.preset, error)) {
            job.error = error; (void)save_locked(); return false;
        }
        const Job previous = job;
        job.state = JobState::Queued;
        job.error.clear();
        job.progress_percent = -1;
        if (!save_locked()) { job = previous; return false; }
        cv_.notify_one();
        return true;
    }
    return false;
}
std::vector<Job> Engine::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return jobs_;
}
bool Engine::persistence_ok() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return persistence_ok_;
}
std::string Engine::persistence_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return persistence_error_;
}
void Engine::interrupt_child() {
    const int group = child_group_.load();
    if (group > 1) kill(-group, SIGTERM);
}

void Engine::worker_loop() {
    for (;;) {
        Job selected;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [&] {
                return closing_.load() || std::any_of(jobs_.begin(), jobs_.end(),
                    [](const Job& j) { return j.state == JobState::Queued; });
            });
            if (closing_.load()) return;
            auto job = std::find_if(jobs_.begin(), jobs_.end(),
                [](const Job& j) { return j.state == JobState::Queued; });
            if (job == jobs_.end()) continue;
            selected = *job;
            job->state = JobState::Running;
            cancel_running_.store(false);
            if (!save_locked()) {
                job->state = JobState::Interrupted;
                job->error = persistence_error_;
                return;
            }
        }
        std::string error;
        const bool completed = process(selected, error, [&](int progress) {
            std::lock_guard<std::mutex> lock(mutex_);
            for (Job& j : jobs_) if (j.id == selected.id) {
                if (progress > j.progress_percent && (j.progress_percent < 0 || progress - j.progress_percent >= 5)) {
                    j.progress_percent = progress;
                    (void)save_locked();
                }
                break;
            }
        });
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (Job& j : jobs_) if (j.id == selected.id) {
                j.state = (closing_.load() ? JobState::Interrupted :
                    cancel_running_.load() ? JobState::Cancelled :
                    completed ? JobState::Completed : JobState::Failed);
                j.progress_percent = completed ? 100 : j.progress_percent;
                j.error = completed ? "" : error;
                (void)save_locked();
                break;
            }
        }
    }
}

bool Engine::process(const Job& job, std::string& error,
                     const std::function<void(int)>& report) {
    if (!validate_files(job.input, job.output, job.preset, error)) return false;
    struct stat encoder_info{};
    if (ffmpeg_.empty() || ffmpeg_.front() != '/' ||
        stat(ffmpeg_.c_str(), &encoder_info) != 0 ||
        !S_ISREG(encoder_info.st_mode) || access(ffmpeg_.c_str(), X_OK) != 0) {
        error = "The configured FFmpeg executable is unavailable."; return false;
    }
    std::string pattern = temporary_pattern(job.output);
    std::vector<char> temp_name(pattern.begin(), pattern.end());
    temp_name.push_back('\0');
    const int fd = mkstemp(temp_name.data());
    if (fd < 0) { error = "Cannot reserve a temporary export in the output directory."; return false; }
    close(fd);
    const std::string temporary = temp_name.data();
    // A scoped cleanup protects against errors, cancellation and failure.
    struct TempCleanup {
        std::string path;
        ~TempCleanup() { if (!path.empty()) unlink(path.c_str()); }
    } cleanup{temporary};

    int pipefd[2]{};
    if (pipe(pipefd) != 0) { error = "Cannot start FFmpeg progress tracking."; return false; }
    const Format format = format_for(job.preset);
    std::vector<std::string> args{
        ffmpeg_, "-nostdin", "-hide_banner", "-loglevel", "error",
        "-nostats", "-progress", "pipe:1", "-y", "-i", job.input.string()
    };
    args.insert(args.end(), format.options.begin(), format.options.end());
    args.insert(args.end(), {"-f", format.muxer, temporary});
    std::vector<char*> argv;
    argv.reserve(args.size() + 1U);
    for (auto& value : args) argv.push_back(const_cast<char*>(value.c_str()));
    argv.push_back(nullptr);

    // Stderr is written only to the application-owned queue state directory.
    // It never lands next to the source media or in the target output.
    const fs::path logs = queue_path_.parent_path() / "studio-logs";
    std::error_code ec;
    fs::create_directories(logs, ec);
    if (ec) { close(pipefd[0]); close(pipefd[1]); error = "Could not create private Studio job log folder."; return false; }
    const std::string log_path = (logs / ("job-" + std::to_string(job.id) + ".log")).string();

    const pid_t child = fork();
    if (child < 0) {
        close(pipefd[0]); close(pipefd[1]);
        error = "Could not create FFmpeg process."; return false;
    }
    if (child == 0) {
        setpgid(0, 0);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        if (pipefd[1] > STDERR_FILENO) close(pipefd[1]);
        const int null_in = open("/dev/null", O_RDONLY);
        if (null_in >= 0) { dup2(null_in, STDIN_FILENO); if (null_in > STDERR_FILENO) close(null_in); }
        const int log_fd = open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
        if (log_fd >= 0) { dup2(log_fd, STDERR_FILENO); if (log_fd > STDERR_FILENO) close(log_fd); }
        else {
            const int error_sink = open("/dev/null", O_WRONLY);
            if (error_sink >= 0) { dup2(error_sink, STDERR_FILENO); if (error_sink > STDERR_FILENO) close(error_sink); }
        }
        // All allocation was completed before fork(), important in a
        // multi-threaded GUI process. Only async-signal-safe calls here.
        execv(ffmpeg_.c_str(), argv.data());
        _exit(127);
    }
    setpgid(child, child);
    close(pipefd[1]);
    child_group_.store(static_cast<int>(child));

    std::string pending;
    bool pipe_open = true;
    int status = 0;
    bool reaped = false;
    auto terminate_at = std::chrono::steady_clock::time_point{};
    auto last_output = std::chrono::steady_clock::now();
    bool stalled = false;
    while (pipe_open || !reaped) {
        // A hung encoder, or a grandchild retaining the progress pipe, must
        // not keep Nougat's single Studio worker blocked indefinitely.
        if (std::chrono::steady_clock::now() - last_output > std::chrono::seconds(120)) {
            stalled = true;
            if (!reaped) { kill(-child, SIGKILL); kill(child, SIGKILL); }
            break;
        }
        const bool cancelling = closing_.load() || cancel_running_.load();
        if (cancelling && terminate_at == std::chrono::steady_clock::time_point{}) {
            kill(-child, SIGTERM);
            terminate_at = std::chrono::steady_clock::now();
        }
        if (cancelling && terminate_at != std::chrono::steady_clock::time_point{} &&
            std::chrono::steady_clock::now() - terminate_at > std::chrono::seconds(2)) {
            kill(-child, SIGKILL);
        }
        pollfd watcher{pipefd[0], static_cast<short>(POLLIN | POLLHUP | POLLERR), 0};
        const int ready = pipe_open ? poll(&watcher, 1, 200) : 0;
        if (ready > 0) {
            char buffer[4096];
            const ssize_t read_bytes = read(pipefd[0], buffer, sizeof(buffer));
            if (read_bytes == 0) { close(pipefd[0]); pipe_open = false; }
            else if (read_bytes < 0 && errno != EINTR) {
                close(pipefd[0]); pipe_open = false;
            } else if (read_bytes > 0) {
                last_output = std::chrono::steady_clock::now();
                pending.append(buffer, static_cast<std::size_t>(read_bytes));
                if (pending.size() > 65536U) pending.erase(0, pending.size() - 32768U);
                std::size_t end = 0;
                while ((end = pending.find('\n')) != std::string::npos) {
                    const std::string line = pending.substr(0, end);
                    pending.erase(0, end + 1U);
                    const auto sep = line.find('=');
                    if (sep == std::string::npos || job.duration_ms == 0) continue;
                    const std::string field = line.substr(0, sep);
                    if (field != "out_time_ms" && field != "out_time_us") continue;
                    std::uint64_t microseconds=0;
                    if (!parse_uint64(line.substr(sep + 1U), microseconds)) continue;
                    const std::uint64_t milliseconds = microseconds / 1000U;
                    const long double fraction =
                        static_cast<long double>(milliseconds) /
                        static_cast<long double>(job.duration_ms);
                    const int percentage = static_cast<int>(
                        std::min(99.0L, fraction * 100.0L));
                    report(percentage);
                }
            }
        } else if (ready < 0 && errno != EINTR) {
            close(pipefd[0]); pipe_open = false;
        }
        if (!reaped) {
            const pid_t waited = waitpid(child, &status, WNOHANG);
            if (waited == child) reaped = true;
            else if (waited < 0 && errno == ECHILD) { reaped = true; status = -1; }
        }
    }
    if (pipe_open) close(pipefd[0]);
    if (!reaped) {
        int last_status = -1;
        while (waitpid(child, &last_status, 0) < 0 && errno == EINTR) {}
        status = last_status;
    }
    child_group_.store(-1);
    if (stalled) {
        error = "FFmpeg made no progress for two minutes and was stopped.";
        return false;
    }
    if (closing_.load() || cancel_running_.load()) {
        error = "Studio job cancelled or interrupted."; return false;
    }
    if (status < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        error = "FFmpeg failed (see the private Studio job log)."; return false;
    }
    struct stat file_info{};
    if (stat(temporary.c_str(), &file_info) != 0 || !S_ISREG(file_info.st_mode) || file_info.st_size == 0) {
        error = "FFmpeg produced no valid output file."; return false;
    }
    // link() creates the completed destination only when it does not already
    // exist. rename() here would silently overwrite a late-arriving user file.
    if (closing_.load() || cancel_running_.load()) {
        error = "Studio export was cancelled before publication.";
        return false;
    }
    if (link(temporary.c_str(), job.output.c_str()) != 0) {
        error = "Cannot commit export without overwriting an existing file."; return false;
    }
    const int dir_fd = open(job.output.parent_path().c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_fd >= 0) { (void)fsync(dir_fd); close(dir_fd); }
    return true;
}

} // namespace nougat::studio
