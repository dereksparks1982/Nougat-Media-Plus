#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import sys

BASE_SHA = "e4d7ed25aeecc02fda60ae6f9f929fd52bcf6811"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly 1 match, found {count}")
    return text.replace(old, new, 1)


def replace_all_exact(text: str, old: str, new: str, expected: int, label: str) -> str:
    count = text.count(old)
    if count != expected:
        raise RuntimeError(f"{label}: expected {expected} matches, found {count}")
    return text.replace(old, new)


def patch_cmake(path: Path) -> None:
    text = path.read_text()
    text = replace_once(text,
        "project(NougatMediaSuite VERSION 0.0.54 LANGUAGES CXX)",
        "project(NougatMediaSuite VERSION 0.0.55 LANGUAGES CXX)", "cmake version")
    text = replace_all_exact(text, "Nougat_Media_Suite_v54", "Nougat_Media_Suite_v55", 10, "cmake target")
    text = replace_once(text,
        "    src/games/emulator_host.cpp\n",
        "    src/games/emulator_host.cpp\n    src/games/emulator_registry.cpp\n    src/library/user_library_state.cpp\n    src/player/up_next_title.cpp\n",
        "cmake v55 helper sources")
    path.write_text(text)


def patch_desktop(path: Path) -> None:
    text = path.read_text()
    text = replace_once(text, "Nougat_Media_Suite_v54", "Nougat_Media_Suite_v55", f"{path.name} exec")
    path.write_text(text)


def patch_jellyfin(path: Path) -> None:
    text = path.read_text()
    text = replace_once(text,
        '#include "library_poster.hpp"\n',
        '#include "library_poster.hpp"\n#include "../library/user_library_state.hpp"\n',
        "jellyfin migration include")

    old = '''JellyfinApiClient::JellyfinApiClient(std::string state_file)\n    : state_file_(std::move(state_file)) {\n    if (state_file_.empty()) {\n        const char* override_file = std::getenv("REDDMEDIA_SERVER_CLIENT_CONFIG");\n        if (override_file && *override_file) {\n            state_file_ = override_file;\n        } else {\n            const char* home = std::getenv("HOME");\n            state_file_ = std::string(home ? home : ".") + "/.config/reddmedia/server/client.json";\n        }\n    }\n    // Keep the compatibility directory used by accepted releases, but give\n    // library mappings their own owner-controlled state file. This survives\n    // versioned binaries and can repair Jellyfin virtual-folder loss.\n    mapping_state_file_ = parent_directory(state_file_) + "/library_mappings.tsv";\n}\n'''
    new = '''JellyfinApiClient::JellyfinApiClient(std::string state_file)\n    : state_file_(std::move(state_file)) {\n    if (state_file_.empty()) {\n        const char* override_file = std::getenv("REDDMEDIA_SERVER_CLIENT_CONFIG");\n        if (override_file && *override_file) {\n            // An explicit owner/test override remains authoritative and is not\n            // silently relocated by an upgrade.\n            state_file_ = override_file;\n        } else {\n            const char* home = std::getenv("HOME");\n            const std::string config_root = std::string(home ? home : ".") + "/.config/reddmedia";\n            const std::string legacy_state = config_root + "/server/client.json";\n            const std::string legacy_mappings = config_root + "/server/library_mappings.tsv";\n            state_file_ = config_root + "/library/client.json";\n            mapping_state_file_ = config_root + "/library/library_mappings.tsv";\n\n            // v0.0.55: library ownership state is no longer stored inside the\n            // integrated Jellyfin server-config namespace. Copy the accepted\n            // legacy state only when the canonical files do not yet exist.\n            // The legacy files are deliberately retained for rollback.\n            (void)migrate_private_state_file(legacy_state, state_file_);\n            (void)migrate_private_state_file(legacy_mappings, mapping_state_file_);\n        }\n    }\n    if (mapping_state_file_.empty()) {\n        // Custom/explicit state files retain the accepted same-directory\n        // mapping-registry behavior.\n        mapping_state_file_ = parent_directory(state_file_) + "/library_mappings.tsv";\n    }\n}\n'''
    text = replace_once(text, old, new, "jellyfin canonical mapping migration")
    path.write_text(text)


def patch_main(path: Path) -> None:
    text = path.read_text()
    text = replace_once(text,
        '#include "games/emulator_host.hpp"\n',
        '#include "games/emulator_host.hpp"\n#include "games/emulator_registry.hpp"\n#include "library/user_library_state.hpp"\n#include "player/up_next_title.hpp"\n',
        "main v55 includes")
    text = replace_once(text, "// NOUGAT_V54_FILE_SPLITTER_PROFESSIONAL", "// NOUGAT_V55_INTEGRATED_REPAIR")

    text = replace_once(text,
        "    CardPlay, CardOpenSource, CardInfo, CardRefresh, CardOpenOfficial, CardRefreshArtwork, CardOpenArtwork\n",
        "    CardPlay, CardRemove, CardDeleteRequest, CardDeleteCancel, CardDeleteConfirm, CardOpenSource, CardInfo, CardRefresh, CardOpenOfficial, CardRefreshArtwork, CardOpenArtwork\n",
        "context actions")

    text = replace_once(text,
        "    PlaybackResumeStore resumeStore;\n",
        "    PlaybackResumeStore resumeStore;\n    reddmedia::ContinueWatchingSuppressionStore continueWatchingSuppressions;\n    reddmedia::LibraryExclusionStore libraryExclusions;\n",
        "persistent home/library state")

    # Only the Up Next overlay changes. The general library/player formatter remains untouched.
    text = replace_once(text,
        '                const std::string title = "Up Next: " + library_display_title(upNextEpisode);\n',
        '                const std::string title = "Up Next: " + reddmedia::up_next_episode_title(\n                    upNextEpisode.season_number, upNextEpisode.episode_number,\n                    upNextEpisode.series_name, upNextEpisode.episode_title, upNextEpisode.name);\n',
        "Up Next title formatter")

    # Continue Watching refresh filtering plus Library exclusions for Home sections.
    text = replace_once(text,
        "        const std::vector<ResumeRecord> continue_records = resumeStore.unfinished();\n",
        "        std::vector<ResumeRecord> continue_records = resumeStore.unfinished();\n        const std::set<std::string> continue_hidden = continueWatchingSuppressions.snapshot();\n        const std::set<std::string> library_excluded = libraryExclusions.snapshot();\n        continue_records.erase(std::remove_if(continue_records.begin(), continue_records.end(),\n            [&continue_hidden, &library_excluded](const ResumeRecord& record) {\n                return continue_hidden.count(record.path) != 0U || library_excluded.count(record.path) != 0U;\n            }), continue_records.end());\n",
        "Continue Watching persistent suppression")
    text = replace_once(text,
        "        homeWorker = std::thread([state, client, engine, continue_records]() {\n",
        "        homeWorker = std::thread([state, client, engine, continue_records, library_excluded]() {\n",
        "Home worker exclusions capture")
    old_filter = '''                nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const reddmedia::LibraryNode& node) {\n                    return node.kind != reddmedia::LibraryNodeKind::Movie &&\n                           node.kind != reddmedia::LibraryNodeKind::Series;\n                }), nodes.end());\n'''
    new_filter = '''                nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [&library_excluded](const reddmedia::LibraryNode& node) {\n                    if (!node.path.empty() && library_excluded.count(node.path) != 0U) return true;\n                    return node.kind != reddmedia::LibraryNodeKind::Movie &&\n                           node.kind != reddmedia::LibraryNodeKind::Series;\n                }), nodes.end());\n'''
    text = replace_once(text, old_filter, new_filter, "Home exclusion filter")

    # Library rendering/selection obeys the same persistent exclusion list.
    text = replace_once(text,
        "        for (std::size_t i=0; i<libraryState->nodes.size(); ++i) {\n            if (library_node_matches_search(libraryState->nodes[i])) indices.push_back(static_cast<int>(i));\n        }\n",
        "        for (std::size_t i=0; i<libraryState->nodes.size(); ++i) {\n            const auto& node = libraryState->nodes[i];\n            if (!node.path.empty() && libraryExclusions.contains(node.path)) continue;\n            if (library_node_matches_search(node)) indices.push_back(static_cast<int>(i));\n        }\n",
        "Library exclusion filter")

    # Replay/resume makes a previously shelf-only Remove eligible to return naturally.
    text = replace_once(text,
        "        ResumeRecord record = current_resume_record(position, duration);\n        resumeStore.update(record);\n        homeNeedsRefresh.store(true);\n",
        "        ResumeRecord record = current_resume_record(position, duration);\n        resumeStore.update(record);\n        (void)continueWatchingSuppressions.restore(record.path);\n        homeNeedsRefresh.store(true);\n",
        "resume suppression restoration")
    text = replace_once(text,
        "        ResumeRecord record = current_resume_record(stopped_at, duration);\n        if (!record.path.empty()) resumeStore.update(record);\n",
        "        ResumeRecord record = current_resume_record(stopped_at, duration);\n        if (!record.path.empty()) {\n            resumeStore.update(record);\n            (void)continueWatchingSuppressions.restore(record.path);\n        }\n",
        "stop suppression restoration")

    # Home right-click menu. Delete is always last and requires a second explicit click.
    old_menu = '''            std::vector<MenuItem> items={\n                {"Play / Open",MenuAction::CardPlay,0,true},\n                {"Open Source",MenuAction::CardOpenSource,0,true},\n                {"Media Information",MenuAction::CardInfo,0,true},\n                {"Refresh Home",MenuAction::CardRefresh,0,true}\n            };\n            show_context_menu(event.x_root,event.y_root,items,CardContextKind::Home,hitIndex); return;\n'''
    new_menu = '''            std::vector<MenuItem> items;\n            const HomeCardHitbox& hit = homeCardHitboxes[static_cast<std::size_t>(hitIndex)];\n            if (hit.continue_watching) {\n                items={\n                    {"Play / Open",MenuAction::CardPlay,0,true},\n                    {"Remove",MenuAction::CardRemove,0,true},\n                    {"Open Source",MenuAction::CardOpenSource,0,true},\n                    {"Media Information",MenuAction::CardInfo,0,true},\n                    {"Refresh Home",MenuAction::CardRefresh,0,true},\n                    {"Delete",MenuAction::CardDeleteRequest,0,true}\n                };\n            } else {\n                items={\n                    {"Play / Open",MenuAction::CardPlay,0,true},\n                    {"Open Source",MenuAction::CardOpenSource,0,true},\n                    {"Media Information",MenuAction::CardInfo,0,true},\n                    {"Refresh Home",MenuAction::CardRefresh,0,true}\n                };\n            }\n            show_context_menu(event.x_root,event.y_root,items,CardContextKind::Home,hitIndex); return;\n'''
    text = replace_once(text, old_menu, new_menu, "Continue Watching context menu")

    old_home_actions = '''            if (action==MenuAction::CardPlay) open_home_card(hit);\n            else if (action==MenuAction::CardOpenSource) open_source_path_in_files(hit.node.path);\n            else if (action==MenuAction::CardInfo) { { std::lock_guard<std::mutex> lock(homeState->mutex); homeState->status="Selected: "+library_display_title(hit.node)+(hit.node.path.empty()?"":" | "+hit.node.path); homeState->updated=true; } redraw(); }\n            else if (action==MenuAction::CardRefresh) start_home_task();\n            return;\n'''
    new_home_actions = '''            if (action==MenuAction::CardPlay) open_home_card(hit);\n            else if (action==MenuAction::CardRemove && hit.continue_watching) {\n                const bool saved = continueWatchingSuppressions.suppress(hit.node.path);\n                {\n                    std::lock_guard<std::mutex> lock(homeState->mutex);\n                    if (saved) {\n                        auto& rows = homeState->continue_watching;\n                        rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const ResumeRecord& record) {\n                            return record.path == hit.node.path;\n                        }), rows.end());\n                        homeState->status = "Removed from Continue Watching.";\n                    } else homeState->status = "Nougat could not save the Continue Watching removal.";\n                    homeState->updated = true;\n                }\n                if (saved) homeNeedsRefresh.store(true);\n                redraw();\n            }\n            else if (action==MenuAction::CardDeleteRequest && hit.continue_watching) {\n                const std::vector<MenuItem> confirmation={\n                    {"Are you sure?",MenuAction::NoAction,0,false},\n                    {"Cancel",MenuAction::CardDeleteCancel,0,true},\n                    {"Delete",MenuAction::CardDeleteConfirm,0,true}\n                };\n                show_context_menu(pointerRootX,pointerRootY,confirmation,CardContextKind::Home,cardContextIndex);\n            }\n            else if (action==MenuAction::CardDeleteCancel) { redraw(); }\n            else if (action==MenuAction::CardDeleteConfirm && hit.continue_watching) {\n                const bool excluded = libraryExclusions.exclude(hit.node.path);\n                if (excluded) (void)continueWatchingSuppressions.suppress(hit.node.path);\n                {\n                    std::lock_guard<std::mutex> lock(homeState->mutex);\n                    if (excluded) {\n                        auto& rows = homeState->continue_watching;\n                        rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const ResumeRecord& record) {\n                            return record.path == hit.node.path;\n                        }), rows.end());\n                        for (HomeSection& section : homeState->sections) {\n                            section.items.erase(std::remove_if(section.items.begin(), section.items.end(), [&](const reddmedia::LibraryNode& node) {\n                                return node.path == hit.node.path;\n                            }), section.items.end());\n                        }\n                        homeState->status = "Deleted from the Nougat library. The media file was not deleted.";\n                    } else homeState->status = "Nougat could not save the library deletion.";\n                    homeState->updated = true;\n                }\n                if (excluded) { homeNeedsRefresh.store(true); start_home_task(); }\n                redraw();\n            }\n            else if (action==MenuAction::CardOpenSource) open_source_path_in_files(hit.node.path);\n            else if (action==MenuAction::CardInfo) { { std::lock_guard<std::mutex> lock(homeState->mutex); homeState->status="Selected: "+library_display_title(hit.node)+(hit.node.path.empty()?"":" | "+hit.node.path); homeState->updated=true; } redraw(); }\n            else if (action==MenuAction::CardRefresh) start_home_task();\n            return;\n'''
    text = replace_once(text, old_home_actions, new_home_actions, "Home Remove/Delete behavior")

    # Emulator discovery is centralized so readiness and launching use the same truth.
    start = text.find("        std::string installed_game_emulator(const std::string& system) const {")
    if start < 0:
        start = text.find("    std::string installed_game_emulator(const std::string& system) const {")
    if start < 0:
        raise RuntimeError("installed_game_emulator: start not found")
    end_marker = "\n    std::string extracted_game_path(const GameEntry& selected) const {"
    end = text.find(end_marker, start)
    if end < 0:
        raise RuntimeError("installed_game_emulator: end not found")
    indent = "        " if text.startswith("        std::string installed_game_emulator", start) else "    "
    replacement = indent + '''std::string installed_game_emulator(const std::string& system) const {\n        return nougat::games::find_emulator(exe_dir(), system);\n    }\n'''
    text = text[:start] + replacement + text[end:]

    old_pcsx2 = '''        if (backend_lower.find("pcsx2") != std::string::npos) {\n            request.argv = {emulator, "-fullscreen", launchPath};\n            return true;\n        }\n'''
    new_pcsx2 = '''        if (selected.system == "PlayStation 2") {\n            if (!nougat::games::pcsx2_bios_available(exe_dir())) {\n                error = "PCSX2 is installed, but no user-provided PlayStation 2 BIOS was found. Add your legally obtained BIOS to PCSX2 or set NOUGAT_PCSX2_BIOS_DIR.";\n                return false;\n            }\n            request.backend = "PCSX2";\n            request.argv = {emulator, "-batch", "-fullscreen", launchPath};\n            return true;\n        }\n'''
    text = replace_once(text, old_pcsx2, new_pcsx2, "PCSX2 embedded launch")

    # Make missing-PS2 diagnostics specific and useful.
    text = replace_once(text,
        '''        if (emulator.empty()) {\n            error =\n                "No supported " + selected.system +\n                " emulator is available.";\n            return false;\n        }\n''',
        '''        if (emulator.empty()) {\n            if (selected.system == "PlayStation 2") {\n                error = "PCSX2 is unavailable. Install PCSX2, place a runtime in components/games/runtime/pcsx2/, or set NOUGAT_PCSX2.";\n            } else {\n                error = "No supported " + selected.system + " emulator is available.";\n            }\n            return false;\n        }\n''',
        "emulator missing diagnostics")

    # Only ready backends are advertised. No theoretical launcher list.
    old_systems = '''        if (gamesPanel==GamesPanel::Systems) {\n            section_text(target,gamesListBox.x+14,y,"EMULATION BACKENDS",palette.text); y+=30;\n            const std::vector<std::string> systems={"NES","SNES","Game Boy","Game Boy Color","Game Boy Advance","Nintendo 64","Sega Genesis","Sega Master System","Sega Game Gear","Atari 2600","Atari 5200","Atari 7800","Atari 8-bit","Atari Lynx","PlayStation","PlayStation 2","PlayStation Portable","PlayStation 3","GameCube","Wii","Wii U","Arcade","Nintendo Switch"};\n            for (const std::string& system:systems) {\n                const std::string emulator=installed_game_emulator(system);\n                text(target,gamesListBox.x+14,y,system+": "+(emulator.empty()?"No supported backend installed":basename_only(emulator)+" (automatic)"),emulator.empty()?palette.muted:palette.text);\n                y+=22;\n            }\n            text(target,gamesListBox.x+14,y+8,"MesenCE: Nintendo | RMG: N64 | BlastEm: Sega | Stella 7.0: Atari 2600 | Atari800: 5200/8-bit. 7800/Lynx use compatible installed backends.",palette.muted);\n'''
    new_systems = '''        if (gamesPanel==GamesPanel::Systems) {\n            section_text(target,gamesListBox.x+14,y,"EMULATION SUPPORT",palette.text); y+=30;\n            const auto ready = nougat::games::ready_emulation_support(exe_dir());\n            if (ready.empty()) {\n                text(target,gamesListBox.x+14,y,"No emulator backend is currently ready on this system.",palette.muted);\n                y+=22;\n            } else {\n                for (const auto& backend : ready) {\n                    text(target,gamesListBox.x+14,y,backend.system+": "+backend.backend+" (automatic)",palette.text);\n                    y+=22;\n                    if (y > gamesListBox.y + gamesListBox.h - 44) break;\n                }\n            }\n            text(target,gamesListBox.x+14,std::min(gamesListBox.y+gamesListBox.h-18,y+8),\n                 "Only detected, usable backends are listed. PlayStation 2 appears only when PCSX2 and a user-provided BIOS are both ready.",palette.muted);\n'''
    text = replace_once(text, old_systems, new_systems, "ready-only emulation support")

    # Version identity strings.
    text = replace_once(text,
        'std::string v53SystemStatus = "v0.0.54 File Splitter candidate ready for owner testing.";',
        'std::string v53SystemStatus = "v0.0.55 integrated repair candidate ready for validation.";',
        "system candidate status")
    text = replace_once(text,
        'const std::string versionLabel = "v0.0.54";',
        'const std::string versionLabel = "v0.0.55";',
        "top-bar version")
    text = replace_all_exact(text,
        'Nougat Media Suite v0.0.54',
        'Nougat Media Suite v0.0.55', 2,
        "diagnostic/CLI version")

    path.write_text(text)


def copy_new_files(repo: Path, payload: Path) -> None:
    for relative in [
        "src/games/emulator_registry.hpp", "src/games/emulator_registry.cpp",
        "src/library/user_library_state.hpp", "src/library/user_library_state.cpp",
        "src/player/up_next_title.hpp", "src/player/up_next_title.cpp",
        "tools/test_v55_helpers.cpp",
    ]:
        source = payload / relative
        target = repo / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)


def main() -> int:
    parser = argparse.ArgumentParser(description="Apply Nougat v0.0.55 source integration to exact v0.0.54 source tree.")
    parser.add_argument("repo", type=Path)
    parser.add_argument("--payload", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    repo = args.repo.resolve()
    payload = args.payload.resolve()

    required = [repo / "src/main.cpp", repo / "src/media_server/jellyfin_api_client.cpp",
                repo / "CMakeLists.txt", repo / "NougatMediaSuite.desktop",
                repo / "com.elderredsoftworks.NougatMediaSuite.desktop"]
    missing = [str(p) for p in required if not p.is_file()]
    if missing:
        raise RuntimeError("missing required v54 files: " + ", ".join(missing))

    copy_new_files(repo, payload)
    patch_main(repo / "src/main.cpp")
    patch_jellyfin(repo / "src/media_server/jellyfin_api_client.cpp")
    patch_cmake(repo / "CMakeLists.txt")
    patch_desktop(repo / "NougatMediaSuite.desktop")
    patch_desktop(repo / "com.elderredsoftworks.NougatMediaSuite.desktop")
    print("Nougat v0.0.55 source integration applied.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"STOP: {exc}", file=sys.stderr)
        raise SystemExit(2)
