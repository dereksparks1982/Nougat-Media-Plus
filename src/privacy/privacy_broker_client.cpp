#include "privacy_broker_client.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace reddmedia {
namespace {

std::string request_line(const std::string& socket_path, const std::string& line, std::string& error) {
    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        error = std::string("socket failed: ") + std::strerror(errno);
        return {};
    }

    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    if (socket_path.size() >= sizeof(address.sun_path)) {
        close(fd);
        error = "privacy broker socket path is too long";
        return {};
    }
    std::strncpy(address.sun_path, socket_path.c_str(), sizeof(address.sun_path) - 1U);
    if (connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        error = std::string("connect failed: ") + std::strerror(errno);
        close(fd);
        return {};
    }

    std::string payload = line;
    if (payload.empty() || payload.back() != '\n') payload.push_back('\n');
    std::size_t sent = 0;
    while (sent < payload.size()) {
        const ssize_t n = write(fd, payload.data() + sent, payload.size() - sent);
        if (n > 0) sent += static_cast<std::size_t>(n);
        else if (n < 0 && errno == EINTR) continue;
        else {
            error = std::string("write failed: ") + std::strerror(errno);
            close(fd);
            return {};
        }
    }

    std::string response;
    char buffer[512];
    for (;;) {
        const ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            response.append(buffer, static_cast<std::size_t>(n));
            if (response.find('\n') != std::string::npos || response.size() >= 4096U) break;
        } else if (n == 0) break;
        else if (errno != EINTR) break;
    }
    close(fd);
    const auto newline = response.find('\n');
    if (newline != std::string::npos) response.resize(newline);
    return response;
}

} // namespace

std::string PrivacyBrokerClient::default_socket_path() {
    if (const char* runtime = std::getenv("XDG_RUNTIME_DIR")) {
        if (*runtime) return std::string(runtime) + "/nougat/privacy-broker-v1.sock";
    }
    return "/tmp/nougat-privacy-broker-v1.sock";
}

PrivacyBrokerClient::PrivacyBrokerClient(std::string socket_path)
    : socket_path_(socket_path.empty() ? default_socket_path() : std::move(socket_path)) {}

PrivacyBrokerStatus PrivacyBrokerClient::status() const {
    PrivacyBrokerStatus out;
    std::string error;
    const std::string hello = request_line(socket_path_, "HELLO 1", error);
    if (!error.empty()) {
        out.detail = "Privacy broker unavailable. Secure remote search remains fail-closed.";
        return out;
    }
    out.reachable = true;
    out.protocol_compatible = hello == "OK PrivacyBrokerProtocol/1";
    out.protocol = out.protocol_compatible ? "PrivacyBrokerProtocol/1" : hello;
    if (!out.protocol_compatible) {
        out.detail = "Privacy broker protocol mismatch. Search not sent.";
        return out;
    }

    error.clear();
    const std::string state = request_line(socket_path_, "STATUS", error);
    if (!error.empty()) {
        out.detail = "Privacy broker status unavailable. Search not sent.";
        return out;
    }
    out.remote_search_ready = state.find("remote-search-ready") != std::string::npos;
    out.detail = state.empty() ? "Privacy broker returned no status." : state;
    return out;
}

} // namespace reddmedia
