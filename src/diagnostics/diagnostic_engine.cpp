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
#include <map>
#include <set>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
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

bool path_writable(const std::string& path) {
    if (path.empty()) return false;
    fs::path p(path);
    std::error_code ec;
    if (!fs::exists(p, ec)) p = p.parent_path();
    return !p.empty() && access(p.c_str(), W_OK) == 0;
}

long long file_size_or_negative(const std::string& path) {
    std::error_code ec;
    if (path.empty() || !fs::is_regular_file(path, ec)) return -1;
    const auto size = fs::file_size(path, ec);
    return ec ? -1 : static_cast<long long>(size);
}

long long file_mtime(const std::string& path) {
    struct stat st {};
    if (path.empty() || ::stat(path.c_str(), &st) != 0) return 0;
    return static_cast<long long>(st.st_mtime);
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
    std::string out(max_bytes, '\0');
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
    std::string key, unit;
    long long value = 0;
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

std::pair<bool, long long> probe_port_8096() {
    const int descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (descriptor < 0) return {false, -1};
    timeval timeout {0, 300000};
    (void)setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    (void)setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(8096);
    (void)inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    const auto start = std::chrono::steady_clock::now();
    const bool open = connect(descriptor, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
    const auto stop = std::chrono::steady_clock::now();
    close(descriptor);
    return {open, std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()};
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
    if (value <= 0) return "Unknown";
    const std::time_t timestamp = static_cast<std::time_t>(value);
    std::tm local {};
    localtime_r(&timestamp, &local);
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

std::string age_text(long long then, long long now) {
    if (then <= 0 || now < then) return "Unknown";
    const long long seconds = now - then;
    if (seconds < 60) return std::to_string(seconds) + " seconds";
    if (seconds < 3600) return std::to_string(seconds / 60) + " minutes";
    if (seconds < 86400) return std::to_string(seconds / 3600) + " hours";
    return std::to_string(seconds / 86400) + " days";
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
            if (c < 0x20U) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec;
            else out << static_cast<char>(c);
        }
    }
    return out.str();
}

void add_fact(DiagnosticReport& report, std::string section, std::string name,
              std::string value, std::string evidence) {
    report.facts.push_back({std::move(section), std::move(name), std::move(value), std::move(evidence)});
}

void add_finding(DiagnosticReport& report,
                 const std::string& subsystem,
                 DiagnosticSeverity severity,
                 std::string code,
                 std::string title,
                 std::string expected,
                 std::string observed,
                 std::string evidence,
                 std::string action) {
    DiagnosticIssue issue;
    issue.subsystem = subsystem;
    issue.section = subsystem;
    issue.code = std::move(code);
    issue.title = std::move(title);
    issue.name = issue.title;
    issue.detail = issue.observed = std::move(observed);
    issue.action = std::move(action);
    issue.severity = severity;
    issue.expected = std::move(expected);
    issue.evidence = std::move(evidence);
    issue.tested_at = report.checked_at;
    report.issues.push_back(std::move(issue));
}

int severity_rank(DiagnosticSeverity severity) {
    switch (severity) {
    case DiagnosticSeverity::Problem: return 4;
    case DiagnosticSeverity::NeedsAttention: return 3;
    case DiagnosticSeverity::Passed: return 2;
    case DiagnosticSeverity::NotTested: return 1;
    case DiagnosticSeverity::Information: return 0;
    default: return 1;
    }
}

void finalize_report(DiagnosticReport& report) {
    report.subsystems.clear();
    report.checks.clear();
    report.passed_count = 0;
    report.needs_attention_count = 0;
    report.attention_count = 0;
    report.problem_count = 0;
    report.not_tested_count = 0;
    report.information_count = 0;

    std::map<std::string, DiagnosticSubsystem> by_name;
    for (auto& issue : report.issues) {
        if (issue.section.empty()) issue.section = issue.subsystem.empty() ? "General" : issue.subsystem;
        if (issue.subsystem.empty()) issue.subsystem = issue.section.empty() ? "General" : issue.section;
        if (issue.title.empty() && !issue.name.empty()) issue.title = issue.name;
        if (issue.name.empty()) issue.name = issue.title;
        report.checks.push_back(issue);

        auto& subsystem = by_name[issue.subsystem.empty() ? "General" : issue.subsystem];
        subsystem.name = issue.subsystem.empty() ? "General" : issue.subsystem;
        subsystem.section = subsystem.name;
        if (severity_rank(issue.severity) > severity_rank(subsystem.severity)) subsystem.severity = issue.severity;
        switch (issue.severity) {
        case DiagnosticSeverity::Passed: ++subsystem.passed; ++report.passed_count; break;
        case DiagnosticSeverity::NeedsAttention: ++subsystem.needs_attention; ++report.needs_attention_count; break;
        case DiagnosticSeverity::Problem: ++subsystem.problems; ++report.problem_count; break;
        case DiagnosticSeverity::NotTested: ++subsystem.not_tested; ++report.not_tested_count; break;
        case DiagnosticSeverity::Information: ++subsystem.information; ++report.information_count; break;
        default: ++subsystem.not_tested; ++report.not_tested_count; break;
        }
    }
    report.attention_count = report.needs_attention_count;
    for (auto& pair : by_name) {
        auto& subsystem = pair.second;
        subsystem.section = subsystem.name;
        std::ostringstream summary;
        summary << "Passed " << subsystem.passed << " | Attention " << subsystem.needs_attention
                << " | Problems " << subsystem.problems << " | Not Tested " << subsystem.not_tested;
        subsystem.summary = summary.str();
        report.subsystems.push_back(subsystem);
    }
    if (report.problem_count > 0U) report.overall = DiagnosticSeverity::Problem;
    else if (report.needs_attention_count > 0U) report.overall = DiagnosticSeverity::NeedsAttention;
    else if (report.passed_count > 0U) report.overall = DiagnosticSeverity::Passed;
    else report.overall = DiagnosticSeverity::NotTested;
}

std::string redact_sensitive_text(const std::string& input) {
    std::istringstream lines(input);
    std::ostringstream out;
    std::string line;
    while (std::getline(lines, line)) {
        const std::string lower = lower_copy(line);
        const bool sensitive = lower.find("authorization") != std::string::npos ||
            lower.find("bearer ") != std::string::npos || lower.find("access_token") != std::string::npos ||
            lower.find("x-emby-token") != std::string::npos || lower.find("api_key") != std::string::npos ||
            lower.find("apikey") != std::string::npos || lower.find("api key") != std::string::npos ||
            lower.find("password") != std::string::npos || lower.find("cookie") != std::string::npos ||
            lower.find("secret") != std::string::npos || lower.find("token=") != std::string::npos;
        out << (sensitive ? "[REDACTED SENSITIVE LINE]" : line) << '\n';
    }
    return out.str();
}

bool write_file(const std::string& path, const std::string& bytes, std::string& error) {
    std::error_code ec;
    const fs::path target(path);
    if (!target.parent_path().empty()) fs::create_directories(target.parent_path(), ec);
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
            if (entry.is_regular_file(ec)) files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        std::error_code ea, eb;
        return fs::last_write_time(a, ea) > fs::last_write_time(b, eb);
    });
    if (files.size() > 6U) files.resize(6U);
    return files;
}

bool create_tar_gz(const fs::path& directory, const std::string& archive_path, std::string& error) {
    const pid_t pid = fork();
    if (pid < 0) { error = "Could not start tar for the diagnostic support bundle."; return false; }
    if (pid == 0) {
        execlp("tar", "tar", "-czf", archive_path.c_str(), "-C", directory.c_str(), ".", static_cast<char*>(nullptr));
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

std::string subsystem_status_text(const DiagnosticSubsystem& s) {
    return std::string(DiagnosticEngine::severity_name(s.severity)) + " - " + s.summary;
}

} // namespace

DiagnosticReport DiagnosticEngine::evaluate(const DiagnosticInput& input) const {
    DiagnosticReport report;
    report.checked_at = static_cast<long long>(std::time(nullptr));

    const auto port_probe = probe_port_8096();
    report.port_8096_open = port_probe.first;
    report.port_8096_connect_us = port_probe.second;

    add_fact(report, "Application", "Version", input.app_version.empty() ? "Unknown" : input.app_version,
             "Runtime version constant reported by the active executable.");
    add_fact(report, "Application", "Executable", input.executable_path.empty() ? "Unknown" : input.executable_path,
             "Resolved running executable path.");
    add_fact(report, "Application", "Git HEAD", git_head_from_project(input.project_root),
             "Read from project .git/HEAD when the working tree is available.");
    add_fact(report, "Application", "Current view", input.current_view.empty() ? "Unknown" : input.current_view,
             "Current in-process UI state.");

    if (!input.executable_path.empty() && path_readable(input.executable_path)) {
        add_finding(report, "Application", DiagnosticSeverity::Passed, "APP_EXECUTABLE_READABLE",
                    "Nougat executable is readable", "Running executable exists and is readable.", input.executable_path,
                    "Filesystem access(R_OK) on resolved executable.", "No action required.");
    } else {
        add_finding(report, "Application", DiagnosticSeverity::Problem, "APP_EXECUTABLE_UNREADABLE",
                    "Nougat executable path is not readable", "Running executable exists and is readable.",
                    input.executable_path.empty() ? "No executable path was reported." : input.executable_path,
                    "Filesystem access(R_OK) on resolved executable.", "Restore executable permissions or reinstall the current build.");
    }
    const bool config_writable = path_writable(input.config_path);
    add_finding(report, "Application", config_writable ? DiagnosticSeverity::Passed : DiagnosticSeverity::NeedsAttention,
                "APP_CONFIG_WRITABLE", "Configuration storage",
                "Nougat configuration storage is writable.", config_writable ? "Writable" : "Not writable",
                input.config_path, "Correct ownership/permissions on the Nougat configuration directory.");

    add_fact(report, "System", "Operating system", os_name(), "/etc/os-release PRETTY_NAME.");
    add_fact(report, "System", "Kernel", kernel_name(), "uname(2).");
    add_fact(report, "System", "CPU", cpu_name(), "/proc/cpuinfo.");
    const long long total_kb = meminfo_kb("MemTotal");
    const long long avail_kb = meminfo_kb("MemAvailable");
    add_fact(report, "System", "RAM total", total_kb < 0 ? "Unknown" : human_bytes(total_kb * 1024LL), "/proc/meminfo.");
    add_fact(report, "System", "RAM available", avail_kb < 0 ? "Unknown" : human_bytes(avail_kb * 1024LL), "/proc/meminfo.");
    add_fact(report, "System", "Load average", load_average(), "/proc/loadavg 1/5/15 minute values.");
    struct statvfs disk {};
    const std::string disk_probe = input.project_root.empty() ? "/" : input.project_root;
    if (statvfs(disk_probe.c_str(), &disk) == 0) {
        const long long free_bytes = static_cast<long long>(disk.f_bavail) * static_cast<long long>(disk.f_frsize);
        add_fact(report, "Storage", "Free disk", human_bytes(free_bytes), "statvfs on Nougat project filesystem.");
        const bool low = free_bytes < 2LL * 1024LL * 1024LL * 1024LL;
        add_finding(report, "Storage", low ? DiagnosticSeverity::NeedsAttention : DiagnosticSeverity::Passed,
                    "STORAGE_FREE_SPACE", "Project filesystem free space", "At least 2 GiB free for normal Nougat operation.",
                    human_bytes(free_bytes), "statvfs(f_bavail * f_frsize).",
                    low ? "Free disk space before scans, caches, downloads, or support bundles grow." : "No action required.");
    } else {
        add_finding(report, "Storage", DiagnosticSeverity::NotTested, "STORAGE_NOT_TESTED", "Disk-space test unavailable",
                    "Project filesystem can be queried.", "statvfs failed.", disk_probe,
                    "Check whether the project filesystem is mounted and accessible.");
    }

    add_fact(report, "Server", "Port 8096", report.port_8096_open ? "Open" : "Closed", "Direct TCP connect to 127.0.0.1:8096.");
    add_fact(report, "Server", "Port connect latency",
             report.port_8096_connect_us < 0 ? "Unknown" : std::to_string(report.port_8096_connect_us) + " us",
             "Measured during this diagnostic run.");
    add_fact(report, "Server", "Ownership", input.server_owned ? "Nougat-owned" : "Not Nougat-owned", "MediaServerManager ownership state.");
    add_fact(report, "Server", "Health API", input.server_api_ready ? "Ready" : "Not ready", "MediaServerManager API health state.");
    if (input.server_busy || input.server_state == MediaServerState::Starting) {
        const std::string transition_evidence = input.active_operation.empty()
            ? "MediaServerManager reports Starting/busy while the integrated health API has not reached Ready."
            : input.active_operation;
        add_finding(report, "Server", DiagnosticSeverity::NeedsAttention, "SERVER_TRANSITION", "Integrated server is transitioning",
                    "Server reaches a stable Ready or Stopped state.", "Server operation is still in progress.", transition_evidence,
                    "Allow the operation to finish, then rerun the check if it remains stuck."); // NOUGAT_V57_SERVER_EVIDENCE
    } else if (input.server_state == MediaServerState::RuntimeMissing || !path_exists(input.runtime_path)) {
        add_finding(report, "Server", DiagnosticSeverity::Problem, "SERVER_RUNTIME_MISSING", "Jellyfin runtime is missing",
                    "Bundled Jellyfin runtime exists and is readable.", input.runtime_path, "Runtime path filesystem probe.",
                    "Restore the generated Jellyfin runtime from the last working build.");
    } else if (input.server_state == MediaServerState::Fault) {
        add_finding(report, "Server", DiagnosticSeverity::Problem, "SERVER_FAULT", "Integrated Jellyfin server failed",
                    "Server process and API become healthy.", "MediaServerManager reports Fault.", input.log_path,
                    "Open Logs, correct the reported server failure, then use Start Server.");
    } else if (input.server_state == MediaServerState::Stopped) {
        add_finding(report, "Server", DiagnosticSeverity::NeedsAttention, "SERVER_STOPPED", "Integrated server is stopped",
                    "Server is Ready when Library/server-backed features are needed.", "Server is intentionally or currently stopped.",
                    "MediaServerManager state + direct 8096 probe.", "Press Start Server if you want Library/server-backed features. This is not a suite-wide failure by itself.");
    } else if (!input.server_api_ready || !report.port_8096_open) {
        add_finding(report, "Server", DiagnosticSeverity::Problem, "SERVER_API_UNREACHABLE", "Server API is unreachable",
                    "Running server answers its health API on localhost.", "Process state and API/port evidence disagree.",
                    "MediaServerManager API state + direct 127.0.0.1:8096 TCP probe.", "Refresh Server and inspect the Jellyfin log if it remains unavailable.");
    } else {
        add_finding(report, "Server", DiagnosticSeverity::Passed, "SERVER_HEALTHY", "Integrated server is healthy",
                    "Running server answers its local API.", "API ready and localhost port reachable.",
                    "MediaServerManager API state + direct TCP probe.", "No action required.");
    }

    std::size_t movies = 0, collections = 0, series = 0, seasons = 0, episodes = 0;
    std::size_t missing_posters = 0, missing_overviews = 0, missing_episode_titles = 0;
    std::size_t missing_episode_numbers = 0, unreadable_paths = 0;
    std::set<std::string> seen_paths;
    std::size_t duplicate_paths = 0;
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
        if (!node.path.empty()) {
            if (!path_readable(node.path)) ++unreadable_paths;
            if (!seen_paths.insert(node.path).second) ++duplicate_paths;
        }
    }
    add_fact(report, "Library", "Scan coverage", input.library_full_scan ? "Full catalog scan" : "Current UI sample",
             input.library_full_scan ? "Catalog-wide load succeeded." : input.library_scan_error);
    add_fact(report, "Library", "Linked folders", std::to_string(input.library_folders.size()), "Jellyfin media-folder API.");
    add_fact(report, "Library", "Movies", std::to_string(movies), "Recursive diagnostic catalog count.");
    add_fact(report, "Library", "Collections", std::to_string(collections), "Recursive diagnostic catalog count.");
    add_fact(report, "Library", "Series", std::to_string(series), "Recursive diagnostic catalog count.");
    add_fact(report, "Library", "Seasons", std::to_string(seasons), "Recursive diagnostic catalog count.");
    add_fact(report, "Library", "Episodes", std::to_string(episodes), "Recursive diagnostic catalog count.");
    if (!input.library_full_scan) {
        add_finding(report, "Library", DiagnosticSeverity::NotTested, "LIBRARY_FULL_SCAN_NOT_TESTED", "Full Library integrity was not tested",
                    "Catalog-wide diagnostic scan completes.", input.library_scan_error.empty() ? "Only the current UI sample was available." : input.library_scan_error,
                    "Library diagnostic input.", "Refresh Server/Library and rerun Deep Diagnostic for catalog-wide counts.");
    } else if (unreadable_paths > 0U) {
        add_finding(report, "Library", DiagnosticSeverity::NeedsAttention, "MEDIA_PATH_NOT_DIRECTLY_MOUNTED", "Catalog paths are not directly mounted in the Nougat process",
                    "Direct filesystem paths are optional when the Nougat media server can still catalog and serve the item.", std::to_string(unreadable_paths) + " catalog path(s) are not directly readable by this process.",
                    "Filesystem access(R_OK) compared with server catalog health.", "If playback also fails, reconnect/mount the source drive; otherwise server-internal paths are informational."); // NOUGAT_V58_DIAGNOSTIC_SERVER_PATHS
    } else {
        add_finding(report, "Library", DiagnosticSeverity::Passed, "LIBRARY_SOURCE_HEALTHY", "Cataloged media sources are readable",
                    "Every cataloged local media path is readable.", "No unreadable catalog paths found.",
                    "Filesystem access(R_OK) against catalog paths.", "No action required.");
    }
    if (duplicate_paths > 0U) {
        add_finding(report, "Library", DiagnosticSeverity::NeedsAttention, "LIBRARY_DUPLICATE_PATHS", "Duplicate catalog paths detected",
                    "One catalog record per local media source path.", std::to_string(duplicate_paths) + " duplicate path reference(s).",
                    "Duplicate comparison of non-empty local catalog paths.", "Refresh Library and inspect duplicate/overlapping linked folders.");
    }
    // Metadata completeness is intentionally informational. It must never poison health.
    if (missing_posters > 0U || missing_overviews > 0U || missing_episode_titles > 0U || missing_episode_numbers > 0U || input.poster_failures > 0U) {
        std::ostringstream observed;
        observed << missing_posters << " missing posters, " << missing_overviews << " missing descriptions, "
                 << missing_episode_titles << " missing episode titles, " << missing_episode_numbers
                 << " missing S/E numbers, " << input.poster_failures << " artwork fetch failures.";
        add_finding(report, "Library", DiagnosticSeverity::Information, "LIBRARY_METADATA_COMPLETENESS", "Optional metadata completeness",
                    "Metadata may be incomplete without making playback/library health fail.", observed.str(),
                    "Catalog metadata fields and artwork-request cache.", "Use Refresh Metadata only if you want to fill cosmetic/description gaps.");
    }

    add_fact(report, "Player", "libVLC", input.vlc_version.empty() ? "Unknown" : input.vlc_version, "Loaded runtime version symbol.");
    add_fact(report, "Player", "State", input.playback_state.empty() ? "Unknown" : input.playback_state, "Current libVLC player state snapshot.");
    add_fact(report, "Player", "Source", input.playback_path.empty() ? "Idle" : input.playback_path, "Current media source path/URL.");
    add_fact(report, "Player", "Position", duration_text(input.playback_position_ms), "Current libVLC playback time.");
    add_fact(report, "Player", "Length", duration_text(input.playback_length_ms), "Current libVLC media length.");
    add_fact(report, "Player", "Volume", std::to_string(input.volume_percent) + "%", "Existing 0-200% player value; diagnostics do not modify VOLUME.");
    if (input.vlc_probe_attempted && !input.vlc_loaded) {
        add_finding(report, "Player", DiagnosticSeverity::Problem, "VLC_UNAVAILABLE", "Native playback runtime is unavailable",
                    "libVLC loads successfully.", input.vlc_error.empty() ? "libVLC did not load." : input.vlc_error,
                    "Runtime load/probe result.", "Install/restore VLC and reopen Nougat.");
    } else if (input.vlc_loaded) {
        add_finding(report, "Player", DiagnosticSeverity::Passed, "VLC_READY", "Native playback runtime loaded",
                    "libVLC loads successfully.", input.vlc_version.empty() ? "Loaded" : input.vlc_version,
                    "Runtime handle and version symbol.", "No action required.");
    } else {
        add_finding(report, "Player", DiagnosticSeverity::NotTested, "VLC_NOT_TESTED", "Native playback runtime was not exercised",
                    "Quick/Deep Diagnostic explicitly probes libVLC.", "No runtime probe result was supplied.",
                    "Diagnostic input.", "Run Quick Diagnostic or Deep Diagnostic.");
    }

    // Live TV: hardware and guide evidence are independent of optional metadata.
    // The rejected v0.0.39 main.cpp already emitted a richer scalar snapshot API.
    // Normalize those scalar fields into the v39 vector model so both the existing
    // candidate and the repaired engine describe the same real tuner state.
    std::vector<DiagnosticTunerSnapshot> live_tuners = input.live_tv_tuners;
    if (live_tuners.empty() && input.live_tv_tuner_count > 0) {
        DiagnosticTunerSnapshot tuner;
        tuner.name = input.live_tv_tuner_name;
        tuner.backend = input.live_tv_tuner_backend;
        tuner.status = input.live_tv_tuner_status;
        tuner.frontend_path = input.live_tv_frontend_path;
        tuner.demux_path = input.live_tv_demux_path;
        tuner.dvr_path = input.live_tv_dvr_path;
        tuner.delivery_systems = input.live_tv_delivery_systems;
        tuner.readable = input.live_tv_frontend_accessible &&
                         (input.live_tv_demux_path.empty() || input.live_tv_demux_accessible) &&
                         (input.live_tv_dvr_path.empty() || input.live_tv_dvr_accessible);
        live_tuners.push_back(std::move(tuner));
    }

    const std::size_t live_channel_count = !input.live_tv_channels.empty()
        ? input.live_tv_channels.size() : input.live_tv_channel_count;
    const bool live_playback_active = input.live_tv_playback_active || input.playback_is_live_tv;
    const bool live_signal_tested = input.live_tv_signal_tested || input.live_tv_signal_lock ||
        input.live_tv_signal_percent >= 0 || input.live_tv_quality_percent >= 0;
    const bool live_signal_locked = input.live_tv_signal_tested ? input.live_tv_signal_locked : input.live_tv_signal_lock;
    const std::string tuner_use = !input.live_tv_tuner_use.empty() ? input.live_tv_tuner_use
        : (!input.live_tv_tuner_status.empty() ? input.live_tv_tuner_status : (live_playback_active ? "Watching" : "Unknown"));

    add_fact(report, "Live TV", "Tuner count",
             std::to_string(!live_tuners.empty() ? live_tuners.size() : static_cast<std::size_t>(std::max(0, input.live_tv_tuner_count))),
             "Detected logical DVB frontends.");
    add_fact(report, "Live TV", "Channel count", std::to_string(live_channel_count), "Persisted channel database.");
    add_fact(report, "Live TV", "Tuner use", tuner_use, "Live TV tuner ownership state.");

    if (live_tuners.empty()) {
        const auto status = live_channel_count == 0U ? DiagnosticSeverity::NotTested : DiagnosticSeverity::NeedsAttention;
        add_finding(report, "Live TV", status, "LIVE_TV_TUNER_UNAVAILABLE", "No usable TV tuner is currently detected",
                    "A logical tuner is detected when Live TV hardware is connected.", "No tuner snapshot available.",
                    "Nougat native tuner detection.", "Connect/detect the tuner if you want Live TV. This does not make unrelated Nougat features unhealthy.");
    } else {
        std::size_t unreadable_nodes = 0;
        std::size_t unreachable_network_tuners = 0;
        for (const auto& tuner : live_tuners) {
            const bool network_tuner = lower_copy(tuner.backend).find("hdhomerun") != std::string::npos;
            add_fact(report, "Live TV", network_tuner ? "Network tuner" : "Tuner",
                     tuner.name.empty() ? "Detected tuner" : tuner.name,
                     tuner.frontend_path + (tuner.status.empty() ? std::string() : " | " + tuner.status));
            add_fact(report, "Live TV", "Delivery systems", tuner.delivery_systems.empty() ? "Not reported" : tuner.delivery_systems,
                     tuner.frontend_path);
            if (network_tuner) {
                if (!tuner.readable || tuner.frontend_path.empty()) ++unreachable_network_tuners;
                continue;
            }
            for (const auto& node : {tuner.frontend_path, tuner.demux_path, tuner.dvr_path}) {
                if (!node.empty() && !path_readable(node)) ++unreadable_nodes;
            }
        }
        if (!input.live_tv_net_path.empty()) {
            add_fact(report, "Live TV", "Network node", input.live_tv_net_path,
                     input.live_tv_net_accessible ? "Accessible" : "Not accessible");
        }
        const bool tuner_access_ok = unreadable_nodes == 0U && unreachable_network_tuners == 0U;
        std::ostringstream tuner_access_observed;
        tuner_access_observed << unreadable_nodes << " unreadable DVB node(s), "
                              << unreachable_network_tuners << " unreachable network tuner(s).";
        add_finding(report, "Live TV", tuner_access_ok ? DiagnosticSeverity::Passed : DiagnosticSeverity::Problem,
                    "LIVE_TV_TUNER_ACCESS", "TV tuner access",
                    "Linux DVB nodes are readable and detected HDHomeRun network tuners answer a live status probe.",
                    tuner_access_ok ? "All reported tuner resources are accessible." : tuner_access_observed.str(),
                    "Linux /dev/dvb access plus HDHomeRun per-tuner status probes.",
                    tuner_access_ok ? "No action required." : "Check Linux DVB permissions or HDHomeRun LAN reachability, as applicable.");
    }

    if (live_signal_tested) {
        std::ostringstream observed;
        observed << (live_signal_locked ? "LOCK" : "NO LOCK");
        if (input.live_tv_signal_percent >= 0) observed << ", signal " << input.live_tv_signal_percent << '%';
        if (input.live_tv_quality_percent >= 0) observed << ", quality " << input.live_tv_quality_percent << '%';
        add_finding(report, "Live TV", live_signal_locked ? DiagnosticSeverity::Passed : DiagnosticSeverity::NeedsAttention,
                    "LIVE_TV_SIGNAL", "RF signal/lock test", "Selected RF multiplex obtains signal lock.", observed.str(),
                    "Native DVB frontend scan/tune signal snapshot.", live_signal_locked ? "No action required." : "Check antenna/feed and retry the affected RF multiplex.");
    } else {
        add_finding(report, "Live TV", DiagnosticSeverity::NotTested, "LIVE_TV_SIGNAL_NOT_TESTED", "RF signal was not sampled in this run",
                    "Deep Diagnostic samples a selected/active multiplex when safe.", "No signal snapshot supplied.",
                    "Tuner was idle or no safe RF test was requested.", "Run Deep Diagnostic while a tuner/channel is available.");
    }

    if (live_playback_active) {
        const std::string current_channel = !input.live_tv_current_channel.empty() ? input.live_tv_current_channel : input.live_tv_current_station;
        add_fact(report, "Live TV", "Current channel", current_channel.empty() ? "Unknown" : current_channel,
                 "Active native Live TV player state.");
        if (!input.live_tv_current_frequency.empty()) {
            add_fact(report, "Live TV", "Current frequency", input.live_tv_current_frequency,
                     "Active channel RF frequency metadata.");
        } else {
            add_fact(report, "Live TV", "Current RF", input.live_tv_current_rf > 0 ? std::to_string(input.live_tv_current_rf) : "Unknown",
                     "Persisted channel RF association.");
        }
        if (input.live_tv_current_program_number > 0)
            add_fact(report, "Live TV", "Program number", std::to_string(input.live_tv_current_program_number), "ATSC program number.");
        if (!input.live_tv_current_program_title.empty())
            add_fact(report, "Live TV", "Current program", input.live_tv_current_program_title,
                     time_text(input.live_tv_current_program_start) + " - " + time_text(input.live_tv_current_program_end));
        std::string psip = input.live_tv_psip_state;
        if (psip.empty() && input.live_tv_current_mux_harvest_active) psip = "Receiving PSIP from current RF multiplex during playback";
        add_fact(report, "Live TV", "Current multiplex PSIP", psip.empty() ? "Not reported" : psip,
                 "Live TV guide harvester state.");
    }

    std::size_t guide_covered = 0U;
    std::size_t logos_resolved = 0U;
    for (const auto& channel : input.live_tv_channels) {
        if (channel.guide_programs > 0U) ++guide_covered;
        if (channel.logo_resolved) ++logos_resolved;
    }
    const bool channel_detail_available = !input.live_tv_channels.empty();
    if (!channel_detail_available) guide_covered = std::min(input.live_tv_guide_channels_with_data, live_channel_count);

    if (live_channel_count > 0U) {
        add_fact(report, "Live TV", "Guide coverage", std::to_string(guide_covered) + "/" + std::to_string(live_channel_count) + " channels",
                 "Program cache grouped by logical channel ID.");

        if (channel_detail_available) {
            add_fact(report, "Live TV", "Logo mapping", std::to_string(logos_resolved) + "/" + std::to_string(live_channel_count) + " channels",
                     "Actual packaged/station-specific image resolution; text fallback is forbidden.");
            if (logos_resolved < live_channel_count) {
                add_finding(report, "Live TV", DiagnosticSeverity::NeedsAttention, "LIVE_TV_LOGO_MAPPING_INCOMPLETE", "Some channels do not resolve to real artwork",
                            "Every channel icon slot resolves to correct real station/network artwork.",
                            std::to_string(live_channel_count - logos_resolved) + " channel(s) have no resolved real image.",
                            "Live TV channel-to-image resolver.", "Add/correct real station/network artwork mapping. Do not use letters, numbers, blank boxes, or invented fallback graphics.");
            } else {
                add_finding(report, "Live TV", DiagnosticSeverity::Passed, "LIVE_TV_LOGOS_COMPLETE", "Every stored channel resolves to real artwork",
                            "Every channel icon slot resolves to correct real station/network artwork.", "All stored channels resolved an image.",
                            "Live TV channel-to-image resolver.", "No action required.");
            }
        } else {
            add_finding(report, "Live TV", DiagnosticSeverity::NotTested, "LIVE_TV_LOGO_MAPPING_NOT_SAMPLED", "Channel artwork mapping was not sampled in this diagnostic snapshot",
                        "A per-channel artwork resolver snapshot is supplied when the GUI diagnostic runs.",
                        "Only aggregate channel counts were supplied by the retained v0.0.39 self-test.",
                        "Diagnostic input compatibility path.", "Run the GUI diagnostic or the explicit channel-logo audit for the real persisted lineup.");
        }

        if (guide_covered == 0U) {
            add_finding(report, "Live TV", DiagnosticSeverity::NeedsAttention, "LIVE_TV_GUIDE_EMPTY", "Broadcast guide cache has no channel coverage",
                        "At least current/near-term PSIP exists for broadcasters that transmit it.", "0 channels have cached guide entries.",
                        input.live_tv_guide_path, "Refresh Guide; Nougat should harvest the active multiplex without interrupting playback and sweep other RF multiplexes when the tuner becomes idle.");
        } else if (guide_covered < live_channel_count) {
            add_finding(report, "Live TV", DiagnosticSeverity::NeedsAttention, "LIVE_TV_GUIDE_PARTIAL", "Broadcast guide coverage is incomplete",
                        "Guide cache contains available PSIP for stored services.",
                        std::to_string(guide_covered) + "/" + std::to_string(live_channel_count) + " channels have guide entries.",
                        input.live_tv_guide_path, "Allow queued idle-tuner guide refresh to finish. Missing broadcaster PSIP does not affect current playback.");
        } else {
            add_finding(report, "Live TV", DiagnosticSeverity::Passed, "LIVE_TV_GUIDE_COVERED", "Broadcast guide cache covers all stored channels",
                        "Guide cache contains available PSIP for stored services.", "All stored channels have cached guide entries.", input.live_tv_guide_path,
                        "No action required.");
        }
        const long long guide_time = input.live_tv_guide_mtime > 0 ? input.live_tv_guide_mtime
            : (input.live_tv_guide_cache_mtime > 0 ? input.live_tv_guide_cache_mtime : file_mtime(input.live_tv_guide_path));
        if (guide_time > 0) {
            const long long age = report.checked_at - guide_time;
            add_fact(report, "Live TV", "Guide cache age", age_text(guide_time, report.checked_at), input.live_tv_guide_path);
            if (age > 24LL * 3600LL) {
                add_finding(report, "Live TV", DiagnosticSeverity::NeedsAttention, "LIVE_TV_GUIDE_STALE", "Broadcast guide cache is stale",
                            "Guide cache refreshed within the last 24 hours when tuner access is available.", age_text(guide_time, report.checked_at),
                            input.live_tv_guide_path, "Refresh Guide or allow the queued idle-tuner sweep to complete.");
            }
        }
        if (input.live_tv_guide_refresh_busy || input.live_tv_current_mux_harvest_active ||
            input.live_tv_full_refresh_queued || input.live_tv_guide_refresh_queued) {
            std::string state;
            if (input.live_tv_current_mux_harvest_active) state = "Harvesting current multiplex PSIP";
            else if (input.live_tv_guide_refresh_busy) state = "Guide refresh running";
            else state = "Full guide refresh queued";
            add_finding(report, "Live TV", DiagnosticSeverity::Information, "LIVE_TV_GUIDE_BACKGROUND_WORK", "Broadcast guide background work",
                        "Guide work may run or queue without interrupting current playback.", state,
                        "Live TV guide worker state.", "No action required unless the state remains stuck after the tuner becomes idle.");
        }
    } else {
        add_finding(report, "Live TV", DiagnosticSeverity::NotTested, "LIVE_TV_CHANNELS_NOT_TESTED", "Live TV channel/guide checks have no stored channels",
                    "Stored channels exist after a successful scan.", "Channel database is empty.", "Native channel database snapshot.", "Run Scan Channels if Live TV is configured.");
    }

    const std::string search_db = input.search_data_dir.empty() ? std::string() : (fs::path(input.search_data_dir) / "nougat.db").string();
    add_fact(report, "Search", "Node", input.search_node_running ? "Running" : "Idle", "NougatBridge node process state.");
    add_fact(report, "Search", "Peers", std::to_string(input.search_peer_count), "Nougat peer registry snapshot.");
    if (!input.search_test_requested && !input.search_node_running) {
        add_finding(report, "Search", DiagnosticSeverity::NotTested, "SEARCH_IDLE", "Search network is idle, not failed",
                    "Search/Crawler/P2P network checks run only when explicitly exercised.", "Search node is idle.",
                    "NougatBridge node state.", "Run the Search diagnostic test if you want to exercise node/index/result retrieval.");
    } else if (input.search_test_requested && !input.search_probe_error.empty()) {
        add_finding(report, "Search", DiagnosticSeverity::NeedsAttention, "SEARCH_TEST_FAILED", "Search test did not complete cleanly",
                    "Explicit Search test initializes/reads the index and returns a result without engine errors.", input.search_probe_error,
                    search_db, "Open Search and retry; inspect the index/crawler status if the error repeats.");
    } else if (input.search_node_running) {
        add_finding(report, "Search", DiagnosticSeverity::Passed, "SEARCH_NODE_RUNNING", "Search node is running",
                    "Active Search node reports runtime state.", "Node running on port " + std::to_string(input.search_node_port) + ".",
                    input.search_node_id, "No action required.");
    } else {
        add_finding(report, "Search", DiagnosticSeverity::NotTested, "SEARCH_NOT_EXERCISED", "Search was not exercised",
                    "Explicit test requested when desired.", "No active Search operation.", search_db, "Run the Search diagnostic test if needed.");
    }
    if (!search_db.empty()) add_fact(report, "Search", "Index database", path_exists(search_db) ? "Present" : "Not initialized", search_db);

    add_fact(report, "P2P", "libtorrent", input.p2p_version.empty() ? "Unknown" : input.p2p_version, "Existing P2P runtime version.");
    if (input.p2p_active) {
        add_fact(report, "P2P", "State", input.p2p_state.empty() ? "Active" : input.p2p_state, "Current torrent snapshot.");
        add_fact(report, "P2P", "Peers / seeds", std::to_string(input.p2p_peers) + " / " + std::to_string(input.p2p_seeds), "Current torrent snapshot.");
        add_finding(report, "P2P", input.p2p_error.empty() ? DiagnosticSeverity::Passed : DiagnosticSeverity::NeedsAttention,
                    input.p2p_error.empty() ? "P2P_ACTIVE" : "P2P_TRANSFER_ERROR", "P2P transfer state",
                    "Active transfer has no engine error.", input.p2p_error.empty() ? "Active without reported error." : input.p2p_error,
                    "libtorrent session/torrent status.", input.p2p_error.empty() ? "No action required." : "Inspect/restart the affected transfer in Search > P2P.");
    } else {
        add_finding(report, "P2P", DiagnosticSeverity::NotTested, "P2P_IDLE", "P2P is idle",
                    "Transfer/Range-path checks are exercised only when a transfer is active or explicitly tested.", "No active torrent.",
                    "P2P engine status.", "No action required. Run a P2P diagnostic when troubleshooting a transfer.");
    }

    const long long model_size = file_size_or_negative(input.ai_model_path);
    add_fact(report, "AI", "Model size", human_bytes(model_size), input.ai_model_path);
    add_fact(report, "AI", "Model SHA-256", input.ai_model_sha256.empty() ? "Unknown" : input.ai_model_sha256, "sha256sum captured by Nougat.");
    const bool ai_files = path_readable(input.ai_model_path) && path_exists(input.ai_runtime_path);
    if (!ai_files) {
        add_finding(report, "AI", DiagnosticSeverity::NeedsAttention, "AI_RUNTIME_INCOMPLETE", "Local recommendation AI is unavailable",
                    "Pinned Nomic model and llama runtime exist and are readable.", "Model or runtime is missing/unreadable.",
                    input.ai_model_path + " | " + input.ai_runtime_path, "Restore the pinned model/runtime before using Local Usual recommendations.");
    } else if (input.ai_embedding_test_attempted) {
        add_finding(report, "AI", input.ai_embedding_test_passed ? DiagnosticSeverity::Passed : DiagnosticSeverity::NeedsAttention,
                    "AI_EMBEDDING_TEST", "Embedding generation test",
                    "A test embedding is generated successfully.",
                    input.ai_embedding_test_passed ? "Passed in " + std::to_string(input.ai_embedding_test_ms) + " ms." : input.ai_embedding_test_error,
                    "llama.cpp/Nomic diagnostic invocation.", input.ai_embedding_test_passed ? "No action required." : "Inspect the llama runtime/model and rerun Deep Diagnostic.");
    } else {
        add_finding(report, "AI", DiagnosticSeverity::NotTested, "AI_EMBEDDING_NOT_TESTED", "AI files are present but inference was not exercised",
                    "Deep Diagnostic generates one embedding and measures duration.", "Runtime/model files are present.",
                    input.ai_model_path, "Run Deep Diagnostic to exercise inference.");
    }

    add_fact(report, "TMDb", "Credential", input.tmdb_configured ? "Configured" : "Not configured",
             "Only presence is reported; secret material is never exported.");
    add_finding(report, "Discover", input.tmdb_configured ? DiagnosticSeverity::Passed : DiagnosticSeverity::Information,
                input.tmdb_configured ? "TMDB_CONFIGURED" : "TMDB_OPTIONAL_NOT_CONFIGURED", "TMDb configuration",
                "TMDb is configured only if external discovery/metadata is desired.", input.tmdb_configured ? "Credential present." : "No credential configured.",
                input.tmdb_status, input.tmdb_configured ? "No action required." : "Configure TMDb only if you want external Discover/metadata features.");

    const bool stream_ready = path_readable(input.stream_engine_path);
    add_fact(report, "Stream", "Resolver", input.stream_engine_path.empty() ? "Unknown" : input.stream_engine_path, "Configured yt-dlp/resolver path.");
    add_fact(report, "Stream", "Version", input.stream_engine_version.empty() ? "Unknown" : input.stream_engine_version, "Resolver --version output.");
    add_finding(report, "Stream", stream_ready ? DiagnosticSeverity::Passed : DiagnosticSeverity::NeedsAttention,
                "STREAM_ENGINE", "Stream resolver runtime", "Configured stream resolver is readable.", stream_ready ? "Resolver available." : "Resolver missing/unreadable.",
                input.stream_engine_path, stream_ready ? "No action required." : "Restore the pinned stream resolver before using Direct Watch.");

    if (!input.active_operation.empty()) {
        add_finding(report, "Application", DiagnosticSeverity::Information, "ACTIVE_OPERATION", "Nougat is currently doing background work",
                    "Background operations may legitimately make a snapshot transitional.", input.active_operation,
                    "Current worker state.", "Allow the operation to finish before comparing diagnostics if you need a stable snapshot.");
    }

    finalize_report(report);
    std::string ignored;
    if (!input.diagnostic_history_path.empty()) (void)append_history(report, input, ignored);
    return report;
}

const char* DiagnosticEngine::severity_name(DiagnosticSeverity severity) {
    switch (severity) {
    case DiagnosticSeverity::Passed: return "Passed";
    case DiagnosticSeverity::NeedsAttention: return "Needs Attention";
    case DiagnosticSeverity::Problem: return "Problem";
    case DiagnosticSeverity::NotTested: return "Not Tested";
    case DiagnosticSeverity::Information: return "Information";
    default: return "Not Tested";
    }
}

std::string DiagnosticEngine::report_text(const DiagnosticReport& report, const DiagnosticInput& input) {
    (void)input;
    std::ostringstream output;
    output << "Nougat Media Suite Diagnostic Report\n";
    output << "Checked: " << time_text(report.checked_at) << '\n';
    output << "SYSTEM HEALTH: " << severity_name(report.overall) << '\n';
    output << "Passed: " << report.passed_count << '\n';
    output << "Needs Attention: " << report.needs_attention_count << '\n';
    output << "Problems: " << report.problem_count << '\n';
    output << "Not Tested: " << report.not_tested_count << '\n';
    output << "Information: " << report.information_count << "\n\n";
    output << "== Subsystems ==\n";
    for (const auto& subsystem : report.subsystems) output << subsystem.name << ": " << subsystem_status_text(subsystem) << '\n';
    output << '\n';
    std::string section;
    for (const DiagnosticFact& fact : report.facts) {
        if (fact.section != section) { section = fact.section; output << "== " << section << " ==\n"; }
        output << fact.name << ": " << fact.value << '\n';
        if (!fact.evidence.empty()) output << "  Evidence: " << fact.evidence << '\n';
    }
    output << "\n== Findings ==\n";
    for (const DiagnosticIssue& issue : report.issues) {
        output << '[' << severity_name(issue.severity) << "] [" << issue.subsystem << "] " << issue.title << '\n';
        output << "Code: " << issue.code << '\n';
        output << "Expected: " << issue.expected << '\n';
        output << "Observed: " << issue.observed << '\n';
        output << "Evidence: " << issue.evidence << '\n';
        output << "Tested: " << time_text(issue.tested_at) << '\n';
        output << "Repair: " << issue.action << "\n\n";
    }
    output << "Privacy: credentials, authorization headers, cookies, passwords, tokens, and API keys are excluded/redacted.\n";
    return output.str();
}

std::string DiagnosticEngine::report_json(const DiagnosticReport& report, const DiagnosticInput& input) {
    (void)input;
    std::ostringstream out;
    out << "{\n";
    out << "  \"report\": \"Nougat Media Suite Diagnostic Report\",\n";
    out << "  \"checked\": \"" << json_escape(time_text(report.checked_at)) << "\",\n";
    out << "  \"checked_unix\": " << report.checked_at << ",\n";
    out << "  \"overall\": \"" << severity_name(report.overall) << "\",\n";
    out << "  \"counts\": {\"passed\":" << report.passed_count << ",\"needs_attention\":" << report.needs_attention_count
        << ",\"problems\":" << report.problem_count << ",\"not_tested\":" << report.not_tested_count
        << ",\"information\":" << report.information_count << "},\n";
    out << "  \"subsystems\": [\n";
    for (std::size_t i = 0; i < report.subsystems.size(); ++i) {
        const auto& s = report.subsystems[i];
        out << "    {\"name\":\"" << json_escape(s.name) << "\",\"status\":\"" << severity_name(s.severity)
            << "\",\"summary\":\"" << json_escape(s.summary) << "\"}" << (i + 1U == report.subsystems.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"facts\": [\n";
    for (std::size_t i = 0; i < report.facts.size(); ++i) {
        const auto& fact = report.facts[i];
        out << "    {\"section\":\"" << json_escape(fact.section) << "\",\"name\":\"" << json_escape(fact.name)
            << "\",\"value\":\"" << json_escape(fact.value) << "\",\"evidence\":\"" << json_escape(fact.evidence) << "\"}"
            << (i + 1U == report.facts.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"findings\": [\n";
    for (std::size_t i = 0; i < report.issues.size(); ++i) {
        const auto& issue = report.issues[i];
        out << "    {\"subsystem\":\"" << json_escape(issue.subsystem) << "\",\"severity\":\"" << severity_name(issue.severity)
            << "\",\"code\":\"" << json_escape(issue.code) << "\",\"title\":\"" << json_escape(issue.title)
            << "\",\"expected\":\"" << json_escape(issue.expected) << "\",\"observed\":\"" << json_escape(issue.observed)
            << "\",\"evidence\":\"" << json_escape(issue.evidence) << "\",\"repair\":\"" << json_escape(issue.action)
            << "\",\"tested_unix\":" << issue.tested_at << "}" << (i + 1U == report.issues.size() ? "\n" : ",\n");
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

bool DiagnosticEngine::write_history_snapshot(const DiagnosticReport& report,
                                              const DiagnosticInput& input,
                                              const std::string& path,
                                              std::string& error) {
    if (path.empty()) {
        error = "Diagnostic history snapshot path is empty.";
        return false;
    }
    std::error_code ec;
    fs::path target(path);
    if (fs::exists(target, ec) && fs::is_directory(target, ec)) {
        target /= "diagnostic_" + std::to_string(report.checked_at) + ".json";
    } else if (!target.has_extension()) {
        // The rejected-v0.0.39 UI may pass a history directory that does not yet
        // exist. Treat an extensionless target as that directory.
        fs::create_directories(target, ec);
        if (ec) {
            error = "Could not create diagnostic history directory: " + target.string();
            return false;
        }
        target /= "diagnostic_" + std::to_string(report.checked_at) + ".json";
    }
    return write_file(target.string(), report_json(report, input), error);
}

bool DiagnosticEngine::append_history(const DiagnosticReport& report, const DiagnosticInput& input, std::string& error) {
    if (input.diagnostic_history_path.empty()) return true;
    std::error_code ec;
    const fs::path target(input.diagnostic_history_path);
    if (!target.parent_path().empty()) fs::create_directories(target.parent_path(), ec);
    std::vector<std::string> lines;
    {
        std::ifstream existing(target);
        std::string line;
        while (std::getline(existing, line)) if (!line.empty()) lines.push_back(line);
    }
    std::ostringstream row;
    row << "{\"checked_unix\":" << report.checked_at << ",\"overall\":\"" << severity_name(report.overall)
        << "\",\"passed\":" << report.passed_count << ",\"needs_attention\":" << report.needs_attention_count
        << ",\"problems\":" << report.problem_count << ",\"not_tested\":" << report.not_tested_count << "}";
    lines.push_back(row.str());
    if (lines.size() > 20U) lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(lines.size() - 20U));
    std::ostringstream data;
    for (const auto& line : lines) data << line << '\n';
    return write_file(target.string(), data.str(), error);
}

std::vector<std::string> DiagnosticEngine::read_history(const DiagnosticInput& input, std::size_t limit) {
    std::vector<std::string> lines;
    if (input.diagnostic_history_path.empty()) return lines;
    std::ifstream file(input.diagnostic_history_path);
    std::string line;
    while (std::getline(file, line)) if (!line.empty()) lines.push_back(line);
    if (lines.size() > limit) lines.erase(lines.begin(), lines.end() - static_cast<std::ptrdiff_t>(limit));
    std::reverse(lines.begin(), lines.end());
    return lines;
}

bool DiagnosticEngine::write_support_bundle(const DiagnosticReport& report, const DiagnosticInput& input,
                                            const std::string& archive_path, std::string& error) {
    std::error_code ec;
    const fs::path temp = fs::temp_directory_path(ec) /
        ("nougat-diagnostic-support-" + std::to_string(static_cast<long long>(getpid())) + "-" + std::to_string(report.checked_at));
    if (ec || !fs::create_directories(temp / "logs", ec)) {
        error = "Could not create the temporary diagnostic support-bundle directory.";
        return false;
    }
    bool ok = write_file((temp / "report.txt").string(), report_text(report, input), error) &&
              write_file((temp / "report.json").string(), report_json(report, input), error);
    if (ok) {
        const auto history = read_history(input, 12U);
        std::ostringstream history_text;
        history_text << "Recent diagnostic runs (newest first)\n";
        for (const auto& row : history) history_text << row << '\n';
        ok = write_file((temp / "history.txt").string(), history_text.str(), error);
    }
    if (ok) {
        const std::string readme =
            "Nougat Media Suite diagnostic support bundle\n"
            "Evidence-backed v0.0.39 diagnostics.\n"
            "Sensitive log lines containing credentials, authorization headers, cookies, passwords, tokens, secrets, or API keys are redacted.\n";
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
