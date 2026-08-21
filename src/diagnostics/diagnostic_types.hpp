#pragma once

#include "../media_server/jellyfin_api_client.hpp"
#include "../media_server/media_server_manager.hpp"

#include <string>
#include <vector>

namespace reddmedia {

enum class DiagnosticSeverity { Information, Warning, Critical };

struct DiagnosticIssue {
    std::string code;
    std::string title;
    std::string detail;
    std::string action;
    DiagnosticSeverity severity = DiagnosticSeverity::Information;
};

struct DiagnosticInput {
    MediaServerState server_state = MediaServerState::Stopped;
    bool server_owned = false;
    bool server_busy = false;
    bool server_api_ready = false;
    std::string runtime_path;
    std::string data_path;
    std::string config_path;
    std::string cache_path;
    std::string log_path;
    std::vector<LibraryNode> library_nodes;
    std::size_t poster_failures = 0;
    std::string library_status;
    bool tmdb_configured = false;
    std::string tmdb_status;
    std::string ai_model_path;
    std::string ai_runtime_path;
    std::string active_operation;
};

struct DiagnosticReport {
    DiagnosticSeverity overall = DiagnosticSeverity::Information;
    std::vector<DiagnosticIssue> issues;
    long long checked_at = 0;
    bool port_8096_open = false;
};

} // namespace reddmedia
