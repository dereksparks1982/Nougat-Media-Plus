#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
WORKER = ROOT / "tools" / "nougat_file_splitter.py"


def run(*args: str, timeout: int = 120) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["python3", str(WORKER), *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            block = f.read(1024 * 1024)
            if not block:
                return h.hexdigest()
            h.update(block)


def main() -> int:
    source_text = WORKER.read_text(encoding="utf-8")
    require("zenity" not in source_text.lower(), "v54 worker must not contain Zenity popup paths")
    require("studio-gui" not in source_text, "v54 worker must not expose the legacy popup GUI")
    require('FORMAT = "nougat-split-zip-v3"' in source_text, "v54 manifest format marker missing")

    with tempfile.TemporaryDirectory(prefix="nougat-v54-splitter-test-") as td:
        root = Path(td)

        # Recommendation contract. Sparse input proves analysis does not load the file into RAM.
        sparse = root / "large-test.bin"
        with sparse.open("wb") as f:
            f.truncate((2 * 1024 + 600) * 1024 * 1024)
        result = run("analyze", str(sparse), "--target-piece-mib", "450")
        require(result.returncode == 0, f"large analyze failed: {result.stdout}")
        require("SUGGESTED_PIECES 6" in result.stdout, f"expected 6-piece suggestion under 450 MiB: {result.stdout}")

        tiny = root / "tiny.dat"
        tiny.write_bytes(b"nougat" * 100)
        result = run("analyze", str(tiny))
        require(result.returncode == 0, f"small analyze failed: {result.stdout}")
        require("SUGGESTED_PIECES 2" in result.stdout, f"small sources should default to 2 pieces: {result.stdout}")
        too_large = run("analyze", str(tiny), "--target-piece-mib", "477")
        require(too_large.returncode != 0 and "476 MiB" in too_large.stdout,
                f"oversize target should be rejected: {too_large.stdout}")

        # Exact streaming file split, verify, and reassemble.
        original = root / "original.bin"
        original.write_bytes(os.urandom(7 * 1024 * 1024 + 12345))
        original_hash = sha(original)
        out = root / "downloads"
        result = run("split", str(original), str(out), "--name", "professional-test", "--pieces", "4", "--max-piece-mib", "450")
        require(result.returncode == 0, f"split failed: {result.stdout}")
        require("VERIFY PASS" in result.stdout, "split must read-back verify every part")
        manifest = out / "professional-test.zip.parts.json"
        require(manifest.is_file(), "split manifest missing")
        data = json.loads(manifest.read_text(encoding="utf-8"))
        require(data.get("format") == "nougat-split-zip-v3", "wrong v54 manifest format")
        require(data.get("piece_count") == 4, "wrong piece count in manifest")
        require(len(data.get("parts", [])) == 4, "wrong number of parts")
        require(all(int(part.get("size", 0)) <= 450 * 1024 * 1024 for part in data.get("parts", [])),
                "every split part must stay at or below the selected 450 MiB maximum")

        result = run("verify", str(manifest))
        require(result.returncode == 0 and "VERIFY PASS" in result.stdout,
                f"standalone verify failed: {result.stdout}")

        restored_dir = root / "restored"
        result = run("reassemble", str(manifest), "--output", str(restored_dir))
        require(result.returncode == 0 and "VERIFY PASS" in result.stdout,
                f"reassemble failed: {result.stdout}")
        restored = restored_dir / original.name
        require(restored.is_file(), "restored file missing")
        require(sha(restored) == original_hash, "restored file hash differs from original")

        # Legacy v2 manifests remain readable so existing user split sets are not stranded.
        legacy_manifest = out / "professional-test-legacy.zip.parts.json"
        legacy = dict(data)
        legacy["format"] = "nougat-split-zip-v2"
        legacy_manifest.write_text(json.dumps(legacy, indent=2) + "\n", encoding="utf-8")
        result = run("verify", str(legacy_manifest))
        require(result.returncode == 0 and "VERIFY PASS" in result.stdout,
                f"legacy v2 verify failed: {result.stdout}")

        # Cancellation must not leave a manifest that looks complete.
        cancel_src = root / "cancel-source.bin"
        cancel_src.write_bytes(os.urandom(48 * 1024 * 1024))
        cancel_out = root / "cancel-out"
        proc = subprocess.Popen(
            ["python3", str(WORKER), "split", str(cancel_src), str(cancel_out),
             "--name", "cancel-test", "--pieces", "6"],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            start_new_session=True,
        )
        # Wait until the Python worker has installed its signal handlers and
        # begun real work before requesting cancellation.
        seen = []
        deadline = time.monotonic() + 10.0
        assert proc.stdout is not None
        while time.monotonic() < deadline:
            line = proc.stdout.readline()
            if not line:
                break
            seen.append(line)
            if line.startswith("PROGRESS ") or line.startswith("STATUS Packaging"):
                break
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        remainder, _ = proc.communicate(timeout=30)
        output = "".join(seen) + remainder
        require(proc.returncode in (0, 130), f"unexpected cancellation rc={proc.returncode}: {output}")
        if proc.returncode == 130:
            require(not (cancel_out / "cancel-test.zip.parts.json").exists(),
                    "cancelled operation left a complete-looking manifest")
            require(not list(cancel_out.glob("cancel-test.zip.[0-9][0-9][0-9]")),
                    "cancelled operation left split parts behind")

    print("PASS: v0.0.54 professional File Splitter engine contracts")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
