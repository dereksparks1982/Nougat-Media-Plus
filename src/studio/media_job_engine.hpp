#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace nougat::studio {

enum class JobState { Queued, Running, Completed, Failed, Cancelled, Interrupted };
enum class Preset {
    RemuxMp4, RemuxMkv, VideoMp4H264, VideoWebmVp9,
    AudioMp3, AudioFlac, AudioWav, AudioOpus
};

struct Job {
    std::uint64_t id = 0;
    std::filesystem::path input;
    std::filesystem::path output;
    Preset preset = Preset::VideoMp4H264;
    JobState state = JobState::Queued;
    // -1 means duration unknown; reported progress is advisory, never a proof
    // that an output has been committed to its final destination.
    int progress_percent = -1;
    std::uint64_t duration_ms = 0;
    std::string error;
};

struct EnqueueResult {
    bool ok = false;
    std::uint64_t id = 0;
    std::string error;
};
struct BatchItem {
    std::filesystem::path input;
    std::filesystem::path output;
    std::uint64_t estimated_duration_ms = 0;
};
struct BatchResult {
    bool ok = false;
    std::vector<std::uint64_t> ids;
    std::string error;
};

const char* preset_name(Preset preset);
const char* state_name(JobState state);
std::filesystem::path default_queue_path();

// A single-worker, durable FFmpeg queue. No GUI dependencies. Constructing
// an engine does not launch an encoder until enqueue() or retry() is called.
// Source code integration into CMake is not proof of runtime acceptance.
class Engine {
public:
    explicit Engine(std::filesystem::path queue_path = default_queue_path(),
                    std::string ffmpeg = "/usr/bin/ffmpeg");
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    EnqueueResult enqueue(const std::filesystem::path& input,
                          const std::filesystem::path& output,
                          Preset preset,
                          std::uint64_t estimated_duration_ms = 0);
    BatchResult enqueue_batch(const std::vector<BatchItem>& items, Preset preset);
    bool cancel(std::uint64_t id);
    bool retry(std::uint64_t id);
    std::vector<Job> snapshot() const;
    bool persistence_ok() const;
    std::string persistence_error() const;

private:
    bool save_locked();
    void load();
    void worker_loop();
    bool process(const Job& job, std::string& error,
                 const std::function<void(int)>& report);
    void interrupt_child();

    std::filesystem::path queue_path_;
    std::string ffmpeg_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<Job> jobs_;
    std::thread worker_;
    std::atomic<bool> closing_{false};
    std::atomic<bool> cancel_running_{false};
    std::atomic<int> child_group_{-1};
    std::uint64_t next_id_ = 1;
    bool persistence_ok_ = true;
    std::string persistence_error_;
};

} // namespace nougat::studio
