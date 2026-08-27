#!/usr/bin/env python3
"""Remove the monolithic Games implementation from player core and host it as Plugin #2."""

from __future__ import annotations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
MARKER = "NOUGAT_V50_GAMES_PROCESS_PLUGIN"


def fail(message: str) -> None:
    raise RuntimeError(message)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        fail(f"{label}: expected exactly 1 match, found {count}")
    return text.replace(old, new, 1)


def remove_between(text: str, start: str, end: str, replacement: str, label: str, include_end: bool = False) -> str:
    begin = text.find(start)
    if begin < 0:
        fail(label + ": start marker missing")
    finish = text.find(end, begin)
    if finish < 0:
        fail(label + ": end marker missing")
    if include_end:
        finish += len(end)
    return text[:begin] + replacement + text[finish:]


def validate(text: str) -> None:
    required = [
        MARKER,
        "nougat::plugins::PluginProcessHost gamesPluginHost;",
        "games_plugin_ready()",
        "gamesPluginHost.start(",
        "gamesPluginHost.stop();",
        "draw_games_plugin_screen",
        'draw_tab(gamesTab,"Games",ViewMode::Games);',
    ]
    missing = [item for item in required if item not in text]
    if missing:
        fail("Games process-plugin migration missing: " + ", ".join(missing))
    forbidden = [
        '#include "games/emulator_host.hpp"', "GameEntry", "GameUiState", "GamesPanel", "GamesDisplayMode",
        "gameState", "gameHost", "currentMediaIsGame", "activeGameTitle", "activeGameSystem",
        "gamesLibraryBtn", "gamesSystemsBtn", "gamesAddBtn", "gamesControllersBtn", "gamesSettingsBtn",
        "gamesPlayBtn", "gamesRefreshBtn", "gamesGridBtn", "gamesListViewBtn", "gamesListBox",
        "gamesVerticalScroll", "gameArtwork", "gameScanWorker", "stellaTopOptions", "start_game_", "poll_game_",
        "handle_games_", "flush_games_", "CardContextKind::Games", "game_system_for_path",
        "safe_zip_game_entry", "dos_archive_entrypoint", "stop_game_session",
    ]
    present = [item for item in forbidden if item in text]
    if present:
        fail("Games remains hardwired in player core: " + ", ".join(present))


def main() -> int:
    try:
        text = MAIN.read_text(encoding="utf-8")
        if MARKER in text:
            validate(text)
            print("PASS: Games is already isolated behind the process plugin host")
            return 0
        if "NOUGAT_V50_WORKSHOP_PROCESS_PLUGIN" not in text:
            fail("Workshop process-plugin migration must run before Games isolation")

        text = replace_once(text, '#include "games/emulator_host.hpp"\n', '', "Games core include")
        text = replace_once(text,
            'enum class GamesDisplayMode { Grid, List };\nenum class GamesPanel { Library, Systems, Controllers, Settings };\n',
            '', "Games enums")
        text = replace_once(text,
            'enum class CardContextKind { Unset, Home, Library, Discover, LiveTV, WorldTV, Games };',
            'enum class CardContextKind { Unset, Home, Library, Discover, LiveTV, WorldTV };',
            "Games card context")
        text = replace_once(text, '    GamesPanel games_panel = GamesPanel::Library;\n', '', "Games navigation field")

        text = remove_between(text, 'struct GameEntry {',
            'enum class LiveTvTunerUse { Idle, Scanning, GuideRefreshing, Watching };',
            f'// {MARKER}\nenum class LiveTvTunerUse {{ Idle, Scanning, GuideRefreshing, Watching }};',
            "global Games implementation", True)

        text = replace_once(text,
            '    Rect gamesLibraryBtn, gamesSystemsBtn, gamesAddBtn, gamesControllersBtn, gamesSettingsBtn;\n'
            '    Rect gamesPlayBtn, gamesRefreshBtn, gamesGridBtn, gamesListViewBtn, gamesListBox;\n'
            '    Rect gamesVerticalScrollTrack, gamesVerticalScrollThumb;\n', '', "Games control members")
        text = replace_once(text, '    bool gamesVerticalScrollDragging = false;\n', '', "Games drag state")
        text = replace_once(text, '    int gamesVerticalScrollDragOffset = 0;\n', '', "Games drag offset")

        member_start = '    std::shared_ptr<GameUiState> gameState = std::make_shared<GameUiState>();\n'
        member_end = '    long long lastStellaTopOptionsMs = 0;\n'
        begin = text.find(member_start)
        finish = text.find(member_end, begin)
        if begin < 0 or finish < 0:
            fail("Games member block missing")
        finish += len(member_end)
        text = text[:begin] + (
            '    nougat::plugins::PluginProcessHost gamesPluginHost;\n'
            '    Window gamesPluginSurface = 0;\n'
            '    bool gamesPluginLaunchAttempted = false;\n'
            '    std::string gamesPluginStatus;\n'
        ) + text[finish:]

        text = remove_between(text, '        int gamesX = 28;\n',
            '        const Rect worldFrame = page_content_frame(ViewMode::WorldTV);\n',
            '        const Rect worldFrame = page_content_frame(ViewMode::WorldTV);\n', "Games layout", True)
        text = replace_once(text, '    \n        if (gameHost.active()) gameHost.resize(videoW, videoH);\n', '    \n', "Games player resize")
        text = replace_once(text, '        if (currentMediaIsGame || gameHost.active()) stop_game_session(false);\n', '', "Games player cleanup")
        text = replace_once(text,
            '        if (currentMediaIsGame || gameHost.active()) {\n            stop_game_session(true);\n            return;\n        }\n',
            '', "Games player stop")
        text = remove_between(text, '        if (currentMediaIsGame) {\n', '        if (upNextVisible) {\n',
            '        if (upNextVisible) {\n', "Games video message", True)
        text = replace_once(text,
            '        if (currentView == ViewMode::Games) {\n            std::lock_guard<std::mutex> lock(gameState->mutex);\n            if (gameState->busy) return true;\n        }\n\n',
            '', "Games loading state")

        generic_host = r'''    bool games_plugin_manifest(nougat::plugins::PluginManifest& manifest) const {
        const nougat::plugins::PluginScanResult scan = nougat::plugins::scan_installed_plugins();
        for (const auto& plugin : scan.plugins) {
            if (plugin.id == "games" && plugin.top_level_tab == "Games") {
                manifest = plugin;
                return true;
            }
        }
        return false;
    }

    bool games_plugin_ready() const {
        nougat::plugins::PluginManifest manifest;
        return games_plugin_manifest(manifest);
    }

    void stop_games_plugin_host() {
        if (gamesPluginHost.running()) gamesPluginHost.stop();
        if (d && gamesPluginSurface != 0) XUnmapWindow(d, gamesPluginSurface);
        gamesPluginLaunchAttempted = false;
        gamesPluginStatus.clear();
    }

    void ensure_games_plugin_host() {
        if (!d || !win) return;
        nougat::plugins::PluginManifest manifest;
        if (!games_plugin_manifest(manifest)) {
            gamesPluginStatus = "Games plugin is not installed or failed validation.";
            return;
        }
        const Rect frame = page_content_frame(ViewMode::Games);
        const int x = frame.x + 4;
        const int y = frame.y + 4;
        const int width = std::max(1, frame.w - 8);
        const int height = std::max(1, frame.h - 8);
        if (gamesPluginSurface == 0) {
            gamesPluginSurface = XCreateSimpleWindow(
                d, win, x, y, static_cast<unsigned int>(width), static_cast<unsigned int>(height),
                0, BlackPixel(d, DefaultScreen(d)), WhitePixel(d, DefaultScreen(d)));
        } else {
            XMoveResizeWindow(d, gamesPluginSurface, x, y,
                              static_cast<unsigned int>(width), static_cast<unsigned int>(height));
        }
        XMapRaised(d, gamesPluginSurface);
        if (gamesPluginHost.running()) {
            std::string status;
            if (!gamesPluginHost.poll(&status) && !status.empty()) gamesPluginStatus = status;
            return;
        }
        if (gamesPluginLaunchAttempted) return;
        gamesPluginLaunchAttempted = true;
        std::string error;
        if (!gamesPluginHost.start(manifest, static_cast<unsigned long>(gamesPluginSurface), width, height, error)) {
            gamesPluginStatus = error.empty() ? "Games plugin could not start." : error;
            return;
        }
        gamesPluginStatus = "Games plugin running.";
    }

    void draw_games_plugin_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Games);
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Games);
        if (!games_plugin_ready()) {
            section_text(target, 28, 70, "GAMES", palette.text);
            text(target, 28, 98, "Games plugin is not installed or failed validation.", palette.muted);
            return;
        }
        if (!gamesPluginStatus.empty() && !gamesPluginHost.running()) {
            section_text(target, 28, 70, "GAMES", palette.text);
            text(target, 28, 98, head_to_width(gamesPluginStatus, W - 56), palette.muted);
        }
        ensure_games_plugin_host();
    }

'''
        text = remove_between(text, '    std::string games_config_directory() const {\n',
            '    bool workshop_plugin_manifest(nougat::plugins::PluginManifest& manifest) const {\n',
            generic_host + '    bool workshop_plugin_manifest(nougat::plugins::PluginManifest& manifest) const {\n',
            "hardwired Games methods", True)

        text = replace_once(text, '        draw_tab(gamesTab,"Games",ViewMode::Games);\n',
            '        if (games_plugin_ready()) {\n            draw_tab(gamesTab,"Games",ViewMode::Games);\n        } else {\n            gamesTab = {};\n        }\n',
            "Games tab discovery")
        text = replace_once(text, '        if (currentView == ViewMode::Games) draw_games_screen(buffer);\n',
            '        if (currentView != ViewMode::Games) stop_games_plugin_host();\n'
            '        if (currentView == ViewMode::Games) draw_games_plugin_screen(buffer);\n',
            "Games redraw host")

        text = replace_once(text, '        snapshot.games_panel = gamesPanel;\n', '', "Games navigation capture")
        text = replace_once(text,
            '            a.stream_platform != b.stream_platform ||\n            a.games_panel != b.games_panel ||\n',
            '            a.stream_platform != b.stream_platform ||\n', "Games navigation compare")
        text = replace_once(text, '        gamesPanel = snapshot.games_panel;\n', '', "Games navigation restore")
        text = replace_once(text,
            '        if (currentView == ViewMode::Games) {\n            bool loaded = false;\n'
            '            { std::lock_guard<std::mutex> lock(gameState->mutex); loaded = gameState->loaded; }\n'
            '            if (!loaded) start_game_scan();\n        }\n', '', "Games view scan")

        text = remove_between(text, '        } else if (currentView==ViewMode::Games) {\n',
            '        }\n        if (items.empty())', '        }\n        if (items.empty())', "Games context menu", True)
        text = remove_between(text, '        if (cardContextKind==CardContextKind::Games) {\n',
            '    }\n\n    void run_menu_action', '    }\n\n    void run_menu_action', "Games context action", True)
        text = replace_once(text,
            '            &worldTvPlayBtn,&worldTvOfficialBtn,\n'
            '            &gamesLibraryBtn,&gamesSystemsBtn,&gamesAddBtn,&gamesControllersBtn,&gamesSettingsBtn,&gamesPlayBtn,&gamesRefreshBtn\n',
            '            &worldTvPlayBtn,&worldTvOfficialBtn\n', "Games hover controls")
        text = replace_once(text,
            '        if (currentView == ViewMode::Games && target == win && gamesListBox.contains(x,y) && gamesPanel == GamesPanel::Library) {\n'
            '            handle_games_wheel_steps(target, x, y, button == Button4 ? -1 : 1);\n            return true;\n        }\n',
            '', "Games wheel handling")
        text = replace_once(text,
            '        if (currentView == ViewMode::Games) {\n            handle_games_click(x,y,eventTime);\n            return;\n        }\n',
            '', "Games core click handling")
        text = replace_once(text,
            '        if (gameHost.active()) gameHost.stop();\n        currentMediaIsGame = false;\n'
            '        activeGameTitle.clear();\n        activeGameSystem.clear();\n', '', "Games final player cleanup")
        text = replace_once(text, '            poll_game_session();\n            poll_stella_top_options();\n', '', "Games event polling")

        games_wheel = '''                        if (currentView == ViewMode::Games && e.xbutton.window == win &&
                            gamesPanel == GamesPanel::Library && gamesListBox.contains(e.xbutton.x, e.xbutton.y)) {
                            int steps = e.xbutton.button == Button4 ? -1 : 1;
                            int wheel_x = e.xbutton.x;
                            int wheel_y = e.xbutton.y;
                            XEvent queued;
                            while (XCheckTypedWindowEvent(d, win, ButtonPress, &queued)) {
                                if ((queued.xbutton.button == Button4 || queued.xbutton.button == Button5) &&
                                    gamesListBox.contains(queued.xbutton.x, queued.xbutton.y)) {
                                    steps += queued.xbutton.button == Button4 ? -1 : 1;
                                    wheel_x = queued.xbutton.x;
                                    wheel_y = queued.xbutton.y;
                                    continue;
                                }
                                XPutBackEvent(d, &queued);
                                break;
                            }
                            handle_games_wheel_steps(win, wheel_x, wheel_y, steps);
                        } else {
                            handle_wheel(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button);
                        }
'''
        text = replace_once(text, games_wheel,
            '                        handle_wheel(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button);\n',
            "Games wheel aggregation")
        text = replace_once(text, '                    const bool finishedGamesDrag = gamesVerticalScrollDragging;\n', '', "Games drag release flag")
        text = text.replace('                    gamesVerticalScrollDragging = false;\n', '', 1)
        text = replace_once(text, '                    if (finishedGamesDrag) request_games_interactive_redraw(true);\n', '', "Games drag release redraw")
        text = replace_once(text,
            '                    if ((homeVerticalScrollDragging || homeContinueScrollDragging || libraryVerticalScrollDragging || gamesVerticalScrollDragging ||\n'
            '                         discoverServicesScrollDragging) && (e.xmotion.state & Button1Mask) == 0) {\n',
            '                    if ((homeVerticalScrollDragging || homeContinueScrollDragging || libraryVerticalScrollDragging ||\n'
            '                         discoverServicesScrollDragging) && (e.xmotion.state & Button1Mask) == 0) {\n',
            "Games drag motion condition")
        text = text.replace('                        gamesVerticalScrollDragging = false;\n', '', 1)
        text = replace_once(text, '                            if (currentView == ViewMode::Games) handle_games_scrollbar_motion(e.xmotion.y);\n', '', "Games drag motion")
        text = replace_once(text, '            flush_games_interactive_redraw();\n', '', "Games deferred redraw")
        text = replace_once(text, '            poll_game_scan();\n            poll_game_artwork_prefetch();\n', '', "Games scan polling")
        text = replace_once(text,
            '        stop_game_artwork_prefetch();\n        if (gameScanWorker.joinable()) {\n            bool busy=false;\n'
            '            { std::lock_guard<std::mutex> lock(gameState->mutex); busy=gameState->busy; }\n'
            '            if (busy) gameScanWorker.detach(); else gameScanWorker.join();\n        }\n',
            '        stop_games_plugin_host();\n', "Games shutdown")

        text = remove_between(text,
            '    if (argc > 1 && std::string(argv[1]) == "--v49-games-self-test") {\n',
            '    if (argc > 1 && std::string(argv[1]) == "--v47-fullscreen-controls-self-test") {\n',
            '    if (argc > 1 && std::string(argv[1]) == "--v47-fullscreen-controls-self-test") {\n',
            "old core Games self-test", True)

        v44_start = text.find('        const bool games_layout_ok = ')
        v44_end_marker = '        return 1;\n    }\n    if (argc > 1 && std::string(argv[1]) == "--v25-ui-state-self-test") {'
        v44_end = text.find(v44_end_marker, v44_start)
        if v44_start < 0 or v44_end < 0:
            fail("v44 Games self-test seam missing")
        v44_end += len('        return 1;\n    }\n')
        v44_replacement = (
            '        const bool world_tv_ok = ([&app](){ const auto& stations=app.world_tv_catalog(); return !stations.empty() && std::all_of(stations.begin(),stations.end(),[](const reddmedia::WorldTvStation& s){ const std::string u=lower_copy(s.preferred_url); return s.max_height>0 && s.max_height<=1080 && u.find("youtube.com")==std::string::npos && u.find("youtu.be")==std::string::npos; }); })();\n'
            '        if (grid_ok && world_tv_ok) {\n            printf("Nougat Media Suite v0.0.47 release self-test PASS\\n");\n            return 0;\n        }\n'
            '        fprintf(stderr, "Nougat Media Suite v0.0.47 release self-test FAIL: grid=%d world_tv=%d\\n",\n'
            '                grid_ok ? 1 : 0, world_tv_ok ? 1 : 0);\n        return 1;\n    }\n'
        )
        text = text[:v44_start] + v44_replacement + text[v44_end:]

        validate(text)
        MAIN.write_text(text, encoding="utf-8")
        print("PASS: Games implementation removed from Nougat player core")
        print("PASS: Games tab now requires a strict validated x11-process plugin")
        print("PASS: ROM scanning, emulator hosting, controller handling, artwork and legal starter games belong to Games plugin")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
