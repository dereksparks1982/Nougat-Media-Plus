#include "diagnostic_engine.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace reddmedia {
namespace {

bool path_exists(const std::string& path) {
    return !path.empty() && access(path.c_str(), F_OK) == 0;
}

bool path_readable(const std::string& path) {
    return !path.empty() && access(path.c_str(), R_OK) == 0;
}

bool port_8096_open() {
    const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor < 0) return false;
    timeval timeout {0, 200000};
    setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(8096);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    const bool open = connect(descriptor, reinterpret_cast<sockaddr*>(&address),
                              sizeof(address)) == 0;
    close(descriptor);
    return open;
}

void add_issue(DiagnosticReport& report,
               DiagnosticSeverity severity,
               std::string code,
               std::string title,
               std::string detail,
               std::string action) {
    report.issues.push_back({std::move(code), std::move(title), std::move(detail),
                             std::move(action), severity});
    if (severity == DiagnosticSeverity::Critical ||
        (severity == DiagnosticSeverity::Warning &&
         report.overall == DiagnosticSeverity::Information)) {
        report.overall = severity;
    }
}

std::string time_text(long long value) {
    const std::time_t timestamp = static_cast<std::time_t>(value);
    std::tm local {};
    localtime_r(&timestamp, &local);
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

} // namespace

DiagnosticReport DiagnosticEngine::evaluate(const DiagnosticInput& input) const {
    DiagnosticReport report;
    report.checked_at = static_cast<long long>(std::time(nullptr));
    report.port_8096_open = port_8096_open();

    if (input.server_busy || input.server_state == MediaServerState::Starting) {
        add_issue(report, DiagnosticSeverity::Warning, "SERVER_TRANSITION",
                  "Integrated server is not ready yet",
                  "ReddMedia is starting, stopping, refreshing, or waiting for the server API.",
                  "Wait briefly, then use Refresh Server if the state does not change.");
    } else if (input.server_state == MediaServerState::RuntimeMissing) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_RUNTIME_MISSING",
                  "Integrated server runtime is missing",
                  "The bundled Jellyfin executable was not found at the expected runtime path.",
                  "Restore the generated Jellyfin runtime, then press Start Server.");
    } else if (input.server_state == MediaServerState::Fault) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_FAULT",
                  "Integrated server failed",
                  "The Jellyfin process exited or its health endpoint did not become ready.",
                  "Open Logs, correct the reported failure, and press Start Server.");
    } else if (input.server_state == MediaServerState::Stopped) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_STOPPED",
                  "Integrated server is stopped",
                  "Port 8096 and the Jellyfin API are not available to ReddMedia.",
                  "Press Start Server.");
    } else if (!input.server_api_ready || !report.port_8096_open) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_API_UNREACHABLE",
                  "Server process is present but its API is unreachable",
                  "ReddMedia cannot confirm a healthy response from the local server on port 8096.",
                  "Press Refresh Server and inspect the server log if it remains unavailable.");
    }

    if (!path_exists(input.runtime_path)) {
        add_issue(report, DiagnosticSeverity::Critical, "RUNTIME_PATH_MISSING",
                  "Jellyfin runtime path is missing", input.runtime_path,
                  "Restore the generated runtime from the last working build.");
    }
    if (!path_exists(input.ai_model_path) || !path_readable(input.ai_model_path)) {
        add_issue(report, DiagnosticSeverity::Warning, "AI_MODEL_MISSING",
                  "Local recommendation model is unavailable", input.ai_model_path,
                  "Restore the pinned Nomic model before using Usual recommendations.");
    }
    if (!path_exists(input.ai_runtime_path)) {
        add_issue(report, DiagnosticSeverity::Warning, "AI_RUNTIME_MISSING",
                  "Local AI runtime is unavailable", input.ai_runtime_path,
                  "Restore the generated llama.cpp runtime.");
    }

    std::size_t missing_posters = 0;
    std::size_t missing_overviews = 0;
    std::size_t missing_episode_titles = 0;
    std::size_t missing_episode_numbers = 0;
    std::size_t unreadable_paths = 0;
    for (const LibraryNode& node : input.library_nodes) {
        if (node.poster_item_id.empty() && node.tmdb_poster_path.empty()) ++missing_posters;
        if (node.overview.empty()) ++missing_overviews;
        if (node.kind == LibraryNodeKind::Episode) {
            if (node.episode_title.empty()) ++missing_episode_titles;
            if (node.season_number <= 0 || node.episode_number <= 0) ++missing_episode_numbers;
        }
        if (!node.path.empty() && !path_readable(node.path)) ++unreadable_paths;
    }
    if (missing_posters > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "METADATA_POSTERS_MISSING",
                  std::to_string(missing_posters) + " library items have no usable poster",
                  "ReddMedia checked item, parent/series, and available TMDb artwork sources.",
                  "Use Refresh Metadata and inspect the affected provider IDs if artwork remains missing.");
    }
    if (missing_episode_titles > 0U || missing_episode_numbers > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "EPISODE_IDENTITY_MISSING",
                  "Episode identity metadata is incomplete",
                  std::to_string(missing_episode_titles) + " titles and " +
                      std::to_string(missing_episode_numbers) + " number pairs are missing.",
                  "Use Refresh Metadata; ReddMedia will keep unavailable fields explicit instead of guessing.");
    }
    if (missing_overviews > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "METADATA_OVERVIEWS_MISSING",
                  std::to_string(missing_overviews) + " library items have no description",
                  "The local catalog did not return an overview for these items.",
                  "Use Refresh Metadata and verify that the title has a valid provider ID.");
    }
    if (unreadable_paths > 0U) {
        add_issue(report, DiagnosticSeverity::Critical, "MEDIA_PATH_UNREADABLE",
                  std::to_string(unreadable_paths) + " cataloged media paths are unreadable",
                  "The catalog points to files that ReddMedia cannot currently read.",
                  "Check that the drive is mounted and that your account has read permission.");
    }
    if (input.poster_failures > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "POSTER_REQUEST_FAILURE",
                  std::to_string(input.poster_failures) + " poster requests failed",
                  "Cached artwork remains available, but one or more current requests failed.",
                  "Retry after checking the server and network connection.");
    }
    if (!input.tmdb_configured) {
        add_issue(report, DiagnosticSeverity::Information, "TMDB_NOT_CONFIGURED",
                  "TMDb is not configured",
                  "External recommendations, provider availability, and TMDb metadata fallback need a valid credential.",
                  "Use Save / Replace on Discover when you want those features.");
    }
    if (!input.active_operation.empty()) {
        add_issue(report, DiagnosticSeverity::Information, "BACKGROUND_OPERATION",
                  "Background work is active", input.active_operation,
                  "Allow the progress bar to finish before judging the final metadata state.");
    }
    if (report.issues.empty()) {
        add_issue(report, DiagnosticSeverity::Information, "SYSTEM_HEALTHY",
                  "No active problems were detected",
                  "Server, runtime, current metadata, and configured services passed their checks.",
                  "No action is required.");
    }
    return report;
}

const char* DiagnosticEngine::severity_name(DiagnosticSeverity severity) {
    switch (severity) {
    case DiagnosticSeverity::Information: return "Green";
    case DiagnosticSeverity::Warning: return "Yellow";
    case DiagnosticSeverity::Critical: return "Red";
    }
    return "Red";
}

std::string DiagnosticEngine::report_text(const DiagnosticReport& report,
                                          const DiagnosticInput& input) {
    std::ostringstream output;
    output << "ReddMedia Diagnostic Report\n";
    output << "Checked: " << time_text(report.checked_at) << '\n';
    output << "Overall: " << severity_name(report.overall) << '\n';
    output << "Port 8096: " << (report.port_8096_open ? "open" : "closed") << '\n';
    output << "Server ownership: " << (input.server_owned ? "ReddMedia-owned" : "not owned by ReddMedia") << '\n';
    output << "Runtime: " << input.runtime_path << '\n';
    output << "Data: " << input.data_path << '\n';
    output << "Config: " << input.config_path << '\n';
    output << "Cache: " << input.cache_path << '\n';
    output << "Logs: " << input.log_path << "\n\n";
    for (const DiagnosticIssue& issue : report.issues) {
        output << '[' << severity_name(issue.severity) << "] " << issue.title << '\n';
        output << issue.detail << '\n';
        output << "Action: " << issue.action << "\n\n";
    }
    return output.str();
}

} // namespace reddmedia
