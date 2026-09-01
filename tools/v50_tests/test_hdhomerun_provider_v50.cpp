#include "live_tv/hdhomerun_provider.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << "\n"; std::exit(1); }
}

int main(int argc, char** argv) {
    require(argc == 2, "fake tool directory argument");
    const std::string root = argv[1];
    setenv("HDHOMERUN_CONFIG", (root + "/hdhomerun_config").c_str(), 1);
    setenv("NOUGAT_CURL", (root + "/curl").c_str(), 1);

    reddmedia::HdHomeRunProvider provider;
    std::string status;
    const auto tuners = provider.detect(status);
    require(tuners.size() == 2U, "FLEX DUO should expose two tuners");
    require(tuners[0].backend == "HDHomeRun", "backend name");
    require(tuners[0].frontend_path == "192.168.1.165", "prefer IPv4 discovery address");

    std::vector<reddmedia::LiveTvChannel> lineup;
    require(provider.load_lineup(tuners[0], lineup, status), "load lineup");
    require(lineup.size() == 2U, "lineup count");
    require(lineup[0].id == "3.1", "numeric channel sort");
    require(lineup[0].service.find("HDHomeRun 1091F714") != std::string::npos, "lineup source identity");

    reddmedia::HdHomeRunTunerStatus runtime;
    require(provider.probe_runtime_status(tuners[0], runtime, status), "status probe");
    require(runtime.signal_percent == 91 && runtime.quality_percent == 100, "signal parse");

    std::string mrl;
    std::vector<std::string> options;
    require(provider.live_playback_input(tuners[1], lineup[0], mrl, options, status), "playback input");
    require(mrl == "http://192.168.1.165:5004/auto/v3.1", "documented HTTP stream URL");

    std::vector<reddmedia::LiveTvChannel> scanned;
    int callbacks = 0;
    require(provider.scan_channels(tuners[1], scanned, status,
        [&callbacks](const reddmedia::ChannelScanProgress&) { ++callbacks; return true; }), "scan");
    require(scanned.size() == 2U, "scan channel parse");
    require(scanned[0].physical_channel == 15, "scan RF parse");
    require(callbacks > 0, "scan callback");

    std::cout << "Nougat v0.0.50 HDHomeRun provider tests PASS\n";
    return 0;
}
