#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nougat::games::uo {

struct ServerState {
    long pid{-1};
    std::string token;
    std::string executable;
};

struct ServerLaunchConfig {
    std::string executable;
    std::string working_directory;
    std::string log_file;
    std::vector<std::string> arguments;
    int readiness_timeout_ms{15000};
};

struct ServerStartResult {
    bool ok{false};
    bool reused{false};
    long pid{-1};
    std::string error;
};

class ServerManager {
public:
    explicit ServerManager(std::uint16_t port = 2593);
    ~ServerManager() = default;  // Nougat-owned Sphere persists across GUI close.

    [[nodiscard]] std::string state_directory() const;
    [[nodiscard]] std::string state_file() const;
    [[nodiscard]] std::string default_server_root() const;
    [[nodiscard]] std::string resolve_server_executable() const;
    [[nodiscard]] bool ready() const;
    [[nodiscard]] bool owned_process_matches(const ServerState& state) const;
    [[nodiscard]] bool load_state(ServerState& state) const;
    bool save_state(const ServerState& state) const;
    bool clear_stale_state() const;

    ServerStartResult start_or_reuse(const ServerLaunchConfig& config) const;
    bool stop_owned(std::string& error, int timeout_ms = 5000) const;

private:
    std::uint16_t port_;
};

}  // namespace nougat::games::uo
