#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD = ROOT / "repair_payload"
EXPECTED_HEAD = "f65c320c68cf5451f1151c59fbb2bccc4f5c434e"

OLD_HOST_SHA256 = "abd223e56d912d84275b75b56fb723e15b5af7e896861b431ca61b3524ccb34b"
NEW_HOST_SHA256 = "e7f9975a79d0d0aff60d0ae31519332cb62209a8e1e8369b9616e200d8912426"
HOST_HPP_SHA256 = "7c3530a1a64d40531265400062505cd0c10ec6bcb7dfafaa6c64efc81303078a"
OLD_WORKER_SHA256 = "0f3e9b32eef7c5f4b9c5e007c2cd70bcdabc1212f8902f1aece8d96fb9d95a13"
NEW_WORKER_SHA256 = "28837edafc7552459bafc06ea9b7e5be775544c78ee41b793a0853fdcb390d58"
OLD_TEST_SHA256 = "dd4c3d39d7f89d0737b7a94935345c87a3a412d0dacf1289b05dfc38a7a29519"
NEW_TEST_SHA256 = "5dc2c9118ee12f0bf5b4b3a026e08a31dde1cf1f4e8562763786e629eda7575a"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def command(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def verify_payload() -> None:
    expected = {
        "emulator_host.cpp": NEW_HOST_SHA256,
        "artwork_cache_worker.py": NEW_WORKER_SHA256,
        "test_nougat_media_suite_v49.py": NEW_TEST_SHA256,
    }
    for name, digest in expected.items():
        path = PAYLOAD / name
        need(path.is_file(), "repair payload is missing " + name)
        need(sha256(path) == digest, "repair payload hash mismatch for " + name)


def replace_exactly_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    need(count == 1, f"{label}: expected exactly one current-v49 anchor, found {count}")
    return text.replace(old, new, 1)


def repair_main() -> bool:
    path = ROOT / "src/main.cpp"
    need(path.is_file(), "src/main.cpp is missing")
    text = path.read_text(encoding="utf-8")
    need('printf("Nougat Media Suite v0.0.49\\n")' in text,
         "src/main.cpp is not the built v0.0.49 candidate")
    need("poll_stella_top_options" in text and "filter_game_library_preferences" in text,
         "src/main.cpp is missing the rejected v0.0.49 Games implementation")

    old_header = 'const std::string versionLabel = "v0.0.48";'
    new_header = 'const std::string versionLabel = "v0.0.49";'
    old_diag = 'input.app_version = "Nougat Media Suite v0.0.48";'
    new_diag = 'input.app_version = "Nougat Media Suite v0.0.49";'

    changed = False
    if old_header in text:
        text = replace_exactly_once(text, old_header, new_header, "visible version label")
        changed = True
    else:
        need(text.count(new_header) == 1, "visible version label is neither exact v48 nor repaired v49")

    if old_diag in text:
        text = replace_exactly_once(text, old_diag, new_diag, "diagnostic version identity")
        changed = True
    else:
        need(text.count(new_diag) == 1, "diagnostic version is neither exact v48 nor repaired v49")

    if changed:
        path.write_text(text, encoding="utf-8")
    return changed


def replace_verified(target: Path, payload_name: str,
                     old_digest: str, new_digest: str, label: str) -> bool:
    need(target.is_file(), label + " target is missing")
    actual = sha256(target)
    if actual == new_digest:
        return False
    need(actual == old_digest,
         f"{label} differs from the rejected v0.0.49 candidate; expected {old_digest}, got {actual}")
    source = PAYLOAD / payload_name
    need(sha256(source) == new_digest, label + " payload digest changed")
    temporary = target.with_name(target.name + ".v49-owner-repair.tmp")
    shutil.copy2(source, temporary)
    os.replace(temporary, target)
    return True


def main() -> int:
    try:
        need((ROOT / ".git").exists(), "run this from the Nougat Media Suite project tree")
        head = command(["git", "rev-parse", "HEAD"])
        need(head.returncode == 0, "could not read Git HEAD")
        need(head.stdout.strip() == EXPECTED_HEAD,
             "owner repair requires the rejected v0.0.49 working tree on accepted v0.0.48 HEAD " +
             EXPECTED_HEAD + "; current HEAD is " + head.stdout.strip())
        verify_payload()

        header = ROOT / "src/games/emulator_host.hpp"
        need(header.is_file(), "src/games/emulator_host.hpp is missing")
        need(sha256(header) == HOST_HPP_SHA256,
             "emulator_host.hpp differs from the rejected v0.0.49 candidate; refusing to guess")

        host_changed = replace_verified(
            ROOT / "src/games/emulator_host.cpp", "emulator_host.cpp",
            OLD_HOST_SHA256, NEW_HOST_SHA256, "emulator host")
        worker_changed = replace_verified(
            ROOT / "components/games/artwork_cache_worker.py", "artwork_cache_worker.py",
            OLD_WORKER_SHA256, NEW_WORKER_SHA256, "artwork worker")
        test_changed = replace_verified(
            ROOT / "tools/test_nougat_media_suite_v49.py", "test_nougat_media_suite_v49.py",
            OLD_TEST_SHA256, NEW_TEST_SHA256, "v49 static test")
        main_changed = repair_main()

        (ROOT / "components/games/artwork_cache_worker.py").chmod(0o755)
        (ROOT / "tools/test_nougat_media_suite_v49.py").chmod(0o755)

        changed = host_changed or worker_changed or test_changed or main_changed
        if not changed:
            print("PASS: v0.0.49 Atari owner-test repair is already applied.")
            return 0

        print("=== v0.0.49 ATARI OWNER-TEST REPAIR APPLIED ===")
        print("PASS: recursive X11/XWayland emulator-client capture installed")
        print("PASS: standalone Stella taskbar/pager identity suppression installed")
        print("PASS: persistent indexed Atari artwork matching installed")
        print("PASS: verified box-front fallback installed for the tested 2 Pak Special cartridge")
        print("PASS: visible header and Diagnostic Center version identity repaired to v0.0.49")
        print("PASS: existing v49 USA/English/revision filtering and scroll repairs preserved")
        print("NO GIT COMMIT, TAG, OR PUSH WAS PERFORMED.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. The rejected v0.0.49 candidate was not accepted and nothing was pushed.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
