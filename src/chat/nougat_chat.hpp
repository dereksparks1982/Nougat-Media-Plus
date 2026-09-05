#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace reddmedia {

struct NougatChatMessage {
    std::uint64_t id = 0;
    std::string username;
    std::string text;
    std::uint64_t unix_ms = 0;
};

enum class NougatChatConnectionState {
    Disabled,
    Offline,
    Connecting,
    Connected,
    Error
};

class NougatChatModel {
public:
    void set_enabled(bool enabled);
    bool enabled() const;

    void set_username(std::string username);
    std::string username() const;

    void set_connection_state(NougatChatConnectionState state, std::string status = {});
    NougatChatConnectionState connection_state() const;
    std::string status_text() const;

    NougatChatMessage append_local_message(std::string text, std::uint64_t unix_ms);
    std::vector<NougatChatMessage> messages() const;
    void clear_messages();

private:
    mutable std::mutex mutex_;
    bool enabled_ = true;
    std::string username_ = "Nougat User";
    NougatChatConnectionState state_ = NougatChatConnectionState::Offline;
    std::string status_ = "Chat service not connected.";
    std::uint64_t next_id_ = 1;
    std::vector<NougatChatMessage> messages_;
};

} // namespace reddmedia
