#pragma once

#include <string>

namespace reddmedia {

struct PrivacyBrokerStatus {
    bool reachable = false;
    bool protocol_compatible = false;
    bool remote_search_ready = false;
    std::string protocol;
    std::string detail;
};

class PrivacyBrokerClient {
public:
    static constexpr int protocol_version = 1;

    explicit PrivacyBrokerClient(std::string socket_path = {});
    PrivacyBrokerStatus status() const;
    const std::string& socket_path() const { return socket_path_; }

private:
    std::string socket_path_;
    static std::string default_socket_path();
};

} // namespace reddmedia
