#include "nougat_paths.hpp"

#include <cstdlib>
#include <system_error>

namespace fs = std::filesystem;

namespace nougat::paths {
namespace {

fs::path env_path(const char* name, const fs::path& fallback) {
    if (const char* value = std::getenv(name); value != nullptr && *value != '\0') {
        return fs::path(value);
    }
    return fallback;
}

Layout build_layout() {
    const char* home_env = std::getenv("HOME");
    const fs::path home = (home_env != nullptr && *home_env != '\0')
        ? fs::path(home_env)
        : fs::current_path();

    const fs::path config_home = env_path("XDG_CONFIG_HOME", home / ".config");
    const fs::path data_home = env_path("XDG_DATA_HOME", home / ".local" / "share");
    const fs::path cache_home = env_path("XDG_CACHE_HOME", home / ".cache");
    const fs::path state_home = env_path("XDG_STATE_HOME", home / ".local" / "state");

    Layout out;
    out.home = home;
    out.config = config_home / "nougat";
    out.data = data_home / "nougat";
    out.cache = cache_home / "nougat";
    out.state = state_home / "nougat";
    out.logs = out.state / "logs";
    out.runtime = out.data / "runtime";
    out.server_data = out.data / "server";
    out.server_config = out.config / "server";
    out.server_cache = out.cache / "server";
    out.server_logs = out.logs / "server";
    out.artwork_cache = out.cache / "artwork";
    return out;
}

bool make_dir(const fs::path& path, std::string* error) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (ec) {
        if (error != nullptr) {
            *error = "Unable to create " + path.string() + ": " + ec.message();
        }
        return false;
    }
    return true;
}

} // namespace

const Layout& layout() {
    static const Layout value = build_layout();
    return value;
}

bool ensure_runtime_layout(std::string* error) {
    const Layout& p = layout();
    const fs::path required[] = {
        p.config,
        p.data,
        p.cache,
        p.state,
        p.logs,
        p.runtime,
        p.server_data,
        p.server_config,
        p.server_cache,
        p.server_logs,
        p.artwork_cache,
    };
    for (const fs::path& path : required) {
        if (!make_dir(path, error)) {
            return false;
        }
    }
    return true;
}

fs::path component_runtime(const std::string& component_id) {
    return layout().runtime / component_id;
}

std::vector<fs::path> legacy_config_roots() {
    return {
        layout().home / ".config" / "reddmedia",
        layout().home / ".config" / "ReddMedia",
    };
}

std::vector<fs::path> legacy_cache_roots() {
    return {
        layout().home / ".cache" / "reddmedia",
        layout().home / ".cache" / "ReddMedia",
    };
}

} // namespace nougat::paths
