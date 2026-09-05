#include "satellite_center.h"

extern "C" {
#include "../components/satellite/sgp4/sgp4.h"
}

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace nougat::satellite {
namespace {

std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string run(const char* command) {
    std::array<char, 512> buf{};
    std::string out;
    FILE* pipe = ::popen(command, "r");
    if (!pipe) return {};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe)) out += buf.data();
    ::pclose(pipe);
    return trim(out);
}

std::string home_dir() {
    if (const char* h = std::getenv("HOME")) return h;
    return {};
}

std::string default_tle_path() {
    const std::string home = home_dir();
    if (home.empty()) return {};
    return home + "/.config/nougat/satellite/catalog.tle";
}

std::string available(const char* command) {
    return run(command).empty() ? "not detected" : "available";
}

bool first_tle(const std::string& path, std::string& name, std::string& l1, std::string& l2) {
    std::ifstream f(path);
    if (!f) return false;
    std::string a, b, c;
    while (std::getline(f, a)) {
        a = trim(a);
        if (a.empty()) continue;
        if (a.rfind("1 ", 0) == 0) {
            b = a;
            if (!std::getline(f, c)) return false;
            c = trim(c);
            if (c.rfind("2 ", 0) != 0) continue;
            name = "Unnamed satellite";
            l1 = b;
            l2 = c;
            return true;
        }
        name = a;
        if (!std::getline(f, b) || !std::getline(f, c)) return false;
        b = trim(b); c = trim(c);
        if (b.rfind("1 ", 0) == 0 && c.rfind("2 ", 0) == 0) {
            l1 = b; l2 = c; return true;
        }
    }
    return false;
}

}  // namespace

Snapshot collect_snapshot() {
    Snapshot s;
    const std::string tle_path = default_tle_path();
    s.track.emplace_back("Orbital engine: SGP4/SDP4 MIT component installed");
    s.track.emplace_back("Catalog path: " + (tle_path.empty() ? std::string("unavailable") : tle_path));

    std::string name, line1, line2;
    if (!tle_path.empty() && first_tle(tle_path, name, line1, line2)) {
        sgp4_tle_t tle{};
        const sgp4_error_t parse = sgp4_parse_tle_3line(name.c_str(), line1.c_str(), line2.c_str(), &tle);
        if (parse == SGP4_SUCCESS) {
            sgp4_elements_t elements{};
            sgp4_state_t state{};
            if (sgp4_tle_to_elements(&tle, &elements) == SGP4_SUCCESS && sgp4_init(&state, &elements) == SGP4_SUCCESS) {
                const auto now = std::chrono::system_clock::now().time_since_epoch();
                const double unix_seconds = std::chrono::duration<double>(now).count();
                const double now_jd = SGP4_UNIX_EPOCH_JD + unix_seconds / 86400.0;
                const double tsince_min = (now_jd - elements.epoch_jd) * 1440.0;
                sgp4_result_t result{};
                const sgp4_error_t propagated = sgp4_propagate(&state, tsince_min, &result);
                s.track.emplace_back("Satellite: " + std::string(tle.name[0] ? tle.name : name.c_str()));
                s.track.emplace_back("NORAD ID: " + std::to_string(tle.norad_id));
                if (propagated == SGP4_SUCCESS) {
                    std::ostringstream pos;
                    pos.setf(std::ios::fixed); pos.precision(2);
                    pos << "TEME position km: X " << result.r[0] << "  Y " << result.r[1] << "  Z " << result.r[2];
                    s.track.push_back(pos.str());
                    std::ostringstream vel;
                    vel.setf(std::ios::fixed); vel.precision(3);
                    vel << "Velocity km/s: X " << result.v[0] << "  Y " << result.v[1] << "  Z " << result.v[2];
                    s.track.push_back(vel.str());
                } else {
                    s.track.emplace_back("Propagation: TLE loaded but current propagation failed");
                }
            } else {
                s.track.emplace_back("TLE loaded but SGP4 initialization failed");
            }
        } else {
            s.track.emplace_back("TLE catalog found but first record could not be parsed");
        }
    } else {
        s.track.emplace_back("Catalog status: no local TLE catalog yet");
        s.track.emplace_back("Tracking remains truthful: no satellite position is invented without orbital data.");
    }

    s.passes.emplace_back("Pass prediction foundation: SGP4 orbital propagation ready");
    s.passes.emplace_back("Observer location and pass scheduler are not configured yet.");
    s.passes.emplace_back("Planned outputs: AOS, maximum elevation, LOS, duration, azimuth/elevation and Doppler.");

    s.receive.emplace_back("Receive workspace: hardware-neutral foundation active");
    s.receive.emplace_back("RTL-SDR tools: " + available("command -v rtl_sdr 2>/dev/null"));
    s.receive.emplace_back("SoapySDR tools: " + available("command -v SoapySDRUtil 2>/dev/null"));
    s.receive.emplace_back("No receiver is reported READY unless compatible hardware/software is actually detected.");

    s.decode.emplace_back("Decode workspace foundation active");
    s.decode.emplace_back("Telemetry, image and packet decoders attach here as supported backends are added.");
    s.decode.emplace_back("Encrypted/unsupported data remains labeled encrypted, unsupported or unknown.");

    s.transmit.emplace_back("TX STATE: DISABLED");
    s.transmit.emplace_back("No transmitter backend is armed by this foundation build.");
    s.transmit.emplace_back("Future controls: uplink frequency, modulation, bandwidth, power, Doppler compensation and logging.");
    s.transmit.emplace_back("Authorization remains the operator's responsibility under the Nougat lawful-use notice.");

    s.imagery.emplace_back("Earth Observation / Satellite Imagery foundation active");
    s.imagery.emplace_back("GDAL tools: " + available("command -v gdalinfo 2>/dev/null"));
    s.imagery.emplace_back("Source classes: ONLINE SATELLITE DATA and DIRECT SATELLITE RECEPTION");
    s.imagery.emplace_back("Planned processing: natural color, infrared, thermal, false color, raw bands and before/after comparison where source data supports them.");

    s.antenna.emplace_back("Antenna/rotator control foundation active");
    s.antenna.emplace_back("Hamlib rotator tools: " + available("command -v rotctl 2>/dev/null"));
    s.antenna.emplace_back("Automatic azimuth/elevation tracking remains disabled until a supported rotator and observer location are configured.");

    s.hardware.emplace_back("SGP4 orbital engine: available");
    s.hardware.emplace_back("GDAL command tools: " + available("command -v gdalinfo 2>/dev/null"));
    s.hardware.emplace_back("RTL-SDR: " + available("command -v rtl_test 2>/dev/null"));
    s.hardware.emplace_back("SoapySDR: " + available("command -v SoapySDRUtil 2>/dev/null"));
    s.hardware.emplace_back("Hamlib radio: " + available("command -v rigctl 2>/dev/null"));
    s.hardware.emplace_back("Hamlib rotator: " + available("command -v rotctl 2>/dev/null"));

    s.logs.emplace_back("Satellite Center foundation loaded.");
    s.logs.emplace_back("No RF transmission was performed.");
    s.logs.emplace_back("No satellite position is shown without a valid TLE and successful SGP4 propagation.");
    return s;
}

}  // namespace nougat::satellite
