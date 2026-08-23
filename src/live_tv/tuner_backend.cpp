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
            found[channel.id] = std::move(channel);
        }
        offset += entry_length;
    }
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

    const fs::path v4l_root("/sys/class/video4linux");
    if (fs::exists(v4l_root, ec)) {
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
        out << result.size() << " tuner interface(s) detected";
        if (has_hauppauge) out << " | Hauppauge/WinTV signature found";
        out << ".";
        status = out.str();
    }
    return result;
}

std::string NougatTunerBackend::channels_path() const { return config_dir_ + "/channels.tsv"; }

std::vector<LiveTvChannel> NougatTunerBackend::load_channels() const {
    std::vector<LiveTvChannel> channels;
    std::ifstream in(channels_path());
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        LiveTvChannel channel;
        if (row >> std::quoted(channel.id) >> std::quoted(channel.name) >> std::quoted(channel.service) >> std::quoted(channel.frequency)) {
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
            << std::quoted(channel.service) << '\t' << std::quoted(channel.frequency) << '\n';
    }
    out.close();
    chmod(temporary.c_str(), 0600);
    if (rename(temporary.c_str(), channels_path().c_str()) != 0) {
        error = "Could not replace Live TV channel database.";
        return false;
    }
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
