#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKER = ROOT / "components" / "workshop" / "nougat_split_archive.py"


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


class WorkshopSplitArchiveTest(unittest.TestCase):
    def run_worker(self, *args: str, expected: int = 0) -> subprocess.CompletedProcess[str]:
        proc = subprocess.run(
            [sys.executable, str(WORKER), *args],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(
            proc.returncode,
            expected,
            msg=f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}",
        )
        return proc

    def test_directory_round_trip_and_part_ceiling(self) -> None:
        with tempfile.TemporaryDirectory(prefix="nougat-workshop-test-") as temp:
            temp_path = Path(temp)
            source = temp_path / "Sample Project"
            source.mkdir()
            (source / "empty").mkdir()
            (source / "nested").mkdir()
            (source / "hello.txt").write_text("Nougat Workshop\n", encoding="utf-8")
            # Deterministic bytes make the test reproducible without random data.
            (source / "nested" / "large.bin").write_bytes(bytes(range(256)) * 12288)  # 3 MiB
            (source / "nested" / "other.bin").write_bytes(b"0123456789abcdef" * 131072)  # 2 MiB
            (source / "link-to-hello").symlink_to("hello.txt")

            original_hashes = {
                p.relative_to(source).as_posix(): digest(p)
                for p in source.rglob("*")
                if p.is_file() and not p.is_symlink()
            }

            parts = temp_path / "parts"
            split = self.run_worker(
                "split", str(source), "--output", str(parts),
                "--max-part-bytes", str(2 * 1024 * 1024), "--json",
            )
            manifest = json.loads(split.stdout)
            manifest_path = Path(manifest["manifest"])
            self.assertEqual(manifest["format"], "NOUGAT_SPLIT_ARCHIVE")
            self.assertGreater(len(manifest["parts"]), 1)
            for part in manifest["parts"]:
                self.assertLessEqual(part["size"], 2 * 1024 * 1024)
                self.assertEqual(digest(parts / part["file"]), part["sha256"])

            verified = json.loads(self.run_worker("verify", str(manifest_path), "--json").stdout)
            self.assertTrue(verified["ok"])

            restored_parent = temp_path / "restored"
            restored = json.loads(self.run_worker(
                "reassemble", str(manifest_path), "--output", str(restored_parent), "--json"
            ).stdout)
            self.assertTrue(restored["ok"])
            restored_root = Path(restored["output"])
            self.assertTrue((restored_root / "empty").is_dir())
            self.assertTrue((restored_root / "link-to-hello").is_symlink())
            self.assertEqual((restored_root / "link-to-hello").readlink().as_posix(), "hello.txt")

            restored_hashes = {
                p.relative_to(restored_root).as_posix(): digest(p)
                for p in restored_root.rglob("*")
                if p.is_file() and not p.is_symlink()
            }
            self.assertEqual(original_hashes, restored_hashes)

    def test_corrupt_part_is_rejected_before_reassembly(self) -> None:
        with tempfile.TemporaryDirectory(prefix="nougat-workshop-corrupt-") as temp:
            temp_path = Path(temp)
            source = temp_path / "source.bin"
            source.write_bytes(b"verified transport\n" * 200000)
            parts = temp_path / "parts"
            split = json.loads(self.run_worker(
                "split", str(source), "--output", str(parts),
                "--max-part-bytes", str(2 * 1024 * 1024), "--json"
            ).stdout)
            manifest_path = Path(split["manifest"])
            first_part = parts / split["parts"][0]["file"]
            with first_part.open("r+b") as f:
                f.seek(128)
                byte = f.read(1)
                f.seek(128)
                f.write(bytes([(byte[0] if byte else 0) ^ 0x5A]))

            verified = self.run_worker("verify", str(manifest_path), "--json", expected=2)
            result = json.loads(verified.stdout)
            self.assertFalse(result["ok"])
            self.assertTrue(any("SHA-256 mismatch" in problem for problem in result["problems"]))

    def test_inspection_reports_upload_safe_default(self) -> None:
        with tempfile.TemporaryDirectory(prefix="nougat-workshop-inspect-") as temp:
            path = Path(temp) / "tiny.dat"
            path.write_bytes(b"x" * 1234)
            result = json.loads(self.run_worker("inspect", str(path), "--json").stdout)
            self.assertEqual(result["total_bytes"], 1234)
            self.assertEqual(result["file_count"], 1)
            self.assertEqual(result["default_max_part_bytes"], 450 * 1024 * 1024)


if __name__ == "__main__":
    unittest.main()
