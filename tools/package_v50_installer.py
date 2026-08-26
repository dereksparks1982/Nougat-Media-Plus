#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse
import hashlib
import os
import stat
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "Nougat_Media_Suite_v50"
DEFAULT_OUTPUT = Path.home() / "Downloads" / "Nougat_Media_Suite_v0.0.50_INSTALLER.zip"
TOP = "Nougat_Media_Suite_v0.0.50_INSTALLER"

FILES = [
    ("Nougat_Media_Suite_v50", 0o755),
    ("NougatMediaSuite.desktop", 0o644),
    ("assets/icons/nougat-media-suite-concept-sheet-v24.png", 0o644),
    ("installer/nougat_v50_installer.py", 0o755),
    ("installer/INSTALL.sh", 0o755),
    ("plugins/workshop/plugin.json", 0o644),
    ("components/workshop/nougat_split_archive.py", 0o755),
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def zip_info(archive_name: str, mode: int) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(archive_name)
    info.create_system = 3  # Unix, so POSIX executable bits survive extraction.
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = (stat.S_IFREG | mode) << 16
    info.date_time = (2026, 8, 26, 0, 0, 0)
    return info


def add_file(zf: zipfile.ZipFile, source: Path, archive_name: str, mode: int) -> None:
    with source.open("rb") as handle:
        zf.writestr(zip_info(archive_name, mode), handle.read())


def member_mode(info: zipfile.ZipInfo) -> int:
    return (info.external_attr >> 16) & 0o777


def main() -> int:
    parser = argparse.ArgumentParser(description="Package the Nougat Media Suite v0.0.50 installer bundle")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    try:
        missing = [rel for rel, _ in FILES if not (ROOT / rel).is_file()]
        if missing:
            raise RuntimeError("installer bundle is missing: " + ", ".join(missing))

        version = os.popen(f'"{BINARY}" --version').read().strip()
        if version != "Nougat Media Suite v0.0.50":
            raise RuntimeError("installer binary identity mismatch: " + repr(version))

        output = args.output.expanduser().resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.NamedTemporaryFile(prefix=output.name + ".", suffix=".tmp", dir=output.parent, delete=False) as handle:
            temp = Path(handle.name)

        try:
            with zipfile.ZipFile(temp, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
                for rel, mode in FILES:
                    add_file(zf, ROOT / rel, f"{TOP}/{rel}", mode)

                launcher = (
                    '#!/bin/bash\n'
                    'BASE="$(cd "$(dirname "$0")" && pwd)"\n'
                    'python3 "$BASE/installer/nougat_v50_installer.py" --source-root "$BASE"\n'
                )
                zf.writestr(zip_info(f"{TOP}/INSTALL.sh", 0o755), launcher.encode("utf-8"))

            with zipfile.ZipFile(temp, "r") as zf:
                bad = zf.testzip()
                if bad is not None:
                    raise RuntimeError("installer ZIP integrity failure at " + bad)
                names = set(zf.namelist())
                required = {
                    f"{TOP}/INSTALL.sh",
                    f"{TOP}/Nougat_Media_Suite_v50",
                    f"{TOP}/installer/nougat_v50_installer.py",
                    f"{TOP}/plugins/workshop/plugin.json",
                    f"{TOP}/components/workshop/nougat_split_archive.py",
                }
                absent = sorted(required - names)
                if absent:
                    raise RuntimeError("installer ZIP missing required entries: " + ", ".join(absent))

                executable_members = {
                    f"{TOP}/INSTALL.sh",
                    f"{TOP}/Nougat_Media_Suite_v50",
                    f"{TOP}/installer/nougat_v50_installer.py",
                    f"{TOP}/components/workshop/nougat_split_archive.py",
                }
                for name in executable_members:
                    info = zf.getinfo(name)
                    if info.create_system != 3 or member_mode(info) & 0o111 == 0:
                        raise RuntimeError("installer ZIP lost executable permission metadata: " + name)

            os.replace(temp, output)
        except Exception:
            try:
                temp.unlink()
            except OSError:
                pass
            raise

        print("PASS: Nougat Media Suite v0.0.50 installer ZIP verified")
        print("PASS: Unix executable permissions verified inside installer ZIP")
        print("ZIP:", output)
        print(f"Size: {output.stat().st_size / (1024 * 1024):.1f} MiB")
        print("SHA-256:", sha256(output))
        print("Install: extract the ZIP and run INSTALL.sh")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
