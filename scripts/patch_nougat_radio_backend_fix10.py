#!/usr/bin/env python3
from pathlib import Path
import re
import sys

MARK = "NOUGAT_V67_HVR955Q_FM_END_TO_END_FIX10"

def die(msg):
    raise SystemExit("FAIL: " + msg)

def replace_once(text, old, new, label):
    if new in text:
        return text
    count = text.count(old)
    if count != 1:
        die(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new, 1)

def main():
    if len(sys.argv) != 2:
        die("usage: patch_nougat_radio_backend_v2.py <project-root>")

    project = Path(sys.argv[1]).resolve()
    hpp_path = project / "src/radio/radio_backend.hpp"
    cpp_path = project / "src/radio/radio_backend.cpp"
    cmake_path = project / "CMakeLists.txt"
    main_path = project / "src/main.cpp"
    desktop_path = project / "com.elderredsoftworks.NougatPlayPortal.desktop"

    for p in (hpp_path, cpp_path, cmake_path, main_path):
        if not p.is_file():
            die(f"missing Nougat source: {p}")

    # Work in memory first so a failed anchor check does not partially patch source.
    hpp = hpp_path.read_text()
    cpp = cpp_path.read_text()
    cmake = cmake_path.read_text()
    main_cpp = main_path.read_text()
    desktop = desktop_path.read_text() if desktop_path.is_file() else None

    if '#include "v4l2_radio_provider.hpp"' not in hpp:
        hpp = hpp.replace("#pragma once\n", '#pragma once\n\n#include "v4l2_radio_provider.hpp"\n', 1)

    if "V4l2RadioProvider v4l2_radio_;" not in hpp:
        hpp = replace_once(
            hpp,
            "    pid_t receive_pid_ = -1;",
            "    V4l2RadioProvider v4l2_radio_; // " + MARK + "\n"
            "    pid_t receive_pid_ = -1;",
            "native V4L2 provider member",
        )

    helper_anchor = '''std::vector<std::string> split_csv(const std::string& line) {
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
'''
    helper_code = helper_anchor + r'''
std::string find_cx231xx_alsa_capture() {
    std::ifstream cards("/proc/asound/cards");
    std::string line;
    while (std::getline(cards, line)) {
        if (!contains_case_insensitive(line, "cx231xx") &&
            !contains_case_insensitive(line, "hauppauge") &&
            !contains_case_insensitive(line, "conexant hybrid"))
            continue;

        std::istringstream parser(line);
        int card = -1;
        if (parser >> card && card >= 0)
            return "plughw:" + std::to_string(card) + ",0";
    }
    return {};
}
'''
    if "find_cx231xx_alsa_capture()" not in cpp:
        cpp = replace_once(cpp, helper_anchor, helper_code, "cx231xx ALSA helper")

    if "Native HVR/V4L2 radio receiver" not in cpp:
        radio_pattern = re.compile(
            r'    for \(const std::string& path : glob_paths\("/dev/radio\*"\)\) \{\n'
            r'.*?'
            r'    \}\n\n'
            r'    for \(const std::string& path : glob_paths\("/dev/dvb/adapter\*/frontend\*"\)\)',
            re.S,
        )
        match = radio_pattern.search(cpp)
        if not match:
            die("native V4L2 radio discovery block anchor not found")

        block = r'''    for (const std::string& path : glob_paths("/dev/radio*")) {
        const V4l2RadioProbe probe = V4l2RadioProvider::probe(path);
        if (!probe.usable)
            continue;

        RadioDevice device;
        device.id = path;
        device.name = probe.card.empty() ? ("V4L2 radio " + path) : probe.card;
        device.backend = "V4L2 Radio";
        device.notes = "Native HVR/V4L2 FM receiver. Kernel tuning and hardware AGC are used; rtl_fm is not used.";
        device.receive = true;
        device.minimum_hz = probe.minimum_hz;
        device.maximum_hz = probe.maximum_hz;
        state_.devices.push_back(device);
    }

    for (const std::string& path : glob_paths("/dev/dvb/adapter*/frontend*"))'''
        cpp = cpp[:match.start()] + block + cpp[match.end():]

    if "v4l2_radio_.close_device();" not in cpp:
        cpp = replace_once(
            cpp,
            "    receive_pid_ = -1;\n    state_.receiving = false;",
            "    receive_pid_ = -1;\n"
            "    v4l2_radio_.close_device();\n"
            "    state_.receiving = false;",
            "native V4L2 release",
        )

    receive_anchor = '''bool RadioBackend::spawn_receive_pipeline_locked(std::string& status) {
    if (!state_.rtl_available) {'''
    if MARK + "_NATIVE_RECEIVE" not in cpp:
        receive_branch = r'''bool RadioBackend::spawn_receive_pipeline_locked(std::string& status) {
    if (state_.selected_device >= 0 &&
        state_.selected_device < static_cast<int>(state_.devices.size()) &&
        state_.devices[static_cast<std::size_t>(state_.selected_device)].backend == "V4L2 Radio") {
        // NOUGAT_V67_HVR955Q_FM_FINAL_NATIVE_RECEIVE
        const RadioDevice& device =
            state_.devices[static_cast<std::size_t>(state_.selected_device)];

        if (state_.modulation != RadioModulation::WFM) {
            status = "The WinTV-HVR-955Q native path supports broadcast FM/WFM only.";
            state_.status = status;
            return false;
        }

        if (!executable_available("arecord") || !executable_available("aplay")) {
            status = "ALSA arecord/aplay are required for HVR-955Q FM audio.";
            state_.status = status;
            return false;
        }

        std::string error;
        if (!v4l2_radio_.open_device(device.id, error)) {
            status = error;
            state_.status = status;
            return false;
        }

        if (!v4l2_radio_.tune(state_.frequency_hz, error)) {
            v4l2_radio_.close_device();
            status = error;
            state_.status = status;
            return false;
        }

        const std::string capture = find_cx231xx_alsa_capture();
        if (capture.empty()) {
            v4l2_radio_.close_device();
            status = "FM tuned, but the cx231xx ALSA capture device was not found.";
            state_.status = status;
            return false;
        }

        const int signal = v4l2_radio_.signal_percent(error);
        state_.signal_percent = signal >= 0 ? signal : -1;

        std::ostringstream command;
        command << "exec arecord -q -D " << shell_quote(capture)
                << " -t raw -f S16_LE -r 48000 -c 2";

        if (state_.recording) {
            if (!executable_available("ffmpeg")) {
                v4l2_radio_.close_device();
                status = "Recording requires ffmpeg.";
                state_.status = status;
                return false;
            }

            std::error_code ec;
            std::filesystem::create_directories(recordings_dir(), ec);
            current_recording_path_ =
                recordings_dir() + "/fm-" + timestamp_name() + ".wav";

            command << " | tee >(aplay -q -t raw -f S16_LE -r 48000 -c 2)"
                    << " | ffmpeg -nostdin -loglevel error -y"
                    << " -f s16le -ar 48000 -ac 2 -i pipe:0 "
                    << shell_quote(current_recording_path_);
        } else {
            current_recording_path_.clear();
            command << " | aplay -q -t raw -f S16_LE -r 48000 -c 2";
        }

        const pid_t pid = fork();
        if (pid < 0) {
            v4l2_radio_.close_device();
            status = std::string("Could not start HVR FM audio capture: ") +
                     std::strerror(errno);
            state_.status = status;
            return false;
        }

        if (pid == 0) {
            setsid();
            execl("/bin/bash", "bash", "-c",
                  command.str().c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }

        receive_pid_ = pid;
        state_.receiving = true;

        std::ostringstream text;
        text << "HVR-955Q FM receiving "
             << std::fixed << std::setprecision(1)
             << (state_.frequency_hz / 1000000.0) << " MHz";

        if (state_.signal_percent >= 0)
            text << " signal " << state_.signal_percent << "%";

        status = text.str();
        state_.status = status;
        return true;
    }

    if (!state_.rtl_available) {'''
        cpp = replace_once(cpp, receive_anchor, receive_branch, "native HVR receive branch")

    scan_gate = '''    if (!executable_available("rtl_power")) {
        status = "rtl_power is not available, so a spectrum scan cannot be started with the current runtime.";
        std::lock_guard<std::mutex> lock(mutex_);
        state_.status = status;
        return false;
    }
    cancel_scan();'''
    if "native_v4l2_scan" not in cpp:
        replacement = r'''    bool native_v4l2_scan = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        native_v4l2_scan =
            state_.selected_device >= 0 &&
            state_.selected_device < static_cast<int>(state_.devices.size()) &&
            state_.devices[static_cast<std::size_t>(state_.selected_device)].backend ==
                "V4L2 Radio";
    }

    if (!native_v4l2_scan && !executable_available("rtl_power")) {
        status = "rtl_power is not available for the selected SDR device.";
        std::lock_guard<std::mutex> lock(mutex_);
        state_.status = status;
        return false;
    }

    cancel_scan();'''
        cpp = replace_once(cpp, scan_gate, replacement, "native scan gate")

    finish_anchor = '''void RadioBackend::finish_scan(double minimum_hz, double maximum_hz, double step_hz) {
    const std::string output = cache_dir() + "/scan-" + timestamp_name() + ".csv";'''
    if MARK + "_NATIVE_SCAN" not in cpp:
        finish_branch = r'''void RadioBackend::finish_scan(double minimum_hz, double maximum_hz, double step_hz) {
    std::string native_path;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.selected_device >= 0 &&
            state_.selected_device < static_cast<int>(state_.devices.size()) &&
            state_.devices[static_cast<std::size_t>(state_.selected_device)].backend ==
                "V4L2 Radio") {
            native_path =
                state_.devices[static_cast<std::size_t>(state_.selected_device)].id;
        }
    }

    if (!native_path.empty()) {
        // NOUGAT_V67_HVR955Q_FM_FINAL_NATIVE_SCAN
        V4l2RadioProvider scanner;
        std::string error;

        if (!scanner.open_device(native_path, error)) {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.scanning = false;
            state_.status = error;
            return;
        }

        double strongest_hz = 0.0;
        int strongest_signal = -1;

        const double bounded_min =
            std::max(minimum_hz, scanner.info().minimum_hz);
        const double bounded_max =
            scanner.info().maximum_hz > 0.0
                ? std::min(maximum_hz, scanner.info().maximum_hz)
                : maximum_hz;

        for (double hz = bounded_min;
             hz <= bounded_max && !scan_cancel_.load();
             hz += step_hz) {
            if (!scanner.tune(hz, error))
                continue;

            std::this_thread::sleep_for(std::chrono::milliseconds(90));
            const int signal = scanner.signal_percent(error);

            if (signal > strongest_signal) {
                strongest_signal = signal;
                strongest_hz = hz;
            }
        }

        scanner.close_device();

        std::lock_guard<std::mutex> lock(mutex_);
        state_.scanning = false;

        if (scan_cancel_.load()) {
            state_.status = "FM scan cancelled.";
        } else if (strongest_hz > 0.0 && strongest_signal > 0) {
            state_.frequency_hz = strongest_hz;
            state_.signal_percent = strongest_signal;

            std::ostringstream result;
            result << "Strongest FM activity near "
                   << std::fixed << std::setprecision(1)
                   << (strongest_hz / 1000000.0)
                   << " MHz (" << strongest_signal << "%).";
            state_.status = result.str();
        } else {
            state_.status =
                "FM scan completed; no usable signal level was reported.";
        }
        return;
    }

    const std::string output = cache_dir() + "/scan-" + timestamp_name() + ".csv";'''
        cpp = replace_once(cpp, finish_anchor, finish_branch, "native scan worker")

    # Keep native hardware AGC truthful instead of pretending the UI percentage
    # directly programs a Si2157 gain register.
    gain_anchor = '''void RadioBackend::set_gain_percent(int percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_.gain_percent = clamp_percent(percent);
}'''
    if MARK + "_GAIN" not in cpp:
        gain_replacement = r'''void RadioBackend::set_gain_percent(int percent) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (state_.selected_device >= 0 &&
        state_.selected_device < static_cast<int>(state_.devices.size()) &&
        state_.devices[static_cast<std::size_t>(state_.selected_device)].backend ==
            "V4L2 Radio") {
        // NOUGAT_V67_HVR955Q_FM_FINAL_GAIN
        state_.status =
            "HVR-955Q FM uses the Si2157 hardware AGC; manual gain is not exposed by this driver.";
        return;
    }

    state_.gain_percent = clamp_percent(percent);
}'''
        cpp = replace_once(cpp, gain_anchor, gain_replacement, "native HVR gain truthfulness")

    # Identity/build target.
    cmake = cmake.replace("VERSION 0.0.66", "VERSION 0.0.67")
    cmake = cmake.replace("Nougat_Play_Portal_v66", "Nougat_Play_Portal_v67")
    if "src/radio/v4l2_radio_provider.cpp" not in cmake:
        cmake = replace_once(
            cmake,
            "    src/radio/radio_backend.cpp\n",
            "    src/radio/radio_backend.cpp\n"
            "    src/radio/v4l2_radio_provider.cpp\n",
            "CMake native V4L2 provider source",
        )

    if "v0.0.67" not in main_cpp and "v0.0.66" in main_cpp:
        main_cpp = main_cpp.replace("v0.0.66", "v0.0.67")
    if "Nougat Play Portal v0.0.67" not in main_cpp and "Nougat Play Portal v0.0.66" in main_cpp:
        main_cpp = main_cpp.replace("Nougat Play Portal v0.0.66", "Nougat Play Portal v0.0.67")

    if desktop is not None:
        desktop = re.sub(
            r'^Exec=.*$',
            f'Exec="{project}/Nougat_Play_Portal_v67"',
            desktop,
            flags=re.M,
        )
        if re.search(r'^Icon=', desktop, flags=re.M):
            desktop = re.sub(
                r'^Icon=.*$',
                f'Icon={project}/assets/branding/nougat-play-portal-dock-N.png',
                desktop,
                flags=re.M,
            )
        else:
            desktop += f'\nIcon={project}/assets/branding/nougat-play-portal-dock-N.png\n'
        if re.search(r'^StartupWMClass=', desktop, flags=re.M):
            desktop = re.sub(
                r'^StartupWMClass=.*$',
                'StartupWMClass=NougatPlayPortal',
                desktop,
                flags=re.M,
            )
        else:
            desktop += '\nStartupWMClass=NougatPlayPortal\n'
        if re.search(r'^X-GNOME-Application-ID=', desktop, flags=re.M):
            desktop = re.sub(
                r'^X-GNOME-Application-ID=.*$',
                'X-GNOME-Application-ID=com.elderredsoftworks.NougatPlayPortal',
                desktop,
                flags=re.M,
            )

    # All anchor validation has succeeded. Commit the in-project source edits.
    hpp_path.write_text(hpp)
    cpp_path.write_text(cpp)
    cmake_path.write_text(cmake)
    main_path.write_text(main_cpp)
    if desktop is not None:
        desktop_path.write_text(desktop)

    print("PASS: Nougat native HVR-955Q FM backend patched")
    print("PASS: LISTEN uses V4L2 tune + cx231xx ALSA")
    print("PASS: SCAN uses measured V4L2 signal instead of rtl_power")
    print("PASS: manual gain is not falsely advertised for Si2157")

if __name__ == "__main__":
    main()
