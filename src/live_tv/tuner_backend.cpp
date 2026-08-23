#include "tuner_backend.hpp"

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
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
                tuner.readable = access(tuner.frontend_path.c_str(), R_OK) == 0;
                const fs::path sys_name = fs::path("/sys/class/dvb") / (adapter.path().filename().string() + "." + filename) / "device" / "manufacturer";
                tuner.name = read_first_line(sys_name);
                if (tuner.name.empty()) tuner.name = "DVB frontend " + tuner.id;
                tuner.hauppauge = hauppauge_name(tuner.name);
                tuner.status = tuner.readable ? "Detected / ready to probe" : "Detected / permission required";
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
    if (tuner.frontend_path.empty() && tuner.video_path.empty()) {
        status = "Selected tuner has no Linux device path.";
        return false;
    }
    if (!tuner.readable) {
        status = "Tuner is present but Nougat cannot read it. Check Linux device permissions.";
        return false;
    }
    status = "Tuner probe passed. v0.0.33 channel-scan transport scaffold is ready; frequency tuning/playback comes in the next Live TV stage.";
    return true;
}

} // namespace reddmedia
