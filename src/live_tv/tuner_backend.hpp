#pragma once

#include <string>
#include <vector>

namespace reddmedia {

struct TunerDevice {
    std::string id;
    std::string name;
    std::string frontend_path;
    std::string video_path;
    std::string backend;
    std::string status;
    bool readable = false;
    bool hauppauge = false;
};

struct LiveTvChannel {
    std::string id;
    std::string name;
    std::string service;
    std::string frequency;
};

class NougatTunerBackend {
public:
    NougatTunerBackend();

    std::vector<TunerDevice> detect(std::string& status) const;
    std::vector<LiveTvChannel> load_channels() const;
    bool save_channels(const std::vector<LiveTvChannel>& channels, std::string& error) const;
    bool begin_channel_scan(const TunerDevice& tuner, std::string& status) const;

    std::string channels_path() const;

private:
    std::string config_dir_;
};

} // namespace reddmedia
