#include "tuner_backend.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <thread>
#include <fcntl.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace reddmedia {
namespace fs = std::filesystem;
namespace {

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string read_first_line(const fs::path& path) {
    std::ifstream in(path);
    std::string line;
    if (in && std::getline(in, line)) return line;
    return {};
}

bool looks_like_tv_device(const std::string& name) {
    const std::string lower = lower_copy(name);
    static const char* words[] = {"hauppauge", "wintv", "955q", "atsc", "dvb", "television", "tv tuner", "cx231", nullptr};
    for (int i = 0; words[i]; ++i) if (lower.find(words[i]) != std::string::npos) return true;
    return false;
}

bool hauppauge_name(const std::string& name) {
    const std::string lower = lower_copy(name);
    return lower.find("hauppauge") != std::string::npos || lower.find("wintv") != std::string::npos || lower.find("955q") != std::string::npos;
}

void ensure_directory(const fs::path& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
}

unsigned atsc_frequency_hz(int physical_channel) {
    if (physical_channel >= 2 && physical_channel <= 4) {
        return static_cast<unsigned>((57 + (physical_channel - 2) * 6) * 1000000U);
    }
    if (physical_channel >= 5 && physical_channel <= 6) {
        return static_cast<unsigned>((79 + (physical_channel - 5) * 6) * 1000000U);
    }
    if (physical_channel >= 7 && physical_channel <= 13) {
        return static_cast<unsigned>((177 + (physical_channel - 7) * 6) * 1000000U);
    }
    if (physical_channel >= 14 && physical_channel <= 36) {
        return static_cast<unsigned>((473 + (physical_channel - 14) * 6) * 1000000U);
    }
    return 0;
}

int integer_after_marker(const std::string& text, const std::string& marker) {
    const std::size_t at = text.find(marker);
    if (at == std::string::npos) return 0;
    std::size_t pos = at + marker.size();
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
    int value = 0;
    bool any = false;
    while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
        any = true;
        value = value * 10 + (text[pos] - '0');
        ++pos;
    }
    return any ? value : 0;
}

int adapter_index_from_frontend(const std::string& frontend_path) {
    const std::string marker = "/adapter";
    const std::size_t at = frontend_path.find(marker);
    if (at == std::string::npos) return 0;
    std::size_t pos = at + marker.size();
    int value = 0;
    bool any = false;
    while (pos < frontend_path.size() && std::isdigit(static_cast<unsigned char>(frontend_path[pos]))) {
        any = true;
        value = value * 10 + (frontend_path[pos] - '0');
        ++pos;
    }
    return any ? value : 0;
}

std::string trim_copy(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    return value.substr(first);
}

std::string printable_psip_text(const unsigned char* data, std::size_t size) {
    if (!data || size == 0U) return {};
    std::string out;
    // ATSC multiple_string_structure. Full compression tables can be added
    // later; uncompressed English segments are common and are decoded here.
    std::size_t pos = 0;
    const unsigned strings = data[pos++];
    for (unsigned s = 0; s < strings && pos + 4U <= size; ++s) {
        pos += 3U; // ISO-639 language code.
        const unsigned segments = data[pos++];
        for (unsigned seg = 0; seg < segments && pos + 3U <= size; ++seg) {
            const unsigned compression = data[pos++];
            (void)data[pos++]; // mode; printable fallback below is mode-agnostic.
            const unsigned bytes = data[pos++];
            if (pos + bytes > size) return trim_copy(out);
            if (compression == 0U) {
                for (unsigned i = 0; i < bytes; ++i) {
                    const unsigned char c = data[pos + i];
                    if (c >= 32U && c <= 126U) out.push_back(static_cast<char>(c));
                    else if ((c == '\n' || c == '\r' || c == '\t') && !out.empty() && out.back() != ' ') out.push_back(' ');
                }
            }
            pos += bytes;
            if (!out.empty() && out.back() != ' ') out.push_back(' ');
        }
    }
    return trim_copy(out);
}

std::string sibling_device_path(const std::string& frontend_path, const char* node) {
    const fs::path frontend(frontend_path);
    if (frontend.parent_path().empty()) return {};
    return (frontend.parent_path() / node).string();
}

int legacy_metric_percent(int fd, unsigned long request) {
    std::uint16_t raw = 0;
    if (ioctl(fd, request, &raw) != 0) return -1;
    return static_cast<int>((static_cast<unsigned>(raw) * 100U) / 65535U);
}

bool tune_atsc(int frontend_fd, unsigned frequency_hz, std::string& error) {
    dtv_property props[5]{};
    props[0].cmd = DTV_CLEAR;
    props[1].cmd = DTV_DELIVERY_SYSTEM;
    props[1].u.data = SYS_ATSC;
    props[2].cmd = DTV_FREQUENCY;
    props[2].u.data = frequency_hz;
    props[3].cmd = DTV_MODULATION;
    props[3].u.data = VSB_8;
    props[4].cmd = DTV_TUNE;
    dtv_properties command{};
    command.num = 5;
    command.props = props;
    if (ioctl(frontend_fd, FE_SET_PROPERTY, &command) != 0) {
        error = std::string("DVB tune ioctl failed: ") + std::strerror(errno);
        return false;
    }
    return true;
}

bool wait_for_lock(int frontend_fd, fe_status_t& status, int timeout_ms) {
    status = static_cast<fe_status_t>(0);
    const int step_ms = 80;
    for (int waited = 0; waited < timeout_ms; waited += step_ms) {
        if (ioctl(frontend_fd, FE_READ_STATUS, &status) == 0 && (status & FE_HAS_LOCK)) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(step_ms));
    }
    return ioctl(frontend_fd, FE_READ_STATUS, &status) == 0 && (status & FE_HAS_LOCK);
}

std::string decode_short_name(const unsigned char* p) {
    std::string out;
    for (int i = 0; i < 7; ++i) {
        const unsigned value = (static_cast<unsigned>(p[i * 2]) << 8) | p[i * 2 + 1];
        if (value == 0) continue;
        if (value >= 32 && value <= 126) out.push_back(static_cast<char>(value));
        else if (value <= 255) out.push_back(static_cast<char>(value));
        else out.push_back('?');
    }
    while (!out.empty() && std::isspace(static_cast<unsigned char>(out.back()))) out.pop_back();
    std::size_t first = 0;
    while (first < out.size() && std::isspace(static_cast<unsigned char>(out[first]))) ++first;
    return out.substr(first);
}

void parse_vct_section(const unsigned char* data, std::size_t size, int physical_channel,
                       unsigned frequency_hz, std::map<std::string, LiveTvChannel>& found) {
    if (size < 10 || (data[0] != 0xC8 && data[0] != 0xC9)) return;
    const std::size_t section_length = 3U + static_cast<std::size_t>(((data[1] & 0x0fU) << 8) | data[2]);
    if (section_length > size || section_length < 14U) return;
    const unsigned count = data[9];
    std::size_t offset = 10;
    for (unsigned i = 0; i < count; ++i) {
        if (offset + 32U > section_length - 4U) break;
        const unsigned char* p = data + offset;
        const int major = ((p[14] & 0x0f) << 6) | ((p[15] & 0xfc) >> 2);
        const int minor = ((p[15] & 0x03) << 8) | p[16];
        const unsigned program = (static_cast<unsigned>(p[24]) << 8) | p[25];
        const unsigned service_type = p[27] & 0x3fU;
        const unsigned source_id = (static_cast<unsigned>(p[28]) << 8) | p[29];
        const unsigned descriptors_length = ((p[30] & 0x03U) << 8) | p[31];
        const std::size_t entry_length = 32U + descriptors_length;
        if (offset + entry_length > section_length - 4U) break;

        // Service types 0x02 (ATSC digital television) and 0x03 (audio) are
        // useful to the channel lineup. Hidden/test entries are still kept if
        // they advertise a valid virtual channel number so the owner can see
        // what the tuner actually received.
        if (major > 0 && minor >= 0 && program > 0 && (service_type == 0x02U || service_type == 0x03U || service_type == 0x00U)) {
            std::ostringstream id;
            id << major << '.' << minor;
            std::string name = decode_short_name(p);
            if (name.empty()) name = "Channel " + id.str();
            LiveTvChannel channel;
            channel.id = id.str();
            channel.name = name;
            std::ostringstream service;
            service << "ATSC " << id.str() << " | program " << program << " | RF " << physical_channel;
            channel.service = service.str();
            channel.frequency = std::to_string(frequency_hz);
            channel.program_number = static_cast<int>(program);
            channel.physical_channel = physical_channel;
            channel.source_id = static_cast<std::uint16_t>(source_id);
            found[channel.id] = std::move(channel);
        }
        offset += entry_length;
    }
}

std::vector<unsigned short> collect_eit_pids(const std::string& demux_path) {
    std::vector<unsigned short> pids;
    const int fd = open(demux_path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return pids;
    dmx_sct_filter_params filter{};
    filter.pid = 0x1ffb;
    filter.filter.filter[0] = 0xC7; // Master Guide Table.
    filter.filter.mask[0] = 0xFF;
    filter.timeout = 900;
    filter.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
    if (ioctl(fd, DMX_SET_FILTER, &filter) != 0) { close(fd); return pids; }

    unsigned char buffer[8192];
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1050);
    while (std::chrono::steady_clock::now() < deadline) {
        pollfd pfd{fd, POLLIN, 0};
        if (poll(&pfd, 1, 160) <= 0 || !(pfd.revents & POLLIN)) continue;
        const ssize_t got = read(fd, buffer, sizeof(buffer));
        if (got < 15 || buffer[0] != 0xC7) continue;
        const std::size_t section_size = 3U + static_cast<std::size_t>(((buffer[1] & 0x0fU) << 8) | buffer[2]);
        if (section_size > static_cast<std::size_t>(got) || section_size < 15U) continue;
        const unsigned tables = (static_cast<unsigned>(buffer[9]) << 8) | buffer[10];
        std::size_t offset = 11U;
        for (unsigned i = 0; i < tables && offset + 11U <= section_size - 4U; ++i) {
            const unsigned type = (static_cast<unsigned>(buffer[offset]) << 8) | buffer[offset + 1U];
            const unsigned short pid = static_cast<unsigned short>(((buffer[offset + 2U] & 0x1fU) << 8) | buffer[offset + 3U]);
            const unsigned desc_len = ((buffer[offset + 9U] & 0x0fU) << 8) | buffer[offset + 10U];
            // EIT-0 and EIT-1 are enough for the first classic-guide window
            // while keeping an idle-tuner refresh reasonably quick.
            if (type >= 0x0100U && type <= 0x0101U && pid != 0U &&
                std::find(pids.begin(), pids.end(), pid) == pids.end()) pids.push_back(pid);
            offset += 11U + desc_len;
        }
        if (!pids.empty()) break;
    }
    ioctl(fd, DMX_STOP);
    close(fd);
    return pids;
}

void collect_eit_from_pid(const std::string& demux_path,
                          unsigned short pid,
                          const std::map<std::uint16_t, std::string>& source_to_channel,
                          std::vector<LiveTvProgram>& programs) {
    const int fd = open(demux_path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return;
    dmx_sct_filter_params filter{};
    filter.pid = pid;
    filter.filter.filter[0] = 0xCB;
    filter.filter.mask[0] = 0xFF;
    filter.timeout = 900;
    filter.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
    if (ioctl(fd, DMX_SET_FILTER, &filter) != 0) { close(fd); return; }

    std::set<std::string> seen;
    unsigned char buffer[8192];
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1100);
    while (std::chrono::steady_clock::now() < deadline) {
        pollfd pfd{fd, POLLIN, 0};
        if (poll(&pfd, 1, 130) <= 0 || !(pfd.revents & POLLIN)) continue;
        const ssize_t got = read(fd, buffer, sizeof(buffer));
        if (got < 12 || buffer[0] != 0xCB) continue;
        const std::size_t section_size = 3U + static_cast<std::size_t>(((buffer[1] & 0x0fU) << 8) | buffer[2]);
        if (section_size > static_cast<std::size_t>(got) || section_size < 14U) continue;
        const std::uint16_t source_id = static_cast<std::uint16_t>((static_cast<unsigned>(buffer[3]) << 8) | buffer[4]);
        const auto channel_it = source_to_channel.find(source_id);
        if (channel_it == source_to_channel.end()) continue;
        const unsigned event_count = buffer[9];
        std::size_t offset = 10U;
        for (unsigned i = 0; i < event_count && offset + 10U <= section_size - 4U; ++i) {
            const unsigned event_id = ((static_cast<unsigned>(buffer[offset]) & 0x3fU) << 8) | buffer[offset + 1U];
            const std::uint32_t gps = (static_cast<std::uint32_t>(buffer[offset + 2U]) << 24) |
                                      (static_cast<std::uint32_t>(buffer[offset + 3U]) << 16) |
                                      (static_cast<std::uint32_t>(buffer[offset + 4U]) << 8) |
                                      static_cast<std::uint32_t>(buffer[offset + 5U]);
            const unsigned duration = ((static_cast<unsigned>(buffer[offset + 6U]) & 0x0fU) << 16) |
                                      (static_cast<unsigned>(buffer[offset + 7U]) << 8) |
                                      buffer[offset + 8U];
            const unsigned title_len = buffer[offset + 9U];
            if (offset + 10U + title_len + 2U > section_size - 4U) break;
            std::string title = printable_psip_text(buffer + offset + 10U, title_len);
            std::size_t desc_at = offset + 10U + title_len;
            const unsigned desc_len = ((buffer[desc_at] & 0x0fU) << 8) | buffer[desc_at + 1U];
            if (desc_at + 2U + desc_len > section_size - 4U) break;
            if (title.empty()) title = "Program " + std::to_string(event_id);

            LiveTvProgram program;
            program.channel_id = channel_it->second;
            program.title = title;
            // ATSC start_time is GPS seconds. GPS-UTC remains 18 seconds in
            // 2026; STT/ETT enrichment can supersede this fallback later.
            program.start_unix = static_cast<long long>(gps) + 315964800LL - 18LL;
            program.duration_seconds = static_cast<int>(duration);
            program.event_id = event_id;
            const std::string key = program.channel_id + "|" + std::to_string(program.start_unix) + "|" + std::to_string(event_id);
            if (seen.insert(key).second) programs.push_back(std::move(program));
            offset = desc_at + 2U + desc_len;
        }
    }
    ioctl(fd, DMX_STOP);
    close(fd);
}

void collect_vct(const std::string& demux_path, int physical_channel, unsigned frequency_hz,
                 std::map<std::string, LiveTvChannel>& found) {
    const int fd = open(demux_path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return;

    dmx_sct_filter_params filter{};
    filter.pid = 0x1ffb;
    filter.filter.filter[0] = 0xC8;
    filter.filter.mask[0] = 0xFE; // C8 terrestrial VCT or C9 cable VCT.
    filter.timeout = 1200;
    filter.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
    if (ioctl(fd, DMX_SET_FILTER, &filter) != 0) {
        close(fd);
        return;
    }

    unsigned char buffer[4096];
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1300);
    while (std::chrono::steady_clock::now() < deadline) {
        pollfd pfd{fd, POLLIN, 0};
        const int ready = poll(&pfd, 1, 180);
        if (ready <= 0 || !(pfd.revents & POLLIN)) continue;
        const ssize_t got = read(fd, buffer, sizeof(buffer));
        if (got > 0) parse_vct_section(buffer, static_cast<std::size_t>(got), physical_channel, frequency_hz, found);
    }
    ioctl(fd, DMX_STOP);
    close(fd);
}

} // namespace

NougatTunerBackend::NougatTunerBackend() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) config_dir_ = std::string(xdg) + "/reddmedia/live_tv";
    else {
        const char* home = std::getenv("HOME");
        config_dir_ = std::string(home && *home ? home : ".") + "/.config/reddmedia/live_tv";
    }
}

std::vector<TunerDevice> NougatTunerBackend::detect(std::string& status) const {
    std::vector<TunerDevice> result;
    std::error_code ec;

    const fs::path dvb_root("/dev/dvb");
    if (fs::exists(dvb_root, ec)) {
        for (const auto& adapter : fs::directory_iterator(dvb_root, ec)) {
            if (ec || !adapter.is_directory()) continue;
            for (const auto& node : fs::directory_iterator(adapter.path(), ec)) {
                if (ec) break;
                const std::string filename = node.path().filename().string();
                if (filename.rfind("frontend", 0U) != 0U) continue;
                TunerDevice tuner;
                tuner.id = adapter.path().filename().string() + "/" + filename;
                tuner.frontend_path = node.path().string();
                tuner.backend = "Linux DVB";
                tuner.readable = access(tuner.frontend_path.c_str(), R_OK | W_OK) == 0;
                const fs::path sys_base = fs::path("/sys/class/dvb") / (adapter.path().filename().string() + "." + filename) / "device";
                tuner.name = read_first_line(sys_base / "manufacturer");
                if (tuner.name.empty()) tuner.name = read_first_line(sys_base / "product");
                if (tuner.name.empty()) tuner.name = "DVB frontend " + tuner.id;
                tuner.hauppauge = hauppauge_name(tuner.name);
                tuner.status = tuner.readable ? "Detected / scan-ready" : "Detected / permission required";
                result.push_back(std::move(tuner));
            }
        }
    }

    // A physical USB tuner commonly exposes DVB, video and VBI nodes. Once a
    // usable DVB frontend exists, those sibling V4L2/VBI interfaces are not
    // separate tuners and must not become three owner-visible device rows.
    const bool have_dvb_frontend = !result.empty();
    const fs::path v4l_root("/sys/class/video4linux");
    if (!have_dvb_frontend && fs::exists(v4l_root, ec)) {
        for (const auto& entry : fs::directory_iterator(v4l_root, ec)) {
            if (ec || !entry.is_directory()) continue;
            const std::string dev = entry.path().filename().string();
            const std::string name = read_first_line(entry.path() / "name");
            if (!looks_like_tv_device(name)) continue;
            const std::string video_path = "/dev/" + dev;
            auto same = std::find_if(result.begin(), result.end(), [&](const TunerDevice& item) {
                return !item.name.empty() && lower_copy(item.name) == lower_copy(name);
            });
            if (same != result.end()) {
                same->video_path = video_path;
                same->hauppauge = same->hauppauge || hauppauge_name(name);
                continue;
            }
            TunerDevice tuner;
            tuner.id = dev;
            tuner.name = name.empty() ? ("V4L2 tuner " + dev) : name;
            tuner.video_path = video_path;
            tuner.backend = "V4L2";
            tuner.readable = access(video_path.c_str(), R_OK) == 0;
            tuner.hauppauge = hauppauge_name(tuner.name);
            tuner.status = tuner.readable ? "Detected / ready to probe" : "Detected / permission required";
            result.push_back(std::move(tuner));
        }
    }

    if (result.empty()) {
        status = "No Linux DVB/V4L2 TV tuner detected. Plug in the WinTV-HVR-955Q and press Detect Tuners.";
    } else {
        const bool has_hauppauge = std::any_of(result.begin(), result.end(), [](const TunerDevice& t) { return t.hauppauge; });
        std::ostringstream out;
        out << result.size() << " logical tuner" << (result.size() == 1U ? "" : "s") << " detected";
        if (has_hauppauge) out << " | Hauppauge/WinTV signature found";
        out << ".";
        status = out.str();
    }
    return result;
}

std::string NougatTunerBackend::channels_path() const { return config_dir_ + "/channels.tsv"; }
std::string NougatTunerBackend::guide_path() const { return config_dir_ + "/guide.tsv"; }

std::vector<LiveTvChannel> NougatTunerBackend::load_channels() const {
    std::vector<LiveTvChannel> channels;
    std::ifstream in(channels_path());
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        LiveTvChannel channel;
        if (row >> std::quoted(channel.id) >> std::quoted(channel.name) >> std::quoted(channel.service) >> std::quoted(channel.frequency)) {
            if (!(row >> channel.program_number >> channel.physical_channel >> channel.source_id)) {
                row.clear();
                channel.program_number = integer_after_marker(channel.service, "program");
                channel.physical_channel = integer_after_marker(channel.service, "RF");
                channel.source_id = 0;
            }
            channels.push_back(std::move(channel));
        }
    }
    return channels;
}

bool NougatTunerBackend::save_channels(const std::vector<LiveTvChannel>& channels, std::string& error) const {
    ensure_directory(config_dir_);
    const std::string temporary = channels_path() + ".tmp";
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) { error = "Could not open Live TV channel database for writing."; return false; }
    for (const auto& channel : channels) {
        out << std::quoted(channel.id) << '\t' << std::quoted(channel.name) << '\t'
            << std::quoted(channel.service) << '\t' << std::quoted(channel.frequency) << '\t'
            << channel.program_number << '\t' << channel.physical_channel << '\t' << channel.source_id << '\n';
    }
    out.close();
    chmod(temporary.c_str(), 0600);
    if (rename(temporary.c_str(), channels_path().c_str()) != 0) {
        error = "Could not replace Live TV channel database.";
        return false;
    }
    return true;
}


std::vector<LiveTvProgram> NougatTunerBackend::load_guide() const {
    std::vector<LiveTvProgram> programs;
    std::ifstream in(guide_path());
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        LiveTvProgram program;
        if (row >> std::quoted(program.channel_id) >> std::quoted(program.title) >> std::quoted(program.description)
                >> program.start_unix >> program.duration_seconds >> program.event_id) {
            programs.push_back(std::move(program));
        }
    }
    return programs;
}

bool NougatTunerBackend::save_guide(const std::vector<LiveTvProgram>& programs, std::string& error) const {
    ensure_directory(config_dir_);
    const std::string temporary = guide_path() + ".tmp";
    std::ofstream out(temporary, std::ios::trunc);
    if (!out) { error = "Could not open Live TV guide cache for writing."; return false; }
    for (const auto& program : programs) {
        out << std::quoted(program.channel_id) << '\t' << std::quoted(program.title) << '\t'
            << std::quoted(program.description) << '\t' << program.start_unix << '\t'
            << program.duration_seconds << '\t' << program.event_id << '\n';
    }
    out.close();
    chmod(temporary.c_str(), 0600);
    if (!out) { error = "Could not finish writing Live TV guide cache."; unlink(temporary.c_str()); return false; }
    if (rename(temporary.c_str(), guide_path().c_str()) != 0) {
        error = "Could not replace Live TV guide cache."; unlink(temporary.c_str()); return false;
    }
    return true;
}

bool NougatTunerBackend::live_playback_input(const TunerDevice& tuner,
                                             const LiveTvChannel& channel,
                                             std::string& mrl,
                                             std::vector<std::string>& media_options,
                                             std::string& status) const {
    mrl.clear();
    media_options.clear();
    if (tuner.frontend_path.empty()) { status = "Selected tuner has no Linux DVB frontend."; return false; }
    if (!tuner.readable) { status = "Selected tuner is not accessible. Check Linux DVB permissions."; return false; }
    unsigned long long frequency = 0;
    try { frequency = std::stoull(channel.frequency); } catch (...) { frequency = 0; }
    const int program = channel.program_number > 0 ? channel.program_number : integer_after_marker(channel.service, "program");
    if (frequency == 0ULL || program <= 0) {
        status = "Stored channel is missing the frequency/program metadata needed for Watch Live. Rescan channels.";
        return false;
    }
    const int adapter = adapter_index_from_frontend(tuner.frontend_path);
    mrl = "atsc://";
    media_options.push_back(":dvb-adapter=" + std::to_string(adapter));
    const unsigned long long vlc_frequency_khz = frequency >= 10000000ULL ? frequency / 1000ULL : frequency;
    media_options.push_back(":dvb-frequency=" + std::to_string(vlc_frequency_khz));
    media_options.push_back(":dvb-srate=0");
    media_options.push_back(":dvb-modulation=8VSB");
    media_options.push_back(":program=" + std::to_string(program));
    media_options.push_back(":live-caching=900");
    status = "Tuning " + channel.id + " " + channel.name + "...";
    return true;
}

bool NougatTunerBackend::refresh_guide(const TunerDevice& tuner,
                                       std::vector<LiveTvChannel>& channels,
                                       std::vector<LiveTvProgram>& programs,
                                       std::string& status,
                                       const ChannelScanCallback& callback) const {
    programs.clear();
    if (tuner.frontend_path.empty() || !tuner.readable) {
        status = "A readable Linux DVB frontend is required to refresh the broadcast guide.";
        return false;
    }
    if (channels.empty()) channels = load_channels();
    if (channels.empty()) { status = "No stored channels. Scan channels before refreshing the guide."; return false; }

    const int frontend_fd = open(tuner.frontend_path.c_str(), O_RDWR | O_CLOEXEC);
    if (frontend_fd < 0) { status = std::string("Could not open DVB frontend for guide refresh: ") + std::strerror(errno); return false; }
    const std::string demux_path = sibling_device_path(tuner.frontend_path, "demux0");
    if (access(demux_path.c_str(), R_OK | W_OK) != 0) {
        close(frontend_fd); status = "DVB demux0 is unavailable for guide refresh."; return false;
    }

    std::vector<unsigned> frequencies;
    for (const auto& channel : channels) {
        unsigned value = 0;
        try { value = static_cast<unsigned>(std::stoul(channel.frequency)); } catch (...) { value = 0; }
        if (value > 0U && std::find(frequencies.begin(), frequencies.end(), value) == frequencies.end()) frequencies.push_back(value);
    }
    const int total = static_cast<int>(frequencies.size());
    for (int index = 0; index < total; ++index) {
        const unsigned frequency = frequencies[static_cast<std::size_t>(index)];
        ChannelScanProgress progress;
        progress.frequency_hz = frequency;
        progress.completed = index;
        progress.total = total;
        progress.channels_found = static_cast<int>(programs.size());
        progress.message = "Guide: tuning multiplex " + std::to_string(index + 1) + "/" + std::to_string(total) + "...";
        if (callback && !callback(progress)) { close(frontend_fd); status = "Guide refresh cancelled."; return false; }

        std::string tune_error;
        if (!tune_atsc(frontend_fd, frequency, tune_error)) continue;
        fe_status_t frontend_status{};
        if (!wait_for_lock(frontend_fd, frontend_status, 900)) continue;

        int physical = 0;
        for (const auto& channel : channels) if (channel.frequency == std::to_string(frequency) && channel.physical_channel > 0) { physical = channel.physical_channel; break; }
        std::map<std::string, LiveTvChannel> multiplex;
        collect_vct(demux_path, physical, frequency, multiplex);
        std::map<std::uint16_t, std::string> source_to_channel;
        for (const auto& item : multiplex) {
            if (item.second.source_id != 0U) source_to_channel[item.second.source_id] = item.first;
            for (auto& saved : channels) {
                if (saved.id != item.first) continue;
                saved.source_id = item.second.source_id;
                if (saved.program_number <= 0) saved.program_number = item.second.program_number;
                if (saved.physical_channel <= 0) saved.physical_channel = item.second.physical_channel;
            }
        }
        const std::vector<unsigned short> eit_pids = collect_eit_pids(demux_path);
        for (unsigned short pid : eit_pids) collect_eit_from_pid(demux_path, pid, source_to_channel, programs);

        progress.completed = index + 1;
        progress.locked = true;
        progress.channels_found = static_cast<int>(programs.size());
        progress.message = "Guide: " + std::to_string(index + 1) + "/" + std::to_string(total) +
            " multiplexes | " + std::to_string(programs.size()) + " program(s) cached";
        if (callback && !callback(progress)) { close(frontend_fd); status = "Guide refresh cancelled."; return false; }
    }
    close(frontend_fd);
    std::sort(programs.begin(), programs.end(), [](const LiveTvProgram& a, const LiveTvProgram& b) {
        if (a.channel_id != b.channel_id) return a.channel_id < b.channel_id;
        return a.start_unix < b.start_unix;
    });
    std::string error;
    if (!save_channels(channels, error)) { status = "Guide collected, but channel enrichment could not be saved: " + error; return false; }
    if (!save_guide(programs, error)) { status = "Guide collected, but guide cache could not be saved: " + error; return false; }
    status = "Broadcast guide refreshed: " + std::to_string(programs.size()) + " program(s) cached.";
    return true;
}

bool NougatTunerBackend::begin_channel_scan(const TunerDevice& tuner, std::string& status) const {
    if (tuner.frontend_path.empty()) {
        status = tuner.video_path.empty()
            ? "Selected tuner has no Linux device path."
            : "Selected device exposes V4L2 only; an ATSC Linux DVB frontend is required for channel scanning.";
        return false;
    }
    if (!tuner.readable) {
        status = "Tuner is present but Nougat cannot read/write its DVB frontend. Check Linux device permissions.";
        return false;
    }
    const std::string demux = sibling_device_path(tuner.frontend_path, "demux0");
    if (access(demux.c_str(), R_OK | W_OK) != 0) {
        status = "DVB frontend is present, but its demux0 device is unavailable or not permitted.";
        return false;
    }
    status = "ATSC scan-ready: frontend and demux devices are accessible.";
    return true;
}

bool NougatTunerBackend::scan_channels(const TunerDevice& tuner,
                                       std::vector<LiveTvChannel>& channels,
                                       std::string& status,
                                       const ChannelScanCallback& callback) const {
    channels.clear();
    if (!begin_channel_scan(tuner, status)) return false;

    const int frontend_fd = open(tuner.frontend_path.c_str(), O_RDWR | O_CLOEXEC);
    if (frontend_fd < 0) {
        status = std::string("Could not open DVB frontend: ") + std::strerror(errno);
        return false;
    }

    dvb_frontend_info info{};
    if (ioctl(frontend_fd, FE_GET_INFO, &info) != 0) {
        status = std::string("Could not query DVB frontend: ") + std::strerror(errno);
        close(frontend_fd);
        return false;
    }

    const std::string demux_path = sibling_device_path(tuner.frontend_path, "demux0");
    std::map<std::string, LiveTvChannel> found;
    constexpr int first_channel = 2;
    constexpr int last_channel = 36;
    constexpr int total = last_channel - first_channel + 1;

    int completed = 0;
    for (int physical = first_channel; physical <= last_channel; ++physical) {
        const unsigned frequency = atsc_frequency_hz(physical);
        ChannelScanProgress progress;
        progress.physical_channel = physical;
        progress.frequency_hz = frequency;
        progress.completed = completed;
        progress.total = total;
        progress.channels_found = static_cast<int>(found.size());
        progress.message = "Tuning RF " + std::to_string(physical) + "...";
        if (callback && !callback(progress)) {
            status = "Channel scan canceled.";
            close(frontend_fd);
            return false;
        }

        std::string tune_error;
        if (!tune_atsc(frontend_fd, frequency, tune_error)) {
            ++completed;
            progress.completed = completed;
            progress.message = tune_error;
            if (callback && !callback(progress)) {
                status = "Channel scan canceled.";
                close(frontend_fd);
                return false;
            }
            continue;
        }

        fe_status_t frontend_status{};
        progress.locked = wait_for_lock(frontend_fd, frontend_status, 850);
        progress.signal_percent = legacy_metric_percent(frontend_fd, FE_READ_SIGNAL_STRENGTH);
        progress.quality_percent = legacy_metric_percent(frontend_fd, FE_READ_SNR);
        if (progress.locked) collect_vct(demux_path, physical, frequency, found);
        ++completed;
        progress.completed = completed;
        progress.channels_found = static_cast<int>(found.size());
        std::ostringstream message;
        message << "RF " << physical << " " << (progress.locked ? "LOCK" : "no lock")
                << " | " << (frequency / 1000000U) << " MHz"
                << " | " << found.size() << " channel(s) found";
        progress.message = message.str();
        if (callback && !callback(progress)) {
            status = "Channel scan canceled.";
            close(frontend_fd);
            return false;
        }
    }
    close(frontend_fd);

    for (auto& entry : found) channels.push_back(std::move(entry.second));
    std::sort(channels.begin(), channels.end(), [](const LiveTvChannel& a, const LiveTvChannel& b) {
        auto numeric = [](const std::string& id) {
            const std::size_t dot = id.find('.');
            const int major = std::atoi(id.substr(0, dot).c_str());
            const int minor = dot == std::string::npos ? 0 : std::atoi(id.substr(dot + 1).c_str());
            return std::pair<int,int>(major, minor);
        };
        return numeric(a.id) < numeric(b.id);
    });

    std::string save_error;
    if (!save_channels(channels, save_error)) {
        status = "Scan completed, but channel persistence failed: " + save_error;
        return false;
    }
    std::ostringstream done;
    done << "ATSC scan complete: " << channels.size() << " channel(s) stored.";
    status = done.str();
    return true;
}

} // namespace reddmedia
