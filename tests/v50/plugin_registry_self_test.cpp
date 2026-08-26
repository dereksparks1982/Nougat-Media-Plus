#include "plugins/plugin_registry.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

void need(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void write_text(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot write " + path.string());
    out << content;
    if (!out) throw std::runtime_error("write failed for " + path.string());
}

std::string workshop_manifest() {
    return R"JSON({
  "format": "NOUGAT_PLUGIN",
  "format_version": 1,
  "id": "workshop",
  "display_name": "Workshop",
  "version": "0.0.50",
  "nougat_plugin_api": 1,
  "description": "Reference Workshop plugin.",
  "top_level_tab": "Workshop",
  "required_for_application_start": false,
  "recommended_by_default": true,
  "runtime": {
    "kind": "x11-process",
    "entrypoint": "bin/nougat-workshop-plugin"
  },
  "dependencies": [],
  "features": ["split-reassemble"]
})JSON";
}

void install_valid_workshop(const fs::path& root) {
    const fs::path plugin = root / "workshop";
    write_text(plugin / "plugin.json", workshop_manifest());
    const fs::path executable = plugin / "bin" / "nougat-workshop-plugin";
    write_text(executable, "#!/bin/sh\nexit 0\n");
    if (::chmod(executable.c_str(), 0755) != 0) {
        throw std::runtime_error("chmod failed for Workshop test entrypoint");
    }
}

} // namespace

int main() {
    fs::path root;
    try {
        root = fs::temp_directory_path() /
            ("nougat-plugin-registry-v50-" + std::to_string(static_cast<long long>(::getpid())));
        fs::remove_all(root);
        fs::create_directories(root);

        need(::setenv("NOUGAT_PLUGIN_ROOT", root.c_str(), 1) == 0,
             "could not set isolated plugin root");

        install_valid_workshop(root);
        write_text(root / "broken" / "plugin.json",
                   "{\"format\":\"NOUGAT_PLUGIN\",\"format_version\":1,\"id\":\"broken\"}\n");

        auto first = nougat::plugins::scan_installed_plugins();
        need(first.plugins.size() == 1U, "expected exactly one valid plugin");
        need(first.plugins.front().id == "workshop", "Workshop was not discovered");
        need(first.plugins.front().top_level_tab == "Workshop", "Workshop tab metadata mismatch");
        need(first.plugins.front().runtime_kind == "x11-process", "Workshop runtime kind mismatch");
        need(first.issues.size() == 1U, "broken plugin should be reported and skipped");

        fs::remove_all(root / "workshop");
        auto removed = nougat::plugins::scan_installed_plugins();
        need(removed.plugins.empty(), "removed Workshop directory still appears installed");
        need(removed.issues.size() == 1U, "broken plugin issue should remain isolated");

        install_valid_workshop(root);
        auto restored = nougat::plugins::scan_installed_plugins();
        need(restored.plugins.size() == 1U && restored.plugins.front().id == "workshop",
             "restored Workshop directory was not rediscovered");

        fs::remove_all(root);
        std::cout << "PASS: built-in plugin registry discovers a valid physical plugin folder\n";
        std::cout << "PASS: removing the plugin folder makes the plugin disappear on rescan\n";
        std::cout << "PASS: restoring the plugin folder makes the plugin return on rescan\n";
        std::cout << "PASS: malformed plugins are skipped without failing the player core\n";
        return 0;
    } catch (const std::exception& exc) {
        if (!root.empty()) {
            std::error_code ec;
            fs::remove_all(root, ec);
        }
        std::cerr << "FAIL: " << exc.what() << '\n';
        return 1;
    }
}
