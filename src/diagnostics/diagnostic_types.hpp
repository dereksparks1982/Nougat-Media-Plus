#pragma once

#include "../media_server/jellyfin_api_client.hpp"
#include "../media_server/media_server_manager.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace reddmedia {

enum class DiagnosticSeverity { Unknown, Information, Warning, Critical };

struct DiagnosticIssue {
    std::string code;
    std::string title;
    std::string detail;
    std::string action;
    DiagnosticSeverity severity = DiagnosticSeverity::Information;
};

struct DiagnosticFact {
    std::string section;
    std::string name;
    std::string value;
    std::string evidence;
};

struct DiagnosticInput {
    // Nougat application/build identity.
    std::string app_version;
    std::string executable_path;
    std::string project_root;
    std::string current_view;

    // Integrated Jellyfin state.
    MediaServerState server_state = MediaServerState::Stopped;
    bool server_owned = false;
    bool server_busy = false;
    bool server_api_ready = false;
    std::string runtime_path;
    std::string data_path;
    std::string config_path;
    std::string cache_path;
    std::string log_path;

    // Library/catalog evidence.
    std::vector<MediaFolder> library_folders;
    std::vector<LibraryNode> library_nodes;
    bool library_full_scan = false;
    std::string library_scan_error;
    std::size_t poster_failures = 0;
    std::string library_status;

    // Native player evidence.
    bool vlc_probe_attempted = false;
    bool vlc_loaded = false;
    std::string vlc_version;
    std::string vlc_error;
    bool playback_active = false;
    std::string playback_path;
    std::string playback_state;
    long long playback_position_ms = 0;
    long long playback_length_ms = 0;
    int volume_percent = 100;
    bool tv_autoplay_armed = false;
    bool up_next_visible = false;
    int up_next_seconds = -1;
    std::string up_next_title;

    // Search subsystem evidence.
    std::string search_data_dir;
    bool search_node_running = false;
    int search_node_port = 0;
    std::string search_node_id;
    std::size_t search_peer_count = 0;
    std::string search_probe_error;

    // Existing P2P core evidence. v0.0.26 does not expand P2P functionality.
    std::string p2p_version;
    bool p2p_active = false;
    bool p2p_metadata_ready = false;
    bool p2p_seeding = false;
    bool p2p_paused = false;
    float p2p_progress = 0.0f;
    long long p2p_downloaded = 0;
    long long p2p_total = 0;
    int p2p_download_rate = 0;
    int p2p_upload_rate = 0;
    int p2p_peers = 0;
    int p2p_seeds = 0;
    std::string p2p_name;
    std::string p2p_state;
    std::string p2p_error;
    std::string p2p_save_path;
    float p2p_selected_progress = 0.0f;
    unsigned long long p2p_selected_buffered_bytes = 0;
    bool p2p_stream_running = false;

    // Local AI/TMDb evidence.
    bool tmdb_configured = false;
    std::string tmdb_status;
    std::string ai_model_path;
    std::string ai_model_sha256;
    std::string ai_runtime_path;

    // Stream/yt-dlp evidence.
    std::string stream_engine_path;
    std::string stream_engine_version;
    std::string stream_provider;
    std::string stream_status;
    bool stream_process_running = false;

    std::string active_operation;
};

struct DiagnosticReport {
    DiagnosticSeverity overall = DiagnosticSeverity::Unknown;
    std::vector<DiagnosticIssue> issues;
    std::vector<DiagnosticFact> facts;
    long long checked_at = 0;
    bool port_8096_open = false;
    long long port_8096_connect_us = -1;
};

} // namespace reddmedia
