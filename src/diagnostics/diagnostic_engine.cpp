#include "diagnostic_engine.hpp"

#include <arpa/inet.h>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

namespace reddmedia {
namespace {

namespace fs = std::filesystem;

bool path_exists(const std::string& path) {
    std::error_code ec;
    return !path.empty() && fs::exists(path, ec);
}

bool path_readable(const std::string& path) {
    return !path.empty() && access(path.c_str(), R_OK) == 0;
}

long long file_size_or_negative(const std::string& path) {
    std::error_code ec;
    if (path.empty() || !fs::is_regular_file(path, ec)) return -1;
    const auto size = fs::file_size(path, ec);
    return ec ? -1 : static_cast<long long>(size);
}

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string trim_copy(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) value.erase(value.begin());
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
    return value;
}

std::string read_file_limited(const std::string& path, std::size_t max_bytes = 262144U) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    std::string out;
    out.resize(max_bytes);
    input.read(out.data(), static_cast<std::streamsize>(out.size()));
    out.resize(static_cast<std::size_t>(input.gcount()));
    return out;
}

std::string os_name() {
    std::ifstream input("/etc/os-release");
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("PRETTY_NAME=", 0U) != 0U) continue;
        std::string value = line.substr(12);
        if (value.size() >= 2U && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2U);
        }
        return value;
    }
    return "Unknown";
}

std::string kernel_name() {
    utsname data {};
    if (uname(&data) != 0) return "Unknown";
    return std::string(data.sysname) + " " + data.release + " " + data.machine;
}

std::string cpu_name() {
    std::ifstream input("/proc/cpuinfo");
    std::string line;
    while (std::getline(input, line)) {
        const auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        const std::string key = trim_copy(line.substr(0, pos));
        if (key == "model name" || key == "Hardware") return trim_copy(line.substr(pos + 1));
    }
    return "Unknown";
}

long long meminfo_kb(const char* wanted) {
    std::ifstream input("/proc/meminfo");
    std::string key;
    long long value = 0;
    std::string unit;
    while (input >> key >> value >> unit) {
        if (!key.empty() && key.back() == ':') key.pop_back();
        if (key == wanted) return value;
    }
    return -1;
}

std::string load_average() {
    std::ifstream input("/proc/loadavg");
    std::string a, b, c;
    if (!(input >> a >> b >> c)) return "Unknown";
    return a + " / " + b + " / " + c;
}

std::string human_bytes(long long bytes) {
    if (bytes < 0) return "Unknown";
    static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) { value /= 1024.0; ++unit; }
    std::ostringstream out;
    out << std::fixed << std::setprecision(unit == 0 ? 0 : 2) << value << ' ' << units[unit];
    return out.str();
}

std::string disk_free_text(const std::string& path) {
    struct statvfs stat {};
    const std::string probe = path.empty() ? "/" : path;
    if (statvfs(probe.c_str(), &stat) != 0) return "Unknown";
    const long long bytes = static_cast<long long>(stat.f_bavail) * static_cast<long long>(stat.f_frsize);
    return human_bytes(bytes);
}

std::pair<bool, long long> probe_port_8096() {
    const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor < 0) return {false, -1};
    timeval timeout {0, 300000};
    setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(8096);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    const auto start = std::chrono::steady_clock::now();
    const bool open = connect(descriptor, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
    const auto stop = std::chrono::steady_clock::now();
    close(descriptor);
    const long long usec = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
    return {open, usec};
}

std::string git_head_from_project(const std::string& project_root) {
    if (project_root.empty()) return "Unknown";
    const fs::path git = fs::path(project_root) / ".git";
    std::ifstream head(git / "HEAD");
    std::string value;
    if (!std::getline(head, value)) return "Unknown";
    value = trim_copy(value);
    if (value.rfind("ref: ", 0U) == 0U) {
        std::ifstream ref(git / value.substr(5));
        if (std::getline(ref, value)) return trim_copy(value);
        return "Unknown";
    }
    return value.empty() ? "Unknown" : value;
}

void add_fact(DiagnosticReport& report, std::string section, std::string name,
              std::string value, std::string evidence) {
    report.facts.push_back({std::move(section), std::move(name), std::move(value), std::move(evidence)});
}

void add_issue(DiagnosticReport& report,
               DiagnosticSeverity severity,
               std::string code,
               std::string title,
               std::string detail,
               std::string action) {
    report.issues.push_back({std::move(code), std::move(title), std::move(detail),
                             std::move(action), severity});
    if (severity == DiagnosticSeverity::Critical) report.overall = DiagnosticSeverity::Critical;
    else if (severity == DiagnosticSeverity::Warning && report.overall != DiagnosticSeverity::Critical) {
        report.overall = DiagnosticSeverity::Warning;
    }
}

std::string duration_text(long long ms) {
    if (ms < 0) return "Unknown";
    const long long total_seconds = ms / 1000;
    const long long hours = total_seconds / 3600;
    const long long minutes = (total_seconds % 3600) / 60;
    const long long seconds = total_seconds % 60;
    std::ostringstream output;
    if (hours > 0) output << hours << ':' << std::setw(2) << std::setfill('0') << minutes;
    else output << minutes;
    output << ':' << std::setw(2) << std::setfill('0') << seconds;
    return output.str();
}

std::string time_text(long long value) {
    const std::time_t timestamp = static_cast<std::time_t>(value);
    std::tm local {};
    localtime_r(&timestamp, &local);
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char c : value) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 0x20U) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c)
                    << std::dec << std::setfill(' ');
            } else out << static_cast<char>(c);
        }
    }
    return out.str();
}

std::string redact_sensitive_text(const std::string& input) {
    std::istringstream lines(input);
    std::ostringstream out;
    std::string line;
    while (std::getline(lines, line)) {
        const std::string lower = lower_copy(line);
        const bool sensitive = lower.find("authorization") != std::string::npos ||
            lower.find("bearer ") != std::string::npos ||
            lower.find("access_token") != std::string::npos ||
            lower.find("x-emby-token") != std::string::npos ||
            lower.find("api_key") != std::string::npos ||
            lower.find("apikey") != std::string::npos ||
            lower.find("api key") != std::string::npos ||
            lower.find("password") != std::string::npos ||
            lower.find("cookie") != std::string::npos ||
            lower.find("secret") != std::string::npos;
        if (sensitive) out << "[REDACTED SENSITIVE LINE]";
        else out << line;
        out << '\n';
    }
    return out.str();
}

bool write_file(const std::string& path, const std::string& bytes, std::string& error) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) { error = "Could not open diagnostic output: " + path; return false; }
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!output.good()) { error = "Could not write diagnostic output: " + path; return false; }
    return true;
}

std::vector<fs::path> recent_log_files(const std::string& log_path) {
    std::vector<fs::path> files;
    std::error_code ec;
    const fs::path root(log_path);
    if (fs::is_regular_file(root, ec)) files.push_back(root);
    else if (fs::is_directory(root, ec)) {
        for (const auto& entry : fs::directory_iterator(root, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        std::error_code ea, eb;
        return fs::last_write_time(a, ea) > fs::last_write_time(b, eb);
    });
    if (files.size() > 4U) files.resize(4U);
    return files;
}

bool create_tar_gz(const fs::path& directory, const std::string& archive_path, std::string& error) {
    const pid_t pid = fork();
    if (pid < 0) { error = "Could not start tar for the diagnostic support bundle."; return false; }
    if (pid == 0) {
        execlp("tar", "tar", "-czf", archive_path.c_str(), "-C", directory.c_str(), ".",
               static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        error = "tar could not create the diagnostic support bundle.";
        return false;
    }
    return true;
}

} // namespace

DiagnosticReport DiagnosticEngine::evaluate(const DiagnosticInput& input) const {
    DiagnosticReport report;
    report.checked_at = static_cast<long long>(std::time(nullptr));
    report.overall = DiagnosticSeverity::Information;

    const auto port_probe = probe_port_8096();
    report.port_8096_open = port_probe.first;
    report.port_8096_connect_us = port_probe.second;

    add_fact(report, "Nougat", "Version", input.app_version.empty() ? "Unknown" : input.app_version,
             "Runtime version constant reported by the active executable.");
    add_fact(report, "Nougat", "Executable", input.executable_path.empty() ? "Unknown" : input.executable_path,
             "Resolved running executable path.");
    add_fact(report, "Nougat", "Git HEAD", git_head_from_project(input.project_root),
             "Read from the project .git/HEAD when the working tree is available.");
    add_fact(report, "Nougat", "Current view", input.current_view.empty() ? "Unknown" : input.current_view,
             "Current in-process UI state.");

    add_fact(report, "System", "Operating system", os_name(), "/etc/os-release PRETTY_NAME.");
    add_fact(report, "System", "Kernel", kernel_name(), "uname(2).");
    add_fact(report, "System", "CPU", cpu_name(), "/proc/cpuinfo.");
    const long long total_kb = meminfo_kb("MemTotal");
    const long long avail_kb = meminfo_kb("MemAvailable");
    add_fact(report, "System", "RAM total", total_kb < 0 ? "Unknown" : human_bytes(total_kb * 1024LL), "/proc/meminfo.");
    add_fact(report, "System", "RAM available", avail_kb < 0 ? "Unknown" : human_bytes(avail_kb * 1024LL), "/proc/meminfo.");
    add_fact(report, "System", "Load average", load_average(), "/proc/loadavg 1/5/15 minute values.");
    add_fact(report, "System", "Free disk", disk_free_text(input.project_root), "statvfs on the Nougat project filesystem.");

    add_fact(report, "Server", "Port 8096", report.port_8096_open ? "Open" : "Closed",
             "Direct TCP connect to 127.0.0.1:8096.");
    add_fact(report, "Server", "Port connect latency",
             report.port_8096_connect_us < 0 ? "Unknown" : std::to_string(report.port_8096_connect_us) + " us",
             "Measured during this diagnostic run.");
    add_fact(report, "Server", "Ownership", input.server_owned ? "Nougat-owned" : "Not Nougat-owned",
             "MediaServerManager ownership state.");
    add_fact(report, "Server", "Jellyfin health API", input.server_api_ready ? "Ready" : "Not ready",
             "MediaServerManager health probe state.");
    add_fact(report, "Server", "Runtime", input.runtime_path, "Configured bundled runtime path.");
    add_fact(report, "Server", "Data", input.data_path, "Configured persistent data path.");
    add_fact(report, "Server", "Config", input.config_path, "Configured persistent config path.");
    add_fact(report, "Server", "Cache", input.cache_path, "Configured persistent cache path.");
    add_fact(report, "Server", "Logs", input.log_path, "Configured Jellyfin log path.");

    if (input.server_busy || input.server_state == MediaServerState::Starting) {
        add_issue(report, DiagnosticSeverity::Warning, "SERVER_TRANSITION",
                  "Integrated server is not ready yet",
                  "Nougat is starting, stopping, refreshing, or waiting for the server API.",
                  "Wait briefly, then use Refresh Server if the state does not change.");
    } else if (input.server_state == MediaServerState::RuntimeMissing) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_RUNTIME_MISSING",
                  "Integrated server runtime is missing", input.runtime_path,
                  "Restore the generated Jellyfin runtime, then press Start Server.");
    } else if (input.server_state == MediaServerState::Fault) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_FAULT",
                  "Integrated server failed",
                  "The Jellyfin process exited or its health endpoint did not become ready.",
                  "Open Logs, correct the reported failure, and press Start Server.");
    } else if (input.server_state == MediaServerState::Stopped) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_STOPPED",
                  "Integrated server is stopped",
                  "Port 8096 and the Jellyfin API are not available to Nougat.",
                  "Press Start Server.");
    } else if (!input.server_api_ready || !report.port_8096_open) {
        add_issue(report, DiagnosticSeverity::Critical, "SERVER_API_UNREACHABLE",
                  "Server process is present but its API is unreachable",
                  "Nougat cannot confirm a healthy local server response on port 8096.",
                  "Press Refresh Server and inspect the server log if it remains unavailable.");
    }

    if (!path_exists(input.runtime_path)) {
        add_issue(report, DiagnosticSeverity::Critical, "RUNTIME_PATH_MISSING",
                  "Jellyfin runtime path is missing", input.runtime_path,
                  "Restore the generated runtime from the last working build.");
    }

    std::size_t movies = 0, collections = 0, series = 0, seasons = 0, episodes = 0;
    std::size_t missing_posters = 0, missing_overviews = 0, missing_episode_titles = 0;
    std::size_t missing_episode_numbers = 0, unreadable_paths = 0;
    for (const LibraryNode& node : input.library_nodes) {
        switch (node.kind) {
        case LibraryNodeKind::Movie: ++movies; break;
        case LibraryNodeKind::MovieCollection: ++collections; break;
        case LibraryNodeKind::Series: ++series; break;
        case LibraryNodeKind::Season: ++seasons; break;
        case LibraryNodeKind::Episode: ++episodes; break;
        }
        if (node.poster_item_id.empty() && node.tmdb_poster_path.empty()) ++missing_posters;
        if (node.overview.empty()) ++missing_overviews;
        if (node.kind == LibraryNodeKind::Episode) {
            if (node.episode_title.empty()) ++missing_episode_titles;
            if (node.season_number <= 0 || node.episode_number <= 0) ++missing_episode_numbers;
        }
        if (!node.path.empty() && !path_readable(node.path)) ++unreadable_paths;
    }
    add_fact(report, "Library", "Scan coverage", input.library_full_scan ? "Full catalog scan" : "Current UI sample",
             input.library_full_scan ? "Jellyfin load_all_recommendation_items succeeded." : input.library_scan_error);
    add_fact(report, "Library", "Linked folders", std::to_string(input.library_folders.size()), "Jellyfin media-folder API.");
    add_fact(report, "Library", "Movies", std::to_string(movies), "Catalog items returned to diagnostics.");
    add_fact(report, "Library", "Collections", std::to_string(collections), "Catalog items returned to diagnostics.");
    add_fact(report, "Library", "Series", std::to_string(series), "Catalog items returned to diagnostics.");
    add_fact(report, "Library", "Seasons", std::to_string(seasons), "Catalog items returned to diagnostics.");
    add_fact(report, "Library", "Episodes", std::to_string(episodes), "Catalog items returned to diagnostics.");
    if (!input.library_full_scan && !input.library_scan_error.empty()) {
        add_issue(report, DiagnosticSeverity::Warning, "LIBRARY_SCAN_INCOMPLETE",
                  "Full catalog diagnostic scan did not complete", input.library_scan_error,
                  "Refresh Server and rerun diagnostics before relying on catalog-wide counts.");
    }
    if (missing_posters > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "METADATA_POSTERS_MISSING",
                  std::to_string(missing_posters) + " catalog items have no usable poster",
                  "The diagnostic catalog sample had no Jellyfin or TMDb artwork reference for these items.",
                  "Use Refresh Metadata and inspect the affected provider IDs if artwork remains missing.");
    }
    if (missing_episode_titles > 0U || missing_episode_numbers > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "EPISODE_IDENTITY_MISSING",
                  "Episode identity metadata is incomplete",
                  std::to_string(missing_episode_titles) + " titles and " +
                      std::to_string(missing_episode_numbers) + " season/episode number pairs are missing.",
                  "Use Refresh Metadata; Nougat will keep unavailable fields explicit instead of guessing.");
    }
    if (missing_overviews > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "METADATA_OVERVIEWS_MISSING",
                  std::to_string(missing_overviews) + " catalog items have no description",
                  "The local catalog did not return an overview for these items.",
                  "Use Refresh Metadata and verify that the title has a valid provider ID.");
    }
    if (unreadable_paths > 0U) {
        add_issue(report, DiagnosticSeverity::Critical, "MEDIA_PATH_UNREADABLE",
                  std::to_string(unreadable_paths) + " cataloged media paths are unreadable",
                  "The catalog points to files Nougat cannot currently read.",
                  "Check that the drive is mounted and that your account has read permission.");
    }
    if (input.poster_failures > 0U) {
        add_issue(report, DiagnosticSeverity::Warning, "POSTER_REQUEST_FAILURE",
                  std::to_string(input.poster_failures) + " poster requests failed",
                  "Cached artwork may remain available, but one or more current requests failed.",
                  "Retry after checking the server and network connection.");
    }

    add_fact(report, "Playback", "libVLC", input.vlc_loaded ? (input.vlc_version.empty() ? "Loaded" : input.vlc_version) : "Unavailable",
             input.vlc_loaded ? "Loaded libVLC runtime." : input.vlc_error);
    add_fact(report, "Playback", "State", input.playback_state.empty() ? "Idle" : input.playback_state,
             "Current libVLC/player state captured by Nougat.");
    add_fact(report, "Playback", "Media", input.playback_path.empty() ? "None" : input.playback_path,
             "Current native-player media path/location.");
    add_fact(report, "Playback", "Position", duration_text(input.playback_position_ms) + " / " + duration_text(input.playback_length_ms),
             "Current cached/libVLC playback timing.");
    add_fact(report, "Playback", "Volume", std::to_string(input.volume_percent) + "%", "Nougat native-player volume state (0-200%).");
    add_fact(report, "Playback", "TV autoplay", input.tv_autoplay_armed ? "Armed" : "Not armed",
             "Current episode queue state.");
    if (input.up_next_visible) {
        add_fact(report, "Playback", "Up Next", input.up_next_title,
                 input.up_next_seconds >= 0 ? "Visible countdown: " + std::to_string(input.up_next_seconds) + " seconds." : "Visible without countdown.");
    }
    if (input.vlc_probe_attempted && !input.vlc_loaded) {
        add_issue(report, DiagnosticSeverity::Critical, "VLC_UNAVAILABLE", "Native playback runtime is unavailable",
                  input.vlc_error.empty() ? "libVLC did not load." : input.vlc_error,
                  "Install/restore VLC and reopen Nougat.");
    }

    const std::string search_db = input.search_data_dir.empty() ? std::string() : (fs::path(input.search_data_dir) / "nougat.db").string();
    add_fact(report, "Search", "Data directory", input.search_data_dir.empty() ? "Unknown" : input.search_data_dir,
             "NougatBridge data_directory().");
    add_fact(report, "Search", "FTS database", search_db.empty() ? "Unknown" : (path_exists(search_db) ? "Present" : "Missing"),
             search_db.empty() ? "No data directory was reported." : search_db);
    add_fact(report, "Search", "Database size", human_bytes(file_size_or_negative(search_db)), "Filesystem size of nougat.db.");
    add_fact(report, "Search", "Node", input.search_node_running ? "Running" : "Stopped", "NougatBridge node process state.");
    add_fact(report, "Search", "Node ID", input.search_node_id.empty() ? "Unknown" : input.search_node_id,
             input.search_probe_error.empty() ? "Nougat engine node-id probe." : input.search_probe_error);
    add_fact(report, "Search", "Peers", std::to_string(input.search_peer_count), "Nougat peer registry probe.");
    if (!input.search_data_dir.empty() && !path_exists(search_db)) {
        add_issue(report, DiagnosticSeverity::Warning, "SEARCH_DB_MISSING", "Search database is not present",
                  search_db, "Run a crawl or Search operation to initialize the local Search database.");
    }
    if (!input.search_probe_error.empty()) {
        add_issue(report, DiagnosticSeverity::Warning, "SEARCH_PROBE_INCOMPLETE", "Search diagnostic probe was incomplete",
                  input.search_probe_error, "Open Search and retry diagnostics after the Search engine responds normally.");
    }

    add_fact(report, "P2P", "libtorrent", input.p2p_version.empty() ? "Unknown" : input.p2p_version,
             "Existing Nougat P2P engine runtime version.");
    add_fact(report, "P2P", "Transfer state", input.p2p_active ? (input.p2p_state.empty() ? "Active" : input.p2p_state) : "Idle",
             "Current P2P engine status snapshot.");
    if (input.p2p_active) {
        add_fact(report, "P2P", "Transfer", input.p2p_name.empty() ? "Unnamed" : input.p2p_name, "Current P2P handle.");
        add_fact(report, "P2P", "Progress", std::to_string(static_cast<int>(input.p2p_progress * 100.0f + 0.5f)) + "%", "Current torrent status.");
        add_fact(report, "P2P", "Peers / seeds", std::to_string(input.p2p_peers) + " / " + std::to_string(input.p2p_seeds), "Current torrent status.");
        add_fact(report, "P2P", "Down / up", std::to_string(input.p2p_download_rate) + " B/s / " + std::to_string(input.p2p_upload_rate) + " B/s", "Current payload rates.");
        add_fact(report, "P2P", "Selected media", std::to_string(static_cast<int>(input.p2p_selected_progress * 100.0f + 0.5f)) + "%", "Pieces belonging to the selected media file currently present.");
        add_fact(report, "P2P", "Start buffer", human_bytes(static_cast<long long>(input.p2p_selected_buffered_bytes)), "Contiguous selected-media bytes available from the beginning.");
        add_fact(report, "P2P", "Native stream bridge", input.p2p_stream_running ? "Running" : "Idle", "Loopback-only P2P HTTP Range bridge state.");
        if (!input.p2p_error.empty()) add_issue(report, DiagnosticSeverity::Warning, "P2P_TRANSFER_ERROR", "P2P transfer reports an error", input.p2p_error,
                                                 "Open Search > P2P and inspect or restart the affected transfer.");
    }

    const long long model_size = file_size_or_negative(input.ai_model_path);
    add_fact(report, "AI", "Model", input.ai_model_path.empty() ? "Unknown" : input.ai_model_path, "Pinned model path.");
    add_fact(report, "AI", "Model size", human_bytes(model_size), "Filesystem size of the pinned model.");
    add_fact(report, "AI", "Model SHA-256", input.ai_model_sha256.empty() ? "Unknown" : input.ai_model_sha256,
             "sha256sum executed during this diagnostic capture when available.");
    add_fact(report, "AI", "Runtime", input.ai_runtime_path.empty() ? "Unknown" : input.ai_runtime_path, "Pinned llama.cpp runtime path.");
    add_fact(report, "TMDb", "Credential", input.tmdb_configured ? "Configured" : "Not configured",
             "Only credential presence is reported; the credential itself is never exported.");
    add_fact(report, "TMDb", "Last status", input.tmdb_status.empty() ? "Unknown" : input.tmdb_status,
             "Current Discover/TMDb status text with credentials excluded.");
    if (!path_exists(input.ai_model_path) || !path_readable(input.ai_model_path)) {
        add_issue(report, DiagnosticSeverity::Warning, "AI_MODEL_MISSING", "Local recommendation model is unavailable",
                  input.ai_model_path, "Restore the pinned Nomic model before using Usual recommendations.");
    }
    if (!path_exists(input.ai_runtime_path)) {
        add_issue(report, DiagnosticSeverity::Warning, "AI_RUNTIME_MISSING", "Local AI runtime is unavailable",
                  input.ai_runtime_path, "Restore the generated llama.cpp runtime.");
    }
    if (!input.tmdb_configured) {
        add_issue(report, DiagnosticSeverity::Information, "TMDB_NOT_CONFIGURED", "TMDb is not configured",
                  "External recommendations, watch-provider availability, and TMDb metadata fallback need a valid credential.",
                  "Use Save / Replace on Discover when you want those features.");
    }

    add_fact(report, "Stream", "Provider", input.stream_provider.empty() ? "Unknown" : input.stream_provider,
             "Current Stream provider selection.");
    add_fact(report, "Stream", "yt-dlp", input.stream_engine_version.empty() ? (path_exists(input.stream_engine_path) ? "Present" : "Missing") : input.stream_engine_version,
             input.stream_engine_path.empty() ? "No Stream engine path reported." : input.stream_engine_path);
    add_fact(report, "Stream", "Process", input.stream_process_running ? "Running" : "Idle", "Current Stream feeder/download process state.");
    add_fact(report, "Stream", "Status", input.stream_status.empty() ? "Unknown" : input.stream_status, "Current Stream status text.");
    if (!input.stream_engine_path.empty() && !path_exists(input.stream_engine_path)) {
        add_issue(report, DiagnosticSeverity::Warning, "STREAM_ENGINE_MISSING", "Stream engine is unavailable",
                  input.stream_engine_path, "Restore the bundled yt-dlp Stream engine.");
    }

    if (!input.active_operation.empty()) {
        add_issue(report, DiagnosticSeverity::Information, "BACKGROUND_OPERATION", "Background work is active",
                  input.active_operation, "Allow the progress bar to finish before judging the final state.");
    }

    bool has_warning_or_critical = false;
    for (const auto& issue : report.issues) {
        if (issue.severity == DiagnosticSeverity::Warning || issue.severity == DiagnosticSeverity::Critical) {
            has_warning_or_critical = true;
            break;
        }
    }
    if (!has_warning_or_critical) {
        add_issue(report, DiagnosticSeverity::Information, "SYSTEM_HEALTHY",
                  "No active problems were detected by the checks that ran",
                  "Nougat recorded current process, runtime, filesystem, catalog, playback, Search, P2P, AI, Stream, and server evidence.",
                  "No action is required for the checks that returned healthy evidence.");
    }
    return report;
}

const char* DiagnosticEngine::severity_name(DiagnosticSeverity severity) {
    switch (severity) {
    case DiagnosticSeverity::Unknown: return "Unknown";
    case DiagnosticSeverity::Information: return "Green";
    case DiagnosticSeverity::Warning: return "Yellow";
    case DiagnosticSeverity::Critical: return "Red";
    }
    return "Unknown";
}

std::string DiagnosticEngine::report_text(const DiagnosticReport& report, const DiagnosticInput& input) {
    (void)input;
    std::ostringstream output;
    output << "Nougat Media Suite Diagnostic Report\n";
    output << "Checked: " << time_text(report.checked_at) << '\n';
    output << "Overall: " << severity_name(report.overall) << "\n\n";

    std::string section;
    for (const DiagnosticFact& fact : report.facts) {
        if (fact.section != section) {
            section = fact.section;
            output << "== " << section << " ==\n";
        }
        output << fact.name << ": " << fact.value << '\n';
        if (!fact.evidence.empty()) output << "  Evidence: " << fact.evidence << '\n';
    }
    output << "\n== Findings ==\n";
    for (const DiagnosticIssue& issue : report.issues) {
        output << '[' << severity_name(issue.severity) << "] " << issue.title << '\n';
        output << issue.detail << '\n';
        output << "Action: " << issue.action << "\n\n";
    }
    output << "Privacy: credentials, authorization headers, cookies, passwords, and API keys are not intentionally included.\n";
    return output.str();
}

std::string DiagnosticEngine::report_json(const DiagnosticReport& report, const DiagnosticInput& input) {
    (void)input;
    std::ostringstream out;
    out << "{\n";
    out << "  \"report\": \"Nougat Media Suite Diagnostic Report\",\n";
    out << "  \"checked\": \"" << json_escape(time_text(report.checked_at)) << "\",\n";
    out << "  \"overall\": \"" << severity_name(report.overall) << "\",\n";
    out << "  \"facts\": [\n";
    for (std::size_t i = 0; i < report.facts.size(); ++i) {
        const auto& fact = report.facts[i];
        out << "    {\"section\":\"" << json_escape(fact.section)
            << "\",\"name\":\"" << json_escape(fact.name)
            << "\",\"value\":\"" << json_escape(fact.value)
            << "\",\"evidence\":\"" << json_escape(fact.evidence) << "\"}";
        out << (i + 1U == report.facts.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"findings\": [\n";
    for (std::size_t i = 0; i < report.issues.size(); ++i) {
        const auto& issue = report.issues[i];
        out << "    {\"severity\":\"" << severity_name(issue.severity)
            << "\",\"code\":\"" << json_escape(issue.code)
            << "\",\"title\":\"" << json_escape(issue.title)
            << "\",\"detail\":\"" << json_escape(issue.detail)
            << "\",\"action\":\"" << json_escape(issue.action) << "\"}";
        out << (i + 1U == report.issues.size() ? "\n" : ",\n");
    }
    out << "  ]\n}\n";
    return out.str();
}

bool DiagnosticEngine::write_text_report(const DiagnosticReport& report, const DiagnosticInput& input,
                                         const std::string& path, std::string& error) {
    return write_file(path, report_text(report, input), error);
}

bool DiagnosticEngine::write_json_report(const DiagnosticReport& report, const DiagnosticInput& input,
                                         const std::string& path, std::string& error) {
    return write_file(path, report_json(report, input), error);
}

bool DiagnosticEngine::write_support_bundle(const DiagnosticReport& report, const DiagnosticInput& input,
                                            const std::string& archive_path, std::string& error) {
    std::error_code ec;
    const fs::path temp = fs::temp_directory_path(ec) /
        ("nougat-diagnostic-support-" + std::to_string(static_cast<long long>(getpid())) + "-" +
         std::to_string(report.checked_at));
    if (ec || !fs::create_directories(temp / "logs", ec)) {
        error = "Could not create the temporary diagnostic support-bundle directory.";
        return false;
    }

    bool ok = write_file((temp / "report.txt").string(), report_text(report, input), error) &&
              write_file((temp / "report.json").string(), report_json(report, input), error);
    if (ok) {
        const std::string readme =
            "Nougat Media Suite diagnostic support bundle\n"
            "Generated from evidence gathered by the Debug tab.\n"
            "Sensitive log lines containing credentials, authorization headers, cookies, passwords, tokens, or API keys are redacted.\n";
        ok = write_file((temp / "README.txt").string(), readme, error);
    }
    if (ok) {
        int index = 0;
        for (const fs::path& source : recent_log_files(input.log_path)) {
            std::string bytes = read_file_limited(source.string());
            if (bytes.empty()) continue;
            bytes = redact_sensitive_text(bytes);
            const std::string name = "log_" + std::to_string(++index) + "_" + source.filename().string() + ".txt";
            if (!write_file((temp / "logs" / name).string(), bytes, error)) { ok = false; break; }
        }
    }
    if (ok) ok = create_tar_gz(temp, archive_path, error);
    fs::remove_all(temp, ec);
    return ok;
}

} // namespace reddmedia
