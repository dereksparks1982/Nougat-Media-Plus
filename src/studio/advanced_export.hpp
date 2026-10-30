#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace nougat::studio {
enum class Preset;

// All optional settings are plain data, safe to persist with each queue job.
// 0 means no override for positive sizes/rates; crf -1 means preset default.
// Start and duration are expressed in milliseconds, not floating-point time.
struct ExportEdits {
    std::uint64_t start_ms = 0;
    std::uint64_t duration_ms = 0;
    int width = 0, height = 0, fps = 0;
    int crf = -1, video_kbps = 0, audio_kbps = 0;
    int audio_stream = 0, subtitle_stream = -1;
    int crop_x = 0, crop_y = 0, crop_width = 0, crop_height = 0;
    int rotation = 0, gain_tenths_db = 0;
    bool normalize_audio = false, drop_audio = false, drop_subtitles = false;
    bool burn_subtitles = false;
    std::filesystem::path external_srt;
};

struct ExportCommand {
    bool ok = false;
    std::vector<std::string> argv; // argv[0] is the configured executable.
    std::string error;
};

bool validate_export_edits(const ExportEdits& edits, Preset preset,
                           std::string& error);
// No shell expansion, no arbitrary FFmpeg arguments and no network URL input.
ExportCommand build_export_command(const std::string& ffmpeg, Preset preset,
                                   const ExportEdits& edits,
                                   const std::filesystem::path& source,
                                   const std::filesystem::path& output);
// Stable, bounded payload for queue V3 records. Fail closed on malformed text.
std::string serialize_export_edits(const ExportEdits& edits);
bool deserialize_export_edits(const std::string& data, ExportEdits& edits);

} // namespace nougat::studio
