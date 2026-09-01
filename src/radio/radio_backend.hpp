#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <sys/types.h>

namespace reddmedia {

enum class RadioModulation {
    AM,
    NFM,
    WFM,
    USB,
    LSB,
    CW,
    DAB,
    DRM,
    P25,
    RAW
};

struct RadioDevice {
    std::string id;
    std::string name;
    std::string backend;
    std::string notes;
    bool receive = false;
    bool transmit = false;
    double minimum_hz = 0.0;
    double maximum_hz = 0.0;
};

struct RadioFavorite {
    std::string name;
    double frequency_hz = 0.0;
    RadioModulation modulation = RadioModulation::NFM;
};

struct RadioSnapshot {
    std::vector<RadioDevice> devices;
    int selected_device = -1;
    double frequency_hz = 100100000.0;
    RadioModulation modulation = RadioModulation::WFM;
    double tuning_step_hz = 100000.0;
    int gain_percent = 50;
    int squelch = 0;
    int signal_percent = -1;
    bool receiving = false;
    bool scanning = false;
    bool recording = false;
    bool tx_hardware_available = false;
    bool tx_rf_enabled = false;
    bool soapy_available = false;
    bool rtl_available = false;
    bool op25_available = false;
    bool dab_decoder_available = false;
    bool drm_decoder_available = false;
    bool satellite_decoder_available = false;
    std::string status = "Radio ready.";
};

class RadioBackend {
public:
    RadioBackend();
    ~RadioBackend();

    RadioBackend(const RadioBackend&) = delete;
    RadioBackend& operator=(const RadioBackend&) = delete;

    void refresh();
    RadioSnapshot snapshot() const;

    void set_frequency(double hz);
    void step_frequency(double delta_hz);
    void set_modulation(RadioModulation modulation);
    void cycle_modulation(int direction);
    void set_tuning_step(double hz);
    void cycle_tuning_step(int direction);
    void set_gain_percent(int percent);
    void set_squelch(int value);
    void cycle_device(int direction);

    bool start_receive(std::string& status);
    void stop_receive();

    bool start_scan(double minimum_hz, double maximum_hz, double step_hz, std::string& status);
    void cancel_scan();

    bool toggle_recording(std::string& status);
    bool toggle_favorite(const std::string& name, std::string& status);
    std::vector<RadioFavorite> favorites() const;
    std::vector<std::string> recordings() const;

    // v0.0.52 deliberately keeps RF transmit disabled by default. This test
    // exercises the generated-IQ side of the TX architecture without emitting RF.
    bool tx_chain_self_test(std::string& status);

    static const char* modulation_name(RadioModulation modulation);

private:
    mutable std::mutex mutex_;
    RadioSnapshot state_;
    pid_t receive_pid_ = -1;
    std::thread scan_thread_;
    std::atomic<bool> scan_cancel_{false};

    std::string current_recording_path_;

    static std::string config_dir();
    static std::string data_dir();
    static std::string cache_dir();
    static std::string favorites_path();
    static std::string recordings_dir();
    static std::string shell_quote(const std::string& value);
    static bool executable_available(const std::string& name);
    static std::string run_capture(const std::string& command);
    static std::vector<std::string> glob_paths(const std::string& pattern);
    static std::string timestamp_name();
    static RadioModulation modulation_from_name(const std::string& value);

    void detect_devices_locked();
    void update_runtime_capabilities_locked();
    void stop_receive_locked();
    bool spawn_receive_pipeline_locked(std::string& status);
    void finish_scan(double minimum_hz, double maximum_hz, double step_hz);
};

} // namespace reddmedia
