#include "lan_media_service.hpp"
#include <cstdlib>

namespace reddmedia::lan {

LanMediaService::LanMediaService() {
    status_.endpoints = {
        {"health", "/nougat/v1/health", "LAN service health and version contract"},
        {"catalog", "/nougat/v1/catalog", "Local Library catalog and collection metadata"},
        {"history", "/nougat/v1/history", "Local viewing history and exact resume state"},
        {"artwork", "/nougat/v1/artwork", "Local poster, backdrop, channel and game artwork"},
        {"media", "/nougat/v1/media", "Direct byte-range delivery of locally owned media"},
        {"hls", "/nougat/v1/hls", "Versioned HLS/transcoding surface for browser-incompatible media"},
        {"livetv", "/nougat/v1/live-tv", "Local Live TV channel, guide and stream endpoint foundation"},
        {"devices", "/nougat/v1/devices", "Paired LAN device/session inventory"},
        {"session", "/nougat/v1/session", "Versioned playback/session state"},
        {"pair", "/nougat/v1/pair", "Local pairing/PIN authentication surface"},
        {"web", "/", "Local phone/tablet/laptop/TV browser UI surface"},
    };
}

void LanMediaService::prepare() {
    // v0.0.46 establishes the LAN contract without opening any WAN-facing socket.
    status_.prepared = true;
}

bool LanMediaService::is_private_lan_address(const std::string& address) {
    if (address == "127.0.0.1" || address == "::1") return true;
    if (address.rfind("10.", 0U) == 0U) return true;
    if (address.rfind("192.168.", 0U) == 0U) return true;
    if (address.rfind("169.254.", 0U) == 0U) return true;
    if (address.rfind("fc", 0U) == 0U || address.rfind("fd", 0U) == 0U ||
        address.rfind("fe80:", 0U) == 0U) return true;
    if (address.rfind("172.", 0U) == 0U) {
        const std::size_t second_dot = address.find('.', 4U);
        if (second_dot != std::string::npos) {
            const int second = std::atoi(address.substr(4U, second_dot - 4U).c_str());
            return second >= 16 && second <= 31;
        }
    }
    return false;
}

}  // namespace reddmedia::lan
