#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import subprocess
import sys

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parents[1]).resolve()
BASE_BLOBS = {
    "src/main.cpp": "c2f84b860dc23d1e2703add68d39741fe8cfc27e",
    "CMakeLists.txt": "ff2d5de51d4331e597b03b36b6d30852fb7ba708",
    "NougatMediaSuite.desktop": "67dad2694ea384d0eabfc54b464a10825018b109",
    "com.elderredsoftworks.NougatMediaSuite.desktop": "67dad2694ea384d0eabfc54b464a10825018b109",
    "src/diagnostics/diagnostic_engine.cpp": "26007ad75e68ae26fdcafac65436644ab9f63521",
    "CHANGELOG.md": "f68cb5fbd02b0f22a05dd0bcc9e686dae34167aa",
    "DEPENDENCIES.md": "1ee90dae53c189ba4ace59c7f242f61c729ae337",
    "ROADMAP.md": "b47ffcb5427bf5f77fd7a3ef619bdf207bc07654",
}


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def git_blob(path: pathlib.Path) -> str:
    result = subprocess.run(["git", "hash-object", str(path)], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(result.returncode == 0, f"git hash-object failed for {path}: {result.stdout.strip()}")
    return result.stdout.strip()


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    need(count == 1, f"{label}: expected one exact anchor, found {count}")
    return text.replace(old, new, 1)


def matching_brace(text: str, open_at: int) -> int:
    need(open_at >= 0 and text[open_at] == "{", "invalid function brace")
    depth = 0
    i = open_at
    state = "code"
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c == '/' and n == '/': state = "line"; i += 2; continue
            if c == '/' and n == '*': state = "block"; i += 2; continue
            if c == '"': state = "string"; i += 1; continue
            if c == "'": state = "char"; i += 1; continue
            if c == '{': depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0: return i
        elif state == "line":
            if c == '\n': state = "code"
        elif state == "block":
            if c == '*' and n == '/': state = "code"; i += 2; continue
        elif state in ("string", "char"):
            quote = '"' if state == "string" else "'"
            if c == '\\': i += 2; continue
            if c == quote: state = "code"
        i += 1
    raise RuntimeError("unterminated C++ function while applying v50 patch")


def replace_function(text: str, signature: str, replacement: str) -> str:
    start = text.find(signature)
    need(start >= 0, f"missing function anchor: {signature}")
    need(text.find(signature, start + 1) < 0, f"ambiguous function anchor: {signature}")
    brace = text.find('{', start + len(signature))
    need(brace >= 0, f"missing opening brace: {signature}")
    end = matching_brace(text, brace)
    return text[:start] + replacement.rstrip() + text[end + 1:]


def patch_main(path: pathlib.Path) -> None:
    text = path.read_text(encoding="utf-8")
    text = replace_once(text, '#include "live_tv/tuner_backend.hpp"\n',
                        '#include "live_tv/tuner_backend.hpp"\n#include "live_tv/hdhomerun_provider.hpp"\n',
                        "HDHomeRun include")
    text = replace_once(text,
        '    reddmedia::NougatTunerBackend tunerBackend;\n',
        '    reddmedia::NougatTunerBackend tunerBackend;\n    reddmedia::HdHomeRunProvider hdHomeRunProvider;\n',
        "HDHomeRun provider member")
    text = replace_once(text,
        '    Rect homeTab, videoPlayerTab, libraryTab, discoverTab, liveTvTab, worldTvTab, nougatTab, ytdlpTab, studioTab, gamesTab, debugTab;\n',
        '    Rect homeTab, videoPlayerTab, libraryTab, discoverTab, liveTvTab, worldTvTab, nougatTab, ytdlpTab, studioTab, gamesTab, debugTab;\n'
        '    Rect studioSplitFileBtn, studioSplitFolderBtn, studioReassembleBtn, studioVerifyBtn;\n',
        "Studio splitter button members")
    text = replace_once(text,
        '    LiveTvTunerUse liveTvTunerUse = LiveTvTunerUse::Idle;\n    bool liveTvGuideRefreshQueued = false;\n    int liveTvPlayingChannel = -1;\n',
        '    LiveTvTunerUse liveTvTunerUse = LiveTvTunerUse::Idle;\n'
        '    bool liveTvGuideRefreshQueued = false;\n'
        '    int liveTvPlayingChannel = -1;\n'
        '    int liveTvPlayingTuner = -1;\n'
        '    int liveTvScanTuner = -1;\n'
        '    int liveTvGuideTuner = -1;\n'
        '    bool liveTvScanBusy = false;\n'
        '    bool liveTvGuideBusy = false;\n'
        '    std::string studioStatus = "File Splitter / Reassembler ready.";\n',
        "v50 tuner lease state")

    helpers = r'''
    bool live_tv_tuner_leased(int index) const {
        return index >= 0 && (index == liveTvPlayingTuner || index == liveTvScanTuner || index == liveTvGuideTuner);
    }

    bool live_tv_channel_usable_on_tuner(const reddmedia::TunerDevice& tuner,
                                         const reddmedia::LiveTvChannel* channel) const {
        if (!channel) return true;
        if (reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner)) {
            std::string deviceId;
            int tunerIndex = -1;
            if (!reddmedia::HdHomeRunProvider::decode_tuner_id(tuner, deviceId, tunerIndex)) return false;
            return !channel->id.empty() && channel->service.find("HDHomeRun " + deviceId) != std::string::npos;
        }
        return channel->physical_channel > 0 && channel->program_number > 0;
    }

    bool live_tv_tuner_available(int index, const reddmedia::LiveTvChannel* channel=nullptr) {
        if (index < 0 || index >= static_cast<int>(liveTvTuners.size())) return false;
        if (live_tv_tuner_leased(index)) return false;
        const auto& tuner = liveTvTuners[static_cast<std::size_t>(index)];
        if (!tuner.readable || !live_tv_channel_usable_on_tuner(tuner, channel)) return false;
        if (reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner)) {
            reddmedia::HdHomeRunTunerStatus runtime;
            std::string status;
            if (!hdHomeRunProvider.probe_runtime_status(tuner, runtime, status)) return false;
            if (runtime.busy) return false;
        }
        return true;
    }

    int find_free_live_tv_tuner(int preferred, const reddmedia::LiveTvChannel* channel=nullptr) {
        if (live_tv_tuner_available(preferred, channel)) return preferred;
        if (preferred >= 0 && preferred < static_cast<int>(liveTvTuners.size()) &&
            reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(liveTvTuners[static_cast<std::size_t>(preferred)])) {
            std::string preferredDevice;
            int preferredTuner = -1;
            if (reddmedia::HdHomeRunProvider::decode_tuner_id(
                    liveTvTuners[static_cast<std::size_t>(preferred)], preferredDevice, preferredTuner)) {
                for (int index = 0; index < static_cast<int>(liveTvTuners.size()); ++index) {
                    std::string device;
                    int tunerIndex = -1;
                    if (!reddmedia::HdHomeRunProvider::decode_tuner_id(
                            liveTvTuners[static_cast<std::size_t>(index)], device, tunerIndex)) continue;
                    if (device == preferredDevice && live_tv_tuner_available(index, channel)) return index;
                }
            }
        }
        for (int index = 0; index < static_cast<int>(liveTvTuners.size()); ++index) {
            if (live_tv_tuner_available(index, channel)) return index;
        }
        return -1;
    }

    static std::pair<int,int> live_tv_channel_order(const std::string& id) {
        const std::size_t dot = id.find('.');
        const int major = std::atoi(id.substr(0, dot).c_str());
        const int minor = dot == std::string::npos ? 0 : std::atoi(id.substr(dot + 1U).c_str());
        return {major, minor};
    }

    void merge_live_tv_channels(const std::vector<reddmedia::LiveTvChannel>& incoming) {
        for (const auto& candidate : incoming) {
            auto found = std::find_if(liveTvChannels.begin(), liveTvChannels.end(), [&candidate](const auto& existing) {
                return existing.id == candidate.id;
            });
            if (found == liveTvChannels.end()) {
                liveTvChannels.push_back(candidate);
                continue;
            }
            if ((found->name.empty() || found->name.rfind("Channel ", 0U) == 0U) && !candidate.name.empty())
                found->name = candidate.name;
            if (found->physical_channel <= 0 && candidate.physical_channel > 0) {
                found->physical_channel = candidate.physical_channel;
                found->frequency = candidate.frequency;
                found->program_number = candidate.program_number;
                found->source_id = candidate.source_id;
            }
            if (!candidate.service.empty() && found->service.find(candidate.service) == std::string::npos) {
                if (!found->service.empty()) found->service += " | ";
                found->service += candidate.service;
            }
        }
        std::sort(liveTvChannels.begin(), liveTvChannels.end(), [](const auto& a, const auto& b) {
            return live_tv_channel_order(a.id) < live_tv_channel_order(b.id);
        });
    }

    void release_live_tv_playing_resource() {
        if (liveTvPlayingTuner >= 0 && liveTvPlayingTuner < static_cast<int>(liveTvTuners.size())) {
            const auto& tuner = liveTvTuners[static_cast<std::size_t>(liveTvPlayingTuner)];
            if (reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner)) {
                std::string releaseStatus;
                hdHomeRunProvider.release_tuner(tuner, releaseStatus);
            }
        }
        liveTvPlayingTuner = -1;
        if (liveTvScanBusy) liveTvTunerUse = LiveTvTunerUse::Scanning;
        else if (liveTvGuideBusy) liveTvTunerUse = LiveTvTunerUse::GuideRefreshing;
        else liveTvTunerUse = LiveTvTunerUse::Idle;
    }

    void launch_studio_splitter_action(const std::string& action) {
        const std::string tool = exe_dir() + "/tools/nougat_file_splitter.py";
        if (!exists_file(tool)) {
            studioStatus = "File Splitter tool is missing from the Nougat project.";
            redraw();
            return;
        }
        pid_t first = fork();
        if (first < 0) {
            studioStatus = "Could not launch the File Splitter.";
            redraw();
            return;
        }
        if (first == 0) {
            pid_t second = fork();
            if (second < 0) _exit(127);
            if (second == 0) {
                execlp("python3", "python3", tool.c_str(), "studio-gui", action.c_str(), static_cast<char*>(nullptr));
                _exit(127);
            }
            _exit(0);
        }
        int status = 0;
        waitpid(first, &status, 0);
        studioStatus = "File Splitter opened. Complete the Nougat Studio dialogs.";
        redraw();
    }

    void handle_studio_click(int x, int y) {
        if (studioSplitFileBtn.contains(x,y)) { launch_studio_splitter_action("split-file"); return; }
        if (studioSplitFolderBtn.contains(x,y)) { launch_studio_splitter_action("split-folder"); return; }
        if (studioReassembleBtn.contains(x,y)) { launch_studio_splitter_action("reassemble"); return; }
        if (studioVerifyBtn.contains(x,y)) { launch_studio_splitter_action("verify"); return; }
    }

'''
    marker = '    void refresh_live_tv_tuners(bool announce=true) {'
    need(text.count(marker) == 1, "refresh_live_tv_tuners anchor missing/ambiguous")
    text = text.replace(marker, helpers + marker, 1)

    refresh = r'''    void refresh_live_tv_tuners(bool announce=true) {
        std::string dvbStatus;
        std::string hdhrStatus;
        liveTvTuners = tunerBackend.detect(dvbStatus);
        std::vector<reddmedia::TunerDevice> networkTuners = hdHomeRunProvider.detect(hdhrStatus);
        liveTvTuners.insert(liveTvTuners.end(), networkTuners.begin(), networkTuners.end());

        liveTvChannels = tunerBackend.load_channels();
        std::set<std::string> loadedDevices;
        for (const auto& tuner : networkTuners) {
            std::string deviceId;
            int tunerIndex = -1;
            if (!reddmedia::HdHomeRunProvider::decode_tuner_id(tuner, deviceId, tunerIndex)) continue;
            if (!loadedDevices.insert(deviceId).second) continue;
            std::vector<reddmedia::LiveTvChannel> lineup;
            std::string lineupStatus;
            if (hdHomeRunProvider.load_lineup(tuner, lineup, lineupStatus)) merge_live_tv_channels(lineup);
        }

        restore_live_tv_last_channel();
        if (liveTvTuners.empty()) liveTvSelectedTuner = -1;
        else if (liveTvSelectedTuner < 0 || liveTvSelectedTuner >= static_cast<int>(liveTvTuners.size()) ||
                 !liveTvTuners[static_cast<std::size_t>(liveTvSelectedTuner)].readable) {
            liveTvSelectedTuner = -1;
            for (int i=0; i<static_cast<int>(liveTvTuners.size()); ++i) {
                if (liveTvTuners[static_cast<std::size_t>(i)].readable) { liveTvSelectedTuner=i; break; }
            }
        }
        std::string combined = dvbStatus;
        if (!hdhrStatus.empty()) {
            if (!combined.empty()) combined += " ";
            combined += hdhrStatus;
        }
        liveTvStatus = combined.empty() ? "Tuner detection complete." : combined;
        if (announce && liveTvTuners.empty())
            liveTvStatus += " Connect a Linux DVB/WinTV tuner or an HDHomeRun on this LAN, then press Detect Tuners again.";
    }'''
    text = replace_function(text, '    void refresh_live_tv_tuners(bool announce=true)', refresh)

    start_scan = r'''    void start_live_tv_scan() {
        if (liveTvScanBusy) { liveTvStatus="A channel scan is already running."; return; }
        if (liveTvTuners.empty()) refresh_live_tv_tuners(false);
        if (liveTvTuners.empty()) { liveTvStatus="Detect a tuner before scanning channels."; redraw(); return; }
        const int scanIndex = find_free_live_tv_tuner(liveTvSelectedTuner, nullptr);
        if (scanIndex < 0) {
            liveTvStatus = "No free physical tuner is available for a channel scan.";
            redraw();
            return;
        }
        if (liveTvScanWorker.joinable()) liveTvScanWorker.join();
        const reddmedia::TunerDevice tuner = liveTvTuners[static_cast<std::size_t>(scanIndex)];
        const bool network = reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner);
        const reddmedia::NougatTunerBackend backend = tunerBackend;
        const reddmedia::HdHomeRunProvider hdhr = hdHomeRunProvider;
        const auto state = liveTvScanState;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy=true; state->updated=true; state->cancel=false; state->finished=false; state->success=false;
            state->physical_channel=0; state->frequency_hz=0; state->completed=0; state->total=35;
            state->locked=false; state->signal_percent=-1; state->quality_percent=-1; state->channels_found=0;
            state->channels.clear();
            state->status = network ? "Starting HDHomeRun ATSC channel scan..." : "Starting native Linux DVB ATSC channel scan...";
        }
        liveTvScanBusy = true;
        liveTvScanTuner = scanIndex;
        if (!currentMediaIsLiveTv) liveTvTunerUse = LiveTvTunerUse::Scanning;
        liveTvStatus = state->status;
        liveTvScanWorker = std::thread([state, tuner, backend, hdhr, network]() mutable {
            std::vector<reddmedia::LiveTvChannel> channels;
            std::string status;
            const auto callback = [state](const reddmedia::ChannelScanProgress& progress) {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (state->cancel) return false;
                state->physical_channel=progress.physical_channel;
                state->frequency_hz=progress.frequency_hz;
                state->completed=progress.completed;
                state->total=progress.total;
                state->locked=progress.locked;
                state->signal_percent=progress.signal_percent;
                state->quality_percent=progress.quality_percent;
                state->channels_found=progress.channels_found;
                state->status=progress.message;
                state->updated=true;
                return true;
            };
            const bool ok = network ? hdhr.scan_channels(tuner, channels, status, callback)
                                    : backend.scan_channels(tuner, channels, status, callback);
            std::lock_guard<std::mutex> lock(state->mutex);
            state->busy=false; state->finished=true; state->success=ok;
            state->status=status; state->channels=std::move(channels);
            state->channels_found=static_cast<int>(state->channels.size()); state->updated=true;
        });
    }'''
    text = replace_function(text, '    void start_live_tv_scan()', start_scan)

    text = replace_once(text,
        '        if (!busy && finished) {\n            liveTvTunerUse = LiveTvTunerUse::Idle;\n            if (success) { liveTvChannels = std::move(channels); restore_live_tv_last_channel(); }\n        }\n',
        '        if (!busy && finished) {\n'
        '            liveTvScanBusy = false;\n'
        '            liveTvScanTuner = -1;\n'
        '            if (liveTvPlayingTuner >= 0) liveTvTunerUse = LiveTvTunerUse::Watching;\n'
        '            else if (liveTvGuideBusy) liveTvTunerUse = LiveTvTunerUse::GuideRefreshing;\n'
        '            else liveTvTunerUse = LiveTvTunerUse::Idle;\n'
        '            if (success) {\n'
        '                merge_live_tv_channels(channels);\n'
        '                std::string saveError;\n'
        '                if (!tunerBackend.save_channels(liveTvChannels, saveError) && !saveError.empty()) liveTvStatus += " " + saveError;\n'
        '                restore_live_tv_last_channel();\n'
        '            }\n'
        '        }\n',
        "scan completion allocator")

    watch = r'''    void watch_live_tv_channel(int index) {
        if (index < 0 || index >= static_cast<int>(liveTvChannels.size())) { liveTvStatus="Select a channel to watch."; redraw(); return; }
        if (liveTvTuners.empty()) refresh_live_tv_tuners(false);
        if (currentMediaIsLiveTv) cleanup_player();
        liveTvSelectedChannel=index;
        save_live_tv_last_channel();
        const auto& channel=liveTvChannels[static_cast<std::size_t>(index)];
        const int tunerIndex = find_free_live_tv_tuner(liveTvSelectedTuner, &channel);
        if (tunerIndex < 0) {
            liveTvStatus="No free tuner can play this channel. A physical tuner may already be scanning, refreshing guide data, or in use.";
            redraw(); return;
        }
        liveTvSelectedTuner=tunerIndex;
        const auto& tuner=liveTvTuners[static_cast<std::size_t>(tunerIndex)];
        std::string mrl,status; std::vector<std::string> options;
        const bool network=reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner);
        const bool prepared = network ? hdHomeRunProvider.live_playback_input(tuner,channel,mrl,options,status)
                                      : tunerBackend.live_playback_input(tuner,channel,mrl,options,status);
        if (!prepared) { liveTvStatus=status; redraw(); return; }
        // SiliconDust's documented HTTP /auto path chooses one free physical tuner.
        // Keep the Linux-DVB lease explicit; HDHomeRun availability is re-probed per tuner
        // before any concurrent scan/guide allocation.
        liveTvPlayingTuner=network ? -1 : tunerIndex;
        liveTvStatus=status;
        const std::string label=channel.id+" "+channel.name;
        if (!open_live_tv_location(mrl,options,index,label)) {
            liveTvPlayingTuner=-1;
            if (liveTvScanBusy) liveTvTunerUse=LiveTvTunerUse::Scanning;
            else if (liveTvGuideBusy) liveTvTunerUse=LiveTvTunerUse::GuideRefreshing;
            else liveTvTunerUse=LiveTvTunerUse::Idle;
            liveTvStatus="VLC could not open the tuner stream for "+label+".";
            switch_view(ViewMode::LiveTV); redraw(); return;
        }
    }'''
    text = replace_function(text, '    void watch_live_tv_channel(int index)', watch)

    old_cleanup = '''        if (currentMediaIsLiveTv) {\n            currentMediaIsLiveTv=false;\n            liveTvPlayingChannel=-1;\n            liveTvPlayingLabel.clear();\n            liveTvTunerUse=LiveTvTunerUse::Idle;\n        }'''
    new_cleanup = '''        if (currentMediaIsLiveTv) {\n            release_live_tv_playing_resource();\n            currentMediaIsLiveTv=false;\n            liveTvPlayingChannel=-1;\n            liveTvPlayingLabel.clear();\n        }'''
    count_cleanup = text.count(old_cleanup)
    need(count_cleanup >= 2, f"live TV cleanup anchors: expected at least two, found {count_cleanup}")
    text = text.replace(old_cleanup, new_cleanup)

    studio = r'''    void draw_studio_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Studio);
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Studio);
        section_text(target, 28, 70, "STUDIO", palette.text);
        text(target, 28, 96, "Nougat creation, production, and media-processing workspace.", palette.muted);
        Rect panel{28, 118, std::max(240, W - 56), std::max(220, H - 148)};
        draw_primary_panel(target, panel, palette);
        text(target, panel.x + 16, panel.y + 30, "File Splitter / Reassembler", palette.text);
        text(target, panel.x + 16, panel.y + 52,
             "Split large files or complete folders into SHA-256 verified transport parts, then verify or reconstruct them exactly.",
             palette.muted);
        const int buttonY = panel.y + 72;
        const int gap = 10;
        const int usable = std::max(320, panel.w - 32);
        const int buttonW = std::max(110, (usable - gap * 3) / 4);
        studioSplitFileBtn = {panel.x + 16, buttonY, buttonW, 32};
        studioSplitFolderBtn = {studioSplitFileBtn.x + buttonW + gap, buttonY, buttonW, 32};
        studioReassembleBtn = {studioSplitFolderBtn.x + buttonW + gap, buttonY, buttonW, 32};
        studioVerifyBtn = {studioReassembleBtn.x + buttonW + gap, buttonY, buttonW, 32};
        button_on(target, studioSplitFileBtn, "Split File");
        button_on(target, studioSplitFolderBtn, "Split Folder");
        button_on(target, studioReassembleBtn, "Reassemble");
        button_on(target, studioVerifyBtn, "Verify Parts");
        text(target, panel.x + 16, buttonY + 58, head_to_width(studioStatus, panel.w - 32), palette.text);
        text(target, panel.x + 16, buttonY + 92,
             "Studio roadmap: professional video/audio/photo editing, green-screen compositing, animation, VFX, motion capture, and production-scale workflows.",
             palette.muted);
    }'''
    text = replace_function(text, '    void draw_studio_screen(Drawable target)', studio)

    text = replace_once(text,
        '        if (currentView == ViewMode::Nougat) {\n            handle_nougat_click(x, y);\n            return;\n        }\n        if (currentView == ViewMode::LiveTV) {\n',
        '        if (currentView == ViewMode::Nougat) {\n            handle_nougat_click(x, y);\n            return;\n        }\n'
        '        if (currentView == ViewMode::Studio) {\n            handle_studio_click(x, y);\n            return;\n        }\n'
        '        if (currentView == ViewMode::LiveTV) {\n',
        "Studio click dispatch")


    text = replace_once(text,
'            snapshot.name = tuner.name; snapshot.frontend_path = tuner.frontend_path;\n            snapshot.backend = tuner.backend; snapshot.status = tuner.status; snapshot.readable = tuner.readable;\n            snapshot.delivery_systems = tuner.frontend_path.empty() ? "V4L2 / unknown" : "ATSC 1.0 / 8VSB";\n            if (!tuner.frontend_path.empty()) {\n                const std::size_t slash = tuner.frontend_path.rfind(\'/\');\n                if (slash != std::string::npos) {\n                    const std::string base = tuner.frontend_path.substr(0, slash + 1U);\n                    snapshot.demux_path = base + "demux0"; snapshot.dvr_path = base + "dvr0";\n                }\n            }\n',
'            snapshot.name = tuner.name; snapshot.frontend_path = tuner.frontend_path;\n            snapshot.backend = tuner.backend; snapshot.status = tuner.status; snapshot.readable = tuner.readable;\n            if (reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner)) {\n                reddmedia::HdHomeRunTunerStatus runtime;\n                std::string runtimeStatus;\n                snapshot.readable = hdHomeRunProvider.probe_runtime_status(tuner, runtime, runtimeStatus);\n                snapshot.status = runtimeStatus.empty() ? tuner.status : runtimeStatus;\n                snapshot.delivery_systems = "HDHomeRun LAN / ATSC 1.0 / 8VSB";\n                snapshot.demux_path.clear();\n                snapshot.dvr_path.clear();\n            } else {\n                snapshot.delivery_systems = tuner.frontend_path.empty() ? "V4L2 / unknown" : "ATSC 1.0 / 8VSB";\n                if (!tuner.frontend_path.empty()) {\n                    const std::size_t slash = tuner.frontend_path.rfind(\'/\');\n                    if (slash != std::string::npos) {\n                        const std::string base = tuner.frontend_path.substr(0, slash + 1U);\n                        snapshot.demux_path = base + "demux0"; snapshot.dvr_path = base + "dvr0";\n                    }\n                }\n            }\n',
        "HDHomeRun diagnostic snapshot")

    need(text.count('input.app_version = "Nougat Media Suite v0.0.49";') == 1, "v49 diagnostic identity anchor mismatch")
    text = text.replace('input.app_version = "Nougat Media Suite v0.0.49";',
                        'input.app_version = "Nougat Media Suite v0.0.50";', 1)
    need(text.count('printf("Nougat Media Suite v0.0.49\\n");') == 1, "v49 --version identity anchor mismatch")
    text = text.replace('printf("Nougat Media Suite v0.0.49\\n");',
                        'printf("Nougat Media Suite v0.0.50\\n");', 1)
    path.write_text(text, encoding="utf-8")


def patch_diagnostics(path: pathlib.Path) -> None:
    text = path.read_text(encoding="utf-8")
    old = '''        std::size_t unreadable_nodes = 0;
        for (const auto& tuner : live_tuners) {
            add_fact(report, "Live TV", "Tuner", tuner.name.empty() ? "Detected tuner" : tuner.name,
                     tuner.frontend_path + (tuner.status.empty() ? std::string() : " | " + tuner.status));
            add_fact(report, "Live TV", "Delivery systems", tuner.delivery_systems.empty() ? "Not reported" : tuner.delivery_systems,
                     tuner.frontend_path);
            for (const auto& node : {tuner.frontend_path, tuner.demux_path, tuner.dvr_path}) {
                if (!node.empty() && !path_readable(node)) ++unreadable_nodes;
            }
        }
'''
    new = '''        std::size_t unreadable_nodes = 0;
        std::size_t unreachable_network_tuners = 0;
        for (const auto& tuner : live_tuners) {
            const bool network_tuner = lower_copy(tuner.backend).find("hdhomerun") != std::string::npos;
            add_fact(report, "Live TV", network_tuner ? "Network tuner" : "Tuner",
                     tuner.name.empty() ? "Detected tuner" : tuner.name,
                     tuner.frontend_path + (tuner.status.empty() ? std::string() : " | " + tuner.status));
            add_fact(report, "Live TV", "Delivery systems", tuner.delivery_systems.empty() ? "Not reported" : tuner.delivery_systems,
                     tuner.frontend_path);
            if (network_tuner) {
                if (!tuner.readable || tuner.frontend_path.empty()) ++unreachable_network_tuners;
                continue;
            }
            for (const auto& node : {tuner.frontend_path, tuner.demux_path, tuner.dvr_path}) {
                if (!node.empty() && !path_readable(node)) ++unreadable_nodes;
            }
        }
'''
    text = replace_once(text, old, new, "Live TV diagnostics network-tuner path")
    old_finding = '''        add_finding(report, "Live TV", unreadable_nodes == 0U ? DiagnosticSeverity::Passed : DiagnosticSeverity::Problem,
                    "LIVE_TV_DEVICE_NODES", "TV tuner device nodes",
                    "Frontend/demux/DVR nodes used by Nougat are readable.",
                    unreadable_nodes == 0U ? "Required reported nodes are readable." : std::to_string(unreadable_nodes) + " required node(s) are unreadable.",
                    "/dev/dvb filesystem access checks.", unreadable_nodes == 0U ? "No action required." : "Check Linux DVB driver, group permissions, and device ownership.");
'''
    new_finding = '''        const bool tuner_access_ok = unreadable_nodes == 0U && unreachable_network_tuners == 0U;
        std::ostringstream tuner_access_observed;
        tuner_access_observed << unreadable_nodes << " unreadable DVB node(s), "
                              << unreachable_network_tuners << " unreachable network tuner(s).";
        add_finding(report, "Live TV", tuner_access_ok ? DiagnosticSeverity::Passed : DiagnosticSeverity::Problem,
                    "LIVE_TV_TUNER_ACCESS", "TV tuner access",
                    "Linux DVB nodes are readable and detected HDHomeRun network tuners answer a live status probe.",
                    tuner_access_ok ? "All reported tuner resources are accessible." : tuner_access_observed.str(),
                    "Linux /dev/dvb access plus HDHomeRun per-tuner status probes.",
                    tuner_access_ok ? "No action required." : "Check Linux DVB permissions or HDHomeRun LAN reachability, as applicable.");
'''
    text = replace_once(text, old_finding, new_finding, "Live TV diagnostic access finding")
    path.write_text(text, encoding="utf-8")
def patch_cmake(path: pathlib.Path) -> None:
    text = path.read_text(encoding="utf-8")
    text = replace_once(text, 'project(NougatMediaSuite VERSION 0.0.49 LANGUAGES CXX)',
                        'project(NougatMediaSuite VERSION 0.0.50 LANGUAGES CXX)', "CMake version")
    text = text.replace('Nougat_Media_Suite_v49', 'Nougat_Media_Suite_v50')
    text = replace_once(text, '    src/live_tv/tuner_backend.cpp\n',
                        '    src/live_tv/tuner_backend.cpp\n    src/live_tv/hdhomerun_provider.cpp\n', "HDHomeRun source")
    path.write_text(text, encoding="utf-8")


def patch_desktop(path: pathlib.Path) -> None:
    text = path.read_text(encoding="utf-8")
    need(text.count('Nougat_Media_Suite_v49') == 1, f"{path.name}: v49 Exec anchor mismatch")
    path.write_text(text.replace('Nougat_Media_Suite_v49', 'Nougat_Media_Suite_v50'), encoding="utf-8")


def prepend_section(path: pathlib.Path, heading: str, body: str) -> None:
    text = path.read_text(encoding="utf-8")
    if heading in text:
        return
    if text.startswith('# '):
        first_newline = text.find('\n')
        text = text[:first_newline+1] + '\n' + heading + '\n\n' + body.strip() + '\n\n' + text[first_newline+1:]
    else:
        text = heading + '\n\n' + body.strip() + '\n\n' + text
    path.write_text(text, encoding="utf-8")


def main() -> int:
    try:
        for rel, expected in BASE_BLOBS.items():
            path = ROOT / rel
            need(path.is_file(), f"missing baseline file: {rel}")
            need(git_blob(path) == expected, f"STOP: {rel} is not the exact accepted v0.0.49 baseline")
        patch_main(ROOT / "src/main.cpp")
        patch_diagnostics(ROOT / "src/diagnostics/diagnostic_engine.cpp")
        patch_cmake(ROOT / "CMakeLists.txt")
        patch_desktop(ROOT / "NougatMediaSuite.desktop")
        patch_desktop(ROOT / "com.elderredsoftworks.NougatMediaSuite.desktop")

        prepend_section(ROOT / "CHANGELOG.md", "## v0.0.50 - Studio File Splitter and Unified Tuners", '''
- Adds the Studio File Splitter / Reassembler with configurable part sizes, per-part SHA-256 verification, byte-exact reconstruction, folder packaging, corruption refusal, and Zenity-backed Studio actions.
- Keeps the area named Studio; the old Gold Studio wording is removed from the active Studio screen.
- Adds HDHomeRun LAN tuner discovery alongside the accepted Linux DVB / Hauppauge WinTV backend without using an external viewer.
- Exposes HDHomeRun physical tuners independently so a FLEX DUO can keep playback on one tuner while a channel scan uses the other when free.
- Merges HDHomeRun lineup/scan results into Nougat's existing Live TV channel model and sends HDHomeRun MPEG-TS streams through Nougat's embedded libVLC player.
- Keeps all LAN Web Viewer code out of v0.0.50. v0.0.51 is assigned to remaining emulator support plus the LAN Web Viewer foundation/scaffolding.
''')
        prepend_section(ROOT / "DEPENDENCIES.md", "## Nougat Media Suite v0.0.50 Studio splitter and HDHomeRun requirements", '''
v0.0.50 adds no new linked third-party library. HDHomeRun control/discovery uses the installed `hdhomerun_config` command from Ubuntu's `hdhomerun-config` package behind a Nougat-owned provider boundary. MPEG-TS playback is handed directly to the existing embedded libVLC player. `curl`, Python 3, and Zenity are already part of Nougat's accepted dependency stack and are reused by the HDHomeRun lineup path and Studio File Splitter dialogs. The File Splitter core uses Python standard-library modules only.
''')
        prepend_section(ROOT / "ROADMAP.md", "## Studio professional-production direction after v0.0.50", '''
Studio remains Nougat's integrated production environment. After the v0.0.50 File Splitter foundation, the roadmap includes professional video/audio/photo editing, green-screen keying and compositing, animation, VFX, camera/object tracking, rotoscoping, motion capture and retargeting, production asset management, high-resolution rendering, and render-queue/farm workflows. Architecture should remain capable of scaling toward large live-action/VFX feature-film production rather than a consumer-only editor.

Future Studio work also includes **Nougat Studio Aerial Production** for movie shoots: a Nougat-owned computer ground station with live drone-camera ingest and telemetry, DualSense/gamepad plus keyboard/mouse control, aircraft/gimbal/camera mapping, waypoint missions, automated and repeatable cinematic flight/gimbal paths, shot presets, synchronized flight/gimbal/camera metadata, exact-take replay, and production ingest into Studio for stabilization, camera tracking, compositing, and VFX. The provider boundary should support open autopilots such as PX4/ArduPilot and supported proprietary platforms including DJI. Model-specific custom drone software/firmware swaps and deeper DJI integration may be supported after the exact aircraft, controller, flight controller, and firmware combination is verified. Safety-critical flight behavior stays on a verified flight-control layer with manual takeover, link-loss behavior, return-to-home, battery limits, and geofencing available to the production crew.

No Aerial Production implementation or LAN Web Viewer code is part of v0.0.50. v0.0.51 is assigned to remaining emulator support plus the LAN Web Viewer foundation/scaffolding.
''')
        print("Nougat v0.0.50 source patch applied to exact v0.0.49 baseline.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
