#!/usr/bin/env python3
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
TARGET = "Nougat_Media_Suite_v46"
APPROVED_ICON = ROOT / "assets/icons/nougat-media-suite-concept-sheet-v24.png"
APPROVED_SHA = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"

def run(args, **kwargs):
    print("+", " ".join(str(x) for x in args))
    return subprocess.run([str(x) for x in args], **kwargs)

def need(ok, msg):
    if not ok:
        raise RuntimeError(msg)

def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def main():
    try:
        need((ROOT/"CMakeLists.txt").is_file(), "CMakeLists.txt missing")
        need(APPROVED_ICON.is_file(), "approved Nougat N icon missing")
        need(sha256(APPROVED_ICON) == APPROVED_SHA, "approved Nougat N master hash changed")
        need("/build/" in (ROOT/".gitignore").read_text(), "/build/ is not ignored")

        # Purge stale versioned binaries before configure/build. This covers both
        # the visible project root and a reused CMake build directory.
        for base in (ROOT, BUILD):
            if base.is_dir():
                for p in base.glob("Nougat_Media_Suite_v*"):
                    if p.is_file():
                        print("Removing stale executable before build:", p)
                        p.unlink()

        r = run(["cmake", "-S", ROOT, "-B", BUILD])
        need(r.returncode == 0, "CMake configure failed")
        r = run(["cmake", "--build", BUILD, "-j2"])
        need(r.returncode == 0, "native v0.0.46 build failed")

        built = BUILD/TARGET
        need(built.is_file() and os.access(built, os.X_OK), "built v46 executable missing")

        # Build-tree runtime: the final binary intentionally embeds
        # $ORIGIN/components/... . While it lives in build/, supply
        # the project runtime explicitly for validation only.
        runtime_dirs = [
            ROOT/"components/ai/runtime/lib",
            ROOT/"components/ai/runtime/lib64",
        ]
        runtime_dirs = [p for p in runtime_dirs if p.is_dir()]
        need(runtime_dirs, "Nougat AI runtime directories are missing")
        runtime_env = dict(os.environ)
        runtime_path = os.pathsep.join(str(p) for p in runtime_dirs)
        inherited = runtime_env.get("LD_LIBRARY_PATH", "")
        runtime_env["LD_LIBRARY_PATH"] = runtime_path + (os.pathsep + inherited if inherited else "")

        r = run([built, "--version"], env=runtime_env,
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        need(r.returncode == 0,
             "build-tree v46 could not start with Nougat AI runtime: " + r.stdout.strip())
        need(r.stdout.strip() == "Nougat Media Suite v0.0.46",
             "build-tree v46 identity mismatch: " + repr(r.stdout.strip()))
        print("PASS: build-tree v46 starts with project AI runtime")

        r = run([sys.executable, ROOT/"tools/test_nougat_media_suite_v46.py", ROOT, built],
                env=runtime_env)
        need(r.returncode == 0, "v0.0.46 contract validation failed")

        # Retain the v45 Secure Search privacy contract with a v46-specific\n        # validator. Do not run the old v45 release-identity validator against v46.\n        secure_v46 = ROOT/"tools/test_nougat_secure_search_v46.py"\n        need(secure_v46.is_file(), "v46 Secure Search retention validator is missing")\n        r = run([sys.executable, secure_v46, ROOT], env=runtime_env)\n        need(r.returncode == 0, "v0.0.46 Secure Search retention validation failed")\n\n        # HARD RELEASE GATE 1: no historical versioned root executable survives.
        for p in ROOT.glob("Nougat_Media_Suite_v*"):
            if p.is_file():
                print("Removing obsolete root executable:", p.name)
                p.unlink()

        promoted = ROOT/TARGET
        shutil.copy2(built, promoted)
        promoted.chmod(promoted.stat().st_mode | 0o111)

        # HARD RELEASE GATE 2: apply Files/Nautilus custom icon AFTER the final
        # binary copy. Nothing writes or copies the executable after this point.
        gio = shutil.which("gio")
        need(gio is not None, "gio is required to assign/verify the Files/Nautilus executable icon")
        icon_uri = APPROVED_ICON.resolve().as_uri()

        r = run([gio, "set", "-t", "string", promoted, "metadata::custom-icon", icon_uri],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        need(r.returncode == 0, "gio could not assign approved Nougat N custom icon: " + r.stdout)

        r = run([gio, "info", "-a", "metadata::custom-icon", promoted],
                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        need(r.returncode == 0 and icon_uri in r.stdout,
             "final executable custom-icon readback did not match approved Nougat N")

        # HARD RELEASE GATE 3: exactly one versioned root executable, v46.
        roots = sorted(p.name for p in ROOT.glob("Nougat_Media_Suite_v*") if p.is_file())
        need(roots == [TARGET], "obsolete/multiple root executables survived promotion: " + repr(roots))
        built_versions = sorted(p.name for p in BUILD.glob("Nougat_Media_Suite_v*") if p.is_file())
        need(built_versions == [TARGET],
             "obsolete/multiple CMake-build executables survived: " + repr(built_versions))

        # Identity check occurs after icon assignment without rewriting the binary.
        clean_env = dict(os.environ)
        clean_env.pop("LD_LIBRARY_PATH", None)
        r = run([promoted, "--version"], env=clean_env, text=True,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        need(r.returncode == 0,
             "final root executable could not start from embedded $ORIGIN runtime: " + r.stdout.strip())
        need(r.stdout.strip() == "Nougat Media Suite v0.0.46",
             "final root executable identity failed: " + repr(r.stdout.strip()))
        print("PASS: final root v46 starts without LD_LIBRARY_PATH")

        for name in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
            text = (ROOT/name).read_text()
            need("Nougat_Media_Suite_v46" in text, f"{name} does not target v46")
            need("Icon=nougat-media-suite-concept-sheet-v24" in text, f"{name} lost approved icon key")

        print("=== v0.0.46 NATIVE BUILD + EXECUTABLE PROMOTION PASS ===")
        print("Root executable:", promoted)
        print("Root executable set:", roots)
        print("Approved icon:", icon_uri)
        print("No commit/tag/push was performed. Owner acceptance is still required.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. v0.0.46 is not accepted or promoted as a release.")
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
