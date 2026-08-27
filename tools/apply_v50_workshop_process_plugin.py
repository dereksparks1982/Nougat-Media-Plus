#!/usr/bin/env python3
"""Finish Workshop Plugin #1 by removing Workshop implementation from core.

This runs after apply_v50_core.py and apply_v50_plugin_foundation.py. The older
migration patch first reproduces the accepted v49 Workshop prototype exactly;
this guarded migration then removes that feature implementation from main.cpp
and leaves only a generic validated-plugin host surface in the player core.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
MARKER = "NOUGAT_V50_WORKSHOP_PROCESS_PLUGIN"


def fail(message: str) -> None:
    raise RuntimeError(message)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        fail(f"{label}: expected exactly 1 match, found {count}")
    return text.replace(old, new, 1)


def remove_between(text: str, start: str, end: str, replacement: str, label: str) -> str:
    begin = text.find(start)
    if begin < 0:
        fail(label + ": start marker missing")
    finish = text.find(end, begin)
    if finish < 0:
        fail(label + ": end marker missing")
    return text[:begin] + replacement + text[finish:]


def validate(text: str) -> None:
    required = [
        MARKER,
        '#include "plugins/plugin_registry.hpp"',
        '#include "plugins/plugin_process_host.hpp"',
        "nougat::plugins::PluginProcessHost workshopPluginHost;",
        "workshop_plugin_ready()",
        "workshopPluginHost.start(",
        "workshopPluginHost.stop();",
        'draw_tab(studioTab,"Workshop",ViewMode::Studio);',
    ]
    missing = [item for item in required if item not in text]
    if missing:
        fail("Workshop process-plugin migration missing: " + ", ".join(missing))

    forbidden = [
        '#include "workshop/split_archive_service.hpp"',
        "struct WorkshopUiState",
        "workshopState",
        "workshopWorker",
        "workshopSourcePath",
        "workshopOutputFolder",
        "workshopFileBtn",
        "workshopFolderBtn",
        "workshopOutputBtn",
        "workshopModeBtn",
        "workshopMinusBtn",
        "workshopPlusBtn",
        "workshopSplitBtn",
        "workshopReassembleBtn",
        "start_workshop_inspect(",
        "start_workshop_split()",
        "start_workshop_reassemble(",
        "poll_workshop_worker();",
        "handle_workshop_click(x,y);",
        'nougat::paths::plugin_installed("workshop")',
        "nougat::workshop::SplitArchiveService",
    ]
    present = [item for item in forbidden if item in text]
    if present:
        fail("Workshop remains hardwired in player core: " + ", ".join(present))


def main() -> int:
    try:
        text = MAIN.read_text(encoding="utf-8")
        if MARKER in text:
            validate(text)
            print("PASS: Workshop is already isolated behind the process plugin host")
            return 0

        if "NOUGAT_V50_PLUGIN_FOUNDATION" not in text:
            fail("apply_v50_plugin_foundation.py must run before Workshop process isolation")

        text = replace_once(
            text,
            '#include "workshop/split_archive_service.hpp"\n',
            '#include "plugins/plugin_registry.hpp"\n'
            '#include "plugins/plugin_process_host.hpp"\n',
            "Workshop core include boundary",
        )

        text = remove_between(
            text,
            "// NOUGAT_V50_WORKSHOP_PATCH\nstruct WorkshopUiState {",
            "struct DebugUiState {",
            f"// {MARKER}\n",
            "Workshop UI state removal",
        )

        member_start = "    std::shared_ptr<WorkshopUiState> workshopState = std::make_shared<WorkshopUiState>();\n"
        member_end = "    Rect workshopMinusBtn, workshopPlusBtn, workshopSplitBtn, workshopReassembleBtn;\n"
        begin = text.find(member_start)
        if begin < 0:
            fail("Workshop member block start missing")
        finish = text.find(member_end, begin)
        if finish < 0:
            fail("Workshop member block end missing")
        finish += len(member_end)
        replacement = (
            "    nougat::plugins::PluginProcessHost workshopPluginHost;\n"
            "    Window workshopPluginSurface = 0;\n"
            "    bool workshopPluginLaunchAttempted = false;\n"
            "    std::string workshopPluginStatus;\n"
        )
        text = text[:begin] + replacement + text[finish:]

        generic_host = r'''    bool workshop_plugin_manifest(nougat::plugins::PluginManifest& manifest) const {
        const nougat::plugins::PluginScanResult scan = nougat::plugins::scan_installed_plugins();
        for (const auto& plugin : scan.plugins) {
            if (plugin.id == "workshop" && plugin.top_level_tab == "Workshop") {
                manifest = plugin;
                return true;
            }
        }
        return false;
    }

    bool workshop_plugin_ready() const {
        nougat::plugins::PluginManifest manifest;
        return workshop_plugin_manifest(manifest);
    }

    void stop_workshop_plugin_host() {
        if (workshopPluginHost.running()) workshopPluginHost.stop();
        if (d && workshopPluginSurface != 0) XUnmapWindow(d, workshopPluginSurface);
        workshopPluginLaunchAttempted = false;
        workshopPluginStatus.clear();
    }

    void ensure_workshop_plugin_host() {
        if (!d || !win) return;
        nougat::plugins::PluginManifest manifest;
        if (!workshop_plugin_manifest(manifest)) {
            workshopPluginStatus = "Workshop plugin is not installed or failed validation.";
            return;
        }

        const Rect frame = page_content_frame(ViewMode::Studio);
        const int x = frame.x + 4;
        const int y = frame.y + 4;
        const int width = std::max(1, frame.w - 8);
        const int height = std::max(1, frame.h - 8);
        if (workshopPluginSurface == 0) {
            workshopPluginSurface = XCreateSimpleWindow(
                d, win, x, y,
                static_cast<unsigned int>(width), static_cast<unsigned int>(height),
                0, BlackPixel(d, DefaultScreen(d)), WhitePixel(d, DefaultScreen(d)));
        } else {
            XMoveResizeWindow(d, workshopPluginSurface, x, y,
                              static_cast<unsigned int>(width), static_cast<unsigned int>(height));
        }
        XMapRaised(d, workshopPluginSurface);

        if (workshopPluginHost.running()) {
            std::string status;
            if (!workshopPluginHost.poll(&status) && !status.empty()) workshopPluginStatus = status;
            return;
        }
        if (workshopPluginLaunchAttempted) return;

        workshopPluginLaunchAttempted = true;
        std::string error;
        if (!workshopPluginHost.start(manifest, static_cast<unsigned long>(workshopPluginSurface),
                                      width, height, error)) {
            workshopPluginStatus = error.empty() ? "Workshop plugin could not start." : error;
            return;
        }
        workshopPluginStatus = "Workshop plugin running.";
    }

    void draw_studio_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Studio);
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Studio);
        if (!workshop_plugin_ready()) {
            section_text(target, 28, 70, "WORKSHOP", palette.text);
            text(target, 28, 98, "Workshop plugin is not installed or failed validation.", palette.muted);
            return;
        }
        if (!workshopPluginStatus.empty() && !workshopPluginHost.running()) {
            section_text(target, 28, 70, "WORKSHOP", palette.text);
            text(target, 28, 98, head_to_width(workshopPluginStatus, W - 56), palette.muted);
        }
        ensure_workshop_plugin_host();
    }

'''
        text = remove_between(
            text,
            "    std::string workshop_worker_script() const {",
            "    void draw_debug_screen(Drawable target) {",
            generic_host,
            "hardwired Workshop method removal",
        )

        text = replace_once(
            text,
            '        if (nougat::paths::plugin_installed("workshop")) {\n'
            '            draw_tab(studioTab,"Workshop",ViewMode::Studio);\n'
            '        } else {\n'
            '            studioTab = {};\n'
            '        }\n',
            '        if (workshop_plugin_ready()) {\n'
            '            draw_tab(studioTab,"Workshop",ViewMode::Studio);\n'
            '        } else {\n'
            '            studioTab = {};\n'
            '        }\n',
            "strict Workshop tab discovery",
        )

        text = replace_once(
            text,
            '            &homeTab,&videoPlayerTab,&libraryTab,&discoverTab,&liveTvTab,&worldTvTab,&nougatTab,&ytdlpTab,&studioTab,&gamesTab,&debugTab,\n'
            '            &workshopFileBtn,&workshopFolderBtn,&workshopOutputBtn,&workshopModeBtn,&workshopMinusBtn,&workshopPlusBtn,&workshopSplitBtn,&workshopReassembleBtn,\n',
            '            &homeTab,&videoPlayerTab,&libraryTab,&discoverTab,&liveTvTab,&worldTvTab,&nougatTab,&ytdlpTab,&studioTab,&gamesTab,&debugTab,\n',
            "Workshop core hover target removal",
        )

        text = replace_once(
            text,
            '            poll_server_worker();\n            poll_workshop_worker();\n',
            '            poll_server_worker();\n',
            "Workshop core worker poll removal",
        )

        text = replace_once(
            text,
            '        if (currentView == ViewMode::Studio) {\n'
            '            handle_workshop_click(x,y);\n'
            '            return;\n'
            '        }\n',
            '',
            "Workshop core click handler removal",
        )

        text = replace_once(
            text,
            '        if (serverWorker.joinable()) serverWorker.join();\n'
            '        if (workshopWorker.joinable()) workshopWorker.join();\n'
            '        if (debugWorker.joinable()) {\n',
            '        if (serverWorker.joinable()) serverWorker.join();\n'
            '        if (debugWorker.joinable()) {\n',
            "Workshop core thread shutdown removal",
        )

        text = replace_once(
            text,
            '        if (currentView == ViewMode::Studio) draw_studio_screen(buffer);\n',
            '        if (currentView != ViewMode::Studio) stop_workshop_plugin_host();\n'
            '        if (currentView == ViewMode::Studio) draw_studio_screen(buffer);\n',
            "Workshop process lifecycle redraw hook",
        )

        validate(text)
        MAIN.write_text(text, encoding="utf-8")
        print("PASS: Workshop implementation removed from Nougat player core")
        print("PASS: Workshop tab now requires a strict validated x11-process plugin")
        print("PASS: Workshop process stops when its view is left; plugin folder remains source of truth")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
