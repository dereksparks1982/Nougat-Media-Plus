#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse
import hashlib
import json
import os
import stat
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
CORE_BINARY = ROOT / "Nougat_Media_Suite_v50"
GUI_BINARY = ROOT / "Nougat_Installer_v50"
WORKSHOP_BINARY = ROOT / "plugins" / "workshop" / "bin" / "nougat-workshop-plugin"
DEFAULT_OUTPUT = Path.home() / "Downloads" / "Nougat_Media_Suite_v0.0.50_GRAPHICAL_INSTALLER.zip"
TOP = "Nougat_Media_Suite_v0.0.50"

ROOT_FILES = [
    ("Nougat_Installer_v50", 0o755),
    ("LICENSE", 0o644),
    ("COPYRIGHT.md", 0o644),
    ("THIRD_PARTY_NOTICES.md", 0o644),
]

PAYLOAD_FILES = [
    ("Nougat_Media_Suite_v50", 0o755),
    ("NougatMediaSuite.desktop", 0o644),
    ("assets/icons/nougat-media-suite-concept-sheet-v24.png", 0o644),
    ("installer/nougat_v50_installer.py", 0o755),
    ("installer/nougat_v50_installer_backend.py", 0o755),
    ("plugins/workshop/plugin.json", 0o644),
    ("plugins/workshop/bin/nougat-workshop-plugin", 0o755),
    ("components/workshop/nougat_split_archive.py", 0o755),
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def zip_info(name: str, mode: int) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name)
    info.create_system = 3
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = (stat.S_IFREG | mode) << 16
    info.date_time = (2026, 8, 26, 0, 0, 0)
    return info


def add_file(zf: zipfile.ZipFile, source: Path, name: str, mode: int) -> dict[str, object]:
    data = source.read_bytes()
    zf.writestr(zip_info(name, mode), data)
    return {
        "path": name.removeprefix(TOP + "/"),
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "mode": oct(mode),
    }


def member_mode(info: zipfile.ZipInfo) -> int:
    return (info.external_attr >> 16) & 0o777


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def main() -> int:
    parser = argparse.ArgumentParser(description="Package the Nougat Media Suite v0.0.50 graphical installer")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    try:
        required = [rel for rel, _ in ROOT_FILES if not (ROOT / rel).is_file()]
        required += [rel for rel, _ in PAYLOAD_FILES if not (ROOT / rel).is_file()]
        if required:
            raise RuntimeError("graphical installer bundle is missing: " + ", ".join(sorted(set(required))))

        core_version = run([str(CORE_BINARY), "--version"])
        if core_version.returncode != 0 or core_version.stdout.strip() != "Nougat Media Suite v0.0.50":
            raise RuntimeError("player core identity mismatch: " + repr(core_version.stdout.strip()))
        gui_version = run([str(GUI_BINARY), "--version"])
        if gui_version.returncode != 0 or gui_version.stdout.strip() != "Nougat Media Suite Installer v0.0.50":
            raise RuntimeError("graphical installer identity mismatch: " + repr(gui_version.stdout.strip()))
        workshop_version = run([str(WORKSHOP_BINARY), "--version"])
        if workshop_version.returncode != 0 or workshop_version.stdout.strip() != "Nougat Workshop Plugin v0.0.50":
            raise RuntimeError("Workshop native plugin identity mismatch: " + repr(workshop_version.stdout.strip()))

        output = args.output.expanduser().resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(prefix=output.name + ".", suffix=".tmp", dir=output.parent, delete=False) as handle:
            temporary = Path(handle.name)

        try:
            manifest_entries: list[dict[str, object]] = []
            with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
                for rel, mode in ROOT_FILES:
                    manifest_entries.append(add_file(zf, ROOT / rel, f"{TOP}/{rel}", mode))
                for rel, mode in PAYLOAD_FILES:
                    manifest_entries.append(add_file(zf, ROOT / rel, f"{TOP}/payload/{rel}", mode))

                readme = (
                    "Nougat Media Suite v0.0.50\n"
                    "=========================\n\n"
                    "DOUBLE-CLICK: Nougat_Installer_v50\n\n"
                    "A graphical installer window will open. Video Player + Plugin Core is required.\n"
                    "Choose Default, Custom, or Advanced Custom, select any available optional plugins,\n"
                    "click Install, then choose Launch Nougat Media Suite or Close Installer.\n\n"
                    "Workshop is a real optional process plugin. Player-only installation remains valid.\n"
                    "The player executable is deliberately stored under payload/ so it is not confused\n"
                    "with the installer.\n"
                ).encode("utf-8")
                zf.writestr(zip_info(f"{TOP}/README_INSTALL.txt", 0o644), readme)
                manifest_entries.append({
                    "path": "README_INSTALL.txt",
                    "bytes": len(readme),
                    "sha256": hashlib.sha256(readme).hexdigest(),
                    "mode": oct(0o644),
                })

                manifest = {
                    "format": "NOUGAT_INSTALL_PACKAGE_MANIFEST",
                    "format_version": 1,
                    "application_version": "0.0.50",
                    "entrypoint": "Nougat_Installer_v50",
                    "mandatory_core": "payload/Nougat_Media_Suite_v50",
                    "files": sorted(manifest_entries, key=lambda item: str(item["path"])),
                }
                manifest_bytes = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")
                zf.writestr(zip_info(f"{TOP}/PACKAGE_MANIFEST.json", 0o644), manifest_bytes)

            with zipfile.ZipFile(temporary, "r") as zf:
                bad = zf.testzip()
                if bad is not None:
                    raise RuntimeError("graphical installer ZIP integrity failure at " + bad)
                names = set(zf.namelist())
                required_names = {
                    f"{TOP}/Nougat_Installer_v50",
                    f"{TOP}/README_INSTALL.txt",
                    f"{TOP}/PACKAGE_MANIFEST.json",
                    f"{TOP}/payload/Nougat_Media_Suite_v50",
                    f"{TOP}/payload/installer/nougat_v50_installer.py",
                    f"{TOP}/payload/installer/nougat_v50_installer_backend.py",
                    f"{TOP}/payload/assets/icons/nougat-media-suite-concept-sheet-v24.png",
                    f"{TOP}/payload/plugins/workshop/plugin.json",
                    f"{TOP}/payload/plugins/workshop/bin/nougat-workshop-plugin",
                    f"{TOP}/payload/components/workshop/nougat_split_archive.py",
                }
                missing = sorted(required_names - names)
                if missing:
                    raise RuntimeError("graphical installer ZIP missing required entries: " + ", ".join(missing))
                if f"{TOP}/Nougat_Media_Suite_v50" in names:
                    raise RuntimeError("player core must not be exposed at the package top level")
                for name in [
                    f"{TOP}/Nougat_Installer_v50",
                    f"{TOP}/payload/Nougat_Media_Suite_v50",
                    f"{TOP}/payload/installer/nougat_v50_installer.py",
                    f"{TOP}/payload/installer/nougat_v50_installer_backend.py",
                    f"{TOP}/payload/plugins/workshop/bin/nougat-workshop-plugin",
                    f"{TOP}/payload/components/workshop/nougat_split_archive.py",
                ]:
                    info = zf.getinfo(name)
                    if info.create_system != 3 or member_mode(info) & 0o111 == 0:
                        raise RuntimeError("ZIP lost executable permission metadata: " + name)

            with tempfile.TemporaryDirectory(prefix="nougat-v50-gui-package-") as extract_value:
                extract_root = Path(extract_value)
                with zipfile.ZipFile(temporary, "r") as zf:
                    zf.extractall(extract_root)
                package_root = extract_root / TOP
                installer = package_root / "Nougat_Installer_v50"
                installer.chmod(installer.stat().st_mode | 0o755)
                result = run([str(installer), "--self-test"])
                if result.returncode != 0:
                    raise RuntimeError("extracted graphical installer self-test failed:\n" + result.stdout)

            os.replace(temporary, output)
        except Exception:
            try:
                temporary.unlink()
            except OSError:
                pass
            raise

        print("PASS: Nougat Media Suite v0.0.50 graphical installer ZIP verified")
        print("PASS: top-level double-click installer is Nougat_Installer_v50")
        print("PASS: Workshop native plugin runtime is packaged as an optional component")
        print("PASS: player core is hidden under payload/ and cannot be mistaken for the installer")
        print("ZIP:", output)
        print(f"Size: {output.stat().st_size / (1024 * 1024):.1f} MiB")
        print("SHA-256:", sha256(output))
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
