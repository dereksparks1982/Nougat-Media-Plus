#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import os
import shutil
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]

DOS_URL = "https://github.com/dosbox-staging/dosbox-staging/releases/download/v0.82.2/dosbox-staging-linux-x86_64-v0.82.2.tar.xz"
DOS_SHA256 = "bc229df72ea103b7865cdca67324772dbffa8e58866477e69a79638b723a0442"

XENIA_URL = "https://github.com/xenia-canary/xenia-canary/releases/download/1e834f8/xenia_canary_linux.AppImage"
XENIA_SHA256 = "91df919a912bd305a214c535e0ab8abee43c18eb1bab1ef5e35991d16738b05e"


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url: str, target: Path, expected: str) -> None:
    print("Downloading:", url)
    req = urllib.request.Request(url, headers={"User-Agent": "Nougat-Media-Suite-v0.0.48"})
    with urllib.request.urlopen(req, timeout=90) as response, target.open("wb") as out:
        shutil.copyfileobj(response, out)
    actual = sha256(target)
    if actual != expected:
        target.unlink(missing_ok=True)
        raise RuntimeError(f"SHA-256 mismatch for {url}: {actual}")


def safe_extract_tar(archive: Path, destination: Path) -> None:
    root = destination.resolve()
    with tarfile.open(archive, "r:xz") as tar:
        for member in tar.getmembers():
            resolved = (destination / member.name).resolve()
            if resolved != root and root not in resolved.parents:
                raise RuntimeError("Unsafe path in DOSBox archive: " + member.name)
        tar.extractall(destination)


def install_dosbox(temp: Path) -> None:
    target = ROOT / "components/games/runtime/dosbox-staging/dosbox"
    if target.is_file() and os.access(target, os.X_OK):
        print("DOSBox runtime already present:", target)
        return

    archive = temp / "dosbox.tar.xz"
    download(DOS_URL, archive, DOS_SHA256)
    extracted = temp / "dosbox"
    extracted.mkdir()
    safe_extract_tar(archive, extracted)

    candidates = [
        p for p in extracted.rglob("*")
        if p.is_file() and p.name in {"dosbox", "dosbox-staging"} and os.access(p, os.X_OK)
    ]
    if not candidates:
        raise RuntimeError("Verified DOSBox archive did not contain an executable")
    candidates.sort(key=lambda p: (0 if p.name == "dosbox" else 1, len(p.parts)))

    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(candidates[0], target)
    target.chmod(target.stat().st_mode | 0o111)

    # Keep the upstream archive digest/source metadata beside the managed runtime.
    (target.parent / "UPSTREAM.txt").write_text(
        f"DOSBox Staging v0.82.2\n{DOS_URL}\nSHA-256 {DOS_SHA256}\n",
        encoding="utf-8",
    )
    print("Installed DOSBox runtime:", target)


def install_xenia(temp: Path) -> None:
    target = ROOT / "components/games/runtime/xenia/xenia_canary"
    if target.is_file() and os.access(target, os.X_OK):
        print("Xenia runtime already present:", target)
        return

    image = temp / "xenia_canary_linux.AppImage"
    download(XENIA_URL, image, XENIA_SHA256)

    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(image, target)
    target.chmod(target.stat().st_mode | 0o111)
    (target.parent / "UPSTREAM.txt").write_text(
        f"Xenia Canary 1e834f8, published 2026-08-24\n{XENIA_URL}\nSHA-256 {XENIA_SHA256}\n",
        encoding="utf-8",
    )
    print("Installed Xenia Canary runtime:", target)


def main() -> int:
    try:
        with tempfile.TemporaryDirectory(prefix="nougat-v48-runtimes-") as td:
            temp = Path(td)
            install_dosbox(temp)
            install_xenia(temp)
        print("=== v0.0.48 GAME RUNTIMES READY ===")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("No sudo command was used. Terminal remains open.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
