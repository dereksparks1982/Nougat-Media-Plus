#pragma once

#include <string>
#include <sys/types.h>

namespace reddmedia {

enum class MediaServerState {
    Starting,
    Ready,
    Stopped,
    Fault,
    RuntimeMissing
};

class MediaServerManager {
public:
    MediaServerManager();

    void start();
    bool poll();
    std::string status_label() const;
    MediaServerState state() const;

private:
    bool health_ready() const;
    bool launch_runtime();
    void resolve_paths();

    std::string application_dir_;
    std::string runtime_path_;
    std::string data_path_;
    std::string config_path_;
    std::string cache_path_;
    std::string log_path_;
    pid_t owned_pid_ = -1;
    MediaServerState state_ = MediaServerState::Stopped;
    long long last_poll_ms_ = 0;
    long long next_restart_ms_ = 0;
};

} // namespace reddmedia
