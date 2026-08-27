#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import py_compile
import sys

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def contains_all(text: str, tokens: list[str], label: str) -> None:
    missing = [token for token in tokens if token not in text]
    need(not missing, label + " missing: " + ", ".join(missing))


def validate_process_plugin(plugin: dict, plugin_id: str, display_name: str, entrypoint: str) -> None:
    need(plugin.get("format") == "NOUGAT_PLUGIN", display_name + " plugin format mismatch")
    need(plugin.get("format_version") == 1, display_name + " plugin format version mismatch")
    need(plugin.get("id") == plugin_id, display_name + " plugin ID mismatch")
    need(plugin.get("version") == "0.0.50", display_name + " plugin version mismatch")
    need(plugin.get("nougat_plugin_api") == 1, display_name + " plugin API mismatch")
    need(plugin.get("top_level_tab") == display_name, display_name + " top-level tab mismatch")
    need(plugin.get("required_for_application_start") is False, display_name + " must remain optional")
    runtime = plugin.get("runtime", {})
    need(runtime.get("kind") == "x11-process", display_name + " must use x11-process isolation")
    need(runtime.get("entrypoint") == entrypoint, display_name + " entrypoint mismatch")


def main() -> int:
    try:
        main_cpp = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        media_server = (ROOT / "src/media_server/media_server_manager.cpp").read_text(encoding="utf-8")
        paths_hpp = (ROOT / "src/platform/nougat_paths.hpp").read_text(encoding="utf-8")
        paths_cpp = (ROOT / "src/platform/nougat_paths.cpp").read_text(encoding="utf-8")
        registry_hpp = (ROOT / "src/plugins/plugin_registry.hpp").read_text(encoding="utf-8")
        host_hpp = (ROOT / "src/plugins/plugin_process_host.hpp").read_text(encoding="utf-8")
        manifest = json.loads((ROOT / "config/components/components-v1.json").read_text(encoding="utf-8"))
        workshop_plugin = json.loads((ROOT / "plugins/workshop/plugin.json").read_text(encoding="utf-8"))
        games_plugin = json.loads((ROOT / "plugins/games/plugin.json").read_text(encoding="utf-8"))
        installer_backend = (ROOT / "installer/nougat_v50_installer_backend.py").read_text(encoding="utf-8")

        contains_all(cmake, [
            "VERSION 0.0.50", "Nougat_Media_Suite_v50", "NOUGAT_AI_MODE", "NOUGAT_P2P_MODE",
            "src/platform/nougat_paths.cpp", "src/plugins/plugin_registry.cpp", "src/plugins/plugin_process_host.cpp",
            "add_executable(nougat_workshop_plugin", "plugins/workshop/nougat_workshop_plugin.cpp",
            "src/workshop/split_archive_service.cpp", 'OUTPUT_NAME "nougat-workshop-plugin"',
            "add_executable(nougat_games_plugin", "plugins/games/nougat_games_plugin.cpp",
            "src/games/emulator_host.cpp", 'OUTPUT_NAME "nougat-games-plugin"', "-Wall -Wextra -Werror",
        ], "v50 CMake")
        core_block = cmake.split("add_executable(Nougat_Media_Suite_v50", 1)[1].split(")", 1)[0]
        need("src/workshop/split_archive_service.cpp" not in core_block,
             "Workshop implementation is still linked into mandatory player core")
        need("src/games/emulator_host.cpp" not in core_block,
             "Games EmulatorHost is still linked into mandatory player core")

        contains_all(main_cpp, [
            "NOUGAT_V50_NEUTRAL_PICKERS", "NOUGAT_V50_PLUGIN_FOUNDATION",
            "NOUGAT_V50_WORKSHOP_PROCESS_PLUGIN", "NOUGAT_V50_GAMES_PROCESS_PLUGIN",
            '#include "platform/nougat_paths.hpp"', '#include "plugins/plugin_registry.hpp"',
            '#include "plugins/plugin_process_host.hpp"', 'printf("Nougat Media Suite v0.0.50\\n")',
            'const std::string versionLabel = "v0.0.50";', 'input.app_version = "Nougat Media Suite v0.0.50";',
            'draw_tab(studioTab,"Workshop",ViewMode::Studio);', "workshop_plugin_ready()",
            "workshopPluginHost.start(", "workshopPluginHost.stop();",
            'draw_tab(gamesTab,"Games",ViewMode::Games);', "games_plugin_ready()",
            "gamesPluginHost.start(", "gamesPluginHost.stop();", "draw_games_plugin_screen",
        ], "v50 main source")

        forbidden = [
            '#include "workshop/split_archive_service.hpp"', "struct WorkshopUiState", "workshopState", "workshopWorker",
            "workshopSourcePath", "workshopOutputFolder", "workshopSplitBtn", "start_workshop_split()",
            "start_workshop_reassemble(", "poll_workshop_worker();", "handle_workshop_click(x,y);",
            'nougat::paths::plugin_installed("workshop")',
            '#include "games/emulator_host.hpp"', "GameEntry", "GameUiState", "GamesPanel", "GamesDisplayMode",
            "gameState", "gameHost", "currentMediaIsGame", "activeGameTitle", "activeGameSystem",
            "gamesLibraryBtn", "gamesSystemsBtn", "gamesAddBtn", "gamesControllersBtn", "gamesSettingsBtn",
            "gamesPlayBtn", "gamesRefreshBtn", "gamesGridBtn", "gamesListViewBtn", "gamesListBox",
            "gamesVerticalScroll", "gameArtwork", "gameScanWorker", "stellaTopOptions", "start_game_", "poll_game_",
            "handle_games_", "flush_games_", "CardContextKind::Games", "game_system_for_path",
            "safe_zip_game_entry", "dos_archive_entrypoint", "stop_game_session",
            'draw_tab(studioTab,"Studio",ViewMode::Studio);', 'printf("Nougat Media Suite v0.0.49\\n")',
            'static std::string config_dir() { return home_dir() + "/.config/reddmedia"; }',
            'exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"',
            '/home/dereksparks1982/Downloads',
        ]
        present = [token for token in forbidden if token in main_cpp]
        need(not present, "v50 main source retains hardwired/legacy seams: " + ", ".join(present))

        contains_all(paths_hpp, [
            "std::filesystem::path config;", "std::filesystem::path data;", "std::filesystem::path cache;",
            "std::filesystem::path state;", "std::filesystem::path plugins;", "component_runtime",
            "plugin_root", "plugin_manifest", "plugin_resource", "plugin_installed", "legacy_config_roots",
        ], "v50 XDG/plugin path API")
        contains_all(paths_cpp, [
            'env_path("NOUGAT_PLUGIN_ROOT", out.data / "plugins")', 'return root / "plugin.json";', "safe_relative_resource",
        ], "v50 plugin path implementation")
        contains_all(registry_hpp, ["PluginManifest", "PluginScanResult", "scan_installed_plugins", "runtime_kind", "entrypoint"],
                     "strict plugin registry API")
        contains_all(host_hpp, ["PluginProcessHost", "parent_xid", "stop()", "poll("], "out-of-process plugin host API")
        contains_all(media_server, ['component_runtime("jellyfin")', "paths.server_data", "paths.server_config",
                                    "paths.server_cache", "paths.server_logs", "return false;"], "optional Jellyfin manager")

        need(isinstance(manifest.get("components"), list) and manifest["components"], "component manifest has no components")
        profiles = manifest.get("profiles", {})
        need(profiles.get("minimal") == ["core-player"], "minimal install must contain only core-player")
        need(manifest.get("policy", {}).get("minimum_profile") == "minimal", "minimum profile must be minimal")
        ids = {item.get("id") for item in manifest["components"] if isinstance(item, dict)}
        required_ids = {
            "core-player", "media-server-jellyfin", "local-ai-llama", "local-ai-embedding-model",
            "games-mesen2", "games-stella", "games-blastem", "games-dosbox-staging", "games-rmg",
            "games-atari800", "games-xenia", "live-tv", "security-analysis",
        }
        need(not (required_ids - ids), "component manifest is missing IDs: " + ", ".join(sorted(required_ids - ids)))
        core = next(item for item in manifest["components"] if item.get("id") == "core-player")
        need(core.get("scope") == "video-player-only", "core-player scope must be video-player-only")
        need(core.get("required_for_application_start") is True, "core-player must be required")
        for item in manifest["components"]:
            if item.get("id") != "core-player":
                need(item.get("required_for_application_start") is not True,
                     f"optional component {item.get('id')} incorrectly required for application start")

        validate_process_plugin(workshop_plugin, "workshop", "Workshop", "bin/nougat-workshop-plugin")
        workshop_resources = workshop_plugin.get("resources", [])
        need(any(isinstance(item, dict) and item.get("install_as") == "nougat_split_archive.py" for item in workshop_resources),
             "Workshop split worker is missing")
        need(any(isinstance(item, dict) and item.get("install_as") == "bin/nougat-workshop-plugin" and item.get("kind") == "executable"
                 for item in workshop_resources), "Workshop native executable is missing")

        validate_process_plugin(games_plugin, "games", "Games", "bin/nougat-games-plugin")
        games_resources = games_plugin.get("resources", [])
        needed_games = {
            "bin/nougat-games-plugin", "artwork_cache_worker.py", "bundled/2048.nes", "bundled/Waveforms.nes",
            "bundled/artwork/2048.png", "bundled/artwork/Waveforms.png", "bundled/BUNDLED_GAMES_SOURCES.md",
            "bundled/licenses/2048-nes-UNLICENSE.txt", "bundled/licenses/NES-Waveforms-MIT.txt",
        }
        installed_games = {str(item.get("install_as")) for item in games_resources if isinstance(item, dict)}
        need(not (needed_games - installed_games), "Games plugin package omits: " + ", ".join(sorted(needed_games - installed_games)))

        contains_all(installer_backend, [
            'APP_PREFIX = Path("/opt/nougat-media-suite")', "user_plugin_root()", "install_plugin(", "remove_plugin(",
            'resource.get("kind") in {"python-worker", "executable"}', '"plugins": plugins',
        ], "v50 installer backend")

        for script in [
            ROOT / "components/workshop/nougat_split_archive.py", ROOT / "components/games/artwork_cache_worker.py",
            ROOT / "tools/apply_v50_core.py", ROOT / "tools/apply_v50_dialog_labels.py",
            ROOT / "tools/apply_v50_plugin_foundation.py", ROOT / "tools/apply_v50_workshop_process_plugin.py",
            ROOT / "tools/apply_v50_games_process_plugin.py", ROOT / "tools/apply_v50_candidate.py",
            ROOT / "installer/nougat_v50_installer.py", ROOT / "installer/nougat_v50_installer_backend.py",
            ROOT / "tests/v50/test_plugin_installer.py", ROOT / "tests/v50/test_workshop_plugin_package.py",
        ]:
            py_compile.compile(str(script), doraise=True)

        print("PASS: Nougat Media Suite v0.0.50 source contract")
        print("PASS: minimal installation contract is Video Player + Plugin Core only")
        print("PASS: Workshop Plugin #1 is physically outside player core")
        print("PASS: Games Plugin #2 owns emulator hosting, ROM/DOS library logic and bundled legal starter games")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
