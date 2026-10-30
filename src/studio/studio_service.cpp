#include "studio_service.hpp"
#include "folder_batch_planner.hpp"

#include <utility>
#include <sys/stat.h>

namespace nougat::studio {

StudioService::StudioService(std::filesystem::path queue_path,
                             std::string ffmpeg, std::string ffprobe)
    : profile_library_(queue_path.parent_path() / "studio-profiles.v1"),
      inspector_(std::move(ffprobe)),
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
    case Preset::SubtitleSrt: extension = ".srt"; break;
    }
    if (request.output.empty() || !request.output.is_absolute() ||
        request.output.extension() != extension || request.output == request.input) {
        plan.error = "Studio export needs a different absolute output path with the correct extension.";
        return plan;
    }
    if (!validate_export_edits(request.edits, request.preset, plan.error)) return plan;
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
    const bool needs_subtitle = request.preset == Preset::SubtitleSrt;
    const bool needs_audio = request.preset == Preset::AudioMp3 ||
                             request.preset == Preset::AudioFlac ||
                             request.preset == Preset::AudioWav ||
                             request.preset == Preset::AudioOpus;
    bool subtitle = false;
    for (const auto& track : plan.media.streams)
        if (track.kind == "subtitle") subtitle = true;
    if (needs_subtitle && !subtitle) {
        plan.error = "The source contains no subtitle track to extract.";
        return plan;
    }
    if ((needs_video && !video) || (needs_audio && !audio)) {
        plan.error = needs_audio
            ? "Source has no audio track for the selected audio export."
            : "Source has no video track for the selected video export.";
        return plan;
    }
    if (request.edits.start_ms && plan.media.duration_ms > 0 &&
        request.edits.start_ms >= plan.media.duration_ms) {
        plan.error = "Trim start lies beyond the source duration.";
        return plan;
    }
    if (request.edits.audio_stream > 0 && !request.edits.drop_audio) {
        int found = 0;
        for (const auto& track : plan.media.streams) if (track.kind == "audio") ++found;
        if (request.edits.audio_stream >= found && !request.edits.drop_audio) {
            plan.error = "The selected audio track does not exist.";
            return plan;
        }
    }
    if (request.edits.subtitle_stream >= 0) {
        int count = 0;
        for (const auto& track : plan.media.streams) if (track.kind == "subtitle") ++count;
        if (request.edits.subtitle_stream >= count) {
            plan.error = "The selected subtitle track does not exist.";
            return plan;
        }
    }
    plan.copy = inspector_.assess_copy(plan.media, request.preset);
    if ((request.preset == Preset::RemuxMp4 || request.preset == Preset::RemuxMkv) &&
        plan.copy.assessment == CopyAssessment::ReencodeRecommended &&
        request.edits.subtitle_stream < 0 && request.edits.external_srt.empty()) {
        plan.error = "Lossless stream copy is unlikely to work: " + plan.copy.explanation;
        return plan;
    }
    // Skip uncertain size estimates for CRF-only encodes, lossless codecs
    // and remuxes. Explicit bitrate + a bounded duration permits guidance.
    const auto& e = request.edits;
    const bool mp4 = request.preset == Preset::VideoMp4H264;
    const bool webm = request.preset == Preset::VideoWebmVp9;
    const bool mp3 = request.preset == Preset::AudioMp3;
    const bool opus = request.preset == Preset::AudioOpus;
    const bool video_encode = mp4 || webm;
    const bool known_video_rate = !video_encode || e.video_kbps > 0;
    const int video_rate = video_encode ? e.video_kbps : 0;
    const int audio_rate = e.drop_audio ? 0 : e.audio_kbps > 0 ? e.audio_kbps :
        (mp4 ? 160 : webm || opus ? 128 : mp3 ? 192 : 0);
    std::uint64_t remaining = plan.media.duration_ms;
    if (remaining > 0 && e.start_ms < remaining) remaining -= e.start_ms;
    if (e.duration_ms > 0 && (remaining == 0 || remaining > e.duration_ms))
        remaining = e.duration_ms;
    if (remaining > 0 && remaining <= 604800000ULL &&
        known_video_rate && (video_rate > 0 || audio_rate > 0) &&
        (mp4 || webm || mp3 || opus)) {
        const std::uint64_t bits_per_ms = static_cast<std::uint64_t>(video_rate + audio_rate);
        plan.estimated_output_bytes = remaining * bits_per_ms / 8U;
    }
    std::error_code space_error;
    const auto space = std::filesystem::space(request.output.parent_path(), space_error);
    if (!space_error) {
        plan.available_disk_bytes = space.available;
        if (plan.estimated_output_bytes > 0 &&
            plan.available_disk_bytes < plan.estimated_output_bytes +
                plan.estimated_output_bytes / 10U) {
            plan.warning = "Estimated export may exceed available disk space. This is not an exact size prediction.";
        }
    } else {
        plan.warning = "Destination disk space could not be checked.";
    }
    plan.valid = true;
    return plan;
}

EnqueueResult StudioService::submit(const ExportRequest& request) {
    const ExportPlan proposed = plan(request);
    if (!proposed.valid) return {false, 0, proposed.error};
    std::uint64_t progress_duration = proposed.media.duration_ms;
    if (progress_duration > 0 && request.edits.start_ms < progress_duration)
        progress_duration -= request.edits.start_ms;
    if (request.edits.duration_ms > 0 &&
        (progress_duration == 0 || progress_duration > request.edits.duration_ms))
        progress_duration = request.edits.duration_ms;
    return engine_.enqueue(request.input, request.output, request.preset,
                           progress_duration, request.edits);
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
        std::uint64_t progress_duration = proposed.media.duration_ms;
        if (progress_duration > 0 && request.edits.start_ms < progress_duration)
            progress_duration -= request.edits.start_ms;
        if (request.edits.duration_ms > 0 &&
            (progress_duration == 0 || progress_duration > request.edits.duration_ms))
            progress_duration = request.edits.duration_ms;
        items.push_back({request.input, request.output, progress_duration, request.edits});
    }
    return engine_.enqueue_batch(items, preset);
}
FolderBatchPlan StudioService::preview_folder(const FolderBatchOptions& options) const {
    return plan_folder_batch(options, engine_.snapshot());
}
BatchResult StudioService::submit_folder(const FolderBatchPlan& preview) {
    if (!preview.ok || preview.exports.empty() || preview.exports.size() > 1000U)
        return {false, {}, "A valid folder preview is required before queuing a batch."};
    return submit_batch(preview.exports);
}
bool StudioService::cancel(std::uint64_t id) { return engine_.cancel(id); }
bool StudioService::retry(std::uint64_t id) { return engine_.retry(id); }
void StudioService::set_queue_paused(bool paused) { engine_.set_queue_paused(paused); }
bool StudioService::queue_paused() const { return engine_.queue_paused(); }
bool StudioService::move_queued(std::uint64_t id, int direction) {
    return engine_.move_queued(id, direction);
}
bool StudioService::forget(std::uint64_t id) { return engine_.forget(id); }
std::string StudioService::recent_log(std::uint64_t id, std::size_t limit) const {
    return engine_.recent_log(id, limit);
}
std::vector<Job> StudioService::jobs() const { return engine_.snapshot(); }
bool StudioService::save_profile(const NamedProfile& p, std::string& error) {
    return profile_library_.upsert(p, error);
}
bool StudioService::remove_profile(const std::string& name, std::string& error) {
    return profile_library_.erase(name, error);
}
std::vector<NamedProfile> StudioService::profiles() const { return profile_library_.list(); }
bool StudioService::apply_profile(const std::string& name, ExportRequest& request) const {
    const auto saved = profile_library_.find(name);
    if (!saved) return false;
    request.preset = saved->preset;
    request.edits = saved->edits;
    return true;
}
bool StudioService::profiles_available() const { return profile_library_.available(); }
bool StudioService::persistence_ok() const { return engine_.persistence_ok(); }
std::string StudioService::persistence_error() const { return engine_.persistence_error(); }

} // namespace nougat::studio
