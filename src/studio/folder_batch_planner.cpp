#include "folder_batch_planner.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <utility>
#include <set>
#include <system_error>

namespace nougat::studio {
namespace fs = std::filesystem;
namespace {
constexpr std::size_t kMaxEntries = 20000;
constexpr std::size_t kMaxSkips = 1000;

bool inside(const fs::path& child, const fs::path& parent) {
    auto c = child.begin();
    for (auto p = parent.begin(); p != parent.end(); ++p, ++c)
        if (c == child.end() || *c != *p) return false;
    return true;
}
std::string lowered(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}
bool hidden_component(const fs::path& relative) {
    for (const auto& part : relative) {
        const std::string name = part.string();
        if (!name.empty() && name[0] == '.') return true;
    }
    return false;
}
bool destination_exists(const fs::path& file) {
    std::error_code ec;
    const auto status = fs::symlink_status(file, ec);
    if (ec == std::errc::no_such_file_or_directory) return false;
    return ec || status.type() != fs::file_type::not_found;
}
void add_skip(FolderBatchPlan& plan, const fs::path& path, std::string message) {
    if (plan.skipped.size() < kMaxSkips)
        plan.skipped.push_back({path, std::move(message)});
}
bool is_live(JobState state) {
    return state == JobState::Queued || state == JobState::Running;
}
} // namespace

bool supported_media_extension(const fs::path& file) {
    static const std::set<std::string> known{
        ".mp4", ".mkv", ".mov", ".avi", ".m4v", ".webm", ".wmv",
        ".mpeg", ".mpg", ".ts", ".m2ts", ".mts", ".3gp", ".flv",
        ".mp3", ".m4a", ".aac", ".flac", ".wav", ".opus", ".ogg",
        ".wma", ".aif", ".aiff"
    };
    return known.count(lowered(file.extension().string())) != 0U;
}
const char* output_extension(Preset preset) noexcept {
    switch (preset) {
    case Preset::RemuxMp4:
    case Preset::VideoMp4H264: return ".mp4";
    case Preset::RemuxMkv: return ".mkv";
    case Preset::VideoWebmVp9: return ".webm";
    case Preset::AudioMp3: return ".mp3";
    case Preset::AudioFlac: return ".flac";
    case Preset::AudioWav: return ".wav";
    case Preset::AudioOpus: return ".opus";
    }
    return "";
}

FolderBatchPlan plan_folder_batch(const FolderBatchOptions& options,
                                  const std::vector<Job>& existing_jobs) {
    FolderBatchPlan result;
    std::error_code ec;
    if (!options.source_directory.is_absolute() ||
        !options.output_directory.is_absolute()) {
        result.error = "Choose absolute source and output folders.";
        return result;
    }
    if (options.max_files == 0 || options.max_files > 1000 ||
        options.max_depth > 8) {
        result.error = "Folder batch limit must be 1-1000 files and depth at most 8.";
        return result;
    }
    if (fs::is_symlink(fs::symlink_status(options.source_directory,ec)) || ec) {
        result.error = "Source folder cannot be a symbolic link.";
        return result;
    }
    const fs::path source = fs::canonical(options.source_directory, ec);
    if (ec || !fs::is_directory(source)) {
        result.error = "The source directory is unavailable.";
        return result;
    }
    ec.clear();
    if (fs::is_symlink(fs::symlink_status(options.output_directory,ec)) || ec) {
        result.error = "Destination folder cannot be a symbolic link.";
        return result;
    }
    const fs::path output = fs::canonical(options.output_directory, ec);
    if (ec || !fs::is_directory(output)) {
        result.error = "The destination directory must already exist.";
        return result;
    }
    // Reject input/output overlap in either direction. No output may be
    // discovered as a future input, and no input may be overwritten in place.
    if (inside(output, source) || inside(source, output)) {
        result.error = "Source and output directories must be separate, non-overlapping trees.";
        return result;
    }
    std::set<fs::path> reserved;
    for (const auto& job : existing_jobs)
        if (is_live(job.state)) reserved.insert(job.output.lexically_normal());
    const char* extension = output_extension(options.preset);
    if (extension[0] == '\0') {
        result.error = "Unsupported Studio export preset.";
        return result;
    }

    const auto examine = [&](const fs::directory_entry& entry) {
        ++result.scanned_entries;
        if (result.scanned_entries > kMaxEntries) {
            result.error = "Source folder contains too many entries for one scan.";
            return;
        }
        const fs::path relative = entry.path().lexically_relative(source);
        if (relative.empty() || hidden_component(relative)) return;
        std::error_code check;
        if (entry.is_symlink(check) || check) {
            add_skip(result, entry.path(), "Symbolic link or inaccessible entry."); return;
        }
        if (!entry.is_regular_file(check) || check) return;
        if (!supported_media_extension(entry.path())) return;
        if (result.exports.size() >= options.max_files) {
            result.error = "Source contains more than the selected batch file limit.";
            return;
        }
        const fs::path proposed = output /
            (entry.path().stem().string() + "-nougat" + extension);
        if (destination_exists(proposed)) {
            add_skip(result, entry.path(), "Output already exists (no overwrite).");
            return;
        }
        if (!reserved.insert(proposed.lexically_normal()).second) {
            add_skip(result, entry.path(), "An export or another source already reserves this output name.");
            return;
        }
        const auto bytes = entry.file_size(check);
        if (check) {
            add_skip(result, entry.path(), "Source size could not be read.");
            reserved.erase(proposed.lexically_normal());
            return;
        }
        if (bytes > std::numeric_limits<std::uintmax_t>::max() - result.source_bytes) {
            result.error = "Combined input size exceeds the supported counter.";
            return;
        }
        result.source_bytes += bytes;
        result.exports.push_back({entry.path(), proposed, options.preset});
    };

    // Sort source paths to make collision handling and previews deterministic.
    std::vector<fs::directory_entry> entries;
    const fs::directory_options flags = fs::directory_options::skip_permission_denied;
    if (options.recursive) {
        fs::recursive_directory_iterator it(source, flags, ec), end;
        if (ec) { result.error = "Could not open the source directory."; return result; }
        for (; it != end; it.increment(ec)) {
            if (ec) { result.error = "Directory traversal failed."; return result; }
            if (entries.size() >= kMaxEntries) {
                result.error = "Source directory entry limit reached."; return result;
            }
            const fs::path relative = it->path().lexically_relative(source);
            if (hidden_component(relative)) {
                std::error_code dir_error;
                if (it->is_directory(dir_error) && !dir_error) it.disable_recursion_pending();
                continue;
            }
            if (it.depth() >= static_cast<int>(options.max_depth))
                it.disable_recursion_pending();
            entries.push_back(*it);
        }
    } else {
        fs::directory_iterator it(source, flags, ec), end;
        if (ec) { result.error = "Could not open the source directory."; return result; }
        for (; it != end; it.increment(ec)) {
            if (ec) { result.error = "Directory traversal failed."; return result; }
            if (entries.size() >= kMaxEntries) {
                result.error = "Source directory entry limit reached."; return result;
            }
            entries.push_back(*it);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.path().string() < b.path().string();
    });
    for (const auto& entry : entries) {
        examine(entry);
        if (!result.error.empty()) return result;
    }
    if (result.exports.empty()) {
        result.error = "No eligible media files were found in the selected folder.";
        return result;
    }
    result.ok = true;
    return result;
}

} // namespace nougat::studio
