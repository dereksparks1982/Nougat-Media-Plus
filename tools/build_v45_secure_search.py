#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


def stop(msg: str) -> int:
    print(f"STOP: {msg}")
    return 1


def run(cmd: list[str], cwd: Path) -> int:
    print("+ " + " ".join(cmd))
    return subprocess.run(cmd, cwd=cwd).returncode


def main() -> int:
    root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()
    cmake = root / "CMakeLists.txt"
    if not cmake.is_file():
        return stop(f"CMakeLists.txt not found at {root}")
    if "VERSION 0.0.45" not in cmake.read_text(encoding="utf-8"):
        return stop("project is not the v0.0.45 candidate; apply the v45 build first")

    build_dir = root / "build"
    jobs = max(1, min(8, os.cpu_count() or 2))
    print("=== NOUGAT MEDIA SUITE v0.0.45 NATIVE BUILD ===")
    if run(["cmake", "-S", str(root), "-B", str(build_dir)], root) != 0:
        return stop("CMake configure failed; terminal left open")
    if run(["cmake", "--build", str(build_dir), "-j", str(jobs)], root) != 0:
        return stop("native compile/link failed; terminal left open")

    candidate = build_dir / "Nougat_Media_Suite_v45"
    if not candidate.is_file():
        return stop(f"build succeeded but executable is missing: {candidate}")
    installed = root / "Nougat_Media_Suite_v45"
    shutil.copy2(candidate, installed)
    installed.chmod(installed.stat().st_mode | 0o111)
    print(f"PASS: installed root executable {installed}")

    version = subprocess.run([str(installed), "--version"], cwd=root, capture_output=True, text=True, timeout=10)
    output = (version.stdout + version.stderr).strip()
    if version.returncode != 0 or output != "Nougat Media Suite v0.0.45":
        return stop(f"native version smoke failed: rc={version.returncode} output={output!r}")
    print("PASS: native --version smoke")

    selftest = subprocess.run([str(installed), "--v44-release-self-test"], cwd=root,
                              capture_output=True, text=True, timeout=60)
    if selftest.returncode != 0:
        print((selftest.stdout + selftest.stderr)[-4000:])
        return stop("retained v0.0.44 native release self-test failed")
    print("PASS: retained v0.0.44 native release self-test")

    test = root / "tools/test_nougat_secure_search_v45.py"
    if run([sys.executable, str(test), str(root)], root) != 0:
        return stop("v0.0.45 Secure Search validation failed")

    cargo = shutil.which("cargo")
    if cargo:
        broker = root / "components/privacy_broker"
        if run([cargo, "check", "--manifest-path", str(broker / "Cargo.toml")], root) != 0:
            return stop("Rust Privacy Broker scaffold failed cargo check")
        print("PASS: Rust Privacy Broker scaffold cargo check")
    else:
        print("NOTE: cargo is not installed. Rust broker source is scaffolded but is not a required v0.0.45 app build dependency.")

    print("=== v0.0.45 BUILD + VALIDATION PASS ===")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
