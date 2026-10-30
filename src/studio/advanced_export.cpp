#include "advanced_export.hpp"
#include "media_job_engine.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <cstddef>
#include <utility>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace nougat::studio {
namespace {
bool video_preset(Preset p) {
    return p == Preset::VideoMp4H264 || p == Preset::VideoWebmVp9;
}
bool remux_preset(Preset p) {
    return p == Preset::RemuxMp4 || p == Preset::RemuxMkv;
}
bool audio_preset(Preset p) {
    return p == Preset::AudioMp3 || p == Preset::AudioFlac ||
           p == Preset::AudioWav || p == Preset::AudioOpus;
}
bool subtitle_preset(Preset p) { return p == Preset::SubtitleSrt; }

std::string seconds(std::uint64_t ms) {
    std::ostringstream result;
    result << (ms / 1000U) << '.' << std::setw(3) << std::setfill('0') << (ms % 1000U);
    return result.str();
}
std::string decimal_gain(int tenths) {
    const int absolute = tenths < 0 ? -tenths : tenths;
    return std::string(tenths < 0 ? "-" : "") +
           std::to_string(absolute / 10) + "." + std::to_string(absolute % 10) + "dB";
}
bool regular_file(const std::filesystem::path& path) {
    struct stat info{};
    return path.is_absolute() && stat(path.c_str(), &info) == 0 &&
           S_ISREG(info.st_mode) && access(path.c_str(), R_OK) == 0;
}
bool filter_safe_path(const std::string& path) {
    // FFmpeg filtergraph escaping has multiple nested syntaxes. Restrict
    // burned-in subtitle paths to an unambiguous subset rather than passing
    // user text to a filtergraph interpreter. Soft subtitle inputs may have
    // spaces and Unicode because they are passed as separate argv elements.
    return !path.empty() && path.size() <= 2048U &&
        std::all_of(path.begin(), path.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '/' || c == '_' || c == '-' || c == '.';
        });
}
std::string hex(const std::string& s) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string output; output.reserve(s.size() * 2U);
    for (unsigned char c : s) {
        output.push_back(digits[c >> 4U]);
        output.push_back(digits[c & 15U]);
    }
    return output;
}
bool unhex(const std::string& s, std::string& decoded) {
    if (s.size() > 4096U || s.size() % 2U != 0U) return false;
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    decoded.clear();
    for (std::size_t i = 0; i < s.size(); i += 2U) {
        const int a = nibble(s[i]), b = nibble(s[i + 1U]);
        if (a < 0 || b < 0) return false;
        const char c = static_cast<char>((a << 4) | b);
        if (c == '\0') return false;
        decoded.push_back(c);
    }
    return true;
}
bool integer(const std::string& value, long long& parsed) {
    if (value.empty() || value.size() > 20U) return false;
    const std::size_t at = value[0] == '-' ? 1U : 0U;
    if (at == value.size() || !std::all_of(value.begin() + static_cast<std::ptrdiff_t>(at),
            value.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) return false;
    try {
        std::size_t length = 0;
        parsed = std::stoll(value, &length, 10);
        return length == value.size();
    } catch (...) { return false; }
}
} // namespace

bool validate_export_edits(const ExportEdits& e, Preset preset, std::string& error) {
    error.clear();
    const bool video = video_preset(preset);
    const bool remux = remux_preset(preset);
    const bool audio = audio_preset(preset);
    const bool subtitle = subtitle_preset(preset);
    if (e.start_ms > 604800000ULL || e.duration_ms > 604800000ULL ||
        (e.duration_ms > 0 && e.start_ms > 604800000ULL - e.duration_ms)) {
        error = "Trim times must fit within seven days."; return false;
    }
    if ((e.width != 0 || e.height != 0) &&
        (e.width < 160 || e.height < 120 || e.width > 7680 || e.height > 4320 ||
         e.width % 2 != 0 || e.height % 2 != 0)) {
        error = "Scale width and height must both be even and within 160x120 to 7680x4320.";
        return false;
    }
    if (e.fps < 0 || e.fps > 120 ||
        (e.fps != 0 && e.fps < 1) ||
        e.crf < -1 || e.crf > 51 ||
        e.video_kbps < 0 || e.video_kbps > 100000 ||
        e.audio_kbps < 0 || e.audio_kbps > 512 ||
        e.audio_stream < 0 || e.audio_stream > 127 ||
        e.subtitle_stream < -1 || e.subtitle_stream > 127 ||
        e.gain_tenths_db < -240 || e.gain_tenths_db > 240) {
        error = "Export rate, quality, track index or gain is outside allowed bounds.";
        return false;
    }
    if ((e.crop_width != 0 || e.crop_height != 0 || e.crop_x != 0 || e.crop_y != 0) &&
        (e.crop_width < 16 || e.crop_height < 16 || e.crop_width > 7680 ||
         e.crop_height > 4320 || e.crop_x < 0 || e.crop_y < 0 ||
         e.crop_x > 7680 || e.crop_y > 4320)) {
        error = "Crop must have a positive bounded rectangle."; return false;
    }
    if (e.rotation != 0 && e.rotation != 90 && e.rotation != 180 && e.rotation != 270) {
        error = "Rotation must be 0, 90, 180 or 270 degrees."; return false;
    }
    const bool video_filters = e.width || e.height || e.fps || e.crf != -1 ||
        e.video_kbps || e.crop_width || e.crop_height || e.crop_x || e.crop_y ||
        e.rotation != 0;
    const bool subtitle_options = e.subtitle_stream != -1 || e.drop_subtitles ||
        e.burn_subtitles || !e.external_srt.empty();
    if ((remux && (video_filters || e.normalize_audio || e.gain_tenths_db ||
                   e.video_kbps || e.audio_kbps)) ||
        (audio && (video_filters || e.drop_audio || subtitle_options)) ||
        (subtitle && (video_filters || e.audio_kbps || e.audio_stream ||
            e.gain_tenths_db || e.normalize_audio || e.drop_audio ||
            e.drop_subtitles || e.burn_subtitles || !e.external_srt.empty()))) {
        error = "This preset cannot apply the requested video/audio/subtitle edits.";
        return false;
    }
    if (video && e.subtitle_stream >= 0) {
        error = "Existing subtitle-track selection is available on remux exports only.";
        return false;
    }
    if (e.drop_audio && (e.normalize_audio || e.gain_tenths_db || e.audio_kbps ||
                         e.audio_stream != 0)) {
        error = "Cannot process an audio track while dropping all audio."; return false;
    }
    if (e.burn_subtitles && (!video || e.drop_subtitles)) {
        error = "Burning subtitles needs a video encoding preset with subtitles enabled.";
        return false;
    }
    if (e.burn_subtitles && e.external_srt.empty()) {
        error = "Subtitle burning requires an external SRT file."; return false;
    }
    if (!e.external_srt.empty()) {
        if (e.external_srt.string().size() > 2048U) {
            error = "Subtitle path exceeds the supported limit."; return false;
        }
        if (!regular_file(e.external_srt) || e.external_srt.extension() != ".srt") {
            error = "External subtitles must be a readable local .srt file."; return false;
        }
        if (e.burn_subtitles && !filter_safe_path(e.external_srt.string())) {
            error = "Burned-in SRT paths currently support only letters, numbers, / . _ and -.";
            return false;
        }
        if (!e.burn_subtitles && preset != Preset::RemuxMp4 &&
            preset != Preset::RemuxMkv && preset != Preset::VideoMp4H264) {
            error = "External subtitle muxing requires MP4 or MKV output."; return false;
        }
        if (e.drop_subtitles) {
            error = "Cannot add and remove subtitles in the same export."; return false;
        }
    }
    return true;
}

ExportCommand build_export_command(const std::string& ffmpeg, Preset preset,
                                   const ExportEdits& e,
                                   const std::filesystem::path& input,
                                   const std::filesystem::path& output) {
    ExportCommand result;
    if (!validate_export_edits(e, preset, result.error)) return result;
    if (!input.is_absolute() || !output.is_absolute() || ffmpeg.empty() ||
        ffmpeg.front() != '/') {
        result.error = "Source, destination and executable must be absolute paths."; return result;
    }
    auto& a = result.argv;
    a = {ffmpeg, "-nostdin", "-hide_banner", "-loglevel", "error",
         "-nostats", "-progress", "pipe:1", "-y", "-i", input.string()};
    // An external SRT is a second input for soft subtitles, not for burning.
    if (!e.external_srt.empty() && !e.burn_subtitles) {
        a.insert(a.end(), {"-i", e.external_srt.string()});
    }
    if (e.start_ms) a.insert(a.end(), {"-ss", seconds(e.start_ms)});
    if (e.duration_ms) a.insert(a.end(), {"-t", seconds(e.duration_ms)});
    const bool remux = remux_preset(preset);
    const bool audio = audio_preset(preset);
    if (subtitle_preset(preset)) {
        a.insert(a.end(), {"-map", "0:s:" +
            std::to_string(e.subtitle_stream >= 0 ? e.subtitle_stream : 0),
            "-c:s", "srt"});
    } else if (remux) {
        a.insert(a.end(), {"-map", "0"});
        if (e.drop_audio || e.audio_stream > 0) a.insert(a.end(), {"-map", "-0:a?"});
        if (!e.drop_audio && e.audio_stream > 0)
            a.insert(a.end(), {"-map", "0:a:" + std::to_string(e.audio_stream)});
        if (e.drop_subtitles || e.subtitle_stream >= 0)
            a.insert(a.end(), {"-map", "-0:s?"});
        if (!e.drop_subtitles && e.subtitle_stream >= 0)
            a.insert(a.end(), {"-map", "0:s:" + std::to_string(e.subtitle_stream)});
        a.insert(a.end(), {"-c", "copy"});
        if (!e.external_srt.empty()) {
            a.insert(a.end(), {"-map", "1:0", "-c:s", preset == Preset::RemuxMp4 ? "mov_text" : "srt"});
        } else if (preset == Preset::RemuxMp4 && e.subtitle_stream >= 0) {
            a.insert(a.end(), {"-c:s", "mov_text"});
        }
        if (preset == Preset::RemuxMp4) a.insert(a.end(), {"-movflags", "+faststart"});
    } else {
        if (!audio) a.insert(a.end(), {"-map", "0:v:0"});
        if (!e.drop_audio) a.insert(a.end(),
            {"-map", "0:a:" + std::to_string(e.audio_stream) + (audio ? "" : "?")});
        if (audio) a.push_back("-vn");
        if (!audio) {
            std::vector<std::string> filters;
            if (e.crop_width) filters.push_back("crop=" +
                std::to_string(e.crop_width) + ":" + std::to_string(e.crop_height) +
                ":" + std::to_string(e.crop_x) + ":" + std::to_string(e.crop_y));
            if (e.rotation == 90) filters.push_back("transpose=clock");
            if (e.rotation == 180) filters.push_back("hflip,vflip");
            if (e.rotation == 270) filters.push_back("transpose=cclock");
            if (e.width && e.height) {
                const std::string size = std::to_string(e.width) + ":" + std::to_string(e.height);
                filters.push_back("scale=" + size + ":force_original_aspect_ratio=decrease");
                filters.push_back("pad=" + size + ":(ow-iw)/2:(oh-ih)/2");
            }
            if (e.burn_subtitles)
                filters.push_back("subtitles=filename=" + e.external_srt.string());
            if (!filters.empty()) {
                std::string vf;
                for (const auto& f : filters) {
                    if (!vf.empty()) vf.push_back(',');
                    vf += f;
                }
                a.insert(a.end(), {"-vf", vf});
            }
            if (e.fps) a.insert(a.end(), {"-r", std::to_string(e.fps)});
            const bool h264 = preset == Preset::VideoMp4H264;
            a.insert(a.end(), {"-c:v", h264 ? "libx264" : "libvpx-vp9",
                               "-crf", std::to_string(e.crf < 0 ? (h264 ? 23 : 32) : e.crf)});
            if (e.video_kbps)
                a.insert(a.end(), {"-b:v", std::to_string(e.video_kbps) + "k"});
            else if (!h264) a.insert(a.end(), {"-b:v", "0"});
            if (h264) {
                a.insert(a.end(), {"-preset", "medium", "-pix_fmt", "yuv420p"});
            }
        }
        if (!e.drop_audio) {
            if (preset == Preset::VideoMp4H264) a.insert(a.end(), {"-c:a", "aac"});
            else if (preset == Preset::VideoWebmVp9 || preset == Preset::AudioOpus)
                a.insert(a.end(), {"-c:a", "libopus"});
            else if (preset == Preset::AudioMp3) a.insert(a.end(), {"-c:a", "libmp3lame"});
            else if (preset == Preset::AudioFlac) a.insert(a.end(), {"-c:a", "flac"});
            else if (preset == Preset::AudioWav) a.insert(a.end(), {"-c:a", "pcm_s16le"});
            if (e.audio_kbps > 0) a.insert(a.end(), {"-b:a", std::to_string(e.audio_kbps) + "k"});
            else if (preset == Preset::AudioMp3) a.insert(a.end(), {"-q:a", "3"});
            else if (preset == Preset::VideoMp4H264) a.insert(a.end(), {"-b:a", "160k"});
            else if (preset == Preset::VideoWebmVp9 || preset == Preset::AudioOpus)
                a.insert(a.end(), {"-b:a", "128k"});
            std::string af;
            if (e.normalize_audio) af = "loudnorm=I=-16:LRA=11:TP=-1.5";
            if (e.gain_tenths_db) {
                if (!af.empty()) af.push_back(',');
                af += "volume=" + decimal_gain(e.gain_tenths_db);
            }
            if (!af.empty()) a.insert(a.end(), {"-af", af});
        }
        if (!e.external_srt.empty() && !e.burn_subtitles) {
            a.insert(a.end(), {"-map", "1:0", "-c:s", "mov_text"});
        }
        if (preset == Preset::VideoMp4H264) a.insert(a.end(), {"-movflags", "+faststart"});
    }
    const char* mux = preset == Preset::RemuxMkv ? "matroska" :
        preset == Preset::VideoWebmVp9 ? "webm" :
        preset == Preset::AudioMp3 ? "mp3" :
        preset == Preset::AudioFlac ? "flac" :
        preset == Preset::AudioWav ? "wav" :
        preset == Preset::AudioOpus ? "opus" :
        preset == Preset::SubtitleSrt ? "srt" : "mp4";
    a.insert(a.end(), {"-f", mux, output.string()});
    result.ok = true;
    return result;
}

std::string serialize_export_edits(const ExportEdits& e) {
    const std::array<long long,20> values{{
        static_cast<long long>(e.start_ms), static_cast<long long>(e.duration_ms),
        e.width, e.height, e.fps, e.crf, e.video_kbps, e.audio_kbps,
        e.audio_stream, e.subtitle_stream, e.crop_x, e.crop_y, e.crop_width,
        e.crop_height, e.rotation, e.gain_tenths_db, e.normalize_audio,
        e.drop_audio, e.drop_subtitles, e.burn_subtitles
    }};
    std::string data;
    for (const long long v : values) {
        if (!data.empty()) data.push_back('|');
        data += std::to_string(v);
    }
    data.push_back('|');
    data += hex(e.external_srt.string());
    return data;
}
bool deserialize_export_edits(const std::string& data, ExportEdits& result) {
    if (data.size() > 8192U) return false;
    std::vector<std::string> fields;
    std::size_t pos = 0;
    for (;;) {
        const auto next = data.find('|', pos);
        fields.push_back(data.substr(pos,
            next == std::string::npos ? std::string::npos : next - pos));
        if (next == std::string::npos) break;
        pos = next + 1U;
    }
    if (fields.size() != 21U) return false;
    std::array<long long,20> n{};
    for (std::size_t i = 0; i < n.size(); ++i)
        if (!integer(fields[i], n[i])) return false;
    if (n[0] < 0 || n[1] < 0 ||
        n[16] < 0 || n[16] > 1 || n[17] < 0 || n[17] > 1 ||
        n[18] < 0 || n[18] > 1 || n[19] < 0 || n[19] > 1)
        return false;
    for (std::size_t i = 2; i < n.size(); ++i)
        if (n[i] < INT_MIN || n[i] > INT_MAX) return false;
    std::string subtitle;
    if (!unhex(fields.back(), subtitle)) return false;
    ExportEdits e;
    e.start_ms = static_cast<std::uint64_t>(n[0]);
    e.duration_ms = static_cast<std::uint64_t>(n[1]);
    e.width = static_cast<int>(n[2]); e.height = static_cast<int>(n[3]);
    e.fps = static_cast<int>(n[4]); e.crf = static_cast<int>(n[5]);
    e.video_kbps = static_cast<int>(n[6]); e.audio_kbps = static_cast<int>(n[7]);
    e.audio_stream = static_cast<int>(n[8]); e.subtitle_stream = static_cast<int>(n[9]);
    e.crop_x = static_cast<int>(n[10]); e.crop_y = static_cast<int>(n[11]);
    e.crop_width = static_cast<int>(n[12]); e.crop_height = static_cast<int>(n[13]);
    e.rotation = static_cast<int>(n[14]); e.gain_tenths_db = static_cast<int>(n[15]);
    e.normalize_audio = n[16] != 0; e.drop_audio = n[17] != 0;
    e.drop_subtitles = n[18] != 0; e.burn_subtitles = n[19] != 0;
    e.external_srt = std::filesystem::path(subtitle);
    // Generic bounds check here; preset-specific validation occurs on load
    // before accepting a job for retry/execution.
    if (e.start_ms > 604800000ULL || e.duration_ms > 604800000ULL ||
        e.width < 0 || e.width > 7680 || e.height < 0 || e.height > 4320 ||
        e.fps < 0 || e.fps > 120 || e.crf < -1 || e.crf > 51 ||
        e.audio_stream < 0 || e.audio_stream > 127 ||
        e.subtitle_stream < -1 || e.subtitle_stream > 127 ||
        e.video_kbps < 0 || e.video_kbps > 100000 ||
        e.audio_kbps < 0 || e.audio_kbps > 512 ||
        e.crop_x < 0 || e.crop_x > 7680 || e.crop_y < 0 || e.crop_y > 4320 ||
        e.crop_width < 0 || e.crop_width > 7680 ||
        e.crop_height < 0 || e.crop_height > 4320 ||
        e.gain_tenths_db < -240 || e.gain_tenths_db > 240 ||
        (e.rotation != 0 && e.rotation != 90 && e.rotation != 180 && e.rotation != 270))
        return false;
    result = std::move(e);
    return true;
}
} // namespace nougat::studio
