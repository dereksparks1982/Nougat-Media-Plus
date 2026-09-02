#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace reddmedia::lan {

struct LanEndpoint {
    std::string name;
    std::string path;
    std::string purpose;
};

struct LanMediaStatus {
    bool prepared = false;
    bool serving = false;
    bool wan_independent = true;
    bool cloud_login_required = false;
    bool automatic_upnp = false;
    bool automatic_cloud_relay = false;
    bool pairing_required = false;
    bool private_lan_only = true;
    std::string discovery_name = "nougat.local";
    std::uint16_t preferred_port = 8096;
    std::vector<std::string> access_urls;
    std::string message;
    std::vector<LanEndpoint> endpoints;
};

class LanMediaService {
public:
    LanMediaService();
    ~LanMediaService();

    LanMediaService(const LanMediaService&) = delete;
    LanMediaService& operator=(const LanMediaService&) = delete;

    void prepare();
    void stop();
    const LanMediaStatus& status() const { return status_; }
    static bool is_private_lan_address(const std::string& address);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    LanMediaStatus status_;
};

}  // namespace reddmedia::lan
