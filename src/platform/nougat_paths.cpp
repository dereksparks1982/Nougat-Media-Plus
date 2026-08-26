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

bool safe_id(const std::string& value) {
    if (value.empty() || value == "." || value == "..") return false;
    return value.find('/') == std::string::npos && value.find('\\') == std::string::npos;
}

bool safe_relative_resource(const fs::path& value) {
    if (value.empty() || value.is_absolute()) return false;
    for (const fs::path& part : value) {
        if (part == "." || part == ".." || part.empty()) return false;
    }
    return true;
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
    out.plugins = env_path("NOUGAT_PLUGIN_ROOT", out.data / "plugins");
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
        p.plugins,
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
    if (!safe_id(component_id)) return {};
    return layout().runtime / component_id;
}

fs::path plugin_root(const std::string& plugin_id) {
    if (!safe_id(plugin_id)) return {};
    return layout().plugins / plugin_id;
}

fs::path plugin_manifest(const std::string& plugin_id) {
    const fs::path root = plugin_root(plugin_id);
    if (root.empty()) return {};
    return root / "plugin.json";
}

fs::path plugin_resource(const std::string& plugin_id, const fs::path& relative_path) {
    const fs::path root = plugin_root(plugin_id);
    if (root.empty() || !safe_relative_resource(relative_path)) return {};
    return root / relative_path;
}

bool plugin_installed(const std::string& plugin_id) {
    const fs::path manifest = plugin_manifest(plugin_id);
    if (manifest.empty()) return false;
    std::error_code ec;
    return fs::is_regular_file(manifest, ec) && !ec;
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
