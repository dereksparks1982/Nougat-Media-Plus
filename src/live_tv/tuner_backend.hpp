#pragma once

#include <functional>
#include <cstdint>
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
    int program_number = 0;
    int physical_channel = 0;
    std::uint16_t source_id = 0;
};

struct LiveTvProgram {
    std::string channel_id;
    std::string title;
    std::string description;
    long long start_unix = 0;
    int duration_seconds = 0;
    unsigned event_id = 0;
};

struct TunerRuntimeStatus {
    bool frontend_accessible = false;
    bool demux_accessible = false;
    bool dvr_accessible = false;
    bool net_accessible = false;
    bool signal_lock = false;
    int signal_percent = -1;
    int quality_percent = -1;
    std::string delivery_systems;
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
    std::vector<LiveTvProgram> load_guide() const;
    bool save_guide(const std::vector<LiveTvProgram>& programs, std::string& error) const;

    // Build the libVLC ATSC input used by Nougat's native player. The tuner
    // remains a Linux DVB device; libVLC is only the A/V demux/decode/output
    // endpoint already used everywhere else in Nougat.
    bool live_playback_input(const TunerDevice& tuner,
                             const LiveTvChannel& channel,
                             std::string& mrl,
                             std::vector<std::string>& media_options,
                             std::string& status) const;

    // Harvest the first several hours of ATSC PSIP EIT data from persisted
    // multiplexes. Existing v0.0.35 channel databases are enriched from VCT
    // while harvesting so source/program/RF metadata does not require a rescan.
    // Read EIT/VCT from the multiplex already tuned for Live TV. This opens
    // demux filters only and never retunes the frontend, so playback continues.
    bool harvest_current_multiplex_guide(const TunerDevice& tuner,
                                         const LiveTvChannel& current_channel,
                                         const std::vector<LiveTvChannel>& channels,
                                         std::vector<LiveTvProgram>& programs,
                                         std::string& status) const;
    bool refresh_guide(const TunerDevice& tuner,
                       std::vector<LiveTvChannel>& channels,
                       std::vector<LiveTvProgram>& programs,
                       std::string& status,
                       const ChannelScanCallback& callback = {}) const;
    bool refresh_current_multiplex_guide(const TunerDevice& tuner,
                       const LiveTvChannel& current_channel,
                       std::vector<LiveTvChannel>& channels,
                       std::vector<LiveTvProgram>& programs,
                       std::string& status) const;

    bool probe_runtime_status(const TunerDevice& tuner,
                              TunerRuntimeStatus& runtime,
                              std::string& status) const;

    // Native ATSC 1.0 over-the-air scan for Linux DVB frontends. The callback
    // is invoked after each physical channel attempt; returning false cancels.
    bool scan_channels(const TunerDevice& tuner,
                       std::vector<LiveTvChannel>& channels,
                       std::string& status,
                       const ChannelScanCallback& callback = {}) const;

    // Retained compatibility probe used by older regression lanes.
    bool begin_channel_scan(const TunerDevice& tuner, std::string& status) const;

    std::string channels_path() const;
    std::string guide_path() const;

private:
    std::string config_dir_;
};

} // namespace reddmedia
