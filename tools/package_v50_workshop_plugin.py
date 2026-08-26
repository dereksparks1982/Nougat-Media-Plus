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
PLUGIN_ID = "workshop"
PLUGIN_VERSION = "0.0.50"
TOP = f"Nougat_Plugin_Workshop_v{PLUGIN_VERSION}"
DEFAULT_OUTPUT = Path.home() / "Downloads" / f"Nougat_Plugin_Workshop_v{PLUGIN_VERSION}.zip"
SOURCE_MANIFEST = ROOT / "plugins" / PLUGIN_ID / "plugin.json"
WORKER = ROOT / "components" / "workshop" / "nougat_split_archive.py"
DEFAULT_PLUGIN_BINARY = ROOT / "plugins" / PLUGIN_ID / "bin" / "nougat-workshop-plugin"
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
    if source.get("nougat_plugin_api") != 1:
        raise RuntimeError("Workshop source manifest plugin API mismatch")
    runtime = source.get("runtime", {})
    if runtime.get("kind") != "x11-process" or runtime.get("entrypoint") != "bin/nougat-workshop-plugin":
        raise RuntimeError("Workshop source manifest runtime boundary mismatch")

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
    return f"""Nougat Media Suite - Workshop Plugin v{PLUGIN_VERSION}

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

The Video Player core is not included and is not modified by adding or removing
this plugin. Workshop runs in its own Nougat-hosted X11/XWayland process.

Workshop v{PLUGIN_VERSION} provides the Nougat Split/Reassemble file tool.
""".encode("utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Package standalone Nougat Workshop v0.0.50 plugin")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--plugin-binary", type=Path, default=DEFAULT_PLUGIN_BINARY)
    args = parser.parse_args()

    try:
        plugin_binary = args.plugin_binary.expanduser().resolve()
        required = [SOURCE_MANIFEST, WORKER, plugin_binary] + [ROOT / name for name in LEGAL_FILES]
        missing = [str(path) for path in required if not path.is_file()]
        if missing:
            raise RuntimeError("Workshop plugin package is missing: " + ", ".join(missing))
        if not os.access(plugin_binary, os.X_OK):
            raise RuntimeError("Workshop native plugin entrypoint is not executable: " + str(plugin_binary))
        version = subprocess.run([str(plugin_binary), "--version"], text=True, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT)
        if version.returncode != 0 or version.stdout.strip() != "Nougat Workshop Plugin v0.0.50":
            raise RuntimeError("Workshop native plugin identity mismatch: " + repr(version.stdout.strip()))

        plugin_manifest = distribution_manifest()
        worker = WORKER.read_bytes()
        native = plugin_binary.read_bytes()
        readme = readme_text()

        package_files = [
            {"path": f"{PLUGIN_ID}/plugin.json", "bytes": len(plugin_manifest),
             "sha256": sha256_bytes(plugin_manifest), "mode": "0644"},
            {"path": f"{PLUGIN_ID}/nougat_split_archive.py", "bytes": len(worker),
             "sha256": sha256_bytes(worker), "mode": "0755"},
            {"path": f"{PLUGIN_ID}/bin/nougat-workshop-plugin", "bytes": len(native),
             "sha256": sha256_bytes(native), "mode": "0755"},
        ]
        package_manifest = {
            "format": "NOUGAT_PLUGIN_PACKAGE",
            "format_version": 1,
            "plugin_id": PLUGIN_ID,
            "display_name": "Workshop",
            "plugin_version": PLUGIN_VERSION,
            "compatible_core_version": PLUGIN_VERSION,
            "runtime_kind": "x11-process",
            "entrypoint": "workshop/bin/nougat-workshop-plugin",
            "install_folder": PLUGIN_ID,
            "files": package_files,
        }
        package_manifest_bytes = (json.dumps(package_manifest, indent=2, sort_keys=True) + "\n").encode("utf-8")

        output = args.output.expanduser().resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(prefix=output.name + ".", suffix=".tmp", dir=output.parent, delete=False) as handle:
            temporary = Path(handle.name)

        try:
            with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
                add_bytes(zf, f"{TOP}/{PLUGIN_ID}/plugin.json", plugin_manifest, 0o644)
                add_bytes(zf, f"{TOP}/{PLUGIN_ID}/nougat_split_archive.py", worker, 0o755)
                add_bytes(zf, f"{TOP}/{PLUGIN_ID}/bin/nougat-workshop-plugin", native, 0o755)
                add_bytes(zf, f"{TOP}/PACKAGE_MANIFEST.json", package_manifest_bytes, 0o644)
                add_bytes(zf, f"{TOP}/README_PLUGIN.txt", readme, 0o644)
                for name in LEGAL_FILES:
                    add_file(zf, ROOT / name, f"{TOP}/{name}", 0o644)

            with zipfile.ZipFile(temporary, "r") as zf:
                bad = zf.testzip()
                if bad is not None:
                    raise RuntimeError("Workshop plugin ZIP integrity failure at " + bad)
                names = set(zf.namelist())
                required_names = {
                    f"{TOP}/{PLUGIN_ID}/plugin.json",
                    f"{TOP}/{PLUGIN_ID}/nougat_split_archive.py",
                    f"{TOP}/{PLUGIN_ID}/bin/nougat-workshop-plugin",
                    f"{TOP}/PACKAGE_MANIFEST.json",
                    f"{TOP}/README_PLUGIN.txt",
                    f"{TOP}/LICENSE",
                    f"{TOP}/COPYRIGHT.md",
                    f"{TOP}/THIRD_PARTY_NOTICES.md",
                }
                missing_names = sorted(required_names - names)
                if missing_names:
                    raise RuntimeError("Workshop plugin ZIP missing required entries: " + ", ".join(missing_names))
                for name in [
                    f"{TOP}/{PLUGIN_ID}/nougat_split_archive.py",
                    f"{TOP}/{PLUGIN_ID}/bin/nougat-workshop-plugin",
                ]:
                    info = zf.getinfo(name)
                    mode = (info.external_attr >> 16) & 0o777
                    if info.create_system != 3 or mode & 0o111 == 0:
                        raise RuntimeError("Workshop plugin ZIP lost executable permission metadata: " + name)

            os.replace(temporary, output)
        except Exception:
            try:
                temporary.unlink()
            except OSError:
                pass
            raise

        print("PASS: standalone native Workshop plugin ZIP verified")
        print("Plugin: Workshop v0.0.50")
        print("Runtime: x11-process")
        print("ZIP:", output)
        print(f"Size: {output.stat().st_size / 1024:.1f} KiB")
        print("SHA-256:", sha256_file(output))
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
