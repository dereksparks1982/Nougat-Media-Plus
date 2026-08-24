#pragma once

#include "../media_server/jellyfin_api_client.hpp"
#include "../media_server/media_server_manager.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace reddmedia {

// v0.0.39 health model. Legacy aliases are intentionally retained so older
// source-level regression lanes compile while reporting the new semantics.
enum class DiagnosticSeverity {
    NotTested = 0,
    Information = 1,
    Passed = 2,
    NeedsAttention = 3,
    Problem = 4,
    Unknown = NotTested,
    Warning = NeedsAttention,
    Critical = Problem
};

struct DiagnosticIssue {
    // v0.0.39 compatibility: rejected candidate UI calls this field "section".
    // Keep both names synchronized when findings are created/finalized.
    std::string subsystem;
    std::string section;
    std::string code;
    std::string title;
    std::string name; // rejected-v0.0.39 Diagnostic Center UI compatibility alias for title
    std::string detail;
    std::string action;
    DiagnosticSeverity severity = DiagnosticSeverity::Information;
    std::string expected;
    std::string observed;
    std::string evidence;
    long long tested_at = 0;
};

using DiagnosticCheck = DiagnosticIssue;

struct DiagnosticFact {
    std::string section;
    std::string name;
    std::string value;
    std::string evidence;
};

struct DiagnosticSubsystem {
    std::string name;
    std::string section; // rejected-v0.0.39 UI compatibility alias for name
    DiagnosticSeverity severity = DiagnosticSeverity::NotTested;
    std::string summary;
    std::size_t passed = 0;
    std::size_t needs_attention = 0;
    std::size_t problems = 0;
    std::size_t not_tested = 0;
    std::size_t information = 0;
};

struct DiagnosticTunerSnapshot {
    std::string name;
    std::string frontend_path;
    std::string demux_path;
    std::string dvr_path;
    std::string backend;
    std::string status;
    std::string delivery_systems;
    bool readable = false;
};

struct DiagnosticChannelSnapshot {
    std::string id;
    std::string name;
    std::string service;
    std::string frequency;
    int program_number = 0;
    int physical_channel = 0;
    bool logo_resolved = false;
    std::size_t guide_programs = 0;
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

    // Live TV evidence. All fields are snapshots captured by the UI thread.
    std::vector<DiagnosticTunerSnapshot> live_tv_tuners;
    std::vector<DiagnosticChannelSnapshot> live_tv_channels;
    std::string live_tv_tuner_use;
    int live_tv_selected_tuner = -1;
    bool live_tv_playback_active = false;
    std::string live_tv_current_channel;
    int live_tv_current_rf = 0;
    bool live_tv_signal_tested = false;
    bool live_tv_signal_locked = false;
    int live_tv_signal_percent = -1;
    int live_tv_quality_percent = -1;
    std::string live_tv_psip_state;
    bool live_tv_current_mux_psip_receiving = false;
    bool live_tv_guide_refresh_queued = false;
    std::string live_tv_guide_path;
    long long live_tv_guide_mtime = 0;
    std::size_t live_tv_guide_program_count = 0;

    // Rejected-v0.0.39 diagnostic-input compatibility. The already-built v39
    // main.cpp captured richer scalar Live TV evidence using these names. Keep
    // them as a supported input surface while the vector snapshots above remain
    // the canonical engine representation.
    bool playback_is_live_tv = false;
    int live_tv_tuner_count = 0;
    std::size_t live_tv_channel_count = 0;
    std::size_t live_tv_guide_channels_with_data = 0;
    long long live_tv_guide_cache_mtime = 0;
    bool live_tv_guide_refresh_busy = false;
    bool live_tv_current_mux_harvest_active = false;
    bool live_tv_full_refresh_queued = false;
    std::string live_tv_tuner_name;
    std::string live_tv_tuner_backend;
    std::string live_tv_tuner_status;
    std::string live_tv_frontend_path;
    std::string live_tv_demux_path;
    std::string live_tv_dvr_path;
    std::string live_tv_net_path;
    bool live_tv_frontend_accessible = false;
    bool live_tv_demux_accessible = false;
    bool live_tv_dvr_accessible = false;
    bool live_tv_net_accessible = false;
    bool live_tv_signal_lock = false;
    std::string live_tv_delivery_systems;
    std::string live_tv_current_station;
    std::string live_tv_current_frequency;
    int live_tv_current_program_number = 0;
    std::string live_tv_current_program_title;
    long long live_tv_current_program_start = 0;
    long long live_tv_current_program_end = 0;

    // Search subsystem evidence.
    std::string search_data_dir;
    bool search_node_running = false;
    int search_node_port = 0;
    std::string search_node_id;
    std::size_t search_peer_count = 0;
    std::string search_probe_error;
    bool search_test_requested = false;
    std::string search_status; // rejected-v0.0.39 UI/runtime snapshot

    // Existing P2P core evidence.
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
    bool ai_embedding_test_attempted = false;
    bool ai_embedding_test_passed = false;
    long long ai_embedding_test_ms = -1;
    std::string ai_embedding_test_error;

    // Stream/yt-dlp evidence.
    std::string stream_engine_path;
    std::string stream_engine_version;
    std::string stream_provider;
    std::string stream_status;
    bool stream_process_running = false;

    // Data integrity/history.
    std::string diagnostic_history_path;
    std::string active_operation;
};

struct DiagnosticReport {
    DiagnosticSeverity overall = DiagnosticSeverity::NotTested;
    std::vector<DiagnosticIssue> issues;
    std::vector<DiagnosticCheck> checks; // compatibility mirror of issues
    std::vector<DiagnosticFact> facts;
    std::vector<DiagnosticSubsystem> subsystems;
    long long checked_at = 0;
    bool port_8096_open = false;
    long long port_8096_connect_us = -1;
    std::size_t passed_count = 0;
    std::size_t needs_attention_count = 0;
    std::size_t attention_count = 0; // compatibility mirror of needs_attention_count
    std::size_t problem_count = 0;
    std::size_t not_tested_count = 0;
    std::size_t information_count = 0;
};

} // namespace reddmedia
