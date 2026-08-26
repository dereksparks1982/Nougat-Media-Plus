#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace nougat::plugins {

constexpr int kPluginFormatVersion = 1;
constexpr int kPluginApiVersion = 1;

struct PluginManifest {
    std::string id;
    std::string display_name;
    std::string version;
    std::string description;
    std::string top_level_tab;
    int format_version = 0;
    int api_version = 0;
    bool required_for_application_start = false;
    bool recommended_by_default = false;
    std::string runtime_kind;
    std::filesystem::path entrypoint;
    std::vector<std::string> dependencies;
    std::vector<std::string> features;
    std::filesystem::path root;
    std::filesystem::path manifest_path;
};

struct PluginIssue {
    std::filesystem::path path;
    std::string message;
};

struct PluginScanResult {
    std::vector<PluginManifest> plugins;
    std::vector<PluginIssue> issues;
};

// Parse and validate one plugin directory. expected_id is normally the
// directory name discovered by scan_installed_plugins().
bool load_plugin_manifest(const std::filesystem::path& plugin_root,
                          const std::string& expected_id,
                          PluginManifest& out,
                          std::string& error);

// Scan the managed plugin folder. Broken plugins are reported in issues and
// skipped. They never make the player core fail to start.
PluginScanResult scan_installed_plugins();

// A safe plugin id is intentionally narrower than a generic filesystem name.
// Package/install tooling uses the same rule.
bool safe_plugin_id(const std::string& id);

} // namespace nougat::plugins
