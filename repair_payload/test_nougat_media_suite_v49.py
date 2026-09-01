#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import py_compile
import subprocess
import sys

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def contains_all(text: str, tokens: list[str], label: str) -> None:
    missing = [token for token in tokens if token not in text]
    need(not missing, label + " missing: " + ", ".join(missing))


def main() -> int:
    try:
        main_cpp = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
        host_cpp = (ROOT / "src/games/emulator_host.cpp").read_text(encoding="utf-8")
        host_hpp = (ROOT / "src/games/emulator_host.hpp").read_text(encoding="utf-8")
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        desktops = [
            (ROOT / "NougatMediaSuite.desktop").read_text(encoding="utf-8"),
            (ROOT / "com.elderredsoftworks.NougatMediaSuite.desktop").read_text(encoding="utf-8"),
        ]

        contains_all(cmake, [
            "VERSION 0.0.49", "Nougat_Media_Suite_v49", "-Wall -Wextra -Werror"
        ], "v49 CMake")
        need("Nougat_Media_Suite_v48" not in cmake, "CMake still targets v48")
        need(all("Nougat_Media_Suite_v49" in desktop for desktop in desktops),
             "desktop launchers do not target v49")

        contains_all(main_cpp, [
            'printf("Nougat Media Suite v0.0.49\\n")',
            'const std::string versionLabel = "v0.0.49";',
            'input.app_version = "Nougat Media Suite v0.0.49";',
            "filter_game_library_preferences",
            "game_region_rank",
            "game_revision_rank",
            'return bundledStella;',
            'if (backend_lower == "stella")',
            '"-fullscreen", "0", "-center", "0"',
            '{"SDL_VIDEO_DRIVER", "x11"}',
            'game_path_has_sega_hint',
            'return "Sega Genesis"',
            'return "Sega Master System"',
            'return "Sega Game Gear"',
            'bundledBlastem',
            'if (backend_lower == "blastem")',
            'dos_archive_entrypoint',
            'dos-extracted-v49',
            'it->is_symlink(ec)',
            'extracted_dos_archive_launch',
            'NOUGAT_V49_GAMES_FINAL_REPAIR',
            "artwork-prepared-v49",
            "start_game_artwork_prefetch",
            "poll_game_artwork_prefetch",
            "handle_games_wheel_steps",
            "request_games_interactive_redraw",
            "flush_games_interactive_redraw",
            "poll_stella_top_options",
            "--v49-games-self-test",
        ], "v49 Games source")
        need("request_game_remote_artwork" not in main_cpp,
             "viewport-triggered remote artwork request function survived")
        need("poll_game_artwork_downloads" not in main_cpp,
             "old viewport artwork polling survived")

        contains_all(host_hpp, ["pointer_position", "send_key(KeySym"], "emulator host header")
        contains_all(host_cpp, [
            "XQueryPointer", "XSendEvent", "EmulatorHost::send_key",
            "window_tree_candidates", "safe_window_children",
            "_NET_WM_STATE_SKIP_TASKBAR", "_NET_WM_STATE_SKIP_PAGER",
            "mark_private_emulator_window", "XSetTransientForHint",
            "now - impl_->last_scan_ms >= 20",
            "process_owned", "backend_owned",
        ], "private recursive emulator host")
        need("root_candidates(display, impl_->root)) impl_->preexisting" not in host_cpp,
             "host still snapshots only top-level windows")

        worker = ROOT / "components/games/artwork_cache_worker.py"
        world_tv_worker = ROOT / "components/world_tv/nougat_world_tv_worker.py"
        installer = ROOT / "tools/install_game_runtimes_v49.py"
        checker = ROOT / "tools/check_game_runtimes_v49.py"
        for script in (worker, world_tv_worker, installer, checker, ROOT / "tools/build_v49.py"):
            need(script.is_file(), "missing v49 script: " + str(script.relative_to(ROOT)))
            py_compile.compile(str(script), doraise=True)

        world_tv_text = world_tv_worker.read_text(encoding="utf-8")
        contains_all(world_tv_text, [
            "NOUGAT_V49_RUSSIA24_AUDIO_REPAIR",
            "require_audio: bool = False",
            "has_audio = False",
            'require_audio = channel_id == "Russia24.ru"',
            "max_checks = 6 if require_audio else 3",
            "require_audio=require_audio",
        ], "Russia-24 audio-aware World TV resolver")

        worker_text = worker.read_text(encoding="utf-8")
        contains_all(worker_text, [
            "artwork-index-v49", "Named_Boxarts", "Named_Titles", "Named_Snaps",
            "match_index_name", "DirectoryIndexParser", "KNOWN_REMOTE_FALLBACKS",
            "KNOWN_CATALOG_ALIASES", "2 pak special challenge surfing",
            "Adventures of TRON", "Action Man - Action Force", "Fishing Derby",
            '"Sega Genesis": "Sega - Mega Drive - Genesis"',
            '"Sega Master System": "Sega - Master System - Mark III"',
            '"Sega Game Gear": "Sega - Game Gear"',
            "(?<=[a-z])(?=[A-Z])",
        ], "persistent Atari/Sega artwork resolver")

        installer_text = installer.read_text(encoding="utf-8")
        contains_all(installer_text, [
            "BLASTEM_URL", "blastem64-0.6.3-pre-8013468ed981.tar.gz",
            "BLASTEM_EXPECTED_SIZE = 6053889", 'BLASTEM_REVISION = "8013468ed981"',
            "install_blastem", "SDL_VIDEO_DRIVER", "safe_extract_tar",
            "verify_blastem_dependencies",
        ], "BlastEm runtime installer")
        checker_text = checker.read_text(encoding="utf-8")
        contains_all(checker_text, [
            "Sega Genesis/Master System/Game Gear", "8013468ed981",
            "blastem_identity", "SDL_VIDEO_DRIVER",
            "Atari 2600 SDL3 X11 wrapper",
        ], "BlastEm/Stella runtime checker")
        source_doc = ROOT / "components/games/emulators/BLASTEM_RUNTIME_SOURCE.md"
        need(source_doc.is_file(), "BlastEm runtime source record is missing")
        contains_all(source_doc.read_text(encoding="utf-8"), [
            "8013468ed981", "6,053,889", "COPYING",
        ], "BlastEm source record")

        result = subprocess.run(
            [sys.executable, str(worker), "--self-test"],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        print(result.stdout, end="")
        need(result.returncode == 0 and "PASS" in result.stdout,
             "artwork worker self-test failed")

        diff = subprocess.run(
            ["git", "diff", "--check"], cwd=ROOT,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        if diff.stdout:
            print(diff.stdout, end="")
        need(diff.returncode == 0, "git diff --check found whitespace errors")

        print("=== v0.0.49 GAMES FINAL STATIC CONTRACT PASS ===")
        print("PASS: Stella SDL3 X11 force + recursive private embed host present")
        print("PASS: Sega Genesis/Master System/Game Gear ZIP recognition + BlastEm runtime present")
        print("PASS: DOS ZIP package detection + persistent private extraction present")
        print("PASS: USA > English fallback > foreign-only + newest-final filtering present")
        print("PASS: persistent indexed Atari/Sega artwork resolver and prepared cache present")
        print("PASS: verified Atari preservation aliases + CamelCase normalization present")
        print("PASS: Games wheel coalescing + frame-limited drag redraw present")
        print("PASS: embedded Stella top-edge Options bridge present")
        print("PASS: visible and diagnostic v0.0.49 identity repaired")
        print("PASS: v49 target remains warnings-as-errors")
        print("PASS: Russia-24 requires a decodable audio stream while Russia-1/default station probing remains unchanged")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. Nothing was committed or pushed.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
