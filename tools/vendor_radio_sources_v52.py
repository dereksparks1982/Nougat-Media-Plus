#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "components/radio/UPSTREAM_COMPONENTS.json"
UPSTREAM = ROOT / "components/radio/upstream"


def fail(message: str) -> None:
    raise RuntimeError(message)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def safe_extract(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as zf:
        infos = zf.infolist()
        if not infos:
            fail(f"empty archive: {archive}")
        for info in infos:
            name = info.filename.replace("\\", "/")
            parts = Path(name).parts
            if name.startswith("/") or ".." in parts:
                fail(f"unsafe ZIP path in {archive}: {name}")
            mode = (info.external_attr >> 16) & 0o170000
            if mode == 0o120000:
                fail(f"symlink entry refused in source archive: {name}")
        with tempfile.TemporaryDirectory(prefix="nougat-radio-unpack-") as td:
            tmp = Path(td)
            zf.extractall(tmp)
            entries = [p for p in tmp.iterdir() if p.name != "__MACOSX"]
            source = entries[0] if len(entries) == 1 and entries[0].is_dir() else tmp
            if destination.exists():
                shutil.rmtree(destination)
            shutil.copytree(source, destination)


def curl_download(url: str, output: Path) -> None:
    curl = shutil.which("curl")
    if not curl:
        fail("curl is required when an owner-supplied radio source archive is not found")
    result = subprocess.run(
        [curl, "-L", "--fail", "--silent", "--show-error", "--connect-timeout", "15", "--max-time", "300", "-o", str(output), url],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env={**os.environ, "GIT_TERMINAL_PROMPT": "0"},
    )
    if result.returncode != 0:
        fail(f"download failed: {url}: {result.stderr.strip()}")


def candidate_archives(filename: str) -> list[Path]:
    home = Path.home()
    roots = [ROOT, ROOT.parent, Path.cwd()]
    source_dir = os.environ.get("NOUGAT_RADIO_SOURCE_DIR", "").strip()
    if source_dir:
        roots.append(Path(source_dir).expanduser())
    roots.extend([home / "Downloads", home / "Desktop", Path("/mnt/data")])
    seen: set[Path] = set()
    found: list[Path] = []
    for base in roots:
        path = (base / filename).expanduser()
        try:
            key = path.resolve()
        except OSError:
            key = path
        if key in seen:
            continue
        seen.add(key)
        if path.is_file():
            found.append(path)
    return found


def license_present(destination: Path) -> bool:
    names = {"license", "license.txt", "license.md", "copying", "copying.txt", "copying.md"}
    for path in destination.iterdir():
        if path.is_file() and path.name.lower() in names:
            return True
    return any(p.is_file() and p.name.lower().startswith(("license", "copying")) for p in destination.glob("*"))


def stage_component(component: dict, allow_optional: bool) -> dict:
    if component.get("optional") and not allow_optional:
        return {"id": component["id"], "status": "optional-not-requested"}

    cid = component["id"]
    destination = UPSTREAM / cid
    owner_archive = component.get("owner_archive")
    source_archive: Path | None = None
    origin = ""

    if owner_archive:
        matches = candidate_archives(owner_archive)
        if matches:
            source_archive = matches[0]
            origin = f"owner-archive:{source_archive}"

    temp_path: Path | None = None
    if source_archive is None:
        repo = component["repository"]
        commit = component["commit"]
        fd, temp_name = tempfile.mkstemp(prefix=f"nougat-{cid}-", suffix=".zip")
        os.close(fd)
        temp_path = Path(temp_name)
        url = f"https://github.com/{repo}/archive/{commit}.zip"
        curl_download(url, temp_path)
        source_archive = temp_path
        origin = f"pinned-github:{repo}@{commit}"

    try:
        archive_hash = sha256(source_archive)
        safe_extract(source_archive, destination)
        if not license_present(destination):
            fail(f"{cid}: upstream license/copying file not found after extraction")
        stamp = {
            "id": cid,
            "origin": origin,
            "archive_sha256": archive_hash,
            "repository": component["repository"],
            "pinned_commit": component["commit"],
            "license_declared": component["license"],
            "boundary": component["boundary"],
        }
        (destination / "NOUGAT_UPSTREAM_SOURCE.json").write_text(json.dumps(stamp, indent=2) + "\n", encoding="utf-8")
        return {"id": cid, "status": "staged", "origin": origin, "sha256": archive_hash}
    finally:
        if temp_path is not None:
            temp_path.unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Stage pinned Nougat Radio upstream sources without prompts")
    parser.add_argument("--all", action="store_true", help="also stage specialized DAB/DRM/satellite/trunked worker sources")
    args = parser.parse_args()
    try:
        data = json.loads(MANIFEST.read_text(encoding="utf-8"))
        UPSTREAM.mkdir(parents=True, exist_ok=True)
        results = [stage_component(item, args.all) for item in data["components"]]
        (UPSTREAM / "VENDOR_RESULT.json").write_text(json.dumps({"results": results}, indent=2) + "\n", encoding="utf-8")
        for item in results:
            print(f"{item['id']}: {item['status']}")
        print("PASS: Nougat Radio upstream source staging complete.")
        return 0
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
