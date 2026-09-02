#pragma once

#include <string>
#include <sys/types.h>
#include <vector>

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
    ~MediaServerManager();

    // v0.0.33 persistent lifecycle: Start makes the Nougat-owned server
    // independent of the desktop UI. It remains alive until Stop Server.
    void start();
    void stop();
    void refresh();
    bool poll();
    std::string status_label() const;
    MediaServerState state() const;
    bool owns_server() const;
    bool probe_health() const;
    bool persistent_enabled() const;
    const std::string& runtime_path() const;
    const std::string& data_path() const;
    const std::string& config_path() const;
    const std::string& cache_path() const;
    const std::string& log_path() const;

private:
    bool health_ready() const;
    bool backend_health_ready() const;
    bool web_player_health_ready() const;
    bool ensure_backend_network_configuration() const;
    bool terminate_owned_stack(const std::string& token, pid_t recorded_pid);
    bool launch_runtime();
    bool adopt_owned_server();
    bool process_matches_runtime(pid_t pid) const;
    bool process_matches_nougat_signature(pid_t pid) const;
    bool process_has_owner_token(pid_t pid, const std::string& token) const;
    bool process_alive(pid_t pid) const;
    bool same_user_process(pid_t pid) const;
    std::vector<pid_t> owned_processes(const std::string& token, pid_t recorded_pid) const;
    std::string make_owner_token() const;
    void persist_owned_record(pid_t pid, const std::string& token) const;
    void clear_owned_pid() const;
    bool load_owned_record(pid_t& pid, std::string& token) const;
    void persist_enabled(bool enabled) const;
    bool load_enabled_state() const;
    void resolve_paths();

    std::string application_dir_;
    std::string runtime_path_;
    std::string web_runtime_path_;
    std::string data_path_;
    std::string config_path_;
    std::string cache_path_;
    std::string log_path_;
    std::string ownership_path_;
    std::string enabled_path_;
    std::string owned_token_;
    pid_t owned_pid_ = -1;
    MediaServerState state_ = MediaServerState::Stopped;
    bool persistent_enabled_ = true;
    long long last_poll_ms_ = 0;
};

} // namespace reddmedia
