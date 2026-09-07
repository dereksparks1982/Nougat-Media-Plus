#include "v4l2_radio_provider.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace reddmedia {
namespace {

std::string bytes_to_string(const __u8* data, std::size_t size) {
    if (data == nullptr || size == 0U) return {};
    const char* first = reinterpret_cast<const char*>(data);
    std::size_t length = 0U;
    while (length < size && first[length] != '\0') ++length;
    return std::string(first, length);
}

double unit_hz(bool low_frequency_units) {
    return low_frequency_units ? 62.5 : 62500.0;
}

bool query_radio(int fd, const std::string& path, V4l2RadioProbe& result) {
    v4l2_capability cap{};
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) != 0) {
        result.error = "VIDIOC_QUERYCAP failed for " + path + ": " + std::strerror(errno);
        return false;
    }

    const std::uint32_t capabilities =
        (cap.capabilities & V4L2_CAP_DEVICE_CAPS) != 0U ? cap.device_caps : cap.capabilities;
    if ((capabilities & V4L2_CAP_RADIO) == 0U || (capabilities & V4L2_CAP_TUNER) == 0U) {
        result.error = path + " is not a V4L2 radio+tuner device.";
        return false;
    }

    v4l2_tuner tuner{};
    tuner.index = 0;
    tuner.type = V4L2_TUNER_RADIO;
    if (ioctl(fd, VIDIOC_G_TUNER, &tuner) != 0) {
        result.error = "VIDIOC_G_TUNER failed for " + path + ": " + std::strerror(errno);
        return false;
    }
    if (tuner.type != V4L2_TUNER_RADIO) {
        result.error = path + " did not report a radio tuner.";
        return false;
    }

    result.path = path;
    result.driver = bytes_to_string(cap.driver, sizeof(cap.driver));
    result.card = bytes_to_string(cap.card, sizeof(cap.card));
    result.bus_info = bytes_to_string(cap.bus_info, sizeof(cap.bus_info));
    result.low_frequency_units = (tuner.capability & V4L2_TUNER_CAP_LOW) != 0U;
    result.supports_stereo = (tuner.capability & V4L2_TUNER_CAP_STEREO) != 0U;
    result.minimum_hz = static_cast<double>(tuner.rangelow) * unit_hz(result.low_frequency_units);
    result.maximum_hz = static_cast<double>(tuner.rangehigh) * unit_hz(result.low_frequency_units);
    result.usable = true;
    result.error.clear();
    return true;
}

} // namespace

V4l2RadioProvider::~V4l2RadioProvider() {
    close_device();
}

V4l2RadioProbe V4l2RadioProvider::probe(const std::string& path) {
    V4l2RadioProbe result;
    result.path = path;
    const int fd = open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        result.error = "Could not open " + path + ": " + std::strerror(errno);
        return result;
    }
    (void)query_radio(fd, path, result);
    close(fd);
    return result;
}

std::uint32_t V4l2RadioProvider::frequency_units_from_hz(double hz, bool low_frequency_units) {
    if (!std::isfinite(hz) || hz <= 0.0) return 0U;
    const long double raw = static_cast<long double>(hz) /
                            static_cast<long double>(unit_hz(low_frequency_units));
    const long double limit = static_cast<long double>(std::numeric_limits<std::uint32_t>::max());
    if (raw >= limit) return std::numeric_limits<std::uint32_t>::max();
    return static_cast<std::uint32_t>(std::llround(raw));
}

double V4l2RadioProvider::hz_from_frequency_units(std::uint32_t units, bool low_frequency_units) {
    return static_cast<double>(units) * unit_hz(low_frequency_units);
}

bool V4l2RadioProvider::open_device(const std::string& path, std::string& error) {
    close_device();
    const int fd = open(path.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        error = "Could not open " + path + ": " + std::strerror(errno);
        return false;
    }

    V4l2RadioProbe result;
    result.path = path;
    if (!query_radio(fd, path, result)) {
        error = result.error;
        close(fd);
        return false;
    }

    fd_ = fd;
    info_ = std::move(result);
    error.clear();
    return true;
}

void V4l2RadioProvider::close_device() {
    if (fd_ >= 0) close(fd_);
    fd_ = -1;
    info_ = {};
}

bool V4l2RadioProvider::is_open() const {
    return fd_ >= 0;
}

const V4l2RadioProbe& V4l2RadioProvider::info() const {
    return info_;
}

bool V4l2RadioProvider::tune(double hz, std::string& error) {
    if (fd_ < 0) {
        error = "V4L2 radio device is not open.";
        return false;
    }
    if (!std::isfinite(hz) || hz <= 0.0) {
        error = "Invalid FM frequency.";
        return false;
    }
    if (info_.minimum_hz > 0.0 && hz < info_.minimum_hz) {
        error = "Requested frequency is below the radio tuner's reported range.";
        return false;
    }
    if (info_.maximum_hz > 0.0 && hz > info_.maximum_hz) {
        error = "Requested frequency is above the radio tuner's reported range.";
        return false;
    }

    v4l2_frequency frequency{};
    frequency.tuner = 0;
    frequency.type = V4L2_TUNER_RADIO;
    frequency.frequency = frequency_units_from_hz(hz, info_.low_frequency_units);
    if (frequency.frequency == 0U || ioctl(fd_, VIDIOC_S_FREQUENCY, &frequency) != 0) {
        error = "VIDIOC_S_FREQUENCY failed: " + std::string(std::strerror(errno));
        return false;
    }

    if (info_.supports_stereo) {
        v4l2_tuner tuner{};
        tuner.index = 0;
        tuner.type = V4L2_TUNER_RADIO;
        tuner.audmode = V4L2_TUNER_MODE_STEREO;
        // Stereo preference is advisory. Some drivers reject S_TUNER while
        // still producing valid FM audio, so tuning success does not depend on it.
        (void)ioctl(fd_, VIDIOC_S_TUNER, &tuner);
    }

    error.clear();
    return true;
}

int V4l2RadioProvider::signal_percent(std::string& error) const {
    if (fd_ < 0) {
        error = "V4L2 radio device is not open.";
        return -1;
    }
    v4l2_tuner tuner{};
    tuner.index = 0;
    tuner.type = V4L2_TUNER_RADIO;
    if (ioctl(fd_, VIDIOC_G_TUNER, &tuner) != 0) {
        error = "VIDIOC_G_TUNER signal query failed: " + std::string(std::strerror(errno));
        return -1;
    }
    error.clear();
    const unsigned value = (static_cast<unsigned>(tuner.signal) * 100U + 32767U) / 65535U;
    return static_cast<int>(std::min(100U, value));
}

bool V4l2RadioProvider::stereo_detected(std::string& error) const {
    if (fd_ < 0) {
        error = "V4L2 radio device is not open.";
        return false;
    }
    v4l2_tuner tuner{};
    tuner.index = 0;
    tuner.type = V4L2_TUNER_RADIO;
    if (ioctl(fd_, VIDIOC_G_TUNER, &tuner) != 0) {
        error = "VIDIOC_G_TUNER stereo query failed: " + std::string(std::strerror(errno));
        return false;
    }
    error.clear();
    return (tuner.rxsubchans & V4L2_TUNER_SUB_STEREO) != 0U;
}

} // namespace reddmedia
