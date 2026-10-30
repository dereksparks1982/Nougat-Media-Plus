#include "studio_service.hpp"

#include <utility>

namespace nougat::studio {

StudioService::StudioService(std::filesystem::path queue_path,
                             std::string ffmpeg, std::string ffprobe)
    : inspector_(std::move(ffprobe)),
      engine_(std::move(queue_path), std::move(ffmpeg)) {}

ExportPlan StudioService::plan(const ExportRequest& request) const {
    ExportPlan plan;
    plan.preset = request.preset;
    const char* extension = "";
    switch (request.preset) {
    case Preset::RemuxMp4:
    case Preset::VideoMp4H264: extension = ".mp4"; break;
    case Preset::RemuxMkv: extension = ".mkv"; break;
    case Preset::VideoWebmVp9: extension = ".webm"; break;
    case Preset::AudioMp3: extension = ".mp3"; break;
    case Preset::AudioFlac: extension = ".flac"; break;
    case Preset::AudioWav: extension = ".wav"; break;
    case Preset::AudioOpus: extension = ".opus"; break;
    }
    if (request.output.empty() || !request.output.is_absolute() ||
        request.output.extension() != extension || request.output == request.input) {
        plan.error = "Studio export needs a different absolute output path with the correct extension.";
        return plan;
    }
    plan.media = inspector_.inspect(request.input);
    if (!plan.media.ok) { plan.error = plan.media.error; return plan; }
    bool video = false;
    bool audio = false;
    for (const auto& stream : plan.media.streams) {
        if (stream.kind == "video") video = true;
        if (stream.kind == "audio") audio = true;
    }
    const bool needs_video = request.preset == Preset::VideoMp4H264 ||
                             request.preset == Preset::VideoWebmVp9;
    const bool needs_audio = request.preset == Preset::AudioMp3 ||
                             request.preset == Preset::AudioFlac ||
                             request.preset == Preset::AudioWav ||
                             request.preset == Preset::AudioOpus;
    if ((needs_video && !video) || (needs_audio && !audio)) {
        plan.error = needs_audio
            ? "Source has no audio track for the selected audio export."
            : "Source has no video track for the selected video export.";
        return plan;
    }
    plan.copy = inspector_.assess_copy(plan.media, request.preset);
    if ((request.preset == Preset::RemuxMp4 || request.preset == Preset::RemuxMkv) &&
        plan.copy.assessment == CopyAssessment::ReencodeRecommended) {
        plan.error = "Lossless stream copy is unlikely to work: " + plan.copy.explanation;
        return plan;
    }
    plan.valid = true;
    return plan;
}

EnqueueResult StudioService::submit(const ExportRequest& request) {
    const ExportPlan proposed = plan(request);
    if (!proposed.valid) return {false, 0, proposed.error};
    return engine_.enqueue(request.input, request.output, request.preset,
                           proposed.media.duration_ms);
}
BatchResult StudioService::submit_batch(const std::vector<ExportRequest>& requests) {
    if (requests.empty() || requests.size() > 10000U)
        return {false, {}, "The Studio batch must contain between 1 and 10000 exports."};
    const Preset preset = requests.front().preset;
    std::vector<BatchItem> items;
    items.reserve(requests.size());
    for (const ExportRequest& request : requests) {
        if (request.preset != preset)
            return {false, {}, "One batch must use one consistent export preset."};
        const ExportPlan proposed = plan(request);
        if (!proposed.valid) return {false, {}, proposed.error};
        items.push_back({request.input, request.output, proposed.media.duration_ms});
    }
    return engine_.enqueue_batch(items, preset);
}
bool StudioService::cancel(std::uint64_t id) { return engine_.cancel(id); }
bool StudioService::retry(std::uint64_t id) { return engine_.retry(id); }
std::vector<Job> StudioService::jobs() const { return engine_.snapshot(); }
bool StudioService::persistence_ok() const { return engine_.persistence_ok(); }
std::string StudioService::persistence_error() const { return engine_.persistence_error(); }

} // namespace nougat::studio
