#pragma once
#include "media_inspector.hpp"
#include "media_job_engine.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nougat::studio {

struct ExportPlan {
    bool valid = false;
    MediaProbe media;
    Preset preset = Preset::VideoMp4H264;
    CopyAdvice copy;
    std::string error;
};

struct ExportRequest {
    std::filesystem::path input;
    std::filesystem::path output;
    Preset preset = Preset::VideoMp4H264;
};

// Backend controller for a future Studio UI. Calls to plan/submit perform a
// bounded FFprobe subprocess and must be made off the GUI event thread.
class StudioService {
public:
    explicit StudioService(std::filesystem::path queue_path = default_queue_path(),
                           std::string ffmpeg = "/usr/bin/ffmpeg",
                           std::string ffprobe = "/usr/bin/ffprobe");
    ExportPlan plan(const ExportRequest& request) const;
    EnqueueResult submit(const ExportRequest& request);
    BatchResult submit_batch(const std::vector<ExportRequest>& requests);
    bool cancel(std::uint64_t job_id);
    bool retry(std::uint64_t job_id);
    bool forget(std::uint64_t job_id);
    std::string recent_log(std::uint64_t job_id, std::size_t limit = 4096U) const;
    std::vector<Job> jobs() const;
    bool persistence_ok() const;
    std::string persistence_error() const;
private:
    MediaInspector inspector_;
    Engine engine_;
};

} // namespace nougat::studio
