#include "radio_backend.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glob.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace reddmedia {
namespace {

std::string home_dir() {
    const char* home = std::getenv("HOME");
    return home != nullptr ? std::string(home) : std::string(".");
}

int clamp_percent(int value) {
    return std::max(0, std::min(100, value));
}

bool contains_case_insensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    std::string a = haystack;
    std::string b = needle;
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return a.find(b) != std::string::npos;
}

std::string trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) value.erase(value.begin());
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) value.pop_back();
    return value;
}

std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> out;
    std::string current;
    for (char c : line) {
        if (c == ',') {
            out.push_back(trim(current));
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    out.push_back(trim(current));
    return out;
}

} // namespace

RadioBackend::RadioBackend() {
    std::error_code ec;
    std::filesystem::create_directories(config_dir(), ec);
    std::filesystem::create_directories(data_dir(), ec);
    std::filesystem::create_directories(cache_dir(), ec);
    std::filesystem::create_directories(recordings_dir(), ec);
    refresh();
}

RadioBackend::~RadioBackend() {
    cancel_scan();
    stop_receive();
}

std::string RadioBackend::config_dir() {
    return home_dir() + "/.config/reddmedia";
}

std::string RadioBackend::data_dir() {
    return home_dir() + "/.local/share/reddmedia/radio";
}

std::string RadioBackend::cache_dir() {
    return home_dir() + "/.cache/reddmedia/radio";
}

std::string RadioBackend::favorites_path() {
    return config_dir() + "/radio_favorites.tsv";
}

std::string RadioBackend::recordings_dir() {
    return data_dir() + "/recordings";
}

std::string RadioBackend::shell_quote(const std::string& value) {
    std::string out = "'";
    for (char c : value) {
        if (c == '\'') out += "'\\''";
        else out.push_back(c);
    }
    out += "'";
    return out;
}

bool RadioBackend::executable_available(const std::string& name) {
    if (name.empty()) return false;
    if (name.find('/') != std::string::npos) return access(name.c_str(), X_OK) == 0;
    const char* path = std::getenv("PATH");
    if (path == nullptr) return false;
    std::stringstream ss(path);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) dir = ".";
        const std::string candidate = dir + "/" + name;
        if (access(candidate.c_str(), X_OK) == 0) return true;
    }
    return false;
}

std::string RadioBackend::run_capture(const std::string& command) {
    std::array<char, 4096> buffer{};
    std::string output;
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) return output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
        if (output.size() > 256U * 1024U) break;
    }
    pclose(pipe);
    return output;
}

std::vector<std::string> RadioBackend::glob_paths(const std::string& pattern) {
    glob_t result{};
    std::vector<std::string> paths;
    if (glob(pattern.c_str(), GLOB_NOSORT, nullptr, &result) == 0) {
        for (std::size_t i = 0; i < result.gl_pathc; ++i) paths.emplace_back(result.gl_pathv[i]);
    }
    globfree(&result);
    std::sort(paths.begin(), paths.end());
    return paths;
}

std::string RadioBackend::timestamp_name() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t when = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&when, &tm);
    std::ostringstream out;
    out << std::put_time(&tm, "%Y%m%d-%H%M%S");
    return out.str();
}

const char* RadioBackend::modulation_name(RadioModulation modulation) {
    switch (modulation) {
        case RadioModulation::AM: return "AM";
        case RadioModulation::NFM: return "NFM";
        case RadioModulation::WFM: return "WFM";
        case RadioModulation::USB: return "USB";
        case RadioModulation::LSB: return "LSB";
        case RadioModulation::CW: return "CW";
        case RadioModulation::DAB: return "DAB/DAB+";
        case RadioModulation::DRM: return "DRM";
        case RadioModulation::P25: return "P25";
        case RadioModulation::RAW: return "RAW IQ";
    }
    return "NFM";
}

RadioModulation RadioBackend::modulation_from_name(const std::string& value) {
    if (value == "AM") return RadioModulation::AM;
    if (value == "NFM") return RadioModulation::NFM;
    if (value == "WFM") return RadioModulation::WFM;
    if (value == "USB") return RadioModulation::USB;
    if (value == "LSB") return RadioModulation::LSB;
    if (value == "CW") return RadioModulation::CW;
    if (value == "DAB/DAB+") return RadioModulation::DAB;
    if (value == "DRM") return RadioModulation::DRM;
    if (value == "P25") return RadioModulation::P25;
    if (value == "RAW IQ") return RadioModulation::RAW;
    return RadioModulation::NFM;
}

void RadioBackend::update_runtime_capabilities_locked() {
    state_.soapy_available = executable_available("SoapySDRUtil");
    state_.rtl_available = executable_available("rtl_fm");
    state_.op25_available = executable_available("rx.py") || executable_available("op25.sh") ||
                            std::filesystem::exists("components/radio/upstream/op25");
    state_.dab_decoder_available = executable_available("welle-cli") || executable_available("welle.io") ||
                                   std::filesystem::exists("components/radio/upstream/welle.io");
    state_.drm_decoder_available = executable_available("dream") ||
                                   std::filesystem::exists("components/radio/upstream/dream");
    state_.satellite_decoder_available = executable_available("satdump") ||
                                         std::filesystem::exists("components/radio/upstream/satdump");
}

void RadioBackend::detect_devices_locked() {
    state_.devices.clear();

    for (const std::string& path : glob_paths("/dev/radio*")) {
        RadioDevice device;
        device.id = path;
        device.name = "Linux V4L2 radio " + path;
        device.backend = "V4L2 Radio";
        device.notes = "Kernel radio device. Frequency/audio capabilities come from the attached tuner driver.";
        device.receive = access(path.c_str(), R_OK) == 0;
        device.minimum_hz = 65000000.0;
        device.maximum_hz = 108000000.0;
        state_.devices.push_back(device);
    }

    for (const std::string& path : glob_paths("/dev/dvb/adapter*/frontend*")) {
        RadioDevice device;
        device.id = path;
        device.name = "TV antenna frontend " + path;
        device.backend = "Linux DVB / TV";
        device.notes = "Broadcast-TV RF frontend. Use Nougat's Antenna/TV scan. It is not treated as an AM/FM/shortwave SDR unless Linux exposes a radio/raw-IQ interface.";
        device.receive = access(path.c_str(), R_OK) == 0;
        state_.devices.push_back(device);
    }

    if (state_.soapy_available) {
        const std::string output = run_capture("timeout 4 SoapySDRUtil --find 2>&1");
        std::istringstream lines(output);
        std::string line;
        int index = 0;
        while (std::getline(lines, line)) {
            if (!contains_case_insensitive(line, "driver") && !contains_case_insensitive(line, "device")) continue;
            if (contains_case_insensitive(line, "no devices")) continue;
            RadioDevice device;
            device.id = "soapy:" + std::to_string(index++);
            device.name = trim(line);
            if (device.name.empty()) device.name = "SoapySDR device";
            device.backend = "SoapySDR";
            device.notes = "Vendor-neutral SDR device discovered by SoapySDR. Actual frequency range and TX support are probed by the device driver.";
            device.receive = true;
            state_.devices.push_back(device);
        }
    }

    if (executable_available("hackrf_info")) {
        const std::string output = run_capture("timeout 4 hackrf_info 2>&1");
        if (contains_case_insensitive(output, "serial number") || contains_case_insensitive(output, "board id")) {
            RadioDevice device;
            device.id = "hackrf:0";
            device.name = "HackRF transceiver";
            device.backend = "HackRF";
            device.notes = "Wideband SDR transceiver. RX/TX hardware capability detected; Nougat keeps RF TX disabled by default.";
            device.receive = true;
            device.transmit = true;
            device.minimum_hz = 1000000.0;
            device.maximum_hz = 6000000000.0;
            state_.devices.push_back(device);
        }
    }

    if (executable_available("LimeUtil")) {
        const std::string output = run_capture("timeout 4 LimeUtil --find 2>&1");
        if (!trim(output).empty() && !contains_case_insensitive(output, "no devices")) {
            RadioDevice device;
            device.id = "lime:0";
            device.name = "LimeSDR transceiver";
            device.backend = "LimeSuite";
            device.notes = "RX/TX SDR hardware discovered. Nougat keeps RF TX disabled by default.";
            device.receive = true;
            device.transmit = true;
            state_.devices.push_back(device);
        }
    }

    if (executable_available("uhd_find_devices")) {
        const std::string output = run_capture("timeout 4 uhd_find_devices 2>&1");
        if (contains_case_insensitive(output, "serial") || contains_case_insensitive(output, "product")) {
            RadioDevice device;
            device.id = "uhd:0";
            device.name = "UHD / USRP transceiver";
            device.backend = "UHD";
            device.notes = "USRP-class SDR discovered. RX/TX depends on the attached model; RF TX remains disabled by default.";
            device.receive = true;
            device.transmit = true;
            state_.devices.push_back(device);
        }
    }

    if (executable_available("airspy_info")) {
        const std::string output = run_capture("timeout 4 airspy_info 2>&1");
        if (contains_case_insensitive(output, "serial")) {
            RadioDevice device;
            device.id = "airspy:0";
            device.name = "Airspy receiver";
            device.backend = "Airspy";
            device.notes = "Receive-only SDR detected.";
            device.receive = true;
            state_.devices.push_back(device);
        }
    }

    if (executable_available("airspyhf_info")) {
        const std::string output = run_capture("timeout 4 airspyhf_info 2>&1");
        if (contains_case_insensitive(output, "serial")) {
            RadioDevice device;
            device.id = "airspyhf:0";
            device.name = "Airspy HF+ receiver";
            device.backend = "Airspy HF+";
            device.notes = "HF/VHF receive-only SDR detected.";
            device.receive = true;
            state_.devices.push_back(device);
        }
    }

    state_.tx_hardware_available = std::any_of(state_.devices.begin(), state_.devices.end(),
                                               [](const RadioDevice& device) { return device.transmit; });
    if (state_.selected_device < 0 || state_.selected_device >= static_cast<int>(state_.devices.size())) {
        state_.selected_device = state_.devices.empty() ? -1 : 0;
    }
}

void RadioBackend::refresh() {
    std::lock_guard<std::mutex> lock(mutex_);
    update_runtime_capabilities_locked();
    detect_devices_locked();
    if (state_.devices.empty()) {
        state_.status = "No hardware radio receiver detected. Internet Radio remains available; connect a supported SDR/receiver for RF modes.";
    } else {
        std::ostringstream status;
        status << state_.devices.size() << " receive device" << (state_.devices.size() == 1U ? "" : "s") << " detected.";
        if (state_.rtl_available) status << " RTL receive engine ready.";
        if (state_.soapy_available) status << " SoapySDR ready.";
        state_.status = status.str();
    }
}

RadioSnapshot RadioBackend::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

void RadioBackend::set_frequency(double hz) {
    if (!std::isfinite(hz)) return;
    std::lock_guard<std::mutex> lock(mutex_);
    state_.frequency_hz = std::max(1000.0, std::min(6000000000.0, hz));
}

void RadioBackend::step_frequency(double delta_hz) {
    if (!std::isfinite(delta_hz)) return;
    std::lock_guard<std::mutex> lock(mutex_);
    state_.frequency_hz = std::max(1000.0, std::min(6000000000.0, state_.frequency_hz + delta_hz));
}

void RadioBackend::set_modulation(RadioModulation modulation) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.modulation = modulation;
}

void RadioBackend::cycle_modulation(int direction) {
    static constexpr std::array<RadioModulation, 10> modes = {
        RadioModulation::AM, RadioModulation::NFM, RadioModulation::WFM, RadioModulation::USB,
        RadioModulation::LSB, RadioModulation::CW, RadioModulation::DAB, RadioModulation::DRM,
        RadioModulation::P25, RadioModulation::RAW
    };
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(modes.begin(), modes.end(), state_.modulation);
    std::size_t index = it == modes.end() ? 0U : static_cast<std::size_t>(std::distance(modes.begin(), it));
    if (direction >= 0) index = (index + 1U) % modes.size();
    else index = (index + modes.size() - 1U) % modes.size();
    state_.modulation = modes[index];
}

void RadioBackend::set_tuning_step(double hz) {
    if (!std::isfinite(hz) || hz <= 0.0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    state_.tuning_step_hz = std::max(1.0, std::min(10000000.0, hz));
}

void RadioBackend::cycle_tuning_step(int direction) {
    static constexpr std::array<double, 10> steps = {
        10.0, 100.0, 1000.0, 2500.0, 5000.0, 12500.0, 25000.0, 100000.0, 200000.0, 1000000.0
    };
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::min_element(steps.begin(), steps.end(), [this](double a, double b) {
        return std::fabs(a - state_.tuning_step_hz) < std::fabs(b - state_.tuning_step_hz);
    });
    std::size_t index = static_cast<std::size_t>(std::distance(steps.begin(), it));
    if (direction >= 0) index = (index + 1U) % steps.size();
    else index = (index + steps.size() - 1U) % steps.size();
    state_.tuning_step_hz = steps[index];
}

void RadioBackend::set_gain_percent(int percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.gain_percent = clamp_percent(percent);
}

void RadioBackend::set_squelch(int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.squelch = std::max(0, std::min(100, value));
}

void RadioBackend::cycle_device(int direction) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_.devices.empty()) {
        state_.selected_device = -1;
        return;
    }
    const int count = static_cast<int>(state_.devices.size());
    int index = state_.selected_device < 0 ? 0 : state_.selected_device;
    index = (index + (direction >= 0 ? 1 : count - 1)) % count;
    state_.selected_device = index;
}

void RadioBackend::stop_receive_locked() {
    if (receive_pid_ > 0) {
        kill(-receive_pid_, SIGTERM);
        for (int attempt = 0; attempt < 20; ++attempt) {
            int status = 0;
            const pid_t result = waitpid(receive_pid_, &status, WNOHANG);
            if (result == receive_pid_ || (result < 0 && errno == ECHILD)) break;
            usleep(50000);
        }
        if (kill(receive_pid_, 0) == 0) kill(-receive_pid_, SIGKILL);
        int ignored = 0;
        waitpid(receive_pid_, &ignored, WNOHANG);
    }
    receive_pid_ = -1;
    state_.receiving = false;
}

bool RadioBackend::spawn_receive_pipeline_locked(std::string& status) {
    if (!state_.rtl_available) {
        status = "No active analog SDR receive engine. Install/build an RTL-SDR or compatible Soapy receive frontend first.";
        state_.status = status;
        return false;
    }
    if (!executable_available("aplay")) {
        status = "ALSA aplay is required for the current RTL receive audio path.";
        state_.status = status;
        return false;
    }

    std::string rtl_mode;
    switch (state_.modulation) {
        case RadioModulation::AM: rtl_mode = "am"; break;
        case RadioModulation::NFM: rtl_mode = "fm"; break;
        case RadioModulation::WFM: rtl_mode = "wbfm"; break;
        case RadioModulation::USB: rtl_mode = "usb"; break;
        case RadioModulation::LSB: rtl_mode = "lsb"; break;
        case RadioModulation::CW: rtl_mode = "usb"; break;
        case RadioModulation::RAW: rtl_mode = "raw"; break;
        case RadioModulation::DAB:
            status = state_.dab_decoder_available ? "DAB engine detected; DAB ensemble tuning is handled by the dedicated decoder worker." :
                                                    "DAB/DAB+ decoder runtime is not installed.";
            state_.status = status;
            return false;
        case RadioModulation::DRM:
            status = state_.drm_decoder_available ? "DRM engine detected; DRM decoding is handled by the dedicated decoder worker." :
                                                    "DRM decoder runtime is not installed.";
            state_.status = status;
            return false;
        case RadioModulation::P25:
            status = state_.op25_available ? "OP25 is present; choose/import a trunking system configuration before starting P25 decode." :
                                             "OP25 runtime is not installed.";
            state_.status = status;
            return false;
    }

    const long long hz = static_cast<long long>(std::llround(state_.frequency_hz));
    const int squelch = state_.squelch;
    const int sample_rate = state_.modulation == RadioModulation::WFM ? 200000 :
                            (state_.modulation == RadioModulation::NFM ? 24000 : 12000);
    std::ostringstream command;
    command << "exec rtl_fm -f " << hz << " -M " << rtl_mode
            << " -s " << sample_rate << " -r 48000 -l " << squelch << " 2>/dev/null";

    if (state_.recording) {
        if (!executable_available("ffmpeg")) {
            status = "Recording requires ffmpeg; receive was not started so Record cannot silently fail.";
            state_.status = status;
            return false;
        }
        std::error_code ec;
        std::filesystem::create_directories(recordings_dir(), ec);
        current_recording_path_ = recordings_dir() + "/radio-" + timestamp_name() + ".wav";
        command << " | tee >(aplay -q -r 48000 -f S16_LE -c 1 2>/dev/null)"
                << " | ffmpeg -nostdin -loglevel error -y -f s16le -ar 48000 -ac 1 -i pipe:0 "
                << shell_quote(current_recording_path_);
    } else {
        current_recording_path_.clear();
        command << " | aplay -q -r 48000 -f S16_LE -c 1 2>/dev/null";
    }

    const pid_t pid = fork();
    if (pid < 0) {
        status = std::string("Could not start radio receiver: ") + std::strerror(errno);
        state_.status = status;
        return false;
    }
    if (pid == 0) {
        setsid();
        execl("/bin/bash", "bash", "-c", command.str().c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    receive_pid_ = pid;
    state_.receiving = true;
    std::ostringstream text;
    text << "Receiving " << std::fixed << std::setprecision(6) << (state_.frequency_hz / 1000000.0)
         << " MHz " << modulation_name(state_.modulation);
    if (state_.recording) text << " and recording to " << current_recording_path_;
    status = text.str();
    state_.status = status;
    return true;
}

bool RadioBackend::start_receive(std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_receive_locked();
    return spawn_receive_pipeline_locked(status);
}

void RadioBackend::stop_receive() {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_receive_locked();
    state_.status = "Radio receive stopped.";
}

bool RadioBackend::start_scan(double minimum_hz, double maximum_hz, double step_hz, std::string& status) {
    if (!std::isfinite(minimum_hz) || !std::isfinite(maximum_hz) || !std::isfinite(step_hz) ||
        minimum_hz <= 0.0 || maximum_hz <= minimum_hz || step_hz <= 0.0) {
        status = "Invalid scan range.";
        return false;
    }
    if (!executable_available("rtl_power")) {
        status = "rtl_power is not available, so a spectrum scan cannot be started with the current runtime.";
        std::lock_guard<std::mutex> lock(mutex_);
        state_.status = status;
        return false;
    }
    cancel_scan();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.scanning = true;
        state_.status = "Scanning RF range asynchronously...";
    }
    scan_cancel_.store(false);
    scan_thread_ = std::thread(&RadioBackend::finish_scan, this, minimum_hz, maximum_hz, step_hz);
    status = "Scanning RF range asynchronously...";
    return true;
}

void RadioBackend::finish_scan(double minimum_hz, double maximum_hz, double step_hz) {
    const std::string output = cache_dir() + "/scan-" + timestamp_name() + ".csv";
    std::error_code ec;
    std::filesystem::create_directories(cache_dir(), ec);

    const long long min_hz = static_cast<long long>(std::llround(minimum_hz));
    const long long max_hz = static_cast<long long>(std::llround(maximum_hz));
    const long long step = static_cast<long long>(std::llround(step_hz));
    std::ostringstream range;
    range << min_hz << ':' << max_hz << ':' << step;
    const pid_t scan_pid = fork();
    int rc = -1;
    if (scan_pid == 0) {
        setsid();
        const std::string range_arg = range.str();
        execlp("rtl_power", "rtl_power", "-f", range_arg.c_str(), "-i", "1", "-e", "5s", output.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    } else if (scan_pid > 0) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
        while (true) {
            int status_word = 0;
            const pid_t waited = waitpid(scan_pid, &status_word, WNOHANG);
            if (waited == scan_pid) {
                rc = WIFEXITED(status_word) ? WEXITSTATUS(status_word) : 128;
                break;
            }
            if (waited < 0 && errno == ECHILD) { rc = 0; break; }
            if (scan_cancel_.load() || std::chrono::steady_clock::now() >= deadline) {
                kill(-scan_pid, SIGTERM);
                usleep(100000);
                kill(-scan_pid, SIGKILL);
                waitpid(scan_pid, &status_word, 0);
                rc = scan_cancel_.load() ? 130 : 124;
                break;
            }
            usleep(50000);
        }
    }

    if (scan_cancel_.load()) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.scanning = false;
        state_.status = "RF scan cancelled.";
        return;
    }

    double strongest_db = -1000.0;
    double strongest_hz = 0.0;
    std::ifstream input(output);
    std::string line;
    while (std::getline(input, line)) {
        const std::vector<std::string> fields = split_csv(line);
        if (fields.size() < 7U) continue;
        double start = 0.0;
        double bin = 0.0;
        try {
            start = std::stod(fields[2]);
            bin = std::stod(fields[4]);
        } catch (const std::exception&) {
            continue;
        }
        for (std::size_t i = 6; i < fields.size(); ++i) {
            try {
                const double db = std::stod(fields[i]);
                if (db > strongest_db) {
                    strongest_db = db;
                    strongest_hz = start + static_cast<double>(i - 6U) * bin;
                }
            } catch (const std::exception&) {
            }
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    state_.scanning = false;
    if (strongest_hz > 0.0) {
        state_.frequency_hz = strongest_hz;
        state_.signal_percent = clamp_percent(static_cast<int>(std::lround((strongest_db + 100.0) * 1.25)));
        std::ostringstream status;
        status << "Strongest activity near " << std::fixed << std::setprecision(6)
               << (strongest_hz / 1000000.0) << " MHz (" << std::setprecision(1) << strongest_db << " dB).";
        state_.status = status.str();
    } else {
        std::ostringstream status;
        status << "RF scan produced no usable spectrum result";
        if (rc != 0) status << " (scanner exit " << rc << ')';
        status << '.';
        state_.status = status.str();
    }
}

void RadioBackend::cancel_scan() {
    scan_cancel_.store(true);
    if (scan_thread_.joinable()) scan_thread_.join();
    std::lock_guard<std::mutex> lock(mutex_);
    state_.scanning = false;
}

bool RadioBackend::toggle_recording(std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.recording = !state_.recording;
    const bool was_receiving = state_.receiving;
    if (was_receiving) {
        stop_receive_locked();
        if (!spawn_receive_pipeline_locked(status)) {
            state_.recording = false;
            return false;
        }
    } else {
        status = state_.recording ? "Recording armed. It will start with the next compatible RF receive session." : "Recording disarmed.";
        state_.status = status;
    }
    return true;
}

std::vector<RadioFavorite> RadioBackend::favorites() const {
    std::vector<RadioFavorite> out;
    std::ifstream input(favorites_path());
    std::string line;
    while (std::getline(input, line)) {
        std::stringstream ss(line);
        std::string name;
        std::string frequency;
        std::string modulation;
        if (!std::getline(ss, name, '\t') || !std::getline(ss, frequency, '\t') || !std::getline(ss, modulation)) continue;
        try {
            RadioFavorite favorite;
            favorite.name = name;
            favorite.frequency_hz = std::stod(frequency);
            favorite.modulation = modulation_from_name(modulation);
            out.push_back(favorite);
        } catch (const std::exception&) {
        }
    }
    return out;
}

bool RadioBackend::toggle_favorite(const std::string& name, std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RadioFavorite> entries;
    {
        std::ifstream input(favorites_path());
        std::string line;
        while (std::getline(input, line)) {
            std::stringstream ss(line);
            std::string saved_name;
            std::string frequency;
            std::string modulation;
            if (!std::getline(ss, saved_name, '\t') || !std::getline(ss, frequency, '\t') || !std::getline(ss, modulation)) continue;
            try {
                entries.push_back({saved_name, std::stod(frequency), modulation_from_name(modulation)});
            } catch (const std::exception&) {
            }
        }
    }

    const auto match = std::find_if(entries.begin(), entries.end(), [this](const RadioFavorite& entry) {
        return std::fabs(entry.frequency_hz - state_.frequency_hz) < 1.0 && entry.modulation == state_.modulation;
    });
    bool removed = false;
    if (match != entries.end()) {
        entries.erase(match);
        removed = true;
    } else {
        RadioFavorite favorite;
        favorite.name = name.empty() ? "Radio favorite" : name;
        favorite.frequency_hz = state_.frequency_hz;
        favorite.modulation = state_.modulation;
        entries.push_back(favorite);
    }

    std::error_code ec;
    std::filesystem::create_directories(config_dir(), ec);
    std::ofstream output(favorites_path(), std::ios::trunc);
    if (!output) {
        status = "Could not update radio favorites.";
        state_.status = status;
        return false;
    }
    for (const RadioFavorite& entry : entries) {
        output << entry.name << '\t' << std::fixed << std::setprecision(0) << entry.frequency_hz << '\t'
               << modulation_name(entry.modulation) << '\n';
    }
    status = removed ? "Removed current frequency from Favorites." : "Saved current frequency to Favorites.";
    state_.status = status;
    return true;
}

std::vector<std::string> RadioBackend::recordings() const {
    std::vector<std::string> out;
    std::error_code ec;
    if (!std::filesystem::is_directory(recordings_dir(), ec)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(recordings_dir(), ec)) {
        if (ec) break;
        if (entry.is_regular_file() && entry.path().extension() == ".wav") out.push_back(entry.path().string());
    }
    std::sort(out.begin(), out.end(), std::greater<std::string>());
    return out;
}

bool RadioBackend::tx_chain_self_test(std::string& status) {
    std::error_code ec;
    std::filesystem::create_directories(cache_dir(), ec);
    const std::string path = cache_dir() + "/tx-self-test.iq";
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        status = "TX chain self-test could not create its local IQ test file.";
        return false;
    }
    constexpr double pi = 3.14159265358979323846;
    constexpr double cycles = 37.0;
    constexpr std::size_t samples = 4096U;
    for (std::size_t i = 0; i < samples; ++i) {
        const double phase = 2.0 * pi * cycles * static_cast<double>(i) / static_cast<double>(samples);
        const float iq[2] = {static_cast<float>(0.2 * std::cos(phase)), static_cast<float>(0.2 * std::sin(phase))};
        output.write(reinterpret_cast<const char*>(iq), sizeof(iq));
    }
    output.close();
    if (!output) {
        status = "TX chain self-test failed while writing generated IQ.";
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    state_.tx_rf_enabled = false;
    status = "TX DSP/self-test passed using local IQ only. RF transmit remains disabled by default.";
    state_.status = status;
    return true;
}

} // namespace reddmedia
