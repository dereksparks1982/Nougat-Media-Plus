#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import zipfile


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_installer_bundle.py <installer.zip>")
        return 2

    try:
        archive = Path(sys.argv[1]).expanduser().resolve(strict=True)
        with tempfile.TemporaryDirectory(prefix="nougat-v50-bundle-test-") as tmp_value:
            tmp = Path(tmp_value)
            extract_root = tmp / "extracted"
            stage = tmp / "installed"
            extract_root.mkdir()

            with zipfile.ZipFile(archive, "r") as zf:
                bad = zf.testzip()
                need(bad is None, "installer ZIP failed integrity check at " + str(bad))
                roots = {name.split("/", 1)[0] for name in zf.namelist() if "/" in name}
                need(len(roots) == 1, "installer ZIP must contain one top-level directory")
                top_name = next(iter(roots))
                zf.extractall(extract_root)

            bundle = extract_root / top_name
            installer = bundle / "installer" / "nougat_v50_installer.py"
            binary = bundle / "Nougat_Media_Suite_v50"
            need(installer.is_file(), "packaged installer script is missing")
            need(binary.is_file(), "packaged player core is missing")

            result = run([
                sys.executable, str(installer),
                "--source-root", str(bundle),
                "--binary", str(binary),
                "--mode", "custom",
                "--plugins", "workshop",
                "--staging-root", str(stage),
            ])
            need(result.returncode == 0, "packaged Workshop install failed:\n" + result.stdout)

            installed_core = stage / "opt" / "nougat-media-suite" / "Nougat_Media_Suite_v50"
            plugin_root = stage / "user-data" / "nougat" / "plugins" / "workshop"
            installed_worker = plugin_root / "nougat_split_archive.py"
            need(installed_core.is_file(), "packaged installer did not install player core")
            need((plugin_root / "plugin.json").is_file(), "packaged installer did not install Workshop manifest")
            need(installed_worker.is_file(), "packaged installer did not install Workshop worker")

            sample = tmp / "sample.bin"
            sample.write_bytes(b"Nougat Workshop packaged-resource test\n" * 64)
            result = run([sys.executable, str(installed_worker), "inspect", str(sample), "--json"])
            need(result.returncode == 0, "installed Workshop worker could not inspect a file:\n" + result.stdout)
            payload = json.loads(result.stdout)
            need(payload.get("format") == "NOUGAT_SPLIT_ARCHIVE", "installed Workshop worker returned wrong format")
            need(payload.get("operation") == "inspect", "installed Workshop worker returned wrong operation")
            need(payload.get("file_count") == 1, "installed Workshop worker did not inspect the sample file")

            # Re-run the exact packaged installer as player-only. Workshop must
            # be removed while the player core remains.
            result = run([
                sys.executable, str(installer),
                "--source-root", str(bundle),
                "--binary", str(binary),
                "--mode", "custom",
                "--plugins", "",
                "--staging-root", str(stage),
            ])
            need(result.returncode == 0, "packaged player-only reconciliation failed:\n" + result.stdout)
            need(not plugin_root.exists(), "packaged player-only reinstall left Workshop installed")
            need(installed_core.is_file() and os.access(installed_core, os.X_OK),
                 "packaged player-only reinstall damaged the player core")
            state = json.loads(
                (stage / "user-data" / "nougat" / "install-state.json").read_text(encoding="utf-8")
            )
            need(state.get("plugins") == [], "packaged player-only state still lists Workshop")

        print("PASS: actual v0.0.50 installer ZIP installs Workshop from packaged resources")
        print("PASS: installed Workshop worker executes from the managed plugin path")
        print("PASS: actual installer ZIP reconciles back to player-only cleanly")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
