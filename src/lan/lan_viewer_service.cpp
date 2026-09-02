#include "lan_viewer_service.hpp"
#include "lan_media_service.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <sstream>
#include <sys/wait.h>

namespace reddmedia::lan {
namespace {

struct CommandResult {
    int code = -1;
    std::string output;
};

std::string trim_copy(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    return value.substr(first);
}

std::string shell_quote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\"'\"'";
        else out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

CommandResult run_command(const std::string& command) {
    CommandResult result;
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe) return result;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe)) result.output += buffer;
    const int raw = pclose(pipe);
    if (raw >= 0 && WIFEXITED(raw)) result.code = WEXITSTATUS(raw);
    else result.code = raw;
    result.output = trim_copy(result.output);
    return result;
}

std::vector<std::string> split_semicolon(const std::string& line) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    for (;;) {
        const std::size_t at = line.find(';', start);
        parts.push_back(line.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos) break;
        start = at + 1U;
    }
    return parts;
}

std::string json_string_field(const std::string& object, const std::string& key) {
    const std::string marker = "\"" + key + "\"";
    std::size_t pos = object.find(marker);
    if (pos == std::string::npos) return {};
    pos = object.find(':', pos + marker.size());
    if (pos == std::string::npos) return {};
    ++pos;
    while (pos < object.size() && std::isspace(static_cast<unsigned char>(object[pos]))) ++pos;
    if (pos >= object.size() || object[pos] != '"') return {};
    ++pos;
    std::string out;
    bool escaped = false;
    while (pos < object.size()) {
        const char c = object[pos++];
        if (!escaped && c == '"') break;
        if (!escaped && c == '\\') { escaped = true; continue; }
        if (escaped) {
            if (c == 'n') out.push_back('\n');
            else if (c == 'r') out.push_back('\r');
            else if (c == 't') out.push_back('\t');
            else out.push_back(c);
            escaped = false;
        } else out.push_back(c);
    }
    return out;
}

std::vector<std::string> json_objects(const std::string& text) {
    std::vector<std::string> result;
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    std::size_t begin = std::string::npos;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') { in_string = true; continue; }
        if (c == '{') {
            if (depth == 0) begin = i;
            ++depth;
        } else if (c == '}' && depth > 0) {
            --depth;
            if (depth == 0 && begin != std::string::npos) {
                result.push_back(text.substr(begin, i - begin + 1U));
                begin = std::string::npos;
            }
        }
    }
    return result;
}

LanTrustState parse_trust(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (value == "verified_clean" || value == "verified clean" || value == "clean")
        return LanTrustState::VerifiedClean;
    if (value == "blocked" || value == "malicious" || value == "infected")
        return LanTrustState::Blocked;
    return LanTrustState::Unknown;
}

std::string validated_http_url(const LanPeer& peer, const std::string& path_or_url) {
    if (!LanMediaService::is_private_lan_address(peer.address)) return {};
    const std::string base = "http://" + peer.address + ":" + std::to_string(peer.port);
    if (path_or_url.rfind("http://", 0U) == 0U) {
        const std::string expected = base + "/";
        if (path_or_url == base || path_or_url.rfind(expected, 0U) == 0U) return path_or_url;
        return {};
    }
    if (path_or_url.empty() || path_or_url[0] != '/') return {};
    return base + path_or_url;
}

} // namespace

LanViewerService::LanViewerService() = default;

bool LanViewerService::discover(std::vector<LanPeer>& peers, std::string& status) const {
    peers.clear();
    CommandResult result = run_command("command -v avahi-browse >/dev/null 2>&1 && avahi-browse -prt _nougat._tcp");
    if (result.code != 0) {
        status = "LAN Viewer discovery needs avahi-browse. No WAN discovery fallback was used.";
        return false;
    }

    std::set<std::string> seen;
    std::istringstream lines(result.output);
    std::string line;
    while (std::getline(lines, line)) {
        const auto parts = split_semicolon(line);
        // avahi-browse parsable resolved record:
        // =;iface;proto;name;type;domain;host;address;port;txt
        if (parts.size() < 9U || parts[0] != "=") continue;
        const std::string address = parts[7];
        if (!LanMediaService::is_private_lan_address(address)) continue;
        const int port = std::atoi(parts[8].c_str());
        if (port <= 0 || port > 65535) continue;
        const std::string key = address + ":" + std::to_string(port);
        if (!seen.insert(key).second) continue;
        LanPeer peer;
        peer.service_name = parts[3].empty() ? "Nougat Media Suite" : parts[3];
        peer.host = parts[6];
        peer.address = address;
        peer.port = static_cast<std::uint16_t>(port);
        peers.push_back(std::move(peer));
    }

    status = peers.empty()
        ? "No paired Nougat LAN peers were discovered."
        : "LAN Viewer discovered " + std::to_string(peers.size()) + " private-LAN Nougat peer(s).";
    return true;
}

bool LanViewerService::load_catalog(const LanPeer& peer,
                                    std::vector<LanRemoteItem>& items,
                                    std::string& status) const {
    items.clear();
    if (!LanMediaService::is_private_lan_address(peer.address)) {
        status = "Blocked catalog request outside the private LAN.";
        return false;
    }
    const std::string url = "http://" + peer.address + ":" + std::to_string(peer.port) + "/nougat/v1/catalog";
    const CommandResult response = run_command(
        "curl -fsS --connect-timeout 2 --max-time 8 " + shell_quote(url));
    if (response.code != 0) {
        status = "Could not read the paired peer catalog.";
        return false;
    }

    for (const std::string& object : json_objects(response.output)) {
        LanRemoteItem item;
        item.name = json_string_field(object, "name");
        item.path = json_string_field(object, "path");
        item.media_url = json_string_field(object, "media_url");
        if (item.media_url.empty()) item.media_url = json_string_field(object, "url");
        item.trust = parse_trust(json_string_field(object, "scan_status"));
        if (item.name.empty() && item.path.empty()) continue;
        if (item.name.empty()) item.name = item.path;
        items.push_back(std::move(item));
    }
    status = "Read-only LAN catalog loaded: " + std::to_string(items.size()) + " item(s).";
    return true;
}

bool LanViewerService::direct_stream_url(const LanPeer& peer,
                                         const LanRemoteItem& item,
                                         std::string& url,
                                         std::string& status) const {
    url.clear();
    if (item.trust == LanTrustState::Blocked) {
        status = "Blocked: the remote scanner marked this transfer malicious.";
        return false;
    }
    if (item.trust != LanTrustState::VerifiedClean) {
        status = "Unknown: streaming is blocked until the remote scanner reports Verified Clean.";
        return false;
    }

    const std::string source = item.media_url.empty()
        ? "/nougat/v1/media?path=" + item.path
        : item.media_url;
    url = validated_http_url(peer, source);
    if (url.empty()) {
        status = "Blocked: the media URL did not stay on the selected private-LAN peer.";
        return false;
    }
    status = "Verified Clean: direct private-LAN streaming is allowed without a Nougat proxy.";
    return true;
}

void LanViewerService::set_routing_mode(LanRoutingMode mode) {
    routing_mode_ = mode;
}

std::string LanViewerService::routing_label() const {
    return routing_mode_ == LanRoutingMode::ScannerOnly
        ? "Scanner Only"
        : "Scanner + Mullvad VPN Proxy";
}

const char* LanViewerService::trust_label(LanTrustState state) {
    switch (state) {
    case LanTrustState::VerifiedClean: return "Verified Clean";
    case LanTrustState::Blocked: return "Blocked";
    case LanTrustState::Unknown:
    default: return "Unknown";
    }
}

} // namespace reddmedia::lan
