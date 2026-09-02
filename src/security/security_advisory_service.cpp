#include "security_advisory_service.hpp"

#include <cstdio>
#include <sstream>
#include <sys/wait.h>

namespace reddmedia::security {
namespace {

std::string command_output(const std::string& command) {
    FILE* pipe = popen((command + " 2>/dev/null").c_str(), "r");
    if (!pipe) return {};
    std::string out;
    char buffer[1024];
    while (std::fgets(buffer, sizeof(buffer), pipe)) out += buffer;
    pclose(pipe);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    const std::size_t newline = out.find('\n');
    return newline == std::string::npos ? out : out.substr(0, newline);
}

} // namespace

std::vector<RuntimeComponentAdvisory> SecurityAdvisoryService::inventory(std::string& status) const {
    std::vector<RuntimeComponentAdvisory> result;
    const struct Probe { const char* name; const char* command; } probes[] = {
        {"VLC/libVLC", "vlc --version | head -n1"},
        {"yt-dlp", "yt-dlp --version"},
        {"FFmpeg", "ffmpeg -version | head -n1"},
        {"libtorrent-rasterbar", "pkg-config --modversion libtorrent-rasterbar"},
        {"Jellyfin", "jellyfin --version | head -n1"},
    };
    for (const auto& probe : probes) {
        RuntimeComponentAdvisory item;
        item.component = probe.name;
        item.installed_version = command_output(probe.command);
        if (!item.installed_version.empty()) result.push_back(std::move(item));
    }
    status = "Runtime inventory mapped for advisory/CVE checks: " +
             std::to_string(result.size()) +
             " installed component(s). OSV is the configured public advisory source; no resident updater is started.";
    return result;
}

} // namespace reddmedia::security
