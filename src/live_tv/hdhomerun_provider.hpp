#pragma once

#include "tuner_backend.hpp"

#include <string>
#include <vector>

namespace reddmedia {

struct HdHomeRunDevice {
    std::string device_id;
    std::string address;
    std::string model;
    int tuner_count = 0;
};

struct HdHomeRunTunerStatus {
    bool accessible = false;
    bool busy = false;
    std::string channel;
    std::string lock;
    int signal_percent = -1;
    int quality_percent = -1;
    int symbol_quality_percent = -1;
    long long bits_per_second = 0;
    long long packets_per_second = 0;
    std::string raw;
};

class HdHomeRunProvider {
public:
    std::vector<TunerDevice> detect(std::string& status) const;
    std::vector<HdHomeRunDevice> discover_devices(std::string& status) const;

    bool load_lineup(const TunerDevice& tuner,
                     std::vector<LiveTvChannel>& channels,
                     std::string& status) const;

    bool scan_channels(const TunerDevice& tuner,
                       std::vector<LiveTvChannel>& channels,
                       std::string& status,
                       const ChannelScanCallback& callback = {}) const;

    bool live_playback_input(const TunerDevice& tuner,
                             const LiveTvChannel& channel,
                             std::string& mrl,
                             std::vector<std::string>& media_options,
                             std::string& status) const;

    bool probe_runtime_status(const TunerDevice& tuner,
                              HdHomeRunTunerStatus& runtime,
                              std::string& status) const;

    bool release_tuner(const TunerDevice& tuner, std::string& status) const;

    static bool is_hdhomerun_tuner(const TunerDevice& tuner);
    static bool decode_tuner_id(const TunerDevice& tuner,
                                std::string& device_id,
                                int& tuner_index);
};

} // namespace reddmedia
