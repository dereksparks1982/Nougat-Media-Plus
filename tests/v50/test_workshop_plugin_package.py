#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_workshop_plugin_package.py <Workshop plugin zip>")
        return 2

    try:
        archive = Path(sys.argv[1]).expanduser().resolve(strict=True)
        with tempfile.TemporaryDirectory(prefix="nougat-workshop-plugin-test-") as tmp_value:
            tmp = Path(tmp_value)
            extracted = tmp / "extracted"
            plugin_root = tmp / "plugin-root"
            extracted.mkdir()
            plugin_root.mkdir()

            with zipfile.ZipFile(archive, "r") as zf:
                bad = zf.testzip()
                need(bad is None, "Workshop plugin ZIP failed integrity check at " + str(bad))
                names = zf.namelist()
                roots = {name.split("/", 1)[0] for name in names if "/" in name}
                need(len(roots) == 1, "Workshop plugin ZIP must contain one top-level directory")
                top_name = next(iter(roots))
                need(top_name == "Nougat_Plugin_Workshop_v0.0.50",
                     "Workshop plugin ZIP top-level directory is unexpected: " + top_name)
                need(not any("Nougat_Media_Suite_v50" in name for name in names),
                     "standalone Workshop plugin ZIP incorrectly contains the player core")
                need(not any("Nougat_Installer_v50" in name for name in names),
                     "standalone Workshop plugin ZIP incorrectly contains the suite installer")
                worker_name = f"{top_name}/workshop/nougat_split_archive.py"
                worker_info = zf.getinfo(worker_name)
                worker_mode = (worker_info.external_attr >> 16) & 0o777
                need(worker_info.create_system == 3 and worker_mode & 0o111 != 0,
                     "Workshop plugin worker is not executable in the ZIP")
                zf.extractall(extracted)

            package = extracted / top_name
            package_manifest_path = package / "PACKAGE_MANIFEST.json"
            manifest_path = package / "workshop" / "plugin.json"
            worker_path = package / "workshop" / "nougat_split_archive.py"
            need(package_manifest_path.is_file(), "Workshop package manifest is missing")
            need(manifest_path.is_file(), "Workshop runtime plugin manifest is missing")
            need(worker_path.is_file(), "Workshop split/reassemble worker is missing")

            package_manifest = json.loads(package_manifest_path.read_text(encoding="utf-8"))
            need(package_manifest.get("format") == "NOUGAT_PLUGIN_PACKAGE", "Workshop package format mismatch")
            need(package_manifest.get("format_version") == 1, "Workshop package format version mismatch")
            need(package_manifest.get("plugin_id") == "workshop", "Workshop package plugin ID mismatch")
            need(package_manifest.get("plugin_version") == "0.0.50", "Workshop package version mismatch")
            need(package_manifest.get("install_folder") == "workshop", "Workshop package install folder mismatch")

            expected = {item["path"]: item for item in package_manifest.get("files", []) if isinstance(item, dict) and "path" in item}
            for relative in ("workshop/plugin.json", "workshop/nougat_split_archive.py"):
                need(relative in expected, "Workshop package manifest omits " + relative)
                path = package / relative
                need(path.stat().st_size == int(expected[relative]["bytes"]), relative + " byte count mismatch")
                need(sha256(path) == expected[relative]["sha256"], relative + " SHA-256 mismatch")

            plugin_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            need(plugin_manifest.get("format") == "NOUGAT_PLUGIN", "Workshop runtime manifest format mismatch")
            need(plugin_manifest.get("id") == "workshop", "Workshop runtime manifest ID mismatch")
            need(plugin_manifest.get("version") == "0.0.50", "Workshop runtime manifest version mismatch")
            need(plugin_manifest.get("required_for_application_start") is False,
                 "Workshop runtime manifest incorrectly requires the application core")
            need(plugin_manifest.get("package_kind") == "drop-in-folder",
                 "Workshop runtime manifest does not declare drop-in package semantics")
            resources = plugin_manifest.get("resources", [])
            need(any(isinstance(item, dict) and item.get("source") == "nougat_split_archive.py" and
                     item.get("install_as") == "nougat_split_archive.py" for item in resources),
                 "Workshop package resource path is not self-contained")

            # Simulate the power-user install model: physically place the
            # plugin folder in the plugin root, relaunch/scan, then remove it.
            installed = plugin_root / "workshop"
            shutil.copytree(package / "workshop", installed)
            need((installed / "plugin.json").is_file(), "drop-in Workshop folder did not install")
            installed_worker = installed / "nougat_split_archive.py"
            installed_worker.chmod(installed_worker.stat().st_mode | 0o111)

            sample = tmp / "sample.bin"
            sample.write_bytes(b"Nougat standalone Workshop plugin package test\n" * 64)
            result = run([sys.executable, str(installed_worker), "inspect", str(sample), "--json"])
            need(result.returncode == 0, "standalone Workshop worker could not inspect a file:\n" + result.stdout)
            payload = json.loads(result.stdout)
            need(payload.get("format") == "NOUGAT_SPLIT_ARCHIVE", "Workshop worker returned wrong format")
            need(payload.get("operation") == "inspect", "Workshop worker returned wrong operation")
            need(payload.get("file_count") == 1, "Workshop worker did not inspect the sample file")

            shutil.rmtree(installed)
            need(not installed.exists(), "Workshop drop-in folder could not be removed cleanly")

        print("PASS: standalone Workshop plugin package structure and hashes")
        print("PASS: Workshop plugin drop-in folder installs and removes independently of player core")
        print("PASS: packaged Workshop worker executes successfully")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
