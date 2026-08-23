#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct P2PFileInfo {
    int index = -1;
    std::string path;
    std::uint64_t size = 0;
    bool video = false;
};

struct P2PTrackerInfo {
    std::string url;
    std::string message;
};

struct P2PPlusSettings {
    int download_limit_kib = 0;
    int upload_limit_kib = 0;
    double seed_ratio_limit = 0.0;
    int seed_time_limit_minutes = 0;
};

struct P2PStatus {
    bool active = false;
    bool metadata_ready = false;
    bool seeding = false;
    bool paused = false;
    float progress = 0.0f;
    std::int64_t downloaded = 0;
    std::int64_t total = 0;
    int download_rate = 0;
    int upload_rate = 0;
    int download_limit = 0;
    int upload_limit = 0;
    std::int64_t total_uploaded = 0;
    std::int64_t total_downloaded = 0;
    double share_ratio = 0.0;
    int peers = 0;
    int seeds = 0;
    int known_peers = 0;
    int known_seeds = 0;
    int tracker_complete = -1;
    int tracker_incomplete = -1;
    int uploading_peers = 0;
    float swarm_availability = -1.0f;
    bool has_incoming = false;
    bool announcing_trackers = false;
    bool announcing_dht = false;
    bool announcing_lsd = false;
    std::string name;
    std::string state;
    std::string error;
    std::string save_path;
    float selected_progress = 0.0f;
    std::uint64_t selected_size = 0;
    std::uint64_t selected_buffered_bytes = 0;
    int tracker_count = 0;
    int queue_position = -1;
};

class P2PEngine {
public:
    P2PEngine();
    ~P2PEngine();

    P2PEngine(const P2PEngine&) = delete;
    P2PEngine& operator=(const P2PEngine&) = delete;

    bool start_magnet(const std::string& uri, const std::string& save_path, std::string& error);
    bool start_torrent_file(const std::string& torrent_path, const std::string& save_path, std::string& error);
    bool restore_last(std::string& error);
    bool pause_transfer(std::string& error);
    bool resume_transfer(std::string& error);
    bool remove_transfer(std::string& error);
    bool is_paused() const;

    // P2P Plus management surface. Nougat owns this interface so libtorrent
    // remains a replaceable backend rather than leaking through the UI.
    bool set_speed_limits(int download_kib, int upload_kib, std::string& error);
    bool set_seed_rules(double ratio_limit, int time_limit_minutes, std::string& error);
    P2PPlusSettings plus_settings() const;
    bool queue_up(std::string& error);
    bool queue_down(std::string& error);
    bool force_reannounce(std::string& error);
    bool force_recheck(std::string& error);
    bool set_file_priority(int index, int priority, std::string& error);
    std::vector<P2PTrackerInfo> trackers() const;
    void enforce_seed_rules();

    void shutdown();

    P2PStatus status() const;
    std::vector<P2PFileInfo> files() const;
    bool select_file(int index, std::string& error);
    int selected_file() const;
    std::uint64_t selected_file_size() const;
    std::string selected_file_name() const;

    void prioritize_range(std::uint64_t offset, std::uint64_t length);
    void prioritize_playback_window(std::uint64_t offset);
    bool wait_for_range(std::uint64_t offset, std::uint64_t length, int timeout_ms);
    bool read_selected_range(std::uint64_t offset, char* destination, std::size_t length,
                             std::size_t& bytes_read, std::string& error) const;
    void clear_stream_priority();

    std::string libtorrent_version() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
