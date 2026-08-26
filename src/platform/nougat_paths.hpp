#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace nougat::paths {

struct Layout {
    std::filesystem::path home;
    std::filesystem::path config;
    std::filesystem::path data;
    std::filesystem::path cache;
    std::filesystem::path state;
    std::filesystem::path logs;
    std::filesystem::path runtime;
    std::filesystem::path server_data;
    std::filesystem::path server_config;
    std::filesystem::path server_cache;
    std::filesystem::path server_logs;
    std::filesystem::path artwork_cache;
};

// Resolve Nougat's v0.0.50 XDG layout. Environment overrides follow the
// freedesktop XDG base-directory conventions. No directory is created merely
// by calling layout().
const Layout& layout();

// Create the Nougat-owned directories required for normal runtime operation.
// User media directories are never created, moved, or modified here.
bool ensure_runtime_layout(std::string* error = nullptr);

// Preferred managed component root, e.g. ~/.local/share/nougat/runtime/mesen2.
std::filesystem::path component_runtime(const std::string& component_id);

// Legacy ReddMedia locations are returned only for migration/compatibility.
// Callers must verify a successful migration before removing anything.
std::vector<std::filesystem::path> legacy_config_roots();
std::vector<std::filesystem::path> legacy_cache_roots();

} // namespace nougat::paths
