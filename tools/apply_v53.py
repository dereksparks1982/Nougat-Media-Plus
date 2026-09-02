#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import shutil
import sys

BASE = "8e346237928d4d358136b926f70e27729b6bd731"
TARGET = "Nougat_Media_Suite_v53"
MARKER = "NOUGAT_V53_CANDIDATE"

class ApplyError(RuntimeError):
    pass

def need(condition: bool, message: str) -> None:
    if not condition:
        raise ApplyError(message)

def replace_exact(text: str, old: str, new: str, label: str, count: int = 1) -> str:
    actual = text.count(old)
    need(actual == count, f"{label}: expected {count} exact anchor(s), found {actual}")
    return text.replace(old, new, count)

def replace_regex(text: str, pattern: str, replacement: str, label: str, count: int = 1) -> str:
    text, actual = re.subn(pattern, lambda _match: replacement, text, count=count, flags=re.S)
    need(actual == count, f"{label}: expected {count} regex anchor(s), found {actual}")
    return text

def patch_cmake(root: Path) -> None:
    path = root / "CMakeLists.txt"
    text = path.read_text(encoding="utf-8")
    text = replace_exact(text,
        "project(NougatMediaSuite VERSION 0.0.52 LANGUAGES CXX)",
        "project(NougatMediaSuite VERSION 0.0.53 LANGUAGES CXX)",
        "CMake version")
    text = text.replace("Nougat_Media_Suite_v52", "Nougat_Media_Suite_v53")
    anchor = "    src/lan/lan_media_service.cpp\n"
    addition = (
        anchor +
        "    src/lan/lan_viewer_service.cpp\n"
        "    src/safety/child_safe_controls.cpp\n"
        "    src/security/security_advisory_service.cpp\n"
        "    src/alerts/public_safety_alerts.cpp\n"
    )
    text = replace_exact(text, anchor, addition, "CMake v53 sources")
    path.write_text(text, encoding="utf-8")

def patch_desktop(root: Path) -> None:
    for name in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
        path = root / name
        text = path.read_text(encoding="utf-8")
        text = replace_exact(text, "Nougat_Media_Suite_v52", "Nougat_Media_Suite_v53", f"{name} executable")
        text = replace_exact(text, "Icon=text-x-generic", "Icon=nougat-media-suite", f"{name} icon")
        path.write_text(text, encoding="utf-8")

def patch_world_tv_service(root: Path) -> None:
    header = root / "src/world_tv/world_tv_service.hpp"
    text = header.read_text(encoding="utf-8")
    text = replace_exact(text,
        "    std::string user_agent;\n    std::string error;\n",
        "    std::string user_agent;\n    std::string error_class;\n    std::string error;\n",
        "World TV result classification")
    header.write_text(text, encoding="utf-8")

    source = root / "src/world_tv/world_tv_service.cpp"
    text = source.read_text(encoding="utf-8")
    text = replace_exact(text,
        "    bool reaped = false;\n    constexpr std::size_t kMaxOutput",
        "    bool reaped = false;\n    bool timed_out = false;\n    constexpr std::size_t kMaxOutput",
        "World TV timeout state")
    text = replace_exact(text,
        "    if (!reaped) {\n        // Never let a dead/hostile CDN hold Nougat shutdown indefinitely.",
        "    if (!reaped) {\n        timed_out = true;\n        // Never let a dead/hostile CDN hold Nougat shutdown indefinitely.",
        "World TV timeout classification")
    text = replace_exact(text,
        "    if (reaped && WIFEXITED(status)) exit_code = WEXITSTATUS(status);\n    return output;\n",
        "    if (reaped && WIFEXITED(status)) exit_code = WEXITSTATUS(status);\n"
        "    if (timed_out && output.find(\"ERROR_CLASS=\") == std::string::npos) {\n"
        "        output += \"ERROR_CLASS=startup_timeout\\nERROR=World TV startup exceeded the 70-second worker limit.\\n\";\n"
        "    }\n"
        "    return output;\n",
        "World TV timeout output")
    text = replace_exact(text,
        "    result.user_agent = value_for(output, \"USER_AGENT\");\n    result.error = value_for(output, \"ERROR\");\n"
        "    if (!result.ok && result.error.empty()) {\n"
        "        result.error = \"No playable direct World TV source was verified.\";\n"
        "    }\n",
        "    result.user_agent = value_for(output, \"USER_AGENT\");\n"
        "    result.error_class = value_for(output, \"ERROR_CLASS\");\n"
        "    result.error = value_for(output, \"ERROR\");\n"
        "    if (!result.ok && result.error_class.empty()) {\n"
        "        result.error_class = exit_code == 127 ? \"dependency\" : \"stream\";\n"
        "    }\n"
        "    if (!result.ok && result.error.empty()) {\n"
        "        result.error = \"No playable direct World TV source was verified.\";\n"
        "    }\n"
        "    if (!result.ok && !result.error_class.empty()) {\n"
        "        result.error = \"World TV \" + result.error_class + \" failure: \" + result.error;\n"
        "    }\n",
        "World TV actionable error")
    source.write_text(text, encoding="utf-8")

    worker = root / "components/world_tv/nougat_world_tv_worker.py"
    text = worker.read_text(encoding="utf-8")
    text = text.replace("NougatMediaSuite/0.0.51", "NougatMediaSuite/0.0.53")
    old = '''    emit(OK=0, ERROR=f"No playable direct source passed verification after {checked} checks from {len(candidates)} candidates. Last probe: {LAST_PROBE_REASON or 'no candidate completed'}.")
    return 3
'''
    new = '''    if shutil.which("ffprobe") is None:
        error_class = "dependency"
    elif not candidates:
        error_class = "resolver"
    elif checked == 0:
        error_class = "provider"
    elif "timed out" in (LAST_PROBE_REASON or "").lower():
        error_class = "startup_timeout"
    else:
        error_class = "stream"
    emit(
        OK=0,
        ERROR_CLASS=error_class,
        ERROR=f"No playable direct source passed verification after {checked} checks from {len(candidates)} candidates. Last probe: {LAST_PROBE_REASON or 'no candidate completed'}.",
    )
    return 3
'''
    text = replace_exact(text, old, new, "World TV worker failure class")
    text = replace_exact(text,
        '        emit(OK=0, ERROR="missing mode")\n',
        '        emit(OK=0, ERROR_CLASS="resolver", ERROR="missing mode")\n',
        "World TV worker missing mode")
    text = replace_exact(text,
        '    emit(OK=0, ERROR="invalid World TV worker arguments")\n',
        '    emit(OK=0, ERROR_CLASS="resolver", ERROR="invalid World TV worker arguments")\n',
        "World TV worker invalid args")
    worker.write_text(text, encoding="utf-8")


def patch_game_artwork(root: Path) -> None:
    path = root / "components/games/artwork_cache_worker.py"
    text = path.read_text(encoding="utf-8")
    text = replace_exact(text,
        '    "Atari Lynx": "Atari - Lynx",\n',
        '    "Atari Lynx": "Atari - Lynx",\n'
        '    "PlayStation": "Sony - PlayStation",\n'
        '    "PlayStation 2": "Sony - PlayStation 2",\n'
        '    "PlayStation Portable": "Sony - PlayStation Portable",\n'
        '    "PlayStation 3": "Sony - PlayStation 3",\n'
        '    "GameCube": "Nintendo - GameCube",\n'
        '    "Wii": "Nintendo - Wii",\n'
        '    "Wii U": "Nintendo - Wii U",\n'
        '    "Arcade": "MAME",\n'
        '    "Nintendo Switch": "Nintendo - Nintendo Switch",\n',
        "Games artwork collections")
    anchor = 'def artwork_name_candidates(rom_stem: str, display_title: str) -> list[str]:\n'
    helper = r'''def strip_release_noise(value: str) -> str:
    value = html.unescape(urllib.parse.unquote(value)).replace("_", " ")
    value = re.sub(r"\[[^\]]*(?:!|b|h|o|p|t|f|bad|hack|overdump|trainer|translated)[^\]]*\]", " ", value, flags=re.I)
    value = re.sub(r"\((?:usa|us|u|europe|eur|pal|japan|jpn|world|asia|australia|korea|rev(?:ision)?[^)]*|v(?:er(?:sion)?)?\s*\d[^)]*|beta[^)]*|proto(?:type)?[^)]*|demo[^)]*)\)", " ", value, flags=re.I)
    value = re.sub(r"\b(?:rev(?:ision)?\s*[a-z0-9.]+|v(?:er(?:sion)?)?\s*\d+(?:\.\d+)*|beta\s*\d*|prototype|proto|demo|sample|preview)\b", " ", value, flags=re.I)
    value = re.sub(r"\s+", " ", value).strip(" ._-")
    return value


def artwork_name_candidates(rom_stem: str, display_title: str) -> list[str]:
'''
    text = replace_exact(text, anchor, helper, "Games artwork release-noise helper")
    text = replace_exact(text,
        '    for value in (rom_stem, display_title, short_name(rom_stem), short_name(display_title)):\n',
        '    for value in (rom_stem, display_title, short_name(rom_stem), short_name(display_title), strip_release_noise(rom_stem), strip_release_noise(display_title)):\n',
        "Games artwork candidate normalization")
    text = text.replace("Nougat-Media-Suite-v0.0.49", "Nougat-Media-Suite-v0.0.53")
    path.write_text(text, encoding="utf-8")

def patch_hdhomerun(root: Path) -> None:
    path = root / "src/live_tv/hdhomerun_provider.cpp"
    text = path.read_text(encoding="utf-8")
    text = replace_exact(text,
        "#include <algorithm>\n#include <cctype>",
        "#include <algorithm>\n#include <chrono>\n#include <cctype>",
        "HDHR chrono include")
    text = replace_exact(text,
        "#include <sstream>\n#include <sys/wait.h>",
        "#include <sstream>\n#include <thread>\n#include <sys/wait.h>",
        "HDHR thread include")

    old = '''bool parse_program_line(const std::string& line, int& program, std::string& guide, std::string& name) {
    if (line.rfind("PROGRAM ", 0U) != 0U) return false;
    std::size_t pos = 8U;
'''
    new = '''bool parse_program_line(const std::string& line, int& program, std::string& guide, std::string& name) {
    std::size_t pos = 0U;
    if (line.rfind("PROGRAM ", 0U) == 0U) pos = 8U;
    else if (line.empty() || !std::isdigit(static_cast<unsigned char>(line[0]))) return false;
'''
    text = replace_exact(text, old, new, "HDHR streaminfo parser")

    helper_anchor = '''std::string device_label(const HdHomeRunDevice& device) {
    std::string label = device.model.empty() ? "HDHomeRun" : device.model;
    label += " " + device.device_id;
    return label;
}
'''
    helper_add = helper_anchor + r'''
unsigned atsc_physical_frequency_hz(int physical_channel) {
    if (physical_channel >= 2 && physical_channel <= 4)
        return 57000000U + static_cast<unsigned>(physical_channel - 2) * 6000000U;
    if (physical_channel >= 5 && physical_channel <= 6)
        return 79000000U + static_cast<unsigned>(physical_channel - 5) * 6000000U;
    if (physical_channel >= 7 && physical_channel <= 13)
        return 177000000U + static_cast<unsigned>(physical_channel - 7) * 6000000U;
    if (physical_channel >= 14 && physical_channel <= 51)
        return 473000000U + static_cast<unsigned>(physical_channel - 14) * 6000000U;
    return 0U;
}
'''
    text = replace_exact(text, helper_anchor, helper_add, "HDHR ATSC channel plan")

    method = r'''bool HdHomeRunProvider::scan_channels(const TunerDevice& tuner,
                                      std::vector<LiveTvChannel>& channels,
                                      std::string& status,
                                      const ChannelScanCallback& callback) const {
    channels.clear();
    std::string device_id;
    int tuner_index = -1;
    if (!decode_tuner_id(tuner, device_id, tuner_index)) {
        status = "Not an HDHomeRun tuner resource.";
        return false;
    }

    std::map<std::string, LiveTvChannel> found;
    bool cancelled = false;
    bool failed = false;
    std::string failure;
    int rf_attempted = 0;
    int rf_locked = 0;
    int raw_service_rows = 0;
    int parsed_services = 0;
    int rejected_service_rows = 0;
    constexpr int kFirstPhysical = 2;
    constexpr int kLastPhysical = 51;
    constexpr int kTotal = kLastPhysical - kFirstPhysical + 1;

    for (int physical = kFirstPhysical; physical <= kLastPhysical; ++physical) {
        ++rf_attempted;
        const unsigned frequency = atsc_physical_frequency_hz(physical);
        if (frequency == 0U) {
            failed = true;
            failure = "ATSC frequency plan rejected RF " + std::to_string(physical) + ".";
            break;
        }

        const std::string tuner_path = "/tuner" + std::to_string(tuner_index);
        const CommandResult tune = config_command(
            shell_quote(device_id) + " set " + tuner_path + "/channel auto:" + std::to_string(frequency));
        if (tune.code != 0) {
            failed = true;
            failure = tune.output.empty()
                ? "HDHomeRun tune command failed on RF " + std::to_string(physical) + "."
                : tune.output;
            break;
        }

        bool locked = false;
        int signal = -1;
        int quality = -1;
        std::string lock_name = "none";
        for (int attempt = 0; attempt < 6; ++attempt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(attempt == 0 ? 120 : 140));
            const CommandResult runtime = config_command(
                shell_quote(device_id) + " get " + tuner_path + "/status");
            if (runtime.code != 0) continue;
            lock_name = token_field(runtime.output, "lock=");
            signal = integer_field(runtime.output, "ss=");
            quality = integer_field(runtime.output, "snq=");
            locked = !lock_name.empty() && lock_name != "none";
            if (locked) break;
        }

        if (locked) {
            ++rf_locked;
            const CommandResult info = config_command(
                shell_quote(device_id) + " get " + tuner_path + "/streaminfo");
            if (info.code == 0) {
                std::istringstream rows(info.output);
                std::string row;
                while (std::getline(rows, row)) {
                    row = trim_copy(row);
                    if (row.empty()) continue;
                    ++raw_service_rows;
                    int program = 0;
                    std::string guide;
                    std::string name;
                    if (!parse_program_line(row, program, guide, name)) { ++rejected_service_rows; continue; }
                    ++parsed_services;
                    LiveTvChannel channel;
                    channel.id = guide;
                    channel.name = name;
                    channel.service = "HDHomeRun " + device_id + " | program " +
                                      std::to_string(program) + " | RF " + std::to_string(physical);
                    channel.frequency = std::to_string(frequency);
                    channel.program_number = program;
                    channel.physical_channel = physical;
                    channel.source_id = 0;
                    found[channel.id] = std::move(channel);
                }
            }
        }

        if (callback) {
            ChannelScanProgress progress;
            progress.physical_channel = physical;
            progress.frequency_hz = frequency;
            progress.completed = ((physical - kFirstPhysical + 1) * 65) / kTotal;
            progress.total = 100;
            progress.locked = locked;
            progress.signal_percent = signal;
            progress.quality_percent = quality;
            progress.channels_found = static_cast<int>(found.size());
            progress.message = "HDHomeRun tuner " + std::to_string(tuner_index) +
                               " scanning RF " + std::to_string(physical) +
                               " @ " + std::to_string(frequency) + " Hz | " +
                               (locked ? ("lock " + lock_name) : "no lock") + " | " +
                               std::to_string(found.size()) + " program(s) found.";
            if (!callback(progress)) {
                cancelled = true;
                break;
            }
        }
    }

    if (cancelled) {
        config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
        status = "HDHomeRun channel scan cancelled during RF traversal. State returned to Idle.";
        return false;
    }
    if (failed) {
        config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
        status = "HDHomeRun channel scan failed during RF traversal: " + failure + " State returned to Idle.";
        return false;
    }

    if (callback) {
        ChannelScanProgress phase;
        phase.completed = 72; phase.total = 100; phase.channels_found = static_cast<int>(found.size());
        phase.message = "HDHomeRun phase 2/6: service/program parsing complete.";
        if (!callback(phase)) {
            status = "HDHomeRun channel scan cancelled during service/program parsing. State returned to Idle.";
            config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
            return false;
        }
    }

    for (auto& item : found) channels.push_back(std::move(item.second));
    std::sort(channels.begin(), channels.end(), [](const LiveTvChannel& a, const LiveTvChannel& b) {
        const auto numeric=[](const std::string& id) {
            const std::size_t dot=id.find('.');
            const int major=std::atoi(id.substr(0,dot).c_str());
            const int minor=dot==std::string::npos?0:std::atoi(id.substr(dot+1U).c_str());
            return major*1000+minor;
        };
        return numeric(a.id)<numeric(b.id);
    });

    if (callback) {
        ChannelScanProgress phase;
        phase.completed = 82; phase.total = 100; phase.channels_found = static_cast<int>(channels.size());
        phase.message = "HDHomeRun phase 4/6: service/channel resolution complete; handing channels to Nougat import.";
        if (!callback(phase)) {
            config_command(shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");
            status = "HDHomeRun channel scan cancelled before channel import. State returned to Idle.";
            return false;
        }
    }

    const CommandResult clear_result = config_command(
        shell_quote(device_id) + " set /tuner" + std::to_string(tuner_index) + "/channel none");

    status = "HDHomeRun phases 1-4 complete: RF attempted " + std::to_string(rf_attempted) +
             ", multiplexes locked " + std::to_string(rf_locked) +
             ", raw service rows " + std::to_string(raw_service_rows) +
             ", parsed services " + std::to_string(parsed_services) +
             ", rejected rows " + std::to_string(rejected_service_rows) +
             ", unique channels ready for import " + std::to_string(channels.size()) +
             ". Phase 6 tuner finalization complete. Guide update runs where provider data is available.";
    if (clear_result.code != 0)
        status += " Tuner release needs attention: " +
                  (clear_result.output.empty() ? std::string("release command failed") : clear_result.output) + ".";
    else
        status += " Tuner released.";
    return true;
}

'''
    pattern = r'bool HdHomeRunProvider::scan_channels\(const TunerDevice& tuner,.*?\n\}\n\nbool HdHomeRunProvider::live_playback_input'
    replacement = method + 'bool HdHomeRunProvider::live_playback_input'
    text = replace_regex(text, pattern, replacement, "HDHR explicit RF scan")
    path.write_text(text, encoding="utf-8")

def patch_main(root: Path) -> None:
    path = root / "src/main.cpp"
    text = path.read_text(encoding="utf-8")
    need(MARKER not in text, "v53 main patch already appears applied")

    text = replace_exact(text,
        '#include <sys/wait.h>\n',
        '#include <sys/wait.h>\n#include <sys/prctl.h>\n',
        "process identity include")

    text = replace_exact(text,
        '#include "lan/lan_media_service.hpp"\n',
        '#include "lan/lan_media_service.hpp"\n'
        '#include "lan/lan_viewer_service.hpp"\n'
        '#include "safety/child_safe_controls.hpp"\n'
        '#include "security/security_advisory_service.hpp"\n'
        '#include "alerts/public_safety_alerts.hpp"\n',
        "v53 module includes")

    text = text.replace("v0.0.52", "v0.0.53")

    text = replace_exact(text,
        "int main(int argc, char** argv) {\n",
        "int main(int argc, char** argv) {\n    prctl(PR_SET_NAME, \"NougatMediaSuite\", 0, 0, 0);\n",
        "Linux process identity")

    text = replace_exact(text,
        'worldTvListBox = {worldFrame.x + 16, 88, std::max(180, worldFrame.w - 32), std::max(120, worldFrame.y + worldFrame.h - 100)};',
        'worldTvListBox = {worldFrame.x + 16, 96, std::max(180, worldFrame.w - 32), std::max(100, worldFrame.y + worldFrame.h - 108)};',
        "World TV outer geometry")
    text = replace_exact(text, "        const int channelW=210;\n", "        const int channelW=166;\n", "World TV channel width")
    text = replace_exact(text, "        const int rowH=62;\n", "        const int rowH=46;\n", "World TV row height")
    old_wheel = '''        if (currentView == ViewMode::WorldTV && target == win && worldTvListBox.contains(x,y)) {
            const int total=static_cast<int>(world_tv_catalog().size());
            const int rowH=38; const int top=worldTvListBox.y+80;
            const int visible=std::max(1,(worldTvListBox.y+worldTvListBox.h-top-8)/rowH);
            const int maxScroll=std::max(0,total-visible);
            worldTvScroll=std::max(0,std::min(maxScroll,worldTvScroll+(button==Button4?-1:1)));
            redraw(); return true;
        }
'''
    new_wheel = '''        if (currentView == ViewMode::WorldTV && target == win && worldTvListBox.contains(x,y)) {
            const int total=static_cast<int>(world_tv_visible_indices().size());
            const int rowH=46;
            const int top=worldTvListBox.y+66;
            const int headerH=34;
            const int visible=std::max(1,(worldTvListBox.y+worldTvListBox.h-(top+headerH)-8)/rowH);
            const int maxScroll=std::max(0,total-visible);
            worldTvScroll=std::max(0,std::min(maxScroll,worldTvScroll+(button==Button4?-1:1)));
            redraw(); return true;
        }
'''
    text = replace_exact(text, old_wheel, new_wheel, "World TV wheel geometry")

    text = replace_exact(text,
        "            state->physical_channel=0; state->frequency_hz=0; state->completed=0; state->total=35;\n",
        "            state->physical_channel=0; state->frequency_hz=0; state->completed=0; state->total=network ? 100 : 35;\n",
        "Live TV scan total")

    text = replace_exact(text,
        '                std::string saveError;\n'
        '                if (!tunerBackend.save_channels(liveTvChannels, saveError) && !saveError.empty()) liveTvStatus += " " + saveError;\n'
        '                restore_live_tv_last_channel();\n',
        '                std::string saveError;\n'
        '                const bool saveOk=tunerBackend.save_channels(liveTvChannels, saveError);\n'
        '                if (!saveOk && !saveError.empty()) liveTvStatus += " Phase 3 channel import/update failed: " + saveError;\n'
        '                else if (status.find("HDHomeRun") != std::string::npos) {\n'
        '                    liveTvStatus += " Phase 3 channel import/update complete. Phase 5 guide update: current HDHomeRun provider exposes lineup/channel data but no separate guide feed, so no guide update was fabricated. Phase 6 finalization complete.";\n'
        '                    std::lock_guard<std::mutex> phaseLock(liveTvScanState->mutex);\n'
        '                    liveTvScanState->completed=100; liveTvScanState->total=100; liveTvScanState->updated=true;\n'
        '                }\n'
        '                restore_live_tv_last_channel();\n',
        "HDHomeRun post-scan phases")

    text = replace_exact(text,
        "    Rect studioSplitFileBtn, studioSplitFolderBtn, studioReassembleBtn, studioVerifyBtn;\n",
        "    Rect studioSplitFileBtn, studioSplitFolderBtn, studioReassembleBtn, studioVerifyBtn;\n"
        "    Rect studioSourceRect, studioOutputRect, studioNameRect, studioPiecesRect, studioMaxMiBRect;\n"
        "    int studioInputFocus = 0;\n"
        "    std::string studioSourcePath;\n"
        "    std::string studioOutputPath;\n"
        "    std::string studioOutputName;\n"
        "    std::string studioPiecesText = \"3\";\n"
        "    std::string studioMaxMiBText = \"0\";\n",
        "Studio embedded fields")

    studio_logic = r'''    std::string& focused_studio_text() {
        switch (studioInputFocus) {
        case 2: return studioOutputPath;
        case 3: return studioOutputName;
        case 4: return studioPiecesText;
        case 5: return studioMaxMiBText;
        case 1:
        default: return studioSourcePath;
        }
    }

    static bool studio_positive_integer(const std::string& value, bool allow_zero=false) {
        if (value.empty()) return false;
        for (char c : value) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        const long long parsed = std::strtoll(value.c_str(), nullptr, 10);
        return allow_zero ? parsed >= 0 : parsed > 0;
    }

    void handle_studio_key(XKeyEvent& event, KeySym ks) {
        if (studioInputFocus <= 0) return;
        if (ks == XK_Escape) { studioInputFocus=0; redraw(); return; }
        if (ks == XK_Tab) { studioInputFocus = studioInputFocus % 5 + 1; redraw(); return; }
        if (ks == XK_BackSpace) {
            std::string& target=focused_studio_text();
            if (!target.empty()) target.pop_back();
            redraw(); return;
        }
        char buf[64]; KeySym outks=0;
        const int n=XLookupString(&event,buf,sizeof(buf)-1,&outks,nullptr);
        if (n>0) {
            buf[n]=0;
            std::string value(buf,static_cast<std::size_t>(n));
            if ((studioInputFocus==4 || studioInputFocus==5) &&
                !std::all_of(value.begin(),value.end(),[](unsigned char c){ return std::isdigit(c)!=0; })) return;
            focused_studio_text() += value;
            redraw();
        }
    }

    void launch_studio_splitter_action(const std::string& action) {
        const std::string tool = exe_dir() + "/tools/nougat_file_splitter.py";
        if (!exists_file(tool)) {
            studioStatus = "File Splitter worker is missing from the Nougat project.";
            redraw();
            return;
        }

        std::string command = "python3 " + shell_quote(tool) + " ";
        if (action == "split") {
            if (studioSourcePath.empty() || studioOutputPath.empty() || studioOutputName.empty() ||
                !studio_positive_integer(studioPiecesText) || !studio_positive_integer(studioMaxMiBText,true)) {
                studioStatus = "Split needs Source, Output Folder, Output Name, positive Pieces, and Max MiB >= 0.";
                redraw(); return;
            }
            command += "split " + shell_quote(studioSourcePath) + " " + shell_quote(studioOutputPath) +
                       " --name " + shell_quote(studioOutputName) +
                       " --pieces " + studioPiecesText +
                       " --max-piece-mib " + studioMaxMiBText;
            studioStatus = "Splitting inside Nougat Studio...";
        } else if (action == "verify") {
            if (studioSourcePath.empty()) {
                studioStatus = "Verify needs the .zip.parts.json manifest in Source.";
                redraw(); return;
            }
            command += "verify " + shell_quote(studioSourcePath);
            studioStatus = "Verifying parts inside Nougat Studio...";
        } else if (action == "reassemble") {
            if (studioSourcePath.empty()) {
                studioStatus = "Reassemble needs the .zip.parts.json manifest in Source.";
                redraw(); return;
            }
            command += "reassemble " + shell_quote(studioSourcePath);
            if (!studioOutputPath.empty()) command += " --output " + shell_quote(studioOutputPath);
            studioStatus = "Reassembling inside Nougat Studio...";
        } else return;

        redraw();
        XFlush(d);
        const std::string result = run_command_capture(command + " 2>&1");
        if (result.empty()) {
            studioStatus = "File Splitter worker returned no completion message.";
        } else {
            const std::size_t last = result.find_last_of('\n');
            const std::string tail = last == std::string::npos ? result : result.substr(last+1U);
            studioStatus = tail.empty() ? result : tail;
        }
        redraw();
    }

    void handle_studio_click(int x, int y) {
        if (studioSourceRect.contains(x,y)) { studioInputFocus=1; redraw(); return; }
        if (studioOutputRect.contains(x,y)) { studioInputFocus=2; redraw(); return; }
        if (studioNameRect.contains(x,y)) { studioInputFocus=3; redraw(); return; }
        if (studioPiecesRect.contains(x,y)) { studioInputFocus=4; redraw(); return; }
        if (studioMaxMiBRect.contains(x,y)) { studioInputFocus=5; redraw(); return; }
        studioInputFocus=0;
        if (studioSplitFileBtn.contains(x,y)) { launch_studio_splitter_action("split"); return; }
        if (studioReassembleBtn.contains(x,y)) { launch_studio_splitter_action("reassemble"); return; }
        if (studioVerifyBtn.contains(x,y)) { launch_studio_splitter_action("verify"); return; }
    }

'''
    text = replace_regex(
        text,
        r'    void launch_studio_splitter_action\(const std::string& action\) \{.*?\n    void refresh_live_tv_tuners',
        studio_logic + '    void refresh_live_tv_tuners',
        "Studio embedded worker logic")

    studio_draw = r'''    void draw_studio_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Studio);
        draw_quilted_background(target, {0,kTopBarH,W,H-kTopBarH}, ViewMode::Studio);
        section_text(target, 28, 70, "STUDIO", palette.text);
        text(target, 28, 96, "Nougat creation, production, and media-processing workspace.", palette.muted);
        Rect panel{28,118,std::max(240,W-56),std::max(300,H-148)};
        draw_primary_panel(target,panel,palette);
        text(target,panel.x+16,panel.y+26,"FILE SPLITTER / REASSEMBLER",palette.text);
        text(target,panel.x+16,panel.y+50,
             "All routine controls stay inside Nougat. Paste paths directly into these fields.",palette.muted);

        const int labelW=112;
        const int fieldX=panel.x+16+labelW;
        const int fieldW=std::max(120,panel.w-labelW-32);
        const int fieldH=30;
        int y=panel.y+70;
        const auto field=[&](const char* label,Rect& rect,std::string value,int focus) {
            text(target,panel.x+16,y+20,label,palette.text);
            rect={fieldX,y,fieldW,fieldH};
            fill(target,rect,rgb8(250,240,218));
            outline(target,rect,studioInputFocus==focus?rgb8(126,72,28):rgb8(166,112,56));
            const std::string shown=tail_to_width(value,rect.w-16);
            text(target,rect.x+8,rect.y+20,shown,palette.text);
            y+=38;
        };
        field("Source / Manifest",studioSourceRect,studioSourcePath,1);
        field("Output",studioOutputRect,studioOutputPath,2);
        field("Output Name",studioNameRect,studioOutputName,3);

        const int compactW=120;
        text(target,panel.x+16,y+20,"Pieces",palette.text);
        studioPiecesRect={fieldX,y,compactW,fieldH};
        fill(target,studioPiecesRect,rgb8(250,240,218));
        outline(target,studioPiecesRect,studioInputFocus==4?rgb8(126,72,28):rgb8(166,112,56));
        text(target,studioPiecesRect.x+8,studioPiecesRect.y+20,studioPiecesText,palette.text);
        text(target,studioPiecesRect.x+compactW+20,y+20,"Max MiB",palette.text);
        studioMaxMiBRect={studioPiecesRect.x+compactW+84,y,compactW,fieldH};
        fill(target,studioMaxMiBRect,rgb8(250,240,218));
        outline(target,studioMaxMiBRect,studioInputFocus==5?rgb8(126,72,28):rgb8(166,112,56));
        text(target,studioMaxMiBRect.x+8,studioMaxMiBRect.y+20,studioMaxMiBText,palette.text);
        y+=44;

        const int gap=10;
        const int buttonW=std::max(130,std::min(190,(panel.w-52)/3));
        studioSplitFileBtn={panel.x+16,y,buttonW,32};
        studioSplitFolderBtn={0,0,0,0};
        studioReassembleBtn={studioSplitFileBtn.x+buttonW+gap,y,buttonW,32};
        studioVerifyBtn={studioReassembleBtn.x+buttonW+gap,y,buttonW,32};
        button_on(target,studioSplitFileBtn,"Split Folder / File / ZIP");
        button_on(target,studioReassembleBtn,"Reassemble");
        button_on(target,studioVerifyBtn,"Verify Parts");
        text(target,panel.x+16,y+58,head_to_width(studioStatus,panel.w-32),palette.text);
        text(target,panel.x+16,y+84,
             "Verify/Reassemble: put the .zip.parts.json manifest in Source. Reassemble Output is optional.",palette.muted);
    }

'''
    text = replace_regex(
        text,
        r'    void draw_studio_screen\(Drawable target\) \{.*?\n    void draw_debug_screen',
        studio_draw + '    void draw_debug_screen',
        "Studio embedded draw")

    text = replace_exact(text,
        "                    else if (currentView == ViewMode::Nougat) { handle_nougat_key(e.xkey, ks); }\n"
        "                    else if (currentView == ViewMode::Stream && urlFocused) {",
        "                    else if (currentView == ViewMode::Nougat) { handle_nougat_key(e.xkey, ks); }\n"
        "                    else if (currentView == ViewMode::Studio) { handle_studio_key(e.xkey, ks); }\n"
        "                    else if (currentView == ViewMode::Stream && urlFocused) {",
        "Studio keyboard route")

    text = replace_exact(text,
        '    if (ends_with_lower(lower, ".lnx")) return "Atari Lynx";\n',
        '    if (ends_with_lower(lower, ".lnx")) return "Atari Lynx";\n'
        '    if (ends_with_lower(lower, ".gcm")) return "GameCube";\n'
        '    if (ends_with_lower(lower, ".wbfs")) return "Wii";\n'
        '    if (ends_with_lower(lower, ".wud") || ends_with_lower(lower, ".wux")) return "Wii U";\n'
        '    if (ends_with_lower(lower, ".xci") || ends_with_lower(lower, ".nsp")) return "Nintendo Switch";\n'
        '    if (ends_with_lower(lower, ".cso")) return "PlayStation Portable";\n'
        '    if (ends_with_lower(lower, ".pbp") || ends_with_lower(lower, ".cue")) return "PlayStation";\n',
        "Games modern extensions")
    context_anchor = '''    if (ends_with_lower(path, ".bin") && game_path_has_sega_hint(combined))
        return "Sega Genesis";

    std::string system = game_system_for_path(path);
'''
    context_new = '''    if (ends_with_lower(path, ".bin") && game_path_has_sega_hint(combined))
        return "Sega Genesis";

    const std::string context = lower_copy(combined);
    const bool disc_image = ends_with_lower(path, ".iso") || ends_with_lower(path, ".chd") ||
                            ends_with_lower(path, ".bin") || ends_with_lower(path, ".cue") ||
                            ends_with_lower(path, ".rvz");
    if (disc_image) {
        if (context.find("playstation 2") != std::string::npos || context.find("/ps2") != std::string::npos) return "PlayStation 2";
        if (context.find("playstation 3") != std::string::npos || context.find("/ps3") != std::string::npos) return "PlayStation 3";
        if (context.find("playstation") != std::string::npos || context.find("/ps1") != std::string::npos || context.find("/psx") != std::string::npos) return "PlayStation";
        if (context.find("psp") != std::string::npos) return "PlayStation Portable";
        if (context.find("gamecube") != std::string::npos) return "GameCube";
        if (context.find("wii u") != std::string::npos || context.find("wiiu") != std::string::npos) return "Wii U";
        if (context.find("/wii") != std::string::npos || context.find("nintendo wii") != std::string::npos) return "Wii";
    }

    std::string system = game_system_for_path(path);
'''
    text = replace_exact(text, context_anchor, context_new, "Games context-aware disc systems")
    text = replace_exact(text,
        '        else if (system == "Atari Lynx") candidates = {"mednafen"};\n',
        '        else if (system == "Atari Lynx") candidates = {"mednafen"};\n'
        '        else if (system == "GameCube" || system == "Wii") candidates = {"dolphin-emu", "dolphin"};\n'
        '        else if (system == "PlayStation") candidates = {"duckstation-qt", "duckstation"};\n'
        '        else if (system == "PlayStation 2") candidates = {"pcsx2-qt", "pcsx2"};\n'
        '        else if (system == "PlayStation Portable") candidates = {"PPSSPPSDL", "PPSSPPQt", "ppsspp"};\n'
        '        else if (system == "PlayStation 3") candidates = {"rpcs3"};\n'
        '        else if (system == "Wii U") candidates = {"Cemu", "cemu"};\n'
        '        else if (system == "Arcade") candidates = {"mame"};\n'
        '        else if (system == "Nintendo Switch") candidates = {"Ryujinx", "ryujinx", "suyu", "yuzu"};\n',
        "Games installed Linux backends")
    text = replace_exact(text,
        '''        if (backend_lower == "blastem") {
''',
        '''        if (backend_lower.find("dolphin") != std::string::npos) {
            request.argv = {emulator, "-b", "-e", launchPath};
            return true;
        }
        if (backend_lower.find("duckstation") != std::string::npos) {
            request.argv = {emulator, "-batch", "-fullscreen", launchPath};
            return true;
        }
        if (backend_lower.find("pcsx2") != std::string::npos) {
            request.argv = {emulator, "-fullscreen", launchPath};
            return true;
        }
        if (backend_lower.find("cemu") != std::string::npos) {
            request.argv = {emulator, "-g", launchPath};
            return true;
        }
        if (backend_lower == "rpcs3" || backend_lower.find("ppsspp") != std::string::npos ||
            backend_lower == "mame" || backend_lower == "ryujinx" || backend_lower == "suyu" ||
            backend_lower == "yuzu") {
            request.argv = {emulator, launchPath};
            return true;
        }

        if (backend_lower == "blastem") {
''',
        "Games launch arguments")
    old_systems = 'const std::vector<std::string> systems={"NES","SNES","Game Boy","Game Boy Color","Game Boy Advance","Nintendo 64","Sega Genesis","Sega Master System","Sega Game Gear","Atari 2600","Atari 5200","Atari 7800","Atari 8-bit","Atari Lynx"};'
    new_systems = 'const std::vector<std::string> systems={"NES","SNES","Game Boy","Game Boy Color","Game Boy Advance","Nintendo 64","Sega Genesis","Sega Master System","Sega Game Gear","Atari 2600","Atari 5200","Atari 7800","Atari 8-bit","Atari Lynx","PlayStation","PlayStation 2","PlayStation Portable","PlayStation 3","GameCube","Wii","Wii U","Arcade","Nintendo Switch"};'
    text = replace_exact(text, old_systems, new_systems, "Games system status list")

    need('const bool hover = target == win && r.contains(pointerWindowX, pointerWindowY);' in text,
         "Search/button hover authority missing")

    text = replace_exact(text,
        "    reddmedia::lan::LanMediaService lanMedia;\n",
        "    reddmedia::lan::LanMediaService lanMedia;\n"
        "    reddmedia::lan::LanViewerService lanViewer;\n"
        "    reddmedia::safety::ChildSafeControls childSafeControls;\n"
        "    reddmedia::security::SecurityAdvisoryService securityAdvisories;\n"
        "    reddmedia::alerts::PublicSafetyAlertService publicSafetyAlerts;\n"
        "    std::vector<reddmedia::lan::LanPeer> lanViewerPeers;\n"
        "    std::vector<reddmedia::lan::LanRemoteItem> lanViewerItems;\n"
        "    std::vector<reddmedia::security::RuntimeComponentAdvisory> securityInventory;\n"
        "    std::vector<reddmedia::alerts::PublicSafetyAlert> activePublicAlerts;\n"
        "    std::string publicAlertArea;\n"
        "    std::string v53SystemStatus = \"v0.0.53 safety/network services ready.\";\n"
        "    bool parentSystemUnlocked = false;\n",
        "v53 service members")
    text = replace_exact(text,
        "        lanMedia.prepare();\n",
        "        lanMedia.prepare();\n"
        "        { std::string childStatus; childSafeControls.load(childStatus); v53SystemStatus=childStatus; }\n"
        "        if (const char* alertArea=std::getenv(\"NOUGAT_ALERT_AREA\")) publicAlertArea=alertArea;\n",
        "v53 service startup")
    text = replace_exact(text,
        "        if (currentView == v) return;\n",
        "        if (currentView == v) return;\n"
        "        if (currentView == ViewMode::Debug && v != ViewMode::Debug && childSafeControls.enabled()) parentSystemUnlocked=false;\n",
        "Child Safe relock")
    text = replace_exact(text,
        '''        if (topNavHit && debugTab.contains(x,y)) {
            if (currentView != ViewMode::Debug) switch_view(ViewMode::Debug);
            return;
        }
''',
        '''        if (topNavHit && debugTab.contains(x,y)) {
            if (childSafeControls.enabled() && !parentSystemUnlocked) {
                v53SystemStatus="Child Safe: System/settings are locked. Set NOUGAT_PARENT_PASSWORD for this candidate session and click System again.";
                const char* supplied=std::getenv("NOUGAT_PARENT_PASSWORD");
                if (supplied && childSafeControls.verify_password(supplied)) {
                    parentSystemUnlocked=true;
                    v53SystemStatus="Parent password accepted. System/settings unlocked for this visit.";
                    if (currentView != ViewMode::Debug) switch_view(ViewMode::Debug);
                } else redraw();
                return;
            }
            if (currentView != ViewMode::Debug) switch_view(ViewMode::Debug);
            return;
        }
''',
        "Child Safe System gate")
    text = replace_exact(text,
        '''        if (!has_report) {
            text(target, debugListBox.x + 14, debugListBox.y + 30,
                 "Run Quick Diagnostic for normal health or Deep Diagnostic to exercise AI and explicit subsystem probes.",
                 palette.muted);
            return;
        }
''',
        '''        if (!has_report) {
            text(target, debugListBox.x + 14, debugListBox.y + 30,
                 head_to_width(v53SystemStatus,debugListBox.w-28), palette.text);
            text(target, debugListBox.x + 14, debugListBox.y + 54,
                 "LAN Viewer: private-LAN read-only catalog + Verified Clean gate | Alerts: NOAA/NWS | Security advisories: OSV inventory mapping.",
                 palette.muted);
            text(target, debugListBox.x + 14, debugListBox.y + 78,
                 std::string("Child Safe: ")+(childSafeControls.enabled()?"ON - System password protected":"OFF"),
                 childSafeControls.enabled()?palette.text:palette.muted);
            return;
        }
''',
        "v53 System baseline status")

    text = replace_exact(text,
        '            game_system_for_path("probe.lnx") == "Atari Lynx";',
        '            game_system_for_path("probe.lnx") == "Atari Lynx" &&\n'
        '            game_system_for_path("probe.gcm") == "GameCube" &&\n'
        '            game_system_for_path("probe.wbfs") == "Wii" &&\n'
        '            game_system_for_path("probe.xci") == "Nintendo Switch" &&\n'
        '            game_system_for_path("probe.cso") == "PlayStation Portable";',
        "Games v53 self-test")

    text = replace_exact(text,
        '#include "radio/radio_backend.hpp"\n',
        '#include "radio/radio_backend.hpp"\n\n// NOUGAT_V53_CANDIDATE\n',
        "v53 marker")
    path.write_text(text, encoding="utf-8")

def copy_new_files(package: Path, root: Path) -> None:
    new_files = [
        "src/lan/lan_viewer_service.hpp",
        "src/lan/lan_viewer_service.cpp",
        "src/safety/child_safe_controls.hpp",
        "src/safety/child_safe_controls.cpp",
        "src/security/security_advisory_service.hpp",
        "src/security/security_advisory_service.cpp",
        "src/alerts/public_safety_alerts.hpp",
        "src/alerts/public_safety_alerts.cpp",
        "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_SCOPE.md",
        "tools/nougat_icon_alpha_fix_v53.py",
        "tools/test_v53_static.py",
        "tools/apply_v53.py",
        "tools/build_v53.py",
    ]
    for rel in new_files:
        source = package / rel
        need(source.is_file(), f"candidate package missing {rel}")
        destination = root / rel
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)

def main() -> int:
    package = Path(__file__).resolve().parents[1]
    root = Path(sys.argv[1] if len(sys.argv) > 1 else Path.home() / "DKLab/Projects/Nougat Media Suite").resolve()
    need((root / "src/main.cpp").is_file(), f"Nougat project not found: {root}")
    copy_new_files(package, root)
    patch_cmake(root)
    patch_desktop(root)
    patch_world_tv_service(root)
    patch_game_artwork(root)
    patch_hdhomerun(root)
    patch_main(root)

    import subprocess
    result = subprocess.run([sys.executable, root / "tools/nougat_icon_alpha_fix_v53.py", root],
                            cwd=root, text=True)
    need(result.returncode == 0, "v53 icon alpha repair failed")

    print("PASS: v0.0.53 candidate source applied.")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ApplyError as exc:
        print(f"STOP: {exc}", file=sys.stderr)
        raise SystemExit(1)
