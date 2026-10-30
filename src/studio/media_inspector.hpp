#pragma once
#include "media_job_engine.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace nougat::studio {

struct StreamInfo {
    std::string kind;  // video, audio, subtitle, data, attachment
    std::string codec;
    int width = 0;
    int height = 0;
};

struct MediaProbe {
    bool ok = false;
    std::filesystem::path input;
    std::uint64_t duration_ms = 0; // zero means unknown; never fabricate duration
    std::vector<StreamInfo> streams;
    std::string error;
};

enum class CopyAssessment { CompatibleCandidate, ReencodeRecommended, Unknown };

struct CopyAdvice {
    CopyAssessment assessment = CopyAssessment::Unknown;
    std::string explanation;
};

// FFprobe is an optional existing system tool. No shell commands, media
// writes, codec installations or external network requests are made here.
class MediaInspector {
public:
    explicit MediaInspector(std::string ffprobe = "/usr/bin/ffprobe",
                            std::chrono::milliseconds timeout = std::chrono::seconds(8));
    MediaProbe inspect(const std::filesystem::path& input) const;
    CopyAdvice assess_copy(const MediaProbe& probe, Preset preset) const;
private:
    std::string ffprobe_;
    std::chrono::milliseconds timeout_;
};

} // namespace nougat::studio
