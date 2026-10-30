#pragma once
#include "studio_service.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace nougat::studio {

struct FolderBatchOptions {
    std::filesystem::path source_directory;
    std::filesystem::path output_directory;
    Preset preset = Preset::VideoMp4H264;
    bool recursive = false;  // no hidden tree crawl by default
    std::size_t max_files = 500;   // cap work and memory
    std::size_t max_depth = 4;
};

struct FolderBatchSkip {
    std::filesystem::path source;
    std::string reason;
};
struct FolderBatchPlan {
    bool ok = false;
    std::vector<ExportRequest> exports;
    std::vector<FolderBatchSkip> skipped;
    std::size_t scanned_entries = 0;
    std::uintmax_t source_bytes = 0;
    std::string error;
};

// Filesystem-only preflight. Does not execute FFprobe or FFmpeg and does not
// create/delete any input, output, folder or queue state. The caller explicitly
// chooses when to submit a preview with StudioService::submit_batch().
FolderBatchPlan plan_folder_batch(
    const FolderBatchOptions& options,
    const std::vector<Job>& existing_jobs = {});

bool supported_media_extension(const std::filesystem::path& file);
const char* output_extension(Preset preset) noexcept;

} // namespace nougat::studio
