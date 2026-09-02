#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace reddmedia::lan {

enum class LanTrustState {
    Unknown,
    VerifiedClean,
    Blocked
};

enum class LanRoutingMode {
    ScannerOnly,
    ScannerPlusMullvadVpnProxy
};

struct LanPeer {
    std::string service_name;
    std::string host;
    std::string address;
    std::uint16_t port = 8097;
};

struct LanRemoteItem {
    std::string name;
    std::string path;
    std::string media_url;
    LanTrustState trust = LanTrustState::Unknown;
};

class LanViewerService {
public:
    LanViewerService();

    bool discover(std::vector<LanPeer>& peers, std::string& status) const;
    bool load_catalog(const LanPeer& peer,
                      std::vector<LanRemoteItem>& items,
                      std::string& status) const;

    bool direct_stream_url(const LanPeer& peer,
                           const LanRemoteItem& item,
                           std::string& url,
                           std::string& status) const;

    void set_routing_mode(LanRoutingMode mode);
    LanRoutingMode routing_mode() const { return routing_mode_; }
    std::string routing_label() const;

    static const char* trust_label(LanTrustState state);

private:
    LanRoutingMode routing_mode_ = LanRoutingMode::ScannerOnly;
};

} // namespace reddmedia::lan
