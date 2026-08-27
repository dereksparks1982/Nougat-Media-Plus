#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
INSTALLER = ROOT / "installer" / "nougat_v50_installer.py"
NATIVE_RESOURCES = {
    "workshop": ROOT / "plugins" / "workshop" / "bin" / "nougat-workshop-plugin",
    "games": ROOT / "plugins" / "games" / "bin" / "nougat-games-plugin",
}


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def fake_player(directory: Path) -> Path:
    binary = directory / "Nougat_Media_Suite_v50"
    binary.write_text("#!/bin/sh\necho 'Nougat Media Suite v0.0.50'\n", encoding="utf-8")
    binary.chmod(0o755)
    return binary


def prepare_native_resources() -> dict[str, bool]:
    existed: dict[str, bool] = {}
    for plugin_id, path in NATIVE_RESOURCES.items():
        existed[plugin_id] = path.exists()
        if not existed[plugin_id]:
            path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2("/bin/true", path)
            path.chmod(0o755)
    return existed


def cleanup_native_resources(existed: dict[str, bool]) -> None:
    for plugin_id, path in NATIVE_RESOURCES.items():
        if existed.get(plugin_id):
            continue
        try:
            path.unlink()
        except FileNotFoundError:
            pass
        try:
            path.parent.rmdir()
        except OSError:
            pass


def validate_manifest(plugin_id: str, display_name: str, entrypoint: str) -> dict:
    path = ROOT / "plugins" / plugin_id / "plugin.json"
    need(path.is_file(), display_name + " plugin manifest is missing")
    plugin = json.loads(path.read_text(encoding="utf-8"))
    need(plugin.get("id") == plugin_id, display_name + " plugin ID mismatch")
    need(plugin.get("required_for_application_start") is False, display_name + " must remain optional")
    need(plugin.get("recommended_by_default") is True, display_name + " should be in Default install")
    need(plugin.get("nougat_plugin_api") == 1, display_name + " plugin API mismatch")
    runtime = plugin.get("runtime", {})
    need(runtime.get("kind") == "x11-process", display_name + " must be process-isolated")
    need(runtime.get("entrypoint") == entrypoint, display_name + " entrypoint mismatch")
    return plugin


def main() -> int:
    existed: dict[str, bool] = {}
    try:
        need(INSTALLER.is_file(), "v50 installer is missing")
        validate_manifest("workshop", "Workshop", "bin/nougat-workshop-plugin")
        validate_manifest("games", "Games", "bin/nougat-games-plugin")
        existed = prepare_native_resources()

        with tempfile.TemporaryDirectory(prefix="nougat-v50-plugin-test-") as tmp_value:
            tmp = Path(tmp_value)
            binary = fake_player(tmp)

            player_only = tmp / "player-only"
            result = run([sys.executable, str(INSTALLER), "--source-root", str(ROOT), "--binary", str(binary),
                          "--mode", "custom", "--plugins", "", "--staging-root", str(player_only)])
            need(result.returncode == 0, "player-only staged install failed:\n" + result.stdout)
            core = player_only / "opt" / "nougat-media-suite" / "Nougat_Media_Suite_v50"
            plugin_root = player_only / "user-data" / "nougat" / "plugins"
            need(core.is_file(), "player-only install did not install the core player")
            need(not (plugin_root / "workshop").exists(), "player-only install incorrectly installed Workshop")
            need(not (plugin_root / "games").exists(), "player-only install incorrectly installed Games")
            state = json.loads((player_only / "user-data" / "nougat" / "install-state.json").read_text(encoding="utf-8"))
            need(state.get("plugins") == [], "player-only install state contains optional plugins")

            default_root = tmp / "default"
            result = run([sys.executable, str(INSTALLER), "--source-root", str(ROOT), "--binary", str(binary),
                          "--mode", "default", "--staging-root", str(default_root)])
            need(result.returncode == 0, "default staged install failed:\n" + result.stdout)
            default_core = default_root / "opt" / "nougat-media-suite" / "Nougat_Media_Suite_v50"
            default_plugins = default_root / "user-data" / "nougat" / "plugins"
            workshop = default_plugins / "workshop"
            games = default_plugins / "games"
            state_path = default_root / "user-data" / "nougat" / "install-state.json"
            need(default_core.is_file(), "Default install lost the player core")
            need((workshop / "plugin.json").is_file(), "Default install did not install Workshop")
            need((workshop / "nougat_split_archive.py").is_file(), "Default install lost Workshop worker")
            need(os.access(workshop / "bin" / "nougat-workshop-plugin", os.X_OK), "Workshop native entrypoint is not executable")
            need((games / "plugin.json").is_file(), "Default install did not install Games")
            need(os.access(games / "bin" / "nougat-games-plugin", os.X_OK), "Games native entrypoint is not executable")
            need(os.access(games / "artwork_cache_worker.py", os.X_OK), "Games artwork worker is not executable")
            need((games / "bundled" / "2048.nes").is_file(), "Games 2048 starter ROM is missing")
            need((games / "bundled" / "Waveforms.nes").is_file(), "Games Waveforms diagnostic ROM is missing")
            need((games / "bundled" / "licenses" / "2048-nes-UNLICENSE.txt").is_file(), "2048 license is missing")
            need((games / "bundled" / "licenses" / "NES-Waveforms-MIT.txt").is_file(), "Waveforms license is missing")
            default_state = json.loads(state_path.read_text(encoding="utf-8"))
            need(default_state.get("plugins") == ["games", "workshop"], "Default state must record Games and Workshop")

            result = run([sys.executable, str(INSTALLER), "--source-root", str(ROOT), "--binary", str(binary),
                          "--mode", "custom", "--plugins", "", "--staging-root", str(default_root)])
            need(result.returncode == 0, "Default-to-player-only reconciliation failed:\n" + result.stdout)
            need(not workshop.exists() and not games.exists(), "player-only reconciliation left optional plugins installed")
            need(default_core.is_file() and os.access(default_core, os.X_OK), "player-only reconciliation damaged core")
            need(json.loads(state_path.read_text(encoding="utf-8")).get("plugins") == [], "player-only state remains dirty")

            result = run([sys.executable, str(INSTALLER), "--source-root", str(ROOT), "--binary", str(binary),
                          "--mode", "custom", "--plugins", "games", "--staging-root", str(default_root)])
            need(result.returncode == 0, "Games-only install failed:\n" + result.stdout)
            need(games.is_dir() and not workshop.exists(), "Games-only selection did not remain independent")
            need(json.loads(state_path.read_text(encoding="utf-8")).get("plugins") == ["games"], "Games-only state mismatch")

            result = run([sys.executable, str(INSTALLER), "--source-root", str(ROOT),
                          "--remove-plugin", "games", "--staging-root", str(default_root)])
            need(result.returncode == 0, "Games removal failed:\n" + result.stdout)
            need(not games.exists(), "Games resources remain after removal")
            need(default_core.is_file() and os.access(default_core, os.X_OK), "removing Games damaged player core")
            need(json.loads(state_path.read_text(encoding="utf-8")).get("plugins") == [], "Games removal left stale state")

            result = run([sys.executable, str(INSTALLER), "--source-root", str(ROOT), "--binary", str(binary),
                          "--mode", "custom", "--plugins", "workshop", "--staging-root", str(default_root)])
            need(result.returncode == 0, "Workshop-only install failed:\n" + result.stdout)
            need(workshop.is_dir() and not games.exists(), "Workshop-only selection did not remain independent")
            result = run([sys.executable, str(INSTALLER), "--source-root", str(ROOT),
                          "--remove-plugin", "workshop", "--staging-root", str(default_root)])
            need(result.returncode == 0 and not workshop.exists(), "Workshop independent removal failed:\n" + result.stdout)

        print("PASS: player-only installer installs no optional plugins")
        print("PASS: Default installs independent Workshop and Games process plugins")
        print("PASS: Games carries its legal starter ROMs, artwork worker, licenses and native entrypoint")
        print("PASS: Games and Workshop can each be installed and removed without damaging player core")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1
    finally:
        cleanup_native_resources(existed)


if __name__ == "__main__":
    raise SystemExit(main())
