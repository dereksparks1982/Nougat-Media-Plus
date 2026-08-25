#pragma once
#include <cstdint>
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
    bool wan_independent = true;
    bool cloud_login_required = false;
    bool automatic_upnp = false;
    bool automatic_cloud_relay = false;
    std::string discovery_name = "nougat.local";
    std::uint16_t preferred_port = 8097;
    std::vector<LanEndpoint> endpoints;
};

class LanMediaService {
public:
    LanMediaService();
    void prepare();
    const LanMediaStatus& status() const { return status_; }
    static bool is_private_lan_address(const std::string& address);

private:
    LanMediaStatus status_;
};

}  // namespace reddmedia::lan
