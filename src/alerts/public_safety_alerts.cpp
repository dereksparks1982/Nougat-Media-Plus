#include "public_safety_alerts.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>

namespace reddmedia::alerts {
namespace {

struct CommandResult { int code = -1; std::string output; };

CommandResult run_command(const std::string& command) {
    CommandResult result;
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe) return result;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe)) result.output += buffer;
    const int raw = pclose(pipe);
    if (raw >= 0 && WIFEXITED(raw)) result.code = WEXITSTATUS(raw);
    else result.code = raw;
    return result;
}

std::string json_string_field(const std::string& object, const std::string& key) {
    const std::string marker = "\"" + key + "\"";
    std::size_t pos = object.find(marker);
    if (pos == std::string::npos) return {};
    pos = object.find(':', pos + marker.size());
    if (pos == std::string::npos) return {};
    ++pos;
    while (pos < object.size() && std::isspace(static_cast<unsigned char>(object[pos]))) ++pos;
    if (pos >= object.size() || object[pos] != '"') return {};
    ++pos;
    std::string out;
    bool escaped = false;
    while (pos < object.size()) {
        const char c = object[pos++];
        if (!escaped && c == '"') break;
        if (!escaped && c == '\\') { escaped = true; continue; }
        if (escaped) {
            if (c == 'n') out.push_back(' ');
            else if (c == 'r' || c == 't') out.push_back(' ');
            else out.push_back(c);
            escaped = false;
        } else out.push_back(c);
    }
    return out;
}

std::vector<std::string> json_objects(const std::string& document) {
    std::vector<std::string> result;
    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    std::size_t begin = std::string::npos;
    for (std::size_t i = 0; i < document.size(); ++i) {
        const char c = document[i];
        if (in_string) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') in_string = false;
            continue;
        }
        if (c == '"') { in_string = true; continue; }
        if (c == '{') {
            if (depth == 0) begin = i;
            ++depth;
        } else if (c == '}' && depth > 0) {
            --depth;
            if (depth == 0 && begin != std::string::npos) {
                result.push_back(document.substr(begin, i - begin + 1U));
                begin = std::string::npos;
            }
        }
    }
    return result;
}


std::string home_dir() {
    const char* home = std::getenv("HOME");
    return home && *home ? std::string(home) : std::string(".");
}

std::string clean_history_field(std::string value) {
    for (char& c : value) if (c == '\t' || c == '\r' || c == '\n') c = ' ';
    return value;
}

std::set<std::string> read_history_ids(const std::string& path) {
    std::set<std::string> ids;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t tab = line.find('\t');
        const std::string id = line.substr(0, tab);
        if (!id.empty()) ids.insert(id);
    }
    return ids;
}

void append_history(const std::string& path, const PublicSafetyAlert& alert) {
    const std::string config = home_dir() + "/.config";
    const std::string app = config + "/reddmedia";
    mkdir(config.c_str(), 0700);
    mkdir(app.c_str(), 0700);
    std::ofstream out(path, std::ios::app);
    if (!out) return;
    out << clean_history_field(alert.id) << '\t'
        << clean_history_field(alert.sent) << '\t'
        << clean_history_field(alert.expires) << '\t'
        << clean_history_field(alert.event) << '\t'
        << clean_history_field(alert.severity) << '\t'
        << clean_history_field(alert.area) << '\t'
        << clean_history_field(alert.source_url) << '\n';
    out.flush();
    chmod(path.c_str(), 0600);
}

bool valid_area(std::string area) {
    if (area.size() != 2U) return false;
    for (char c : area) if (!std::isalpha(static_cast<unsigned char>(c))) return false;
    return true;
}

} // namespace

std::string PublicSafetyAlertService::history_path() const {
    return home_dir() + "/.config/reddmedia/public_alert_history.tsv";
}

bool PublicSafetyAlertService::refresh_area(const std::string& area_code,
                                            std::vector<PublicSafetyAlert>& alerts,
                                            std::string& status) const {
    alerts.clear();
    std::string area = area_code;
    std::transform(area.begin(), area.end(), area.begin(),
                   [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    if (!valid_area(area)) {
        status = "Set a two-letter local alert area before refreshing public-safety alerts.";
        return false;
    }
    const std::string url = "https://api.weather.gov/alerts/active?area=" + area;
    const CommandResult response = run_command(
        "curl -fsS --connect-timeout 3 --max-time 12 -H 'User-Agent: Nougat-Media-Suite/0.0.53' '" + url + "'");
    if (response.code != 0) {
        status = "NOAA/NWS public alert refresh failed.";
        return false;
    }

    std::set<std::string> ids;
    std::set<std::string> historical = read_history_ids(history_path());
    for (const std::string& object : json_objects(response.output)) {
        PublicSafetyAlert alert;
        alert.id = json_string_field(object, "id");
        alert.event = json_string_field(object, "event");
        alert.severity = json_string_field(object, "severity");
        alert.sent = json_string_field(object, "sent");
        alert.expires = json_string_field(object, "expires");
        alert.area = json_string_field(object, "areaDesc");
        alert.headline = json_string_field(object, "headline");
        alert.source_url = json_string_field(object, "@id");
        if (alert.source_url.empty()) alert.source_url = json_string_field(object, "id");
        if (alert.id.empty() || alert.event.empty() || !ids.insert(alert.id).second) continue;
        if (historical.insert(alert.id).second) append_history(history_path(), alert);
        alerts.push_back(std::move(alert));
    }
    status = "NOAA/NWS active alerts for " + area + ": " +
             std::to_string(alerts.size()) + " deduplicated alert(s). History: " + history_path();
    return true;
}

} // namespace reddmedia::alerts
