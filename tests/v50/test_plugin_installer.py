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
WORKSHOP_NATIVE_SOURCE = ROOT / "plugins" / "workshop" / "bin" / "nougat-workshop-plugin"


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


def prepare_native_resource() -> bool:
    existed = WORKSHOP_NATIVE_SOURCE.exists()
    if not existed:
        WORKSHOP_NATIVE_SOURCE.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2("/bin/true", WORKSHOP_NATIVE_SOURCE)
        WORKSHOP_NATIVE_SOURCE.chmod(0o755)
    return existed


def cleanup_native_resource(existed: bool) -> None:
    if existed:
        return
    try:
        WORKSHOP_NATIVE_SOURCE.unlink()
    except FileNotFoundError:
        pass
    try:
        WORKSHOP_NATIVE_SOURCE.parent.rmdir()
    except OSError:
        pass


def main() -> int:
    resource_existed = False
    try:
        need(INSTALLER.is_file(), "v50 installer is missing")
        plugin_manifest = ROOT / "plugins" / "workshop" / "plugin.json"
        need(plugin_manifest.is_file(), "Workshop plugin manifest is missing")
        plugin = json.loads(plugin_manifest.read_text(encoding="utf-8"))
        need(plugin.get("id") == "workshop", "Workshop plugin ID mismatch")
        need(plugin.get("required_for_application_start") is False,
             "Workshop must not be required for application start")
        need(plugin.get("recommended_by_default") is True,
             "Workshop should be part of the Default plugin set")
        need(plugin.get("nougat_plugin_api") == 1, "Workshop plugin API mismatch")
        runtime = plugin.get("runtime", {})
        need(runtime.get("kind") == "x11-process", "Workshop must be a process-isolated plugin")
        need(runtime.get("entrypoint") == "bin/nougat-workshop-plugin", "Workshop entrypoint mismatch")

        resource_existed = prepare_native_resource()

        with tempfile.TemporaryDirectory(prefix="nougat-v50-plugin-test-") as tmp_value:
            tmp = Path(tmp_value)
            binary = fake_player(tmp)

            player_only = tmp / "player-only"
            result = run([
                sys.executable, str(INSTALLER),
                "--source-root", str(ROOT),
                "--binary", str(binary),
                "--mode", "custom",
                "--plugins", "",
                "--staging-root", str(player_only),
            ])
            need(result.returncode == 0, "player-only staged install failed:\n" + result.stdout)
            core = player_only / "opt" / "nougat-media-suite" / "Nougat_Media_Suite_v50"
            workshop = player_only / "user-data" / "nougat" / "plugins" / "workshop"
            need(core.is_file(), "player-only install did not install the core player")
            need(not workshop.exists(), "player-only install incorrectly installed Workshop")
            state = json.loads((player_only / "user-data" / "nougat" / "install-state.json").read_text(encoding="utf-8"))
            need(state.get("plugins") == [], "player-only install state contains optional plugins")

            default_root = tmp / "default"
            result = run([
                sys.executable, str(INSTALLER),
                "--source-root", str(ROOT),
                "--binary", str(binary),
                "--mode", "default",
                "--staging-root", str(default_root),
            ])
            need(result.returncode == 0, "default staged install failed:\n" + result.stdout)
            default_core = default_root / "opt" / "nougat-media-suite" / "Nougat_Media_Suite_v50"
            default_workshop = default_root / "user-data" / "nougat" / "plugins" / "workshop"
            worker = default_workshop / "nougat_split_archive.py"
            native = default_workshop / "bin" / "nougat-workshop-plugin"
            state_path = default_root / "user-data" / "nougat" / "install-state.json"
            need(default_core.is_file(), "Default install lost the player core")
            need((default_workshop / "plugin.json").is_file(), "Default install did not install Workshop manifest")
            need(worker.is_file(), "Default install did not install Workshop split worker")
            need(native.is_file() and os.access(native, os.X_OK),
                 "Default install did not install executable Workshop native entrypoint")
            worker_source = (ROOT / "components" / "workshop" / "nougat_split_archive.py").read_bytes()
            need(worker.read_bytes() == worker_source, "installed Workshop worker differs from source resource")
            default_state = json.loads(state_path.read_text(encoding="utf-8"))
            need(default_state.get("plugins") == ["workshop"], "Default state does not record Workshop")

            result = run([
                sys.executable, str(INSTALLER),
                "--source-root", str(ROOT),
                "--binary", str(binary),
                "--mode", "custom",
                "--plugins", "",
                "--staging-root", str(default_root),
            ])
            need(result.returncode == 0, "Default-to-player-only reconciliation failed:\n" + result.stdout)
            need(not default_workshop.exists(), "Custom player-only reinstall left Workshop installed")
            need(default_core.is_file() and os.access(default_core, os.X_OK),
                 "player-only reconciliation damaged the player core")
            reconciled_state = json.loads(state_path.read_text(encoding="utf-8"))
            need(reconciled_state.get("plugins") == [],
                 "player-only reconciliation state still lists optional plugins")

            result = run([
                sys.executable, str(INSTALLER),
                "--source-root", str(ROOT),
                "--binary", str(binary),
                "--mode", "custom",
                "--plugins", "workshop",
                "--staging-root", str(default_root),
            ])
            need(result.returncode == 0, "Workshop reinstall failed:\n" + result.stdout)
            need(default_workshop.is_dir(), "Workshop reinstall did not restore plugin resources")
            need((default_workshop / "bin" / "nougat-workshop-plugin").is_file(),
                 "Workshop reinstall did not restore native entrypoint")

            result = run([
                sys.executable, str(INSTALLER),
                "--source-root", str(ROOT),
                "--remove-plugin", "workshop",
                "--staging-root", str(default_root),
            ])
            need(result.returncode == 0, "Workshop removal failed:\n" + result.stdout)
            need(not default_workshop.exists(), "Workshop resources remain after removal")
            need(default_core.is_file() and os.access(default_core, os.X_OK),
                 "removing Workshop damaged the player core")
            removed_state = json.loads(state_path.read_text(encoding="utf-8"))
            need(removed_state.get("plugins") == [],
                 "explicit Workshop removal left stale plugin state")

        print("PASS: v0.0.50 player-only installer installs no optional plugins")
        print("PASS: Default installer installs Workshop manifest, worker, and native process entrypoint")
        print("PASS: Custom empty selection reconciles an existing install to player-only")
        print("PASS: Workshop removal leaves the player core intact and updates install state")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1
    finally:
        cleanup_native_resource(resource_existed)


if __name__ == "__main__":
    raise SystemExit(main())
