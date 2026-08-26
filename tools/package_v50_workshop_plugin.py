#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse
import hashlib
import json
import os
import stat
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PLUGIN_ID = "workshop"
PLUGIN_VERSION = "0.0.50"
TOP = f"Nougat_Plugin_Workshop_v{PLUGIN_VERSION}"
DEFAULT_OUTPUT = Path.home() / "Downloads" / f"Nougat_Plugin_Workshop_v{PLUGIN_VERSION}.zip"
SOURCE_MANIFEST = ROOT / "plugins" / PLUGIN_ID / "plugin.json"
WORKER = ROOT / "components" / "workshop" / "nougat_split_archive.py"
LEGAL_FILES = ["LICENSE", "COPYRIGHT.md", "THIRD_PARTY_NOTICES.md"]


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
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


def add_bytes(zf: zipfile.ZipFile, name: str, data: bytes, mode: int) -> None:
    zf.writestr(zip_info(name, mode), data)


def add_file(zf: zipfile.ZipFile, source: Path, name: str, mode: int) -> bytes:
    data = source.read_bytes()
    add_bytes(zf, name, data, mode)
    return data


def distribution_manifest() -> bytes:
    source = json.loads(SOURCE_MANIFEST.read_text(encoding="utf-8"))
    if source.get("format") != "NOUGAT_PLUGIN" or source.get("format_version") != 1:
        raise RuntimeError("Workshop source manifest format mismatch")
    if source.get("id") != PLUGIN_ID or source.get("version") != PLUGIN_VERSION:
        raise RuntimeError("Workshop source manifest identity mismatch")

    # The source-tree manifest points at build-time resource locations. A
    # standalone plugin package must be self-contained, so resource sources
    # become paths relative to the drop-in workshop folder itself.
    resources = []
    for item in source.get("resources", []):
        if not isinstance(item, dict):
            raise RuntimeError("Workshop source manifest contains an invalid resource record")
        copy = dict(item)
        install_as = str(copy.get("install_as", "")).strip()
        if not install_as or Path(install_as).is_absolute() or ".." in Path(install_as).parts:
            raise RuntimeError("Workshop source manifest contains an unsafe install path")
        copy["source"] = install_as
        resources.append(copy)
    source["resources"] = resources
    source["package_kind"] = "drop-in-folder"
    source["compatible_core_version"] = PLUGIN_VERSION
    return (json.dumps(source, indent=2, sort_keys=True) + "\n").encode("utf-8")


def readme_text() -> bytes:
    text = f"""Nougat Media Suite - Workshop Plugin v{PLUGIN_VERSION}

This ZIP contains one optional Nougat plugin: Workshop.

INSTALL
1. Extract this ZIP.
2. Copy the folder named 'workshop' into:
   ~/.local/share/nougat/plugins/
3. Relaunch Nougat Media Suite.

REMOVE
1. Close Nougat Media Suite.
2. Remove ~/.local/share/nougat/plugins/workshop/
3. Relaunch Nougat Media Suite.

The Video Player core is not included in this package and is not modified by
adding or removing this plugin.

Workshop v{PLUGIN_VERSION} currently provides the Nougat Split/Reassemble file tool.
"""
    return text.encode("utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Package standalone Nougat Workshop v0.0.50 plugin")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    try:
        required = [SOURCE_MANIFEST, WORKER] + [ROOT / name for name in LEGAL_FILES]
        missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
        if missing:
            raise RuntimeError("Workshop plugin package is missing: " + ", ".join(missing))

        plugin_manifest = distribution_manifest()
        worker = WORKER.read_bytes()
        readme = readme_text()

        package_files = [
            {
                "path": f"{PLUGIN_ID}/plugin.json",
                "bytes": len(plugin_manifest),
                "sha256": sha256_bytes(plugin_manifest),
                "mode": "0644",
            },
            {
                "path": f"{PLUGIN_ID}/nougat_split_archive.py",
                "bytes": len(worker),
                "sha256": sha256_bytes(worker),
                "mode": "0755",
            },
        ]
        package_manifest = {
            "format": "NOUGAT_PLUGIN_PACKAGE",
            "format_version": 1,
            "plugin_id": PLUGIN_ID,
            "display_name": "Workshop",
            "plugin_version": PLUGIN_VERSION,
            "compatible_core_version": PLUGIN_VERSION,
            "install_folder": PLUGIN_ID,
            "files": package_files,
        }
        package_manifest_bytes = (json.dumps(package_manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")

        output = args.output.expanduser().resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(prefix=output.name + ".", suffix=".tmp", dir=output.parent, delete=False) as handle:
            temp = Path(handle.name)

        try:
            with zipfile.ZipFile(temp, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
                add_bytes(zf, f"{TOP}/{PLUGIN_ID}/plugin.json", plugin_manifest, 0o644)
                add_bytes(zf, f"{TOP}/{PLUGIN_ID}/nougat_split_archive.py", worker, 0o755)
                add_bytes(zf, f"{TOP}/PACKAGE_MANIFEST.json", package_manifest_bytes, 0o644)
                add_bytes(zf, f"{TOP}/README_PLUGIN.txt", readme, 0o644)
                for name in LEGAL_FILES:
                    add_file(zf, ROOT / name, f"{TOP}/{name}", 0o644)

            with zipfile.ZipFile(temp, "r") as zf:
                bad = zf.testzip()
                if bad is not None:
                    raise RuntimeError("Workshop plugin ZIP integrity failure at " + bad)
                names = set(zf.namelist())
                required_names = {
                    f"{TOP}/{PLUGIN_ID}/plugin.json",
                    f"{TOP}/{PLUGIN_ID}/nougat_split_archive.py",
                    f"{TOP}/PACKAGE_MANIFEST.json",
                    f"{TOP}/README_PLUGIN.txt",
                    f"{TOP}/LICENSE",
                    f"{TOP}/COPYRIGHT.md",
                    f"{TOP}/THIRD_PARTY_NOTICES.md",
                }
                missing_names = sorted(required_names - names)
                if missing_names:
                    raise RuntimeError("Workshop plugin ZIP missing required entries: " + ", ".join(missing_names))
                worker_info = zf.getinfo(f"{TOP}/{PLUGIN_ID}/nougat_split_archive.py")
                worker_mode = (worker_info.external_attr >> 16) & 0o777
                if worker_info.create_system != 3 or worker_mode & 0o111 == 0:
                    raise RuntimeError("Workshop plugin worker lost executable permission metadata")

            os.replace(temp, output)
        except Exception:
            try:
                temp.unlink()
            except OSError:
                pass
            raise

        print("PASS: standalone Workshop plugin ZIP verified")
        print("Plugin: Workshop v0.0.50")
        print("ZIP:", output)
        print(f"Size: {output.stat().st_size / 1024:.1f} KiB")
        print("SHA-256:", sha256_file(output))
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
