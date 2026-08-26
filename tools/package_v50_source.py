#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import os
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = Path.home() / "Downloads" / "Nougat_Media_Suite_v0.0.50_SOURCE_CANDIDATE.zip"
HARD_LIMIT = 500 * 1024 * 1024
TARGET_LIMIT = 450 * 1024 * 1024


def run(args):
    return subprocess.run([str(x) for x in args], cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    output = Path(sys.argv[1]).expanduser().resolve() if len(sys.argv) > 1 else DEFAULT_OUTPUT
    try:
        tracked = run(["git", "ls-files", "-z"])
        if tracked.returncode != 0:
            raise RuntimeError("git ls-files failed: " + tracked.stdout.strip())
        relpaths = [Path(item) for item in tracked.stdout.split("\0") if item]
        if not relpaths:
            raise RuntimeError("no tracked source files found")

        untracked = run(["git", "ls-files", "--others", "--exclude-standard", "-z"])
        if untracked.returncode != 0:
            raise RuntimeError("untracked-file inventory failed: " + untracked.stdout.strip())
        unexpected = [item for item in untracked.stdout.split("\0") if item]
        if unexpected:
            raise RuntimeError("untracked files must be classified before canonical packaging: " + ", ".join(unexpected[:20]))

        forbidden_prefixes = (
            ".git/", "build/", "repair_payload/", "components/games/runtime/",
            "components/security/runtime/", "components/ai/runtime/", "components/jellyfin/runtime/",
            ".github/v50-validation-",
        )
        forbidden_suffixes = (".pyc",)
        selected: list[Path] = []
        for rel in relpaths:
            text = rel.as_posix()
            if text.startswith(forbidden_prefixes) or text.endswith(forbidden_suffixes):
                continue
            full = ROOT / rel
            if full.is_file():
                selected.append(rel)

        if not selected:
            raise RuntimeError("packaging selection is empty")

        output.parent.mkdir(parents=True, exist_ok=True)
        temporary = output.with_suffix(output.suffix + ".tmp")
        if temporary.exists():
            temporary.unlink()
        if output.exists():
            output.unlink()

        prefix = "Nougat Media Suite v0.0.50/"
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9,
                             allowZip64=True) as archive:
            for rel in selected:
                archive.write(ROOT / rel, prefix + rel.as_posix())

        with zipfile.ZipFile(temporary, "r") as archive:
            bad = archive.testzip()
            if bad is not None:
                raise RuntimeError("ZIP verification failed at " + bad)
            names = archive.namelist()
            if any("/.git/" in name or name.endswith("/.git") for name in names):
                raise RuntimeError("canonical source ZIP accidentally contains .git")
            if any("/.github/v50-validation-" in name for name in names):
                raise RuntimeError("canonical source ZIP accidentally contains a temporary validation file")
            if not any(name.endswith("/src/main.cpp") for name in names):
                raise RuntimeError("canonical source ZIP is missing src/main.cpp")
            if not any(name.endswith("/COMPANY_BIBLE.md") for name in names):
                raise RuntimeError("canonical source ZIP is missing COMPANY_BIBLE.md")

        size = temporary.stat().st_size
        if size > HARD_LIMIT:
            raise RuntimeError(f"source ZIP is {size / 1024 / 1024:.1f} MiB, above the 500 MiB hard ceiling")
        os.replace(temporary, output)
        digest = sha256(output)

        print("PASS: canonical v0.0.50 source ZIP verified")
        print("ZIP:", output)
        print(f"Size: {size / 1024 / 1024:.1f} MiB")
        print("SHA-256:", digest)
        if size <= TARGET_LIMIT:
            print("PASS: package is within the preferred 450 MiB transport target")
        else:
            print("PASS: package is below 500 MiB; it is above the preferred 450 MiB target")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("No oversized or unverifiable source package is being presented as canonical.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
