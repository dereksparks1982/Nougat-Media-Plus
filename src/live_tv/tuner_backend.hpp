#pragma once

#include <functional>
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

struct ChannelScanProgress {
    int physical_channel = 0;
    unsigned frequency_hz = 0;
    int completed = 0;
    int total = 0;
    bool locked = false;
    int signal_percent = -1;
    int quality_percent = -1;
    int channels_found = 0;
    std::string message;
};

using ChannelScanCallback = std::function<bool(const ChannelScanProgress&)>;

class NougatTunerBackend {
public:
    NougatTunerBackend();

    std::vector<TunerDevice> detect(std::string& status) const;
    std::vector<LiveTvChannel> load_channels() const;
    bool save_channels(const std::vector<LiveTvChannel>& channels, std::string& error) const;

    // Native ATSC 1.0 over-the-air scan for Linux DVB frontends. The callback
    // is invoked after each physical channel attempt; returning false cancels.
    bool scan_channels(const TunerDevice& tuner,
                       std::vector<LiveTvChannel>& channels,
                       std::string& status,
                       const ChannelScanCallback& callback = {}) const;

    // Retained compatibility probe used by older regression lanes.
    bool begin_channel_scan(const TunerDevice& tuner, std::string& status) const;

    std::string channels_path() const;

private:
    std::string config_dir_;
};

} // namespace reddmedia
