#include "hdhomerun_provider.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#include <sys/wait.h>

namespace reddmedia {
namespace {

struct CommandResult {
    int code = -1;
    std::string output;
};

std::string trim_copy(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    return value.substr(first);
}

std::string shell_quote(const std::string& value) {
    std::string out = "'";
    for (const char c : value) {
        if (c == '\'') out += "'\"'\"'";
        else out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

std::string config_program() {
    const char* override_value = std::getenv("HDHOMERUN_CONFIG");
    return override_value && *override_value ? std::string(override_value) : std::string("hdhomerun_config");
}

std::string curl_program() {
    const char* override_value = std::getenv("NOUGAT_CURL");
    return override_value && *override_value ? std::string(override_value) : std::string("curl");
}

CommandResult run_command(const std::string& command) {
    CommandResult result;
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe) return result;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe)) result.output += buffer;
    const int raw = pclose(pipe);
    if (raw >= 0 && WIFEXITED(raw)) result.code = WEXITSTATUS(raw);
    else result.code = raw;
    result.output = trim_copy(result.output);
    return result;
}

CommandResult config_command(const std::string& arguments) {
    return run_command(shell_quote(config_program()) + " " + arguments);
}

bool is_ipv4_address(const std::string& value) {
    if (value.empty() || value.find(':') != std::string::npos) return false;
    int dots = 0;
    for (char c : value) {
        if (c == '.') ++dots;
        else if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return dots == 3;
}

bool parse_synthetic_id(const std::string& id, std::string& device_id, int& tuner_index) {
    static const std::string prefix = "hdhomerun:";
    if (id.rfind(prefix, 0U) != 0U) return false;
    const std::size_t tuner_pos = id.find(":tuner", prefix.size());
    if (tuner_pos == std::string::npos) return false;
    device_id = id.substr(prefix.size(), tuner_pos - prefix.size());
    const std::string index_text = id.substr(tuner_pos + 6U);
    if (device_id.empty() || index_text.empty()) return false;
    for (char c : index_text) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    tuner_index = std::atoi(index_text.c_str());
    return tuner_index >= 0;
}

int integer_field(const std::string& text, const std::string& marker, int fallback = -1) {
    const std::size_t at = text.find(marker);
    if (at == std::string::npos) return fallback;
    std::size_t pos = at + marker.size();
    bool any = false;
    int value = 0;
    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
        any = true;
        value = value * 10 + (text[pos] - '0');
        ++pos;
    }
    return any ? value : fallback;
}

long long integer64_field(const std::string& text, const std::string& marker, long long fallback = 0) {
    const std::size_t at = text.find(marker);
    if (at == std::string::npos) return fallback;
    std::size_t pos = at + marker.size();
    bool any = false;
    long long value = 0;
    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
        any = true;
        value = value * 10 + (text[pos] - '0');
        ++pos;
    }
    return any ? value : fallback;
}

std::string token_field(const std::string& text, const std::string& marker) {
    const std::size_t at = text.find(marker);
    if (at == std::string::npos) return {};
    std::size_t pos = at + marker.size();
    const std::size_t end = text.find_first_of(" \t\r\n", pos);
    return text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

std::string json_unescape(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        if (c != '\\' || i + 1U >= value.size()) { out.push_back(c); continue; }
        const char n = value[++i];
        switch (n) {
        case '\\': out.push_back('\\'); break;
        case '"': out.push_back('"'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        default: out.push_back(n); break;
        }
    }
    return out;
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
    std::string raw;
    bool escaped = false;
    for (; pos < object.size(); ++pos) {
        const char c = object[pos];
        if (!escaped && c == '"') return json_unescape(raw);
        if (!escaped && c == '\\') { escaped = true; raw.push_back(c); continue; }
        raw.push_back(c);
        escaped = false;
    }
    return {};
}

std::vector<std::string> json_objects(const std::string& document) {
    std::vector<std::string> objects;
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
                objects.push_back(document.substr(begin, i - begin + 1U));
                begin = std::string::npos;
            }
        }
    }
    return objects;
}

[[maybe_unused]] int physical_channel_from_scan(const std::string& line) {
    const std::size_t colon = line.rfind(':');
    const std::size_t close = line.rfind(')');
    if (colon == std::string::npos || close == std::string::npos || colon >= close) return 0;
    const std::string candidate = line.substr(colon + 1U, close - colon - 1U);
    for (char c : candidate) if (!std::isdigit(static_cast<unsigned char>(c))) return 0;
    return std::atoi(candidate.c_str());
}

[[maybe_unused]] unsigned frequency_from_scan(const std::string& line) {
    const std::size_t marker = line.find(':');
    if (marker == std::string::npos) return 0;
    std::size_t pos = marker + 1U;
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
    unsigned long long value = 0;
    bool any = false;
    while (pos < line.size() && std::isdigit(static_cast<unsigned char>(line[pos]))) {
        any = true;
        value = value * 10ULL + static_cast<unsigned>(line[pos] - '0');
        ++pos;
    }
    return any && value <= 0xffffffffULL ? static_cast<unsigned>(value) : 0U;
}

bool parse_program_line(const std::string& line, int& program, std::string& guide, std::string& name) {
    std::size_t pos = 0U;
    if (line.rfind("PROGRAM ", 0U) == 0U) pos = 8U;
    else if (line.empty() || !std::isdigit(static_cast<unsigned char>(line[0]))) return false;
    program = 0;
    bool any = false;
    while (pos < line.size() && std::isdigit(static_cast<unsigned char>(line[pos]))) {
        any = true;
        program = program * 10 + (line[pos] - '0');
        ++pos;
    }
    if (!any) return false;
    const std::size_t colon = line.find(':', pos);
    if (colon == std::string::npos) return false;
    std::string remainder = trim_copy(line.substr(colon + 1U));
    if (remainder.empty() || remainder[0] == '0' || remainder.find("encrypted") != std::string::npos ||
        remainder.find("control") != std::string::npos) return false;
    const std::size_t space = remainder.find_first_of(" \t");
    guide = space == std::string::npos ? remainder : remainder.substr(0, space);
    if (guide.find('.') == std::string::npos) return false;
    for (char c : guide) if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '.')) return false;
    name = space == std::string::npos ? std::string() : trim_copy(remainder.substr(space + 1U));
    if (name.empty()) name = "Channel " + guide;
    return true;
}

std::string device_label(const HdHomeRunDevice& device) {
    std::string label = device.model.empty() ? "HDHomeRun" : device.model;
    label += " " + device.device_id;
    return label;
}

unsigned atsc_physical_frequency_hz(int physical_channel) {
    if (physical_channel >= 2 && physical_channel <= 4)
        return 57000000U + static_cast<unsigned>(physical_channel - 2) * 6000000U;
    if (physical_channel >= 5 && physical_channel <= 6)
        return 79000000U + static_cast<unsigned>(physical_channel - 5) * 6000000U;
    if (physical_channel >= 7 && physical_channel <= 13)
        return 177000000U + static_cast<unsigned>(physical_channel - 7) * 6000000U;
    if (physical_channel >= 14 && physical_channel <= 51)
        return 473000000U + static_cast<unsigned>(physical_channel - 14) * 6000000U;
    return 0U;
}

} // namespace

bool HdHomeRunProvider::is_hdhomerun_tuner(const TunerDevice& tuner) {
    return tuner.backend == "HDHomeRun" || tuner.id.rfind("hdhomerun:", 0U) == 0U;
}

bool HdHomeRunProvider::decode_tuner_id(const TunerDevice& tuner,
                                       std::string& device_id,
                                       int& tuner_index) {
    return parse_synthetic_id(tuner.id, device_id, tuner_index);
}

std::vector<HdHomeRunDevice> HdHomeRunProvider::discover_devices(std::string& status) const {
    const CommandResult discovered = config_command("discover");
    if (discovered.code != 0) {
        status = "HDHomeRun control utility is unavailable. Install hdhomerun-config to enable LAN tuner discovery.";
        return {};
    }

    std::map<std::string, std::string> addresses;
    std::istringstream lines(discovered.output);
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream row(line);
        std::string first, second, device_id, fourth, fifth, address;
        row >> first >> second >> device_id >> fourth >> fifth >> address;
        if (first != "hdhomerun" || second != "device" || device_id.empty() ||
            fourth != "found" || fifth != "at" || address.empty()) continue;
        auto found = addresses.find(device_id);
        if (found == addresses.end() || (!is_ipv4_address(found->second) && is_ipv4_address(address))) addresses[device_id] = address;
    }

    std::vector<HdHomeRunDevice> devices;
    for (const auto& item : addresses) {
        if (!is_ipv4_address(item.second)) continue; // Link-local IPv6 needs an interface scope; prefer the usable IPv4 LAN address.
        HdHomeRunDevice device;
        device.device_id = item.first;
        device.address = item.second;
        const CommandResult model = config_command(shell_quote(device.device_id) + " get /sys/model");
        if (model.code == 0) device.model = trim_copy(model.output);
        for (int tuner = 0; tuner < 16; ++tuner) {
            const CommandResult probe = config_command(shell_quote(device.device_id) + " get /tuner" + std::to_string(tuner) + "/status");
            if (probe.code != 0) break;
            ++device.tuner_count;
        }
        if (device.tuner_count <= 0) continue;
        devices.push_back(std::move(device));
    }

    if (devices.empty()) status = "No HDHomeRun IPv4 LAN tuners were discovered.";
    else {
        int physical = 0;
        for (const auto& device : devices) physical += device.tuner_count;
        status = "HDHomeRun: " + std::to_string(devices.size()) + " device(s), " +
                 std::to_string(physical) + " physical tuner(s) discovered.";
    }
    return devices;
}

std::vector<TunerDevice> HdHomeRunProvider::detect(std::string& status) const {
    const std::vector<HdHomeRunDevice> devices = discover_devices(status);
    std::vector<TunerDevice> tuners;
    for (const auto& device : devices) {
        for (int index = 0; index < device.tuner_count; ++index) {
            TunerDevice tuner;
            tuner.id = "hdhomerun:" + device.device_id + ":tuner" + std::to_string(index);
            tuner.name = device_label(device) + " / Tuner " + std::to_string(index);
            tuner.frontend_path = device.address;
            tuner.video_path.clear();
            tuner.backend = "HDHomeRun";
            HdHomeRunTunerStatus runtime;
            std::string probe_status;
            tuner.readable = probe_runtime_status(tuner, runtime, probe_status);
            std::ostringstream state;
            state << "LAN " << device.address << " | tuner " << index;
            if (runtime.accessible) {
                state << " | " << (runtime.busy ? "busy" : "available");
                if (!runtime.lock.empty() && runtime.lock != "none") state << " | lock " << runtime.lock;
                if (runtime.signal_percent >= 0) state << " | ss " << runtime.signal_percent << '%';
                if (runtime.quality_percent >= 0) state << " | snq " << runtime.quality_percent << '%';
            }
            tuner.status = state.str();
            tuners.push_back(std::move(tuner));
        }
    }
    return tuners;
}

bool HdHomeRunProvider::probe_runtime_status(const TunerDevice& tuner,
                                             HdHomeRunTunerStatus& runtime,
                                             std::string& status) const {
    runtime = HdHomeRunTunerStatus{};
    std::string device_id;
    int tuner_index = -1;
    if (!decode_tuner_id(tuner, device_id, tuner_index)) {
        status = "Not an HDHomeRun tuner resource.";
        return false;
    }
    const CommandResult result = config_command(shell_quote(device_id) + " get /tuner" + std::to_string(tuner_index) + "/status");
    if (result.code != 0) {
        status = result.output.empty() ? "HDHomeRun tuner status probe failed." : result.output;
        return false;
    }
    runtime.accessible = true;
    runtime.raw = result.output;
    runtime.channel = token_field(result.output, "ch=");
    runtime.lock = token_field(result.output, "lock=");
    runtime.signal_percent = integer_field(result.output, "ss=");
    runtime.quality_percent = integer_field(result.output, "snq=");
    runtime.symbol_quality_percent = integer_field(result.output, "seq=");
    runtime.bits_per_second = integer64_field(result.output, "bps=");
    runtime.packets_per_second = integer64_field(result.output, "pps=");
    // `pps` is the device-reported number of packets currently being sent over
    // the network. A tuner may remain tuned/locked with pps=0 after a scan, and
    // that idle lock must not be mistaken for an occupied streaming resource.
    runtime.busy = runtime.packets_per_second > 0;
    status = runtime.raw;
    return true;
}

bool HdHomeRunProvider::load_lineup(const TunerDevice& tuner,
                                    std::vector<LiveTvChannel>& channels,
                                    std::string& status) const {
    channels.clear();
    std::string device_id;
    int tuner_index = -1;
    if (!decode_tuner_id(tuner, device_id, tuner_index)) {
        status = "Not an HDHomeRun tuner resource.";
        return false;
    }
    (void)tuner_index;
    if (!is_ipv4_address(tuner.frontend_path)) {
        status = "HDHomeRun tuner does not have a usable IPv4 LAN address.";
        return false;
    }
    const std::string url = "http://" + tuner.frontend_path + "/lineup.json";
    const CommandResult response = run_command(shell_quote(curl_program()) +
        " -fsS --connect-timeout 2 --max-time 6 " + shell_quote(url));
    if (response.code != 0) {
        status = "Could not read HDHomeRun channel lineup from " + tuner.frontend_path + ".";
        return false;
    }

    std::map<std::string, LiveTvChannel> by_id;
    for (const std::string& object : json_objects(response.output)) {
        const std::string guide = json_string_field(object, "GuideNumber");
        if (guide.empty()) continue;
        std::string name = json_string_field(object, "GuideName");
        if (name.empty()) name = "Channel " + guide;
        LiveTvChannel channel;
        channel.id = guide;
        channel.name = name;
        channel.service = "HDHomeRun " + device_id;
        channel.frequency.clear();
        channel.program_number = 0;
        channel.physical_channel = 0;
        channel.source_id = 0;
        by_id[channel.id] = std::move(channel);
    }
    for (auto& item : by_id) channels.push_back(std::move(item.second));
    std::sort(channels.begin(), channels.end(), [](const LiveTvChannel& a, const LiveTvChannel& b) {
        const auto numeric = [](const std::string& id) {
            const std::size_t dot = id.find('.');
            const int major = std::atoi(id.substr(0, dot).c_str());
            const int minor = dot == std::string::npos ? 0 : std::atoi(id.substr(dot + 1U).c_str());
            return major * 1000 + minor;
        };
        return numeric(a.id) < numeric(b.id);
    });
    status = "HDHomeRun lineup loaded: " + std::to_string(channels.size()) + " channel(s).";
    return true;
}

bool HdHomeRunProvider::scan_channels(const TunerDevice& tuner,
                                      std::vector<LiveTvChannel>& channels,
                                      std::string& status,
                                      const ChannelScanCallback& callback) const {
    channels.clear();
    std::string device_id;
    int tuner_index = -1;
    if (!decode_tuner_id(tuner, device_id, tuner_index)) {
        status = "Not an HDHomeRun tuner resource.";
        return false;
    }

    std::map<std::string, LiveTvChannel> found;
    bool cancelled = false;
    bool failed = false;
    std::string failure;
    int rf_attempted = 0;
    int rf_locked = 0;
    int raw_service_rows = 0;
    int parsed_services = 0;
    int rejected_service_rows = 0;
    constexpr int kFirstPhysical = 2;
    constexpr int kLastPhysical = 51;
    constexpr int kTotal = kLastPhysical - kFirstPhysical + 1;

    for (int physical = kFirstPhysical; physical <= kLastPhysical; ++physical) {
        ++rf_attempted;
        const unsigned frequency = atsc_physical_frequency_hz(physical);
        if (frequency == 0U) {
            failed = true;
            failure = "ATSC frequency plan rejected RF " + std::to_string(physical) + ".";
            break;
        }

        const std::string tuner_path = "/tuner" + std::to_string(tuner_index);
        const CommandResult tune = config_command(
            shell_quote(device_id) + " set " + tuner_path + "/channel auto:" + std::to_string(frequency));
        if (tune.code != 0) {
            failed = true;
            failure = tune.output.empty()
                ? "HDHomeRun tune command failed on RF " + std::to_string(physical) + "."
                : tune.output;
            break;
        }

        bool locked = false;
        int signal = -1;
        int quality = -1;
        std::string lock_name = "none";
        for (int attempt = 0; attempt < 6; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(attempt == 0 ? 120 : 140));
            const CommandResult runtime = config_command(
                shell_quote(device_id) + " get " + tuner_path + "/status");
            if (runtime.code != 0) continue;
            lock_name = token_field(runtime.output, "lock=");
            signal = integer_field(runtime.output, "ss=");
            quality = integer_field(runtime.output, "snq=");
            locked = !lock_name.empty() && lock_name != "none";
            if (locked) break;
        }

        if (locked) {
            ++rf_locked;
            const CommandResult info = config_command(
                shell_quote(device_id) + " get " + tuner_path + "/streaminfo");
            if (info.code == 0) {
                std::istringstream rows(info.output);
                std::string row;
                while (std::getline(rows, row)) {
                    row = trim_copy(row);
                    if (row.empty()) continue;
                    ++raw_service_rows;
                    int program = 0;
                    std::string guide;
                    std::string name;
                    if (!parse_program_line(row, program, guide, name)) { ++rejected_service_rows; continue; }
                    ++parsed_services;
                    LiveTvChannel channel;
                    channel.id = guide;
                    channel.name = name;
                    channel.service = "HDHomeRun " + device_id + " | program " +
                                      std::to_string(program) + " | RF " + std::to_string(physical);
                    channel.frequency = std::to_string(frequency);
                    channel.program_number = program;
                    channel.physical_channel = physical;
                    channel.source_id = 0;
                    found[channel.id] = std::move(channel);
                }
            }
        }

        if (callback) {
            ChannelScanProgress progress;
            progress.physical_channel = physical;
            progress.frequency_hz = frequency;
            progress.completed = ((physical - kFirstPhysical + 1) * 65) / kTotal;
            progress.total = 100;
            progress.locked = locked;
            progress.signal_percent = signal;
            progress.quality_percent = quality;
            progress.channels_found = static_cast<int>(found.size());
            progress.message = "HDHomeRun tuner " + std::to_string(tuner_index) +
                               " scanning RF " + std::to_string(physical) +
                               " @ " + std::to_string(frequency) + " Hz | " +
                               (locked ? ("lock " + lock_name) : "no lock") + " | " +
                               std::to_string(found.size()) + " program(s) found.";
            if (!callback(progress)) {
                cancelled = true;
                break;
            }
        }
    }

    if (cancelled) {
        config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
        status = "HDHomeRun channel scan cancelled during RF traversal. State returned to Idle.";
        return false;
    }
    if (failed) {
        config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
        status = "HDHomeRun channel scan failed during RF traversal: " + failure + " State returned to Idle.";
        return false;
    }

    if (callback) {
        ChannelScanProgress phase;
        phase.completed = 72; phase.total = 100; phase.channels_found = static_cast<int>(found.size());
        phase.message = "HDHomeRun phase 2/6: service/program parsing complete.";
        if (!callback(phase)) {
            status = "HDHomeRun channel scan cancelled during service/program parsing. State returned to Idle.";
            config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
            return false;
        }
    }

    for (auto& item : found) channels.push_back(std::move(item.second));
    std::sort(channels.begin(), channels.end(), [](const LiveTvChannel& a, const LiveTvChannel& b) {
        const auto numeric=[](const std::string& id) {
            const std::size_t dot=id.find('.');
            const int major=std::atoi(id.substr(0,dot).c_str());
            const int minor=dot==std::string::npos?0:std::atoi(id.substr(dot+1U).c_str());
            return major*1000+minor;
        };
        return numeric(a.id)<numeric(b.id);
    });

    if (callback) {
        ChannelScanProgress phase;
        phase.completed = 82; phase.total = 100; phase.channels_found = static_cast<int>(channels.size());
        phase.message = "HDHomeRun phase 4/6: service/channel resolution complete; handing channels to Nougat import.";
        if (!callback(phase)) {
            config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
            status = "HDHomeRun channel scan cancelled before channel import. State returned to Idle.";
            return false;
        }
    }

    const CommandResult clear_result = config_command(
        shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");

    status = "HDHomeRun phases 1-4 complete: RF attempted " + std::to_string(rf_attempted) +
             ", multiplexes locked " + std::to_string(rf_locked) +
             ", raw service rows " + std::to_string(raw_service_rows) +
             ", parsed services " + std::to_string(parsed_services) +
             ", rejected rows " + std::to_string(rejected_service_rows) +
             ", unique channels ready for import " + std::to_string(channels.size()) +
             ". Phase 6 tuner finalization complete. Guide update runs where provider data is available.";
    if (clear_result.code != 0)
        status += " Tuner release needs attention: " +
                  (clear_result.output.empty() ? std::string("release command failed") : clear_result.output) + ".";
    else
        status += " Tuner released.";
    return true;
}

bool HdHomeRunProvider::live_playback_input(const TunerDevice& tuner,
                                            const LiveTvChannel& channel,
                                            std::string& mrl,
                                            std::vector<std::string>& media_options,
                                            std::string& status) const {
    mrl.clear();
    media_options.clear();
    std::string device_id;
    int tuner_index = -1;
    if (!decode_tuner_id(tuner, device_id, tuner_index)) {
        status = "Not an HDHomeRun tuner resource.";
        return false;
    }
    if (!is_ipv4_address(tuner.frontend_path)) {
        status = "HDHomeRun tuner does not have a usable IPv4 LAN address.";
        return false;
    }
    if (channel.id.empty()) { status = "HDHomeRun channel has no virtual channel number."; return false; }
    for (char c : channel.id) {
        if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '.')) {
            status = "HDHomeRun virtual channel number is invalid.";
            return false;
        }
    }

    // SiliconDust documents HTTP virtual-channel playback through /auto/v<channel>.
    // The device assigns one free physical tuner. Nougat probes the per-tuner status
    // before allocating a second operation, so scan/guide work uses another free tuner.
    mrl = "http://" + tuner.frontend_path + ":5004/auto/v" + channel.id;
    media_options.push_back(":network-caching=350");
    media_options.push_back(":http-reconnect=true");
    status = "HDHomeRun " + device_id + " tuner " + std::to_string(tuner_index) +
             " ready for " + channel.id + " " + channel.name + ".";
    return true;
}

bool HdHomeRunProvider::release_tuner(const TunerDevice& tuner, std::string& status) const {
    std::string device_id;
    int tuner_index = -1;
    if (!decode_tuner_id(tuner, device_id, tuner_index)) {
        status = "Not an HDHomeRun tuner resource.";
        return false;
    }
    // Closing the HTTP stream releases the tuner. Explicitly clearing the channel
    // is a second safety net when Nougat owns a known physical tuner operation.
    const CommandResult result = config_command(shell_quote(device_id) + " set /tuner" +
        std::to_string(tuner_index) + "/channel none");
    if (result.code != 0) {
        status = result.output.empty() ? "HDHomeRun tuner release command failed." : result.output;
        return false;
    }
    status = "HDHomeRun tuner " + std::to_string(tuner_index) + " released.";
    return true;
}

} // namespace reddmedia
