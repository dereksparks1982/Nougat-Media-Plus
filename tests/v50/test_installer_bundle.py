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

APPROVED_ICON_SHA256 = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def backend_install(wrapper: Path, payload: Path, stage: Path, plugins: str) -> subprocess.CompletedProcess[str]:
    return run([
        sys.executable,
        str(wrapper),
        "--source-root", str(payload),
        "--mode", "custom",
        "--plugins", plugins,
        "--staging-root", str(stage),
    ])


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_installer_bundle.py <graphical-installer.zip>")
        return 2

    try:
        archive = Path(sys.argv[1]).expanduser().resolve(strict=True)
        with tempfile.TemporaryDirectory(prefix="nougat-v50-gui-bundle-test-") as tmp_value:
            tmp = Path(tmp_value)
            extract_root = tmp / "extracted"
            extract_root.mkdir()

            with zipfile.ZipFile(archive, "r") as zf:
                bad = zf.testzip()
                need(bad is None, "graphical installer ZIP failed integrity check at " + str(bad))
                roots = {name.split("/", 1)[0] for name in zf.namelist() if "/" in name}
                need(len(roots) == 1, "graphical installer ZIP must contain exactly one top-level directory")
                top_name = next(iter(roots))
                zf.extractall(extract_root)

            bundle = extract_root / top_name
            gui = bundle / "Nougat_Installer_v50"
            payload = bundle / "payload"
            binary = payload / "Nougat_Media_Suite_v50"
            wrapper = payload / "installer" / "nougat_v50_installer.py"
            backend = payload / "installer" / "nougat_v50_installer_backend.py"
            icon = payload / "assets" / "icons" / "nougat-media-suite-concept-sheet-v24.png"
            manifest_path = bundle / "PACKAGE_MANIFEST.json"
            workshop_manifest = payload / "plugins" / "workshop" / "plugin.json"
            workshop_worker = payload / "components" / "workshop" / "nougat_split_archive.py"

            need(gui.is_file(), "top-level graphical installer executable is missing")
            need(not (bundle / "Nougat_Media_Suite_v50").exists(),
                 "player core is exposed at the package top level and can be mistaken for the installer")
            need(binary.is_file(), "packaged player core is missing under payload/")
            need(wrapper.is_file() and backend.is_file(), "packaged transactional installer backend is incomplete")
            need(icon.is_file(), "approved Nougat N icon payload is missing")
            need(workshop_manifest.is_file() and workshop_worker.is_file(), "Workshop plugin payload is incomplete")
            need(manifest_path.is_file(), "package manifest is missing")

            # Python's zip extraction may discard mode bits, so repair only the
            # extracted test copy before exercising the exact bundled program.
            gui.chmod(gui.stat().st_mode | 0o755)
            binary.chmod(binary.stat().st_mode | 0o755)
            wrapper.chmod(wrapper.stat().st_mode | 0o755)
            backend.chmod(backend.stat().st_mode | 0o755)
            workshop_worker.chmod(workshop_worker.stat().st_mode | 0o755)

            result = run([str(gui), "--version"])
            need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite Installer v0.0.50",
                 "graphical installer executable has wrong identity:\n" + result.stdout)
            result = run([str(gui), "--self-test"])
            need(result.returncode == 0, "graphical installer package self-test failed:\n" + result.stdout)

            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            need(manifest.get("format") == "NOUGAT_INSTALL_PACKAGE_MANIFEST", "wrong package manifest format")
            need(manifest.get("application_version") == "0.0.50", "wrong package manifest version")
            need(manifest.get("entrypoint") == "Nougat_Installer_v50", "package manifest does not name the GUI installer entry point")
            need(manifest.get("mandatory_core") == "payload/Nougat_Media_Suite_v50", "package manifest does not identify the mandatory player/plugin core")
            need(sha256(icon) == APPROVED_ICON_SHA256, "packaged Nougat icon is not the approved N master")

            # Exercise the exact packaged transactional backend with Workshop.
            stage = tmp / "installed"
            result = backend_install(wrapper, payload, stage, "workshop")
            need(result.returncode == 0, "packaged Workshop installation failed:\n" + result.stdout)
            installed_core = stage / "opt" / "nougat-media-suite" / "Nougat_Media_Suite_v50"
            plugin_root = stage / "user-data" / "nougat" / "plugins" / "workshop"
            installed_worker = plugin_root / "nougat_split_archive.py"
            installed_icon = stage / "user-data" / "icons" / "hicolor" / "256x256" / "apps" / "nougat-media-suite-concept-sheet-v24.png"
            need(installed_core.is_file() and os.access(installed_core, os.X_OK), "backend did not install executable player core")
            need((plugin_root / "plugin.json").is_file(), "backend did not install Workshop manifest")
            need(installed_worker.is_file(), "backend did not install Workshop worker")
            need(installed_icon.is_file() and sha256(installed_icon) == APPROVED_ICON_SHA256,
                 "staged installation did not preserve the approved Nougat N icon")

            sample = tmp / "sample.bin"
            sample.write_bytes(b"Nougat graphical installer packaged-resource test\n" * 64)
            result = run([sys.executable, str(installed_worker), "inspect", str(sample), "--json"])
            need(result.returncode == 0, "installed Workshop worker could not inspect a file:\n" + result.stdout)
            inspected = json.loads(result.stdout)
            need(inspected.get("format") == "NOUGAT_SPLIT_ARCHIVE", "installed Workshop worker returned wrong format")
            need(inspected.get("operation") == "inspect", "installed Workshop worker returned wrong operation")
            need(inspected.get("file_count") == 1, "installed Workshop worker did not inspect the sample file")

            # Force a post-core-install failure and prove the transaction restores
            # the previous working core/plugin application state byte-for-byte.
            before_core = sha256(installed_core)
            before_manifest = sha256(plugin_root / "plugin.json")
            before_worker = sha256(installed_worker)
            disabled_worker = workshop_worker.with_name("nougat_split_archive.py.disabled")
            workshop_worker.rename(disabled_worker)
            try:
                failed = backend_install(wrapper, payload, stage, "workshop")
                need(failed.returncode != 0, "forced missing-plugin-resource install unexpectedly succeeded")
                need("PASS: pre-install application files restored" in failed.stdout,
                     "forced failure did not report successful rollback:\n" + failed.stdout)
            finally:
                disabled_worker.rename(workshop_worker)
                workshop_worker.chmod(workshop_worker.stat().st_mode | 0o755)

            need(sha256(installed_core) == before_core, "rollback did not restore the prior player core exactly")
            need(sha256(plugin_root / "plugin.json") == before_manifest, "rollback did not restore the prior Workshop manifest exactly")
            need(sha256(installed_worker) == before_worker, "rollback did not restore the prior Workshop worker exactly")

            # Reconcile the same exact package to player-only.
            result = backend_install(wrapper, payload, stage, "")
            need(result.returncode == 0, "packaged player-only reconciliation failed:\n" + result.stdout)
            need(not plugin_root.exists(), "player-only reinstall left Workshop installed")
            need(installed_core.is_file() and os.access(installed_core, os.X_OK),
                 "player-only reinstall damaged the player core")
            state = json.loads((stage / "user-data" / "nougat" / "install-state.json").read_text(encoding="utf-8"))
            need(state.get("plugins") == [], "player-only install state still lists Workshop")

            # Finally exercise the actual double-click GUI executable under a
            # virtual X display. Default mode must drive the backend and install
            # the currently recommended real plugin set (Workshop).
            gui_stage = tmp / "gui-installed"
            xvfb = shutil.which("xvfb-run")
            need(xvfb is not None, "xvfb-run is required for graphical installer lifecycle validation")
            result = run([
                xvfb, "-a", str(gui),
                "--automated-install",
                "--staging-root", str(gui_stage),
            ])
            need(result.returncode == 0, "graphical installer could not drive installation end-to-end:\n" + result.stdout)
            need("PASS: graphical installer drove the packaged backend to completion" in result.stdout,
                 "graphical installer did not reach its successful completion page:\n" + result.stdout)
            need((gui_stage / "opt" / "nougat-media-suite" / "Nougat_Media_Suite_v50").is_file(),
                 "graphical installer did not install the player core")
            need((gui_stage / "user-data" / "nougat" / "plugins" / "workshop" / "plugin.json").is_file(),
                 "graphical installer Default mode did not install recommended Workshop plugin")

            result = run([xvfb, "-a", str(gui), "--window-self-test"])
            need(result.returncode == 0, "graphical installer X11 identity/icon self-test failed:\n" + result.stdout)

        print("PASS: package exposes one unmistakable graphical installer and hides the player under payload/")
        print("PASS: graphical installer and backend install the player + Workshop package")
        print("PASS: forced install failure rolls back the previous application state exactly")
        print("PASS: player-only reconciliation removes Workshop without damaging core")
        print("PASS: exact graphical installer drives Default install end-to-end under X11")
        print("PASS: graphical installer publishes the embedded approved Nougat N icon")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
