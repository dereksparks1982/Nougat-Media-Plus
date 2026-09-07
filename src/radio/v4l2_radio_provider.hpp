#pragma once

#include <cstdint>
#include <string>

namespace reddmedia {

struct V4l2RadioProbe {
    std::string path;
    std::string driver;
    std::string card;
    std::string bus_info;
    bool usable = false;
    bool low_frequency_units = false;
    bool supports_stereo = false;
    double minimum_hz = 0.0;
    double maximum_hz = 0.0;
    std::string error;
};

class V4l2RadioProvider {
public:
    V4l2RadioProvider() = default;
    ~V4l2RadioProvider();

    V4l2RadioProvider(const V4l2RadioProvider&) = delete;
    V4l2RadioProvider& operator=(const V4l2RadioProvider&) = delete;

    static V4l2RadioProbe probe(const std::string& path);
    static std::uint32_t frequency_units_from_hz(double hz, bool low_frequency_units);
    static double hz_from_frequency_units(std::uint32_t units, bool low_frequency_units);

    bool open_device(const std::string& path, std::string& error);
    void close_device();
    bool is_open() const;
    const V4l2RadioProbe& info() const;

    bool tune(double hz, std::string& error);
    int signal_percent(std::string& error) const;
    bool stereo_detected(std::string& error) const;

private:
    int fd_ = -1;
    V4l2RadioProbe info_;
};

} // namespace reddmedia
