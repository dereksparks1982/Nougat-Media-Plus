#include "nougat_chat.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace reddmedia {

namespace {
std::string trim_copy(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c){ return std::isspace(c) != 0; });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c){ return std::isspace(c) != 0; }).base();
    if (first >= last) return {};
    return std::string(first, last);
}
}

void NougatChatModel::set_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    if (!enabled_) {
        state_ = NougatChatConnectionState::Disabled;
        status_ = "Chat disabled.";
    } else if (state_ == NougatChatConnectionState::Disabled) {
        state_ = NougatChatConnectionState::Offline;
        status_ = "Chat service not connected.";
    }
}

bool NougatChatModel::enabled() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return enabled_;
}

void NougatChatModel::set_username(std::string username) {
    std::lock_guard<std::mutex> lock(mutex_);
    username = trim_copy(std::move(username));
    username_ = username.empty() ? "Nougat User" : std::move(username);
}

std::string NougatChatModel::username() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return username_;
}

void NougatChatModel::set_connection_state(NougatChatConnectionState state, std::string status) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = enabled_ ? state : NougatChatConnectionState::Disabled;
    status_ = std::move(status);
    if (status_.empty()) {
        switch (state_) {
            case NougatChatConnectionState::Disabled: status_ = "Chat disabled."; break;
            case NougatChatConnectionState::Offline: status_ = "Chat service not connected."; break;
            case NougatChatConnectionState::Connecting: status_ = "Connecting to Nougat Chat..."; break;
            case NougatChatConnectionState::Connected: status_ = "Connected to Nougat Chat."; break;
            case NougatChatConnectionState::Error: status_ = "Nougat Chat connection error."; break;
        }
    }
}

NougatChatConnectionState NougatChatModel::connection_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

std::string NougatChatModel::status_text() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

NougatChatMessage NougatChatModel::append_local_message(std::string text, std::uint64_t unix_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    NougatChatMessage message;
    message.id = next_id_++;
    message.username = username_;
    message.text = trim_copy(std::move(text));
    message.unix_ms = unix_ms;
    if (!message.text.empty()) messages_.push_back(message);
    return message;
}

std::vector<NougatChatMessage> NougatChatModel::messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
}

void NougatChatModel::clear_messages() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
}

} // namespace reddmedia
