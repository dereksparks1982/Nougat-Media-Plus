#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]).resolve()


def need(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def has_all(text: str, needles: list[str], label: str) -> None:
    missing = [item for item in needles if item not in text]
    need(not missing, f"{label} missing: {missing}")


def main() -> int:
    try:
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        main_cpp = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
        radio_hpp = (ROOT / "src/radio/radio_backend.hpp").read_text(encoding="utf-8")
        radio_cpp = (ROOT / "src/radio/radio_backend.cpp").read_text(encoding="utf-8")
        scope = (ROOT / "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_52_SCOPE.md").read_text(encoding="utf-8")
        carry = (ROOT / "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md").read_text(encoding="utf-8")
        upstream = json.loads((ROOT / "components/radio/UPSTREAM_COMPONENTS.json").read_text(encoding="utf-8"))

        has_all(cmake, [
            "project(NougatMediaSuite VERSION 0.0.52 LANGUAGES CXX)",
            "add_executable(Nougat_Media_Suite_v52",
            "src/radio/radio_backend.cpp",
            "target_compile_options(Nougat_Media_Suite_v52 PRIVATE -Wall -Wextra -Werror)",
        ], "CMake")
        need("Nougat_Media_Suite_v51" not in cmake, "CMake still targets v51")

        has_all(main_cpp, [
            '#include "radio/radio_backend.hpp"',
            'const std::string versionLabel = "v0.0.52";',
            'input.app_version = "Nougat Media Suite v0.0.52";',
            'printf("Nougat Media Suite v0.0.52\\n");',
            "reddmedia::RadioBackend radioBackend;",
            "radioSimpleBtn", "radioProBtn", "radioEmergencyBtn", "radioSatelliteBtn",
            "radioFrequencyRect", "radioModeBtn", "radioStepBtn", "radioDeviceBtn",
            "radioGainDownBtn", "radioSquelchUpBtn", "radioTxTestBtn",
            "ISS voice", "145800000.0", "Local Emergency", "TV Antenna Scan",
            "start_live_tv_scan()", "tx_chain_self_test", "play_radio_internet_station",
        ], "main.cpp Radio contracts")
        need("No compatible provider is active for this radio mode yet." not in main_cpp,
             "rejected v51 dead Radio shell text remains")
        need("RadioPanel::DAB" not in main_cpp and "RadioPanel::DRM" not in main_cpp,
             "old v51 decorative RadioPanel shell remains")

        has_all(radio_hpp, [
            "enum class RadioModulation", "AM", "NFM", "WFM", "USB", "LSB", "CW", "DAB", "DRM", "P25", "RAW",
            "bool start_receive", "bool start_scan", "bool toggle_recording", "bool tx_chain_self_test",
        ], "radio_backend.hpp")
        has_all(radio_cpp, [
            'glob_paths("/dev/radio*")', 'glob_paths("/dev/dvb/adapter*/frontend*")',
            "SoapySDRUtil", "rtl_fm", "rtl_power", "hackrf_info", "LimeUtil", "uhd_find_devices",
            "airspy_info", "airspyhf_info", "op25", "welle", "dream", "satdump",
            "RF transmit remains disabled by default", "TX chain self-test",
        ], "radio_backend.cpp")
        need("system(" not in radio_cpp, "radio backend must not use shell system()")

        has_all(scope, [
            "Radio-only", "RADIO", "PRO", "Local Emergency / Scanner", "ISS", "TV Antenna Scan",
            "RF transmit remains disabled by default", "non-radiating TX-chain self-test",
            "$HOME/DKLab/Archive/", "uncommitted, untagged and unpushed",
        ], "v52 scope")

        has_all(carry, [
            "Studio → Tools tab → File Splitter button", "HDHomeRun", "World TV Guide", "World TV playback",
            "Games artwork", "Emulator expansion", "Nougat N icon", "Search top-tab hover", "AMBER Alerts",
            "LAN Web Viewer", "$HOME/DKLab/Archive/",
        ], "v53 carry-forward")

        ids = {item["id"] for item in upstream["components"]}
        for required in {"soapysdr", "liquid-dsp", "op25", "gnuradio4-core", "kissfft"}:
            need(required in ids, f"upstream manifest missing owner-supplied project {required}")

        for launcher in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
            text = (ROOT / launcher).read_text(encoding="utf-8")
            need("Nougat_Media_Suite_v52" in text, f"{launcher} does not target v52")
            need("Nougat_Media_Suite_v51" not in text, f"{launcher} still targets v51")

        print("PASS: v0.0.52 Radio static contracts verified.")
        return 0
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
