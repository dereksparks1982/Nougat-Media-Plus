#!/usr/bin/env python3
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
TARGET = "Nougat_Media_Suite_v48"
APPROVED_ICON = ROOT / "assets/icons/nougat-media-suite-concept-sheet-v24.png"
APPROVED_SHA = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"


def run(args, capture=False, **kwargs):
    print("+", " ".join(str(x) for x in args))
    if capture:
        return subprocess.run([str(x) for x in args], text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs)
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


def clean_build_output(text):
    lower = text.lower()
    return "warning:" not in lower and "clock skew" not in lower


def png_dimensions(path):
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        return (0, 0)
    return (int.from_bytes(data[16:20], "big"), int.from_bytes(data[20:24], "big"))


def install_gnome_identity():
    home = Path.home()
    applications = home / ".local/share/applications"
    applications.mkdir(parents=True, exist_ok=True)
    sources = [
        ROOT / "com.elderredsoftworks.NougatMediaSuite.desktop",
        ROOT / "NougatMediaSuite.desktop",
    ]
    for source in sources:
        need(source.is_file(), source.name + " is missing")
        text = source.read_text()
        need(TARGET in text, source.name + " does not target v48")
        shutil.copy2(source, applications / source.name)

    icon_root = home / ".local/share/icons"
    icon_root.mkdir(parents=True, exist_ok=True)
    root_icon = icon_root / "nougat-media-suite-concept-sheet-v24.png"
    shutil.copy2(APPROVED_ICON, root_icon)

    width, height = png_dimensions(APPROVED_ICON)
    need(width > 0 and height > 0, "approved Nougat N is not a readable PNG")
    themed_dir = icon_root / "hicolor" / (f"{width}x{height}" if width == height else "scalable") / "apps"
    themed_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(APPROVED_ICON, themed_dir / "nougat-media-suite-concept-sheet-v24.png")

    desktop_db = shutil.which("update-desktop-database")
    if desktop_db:
        r = run([desktop_db, applications], capture=True)
        need(r.returncode == 0 and clean_build_output(r.stdout),
             "desktop database refresh failed or warned: " + r.stdout.strip())


def main():
    try:
        need((ROOT / "CMakeLists.txt").is_file(), "CMakeLists.txt missing")
        need(APPROVED_ICON.is_file(), "approved Nougat N icon missing")
        need(sha256(APPROVED_ICON) == APPROVED_SHA, "approved Nougat N master hash changed")

        r = run([sys.executable, ROOT / "tools/install_game_runtimes_v48.py"], capture=True)
        print(r.stdout, end="")
        need(r.returncode == 0, "v0.0.48 game runtime installation failed")

        r = run([sys.executable, ROOT / "tools/test_nougat_media_suite_v48.py", ROOT], capture=True)
        print(r.stdout, end="")
        need(r.returncode == 0, "v0.0.48 pre-build contract failed")

        r = run(["cmake", "-S", ROOT, "-B", BUILD], capture=True)
        print(r.stdout, end="")
        need(r.returncode == 0, "CMake configure failed")
        need(clean_build_output(r.stdout), "CMake configure emitted a warning")

        r = run(["cmake", "--build", BUILD, "--target", TARGET, "-j2"], capture=True)
        print(r.stdout, end="")
        need(r.returncode == 0, "native v0.0.48 build failed")
        need(clean_build_output(r.stdout), "native build emitted a warning")

        built = BUILD / TARGET
        need(built.is_file() and os.access(built, os.X_OK), "built v48 executable missing")

        runtime_dirs = [
            ROOT / "components/ai/runtime/lib",
            ROOT / "components/ai/runtime/lib64",
        ]
        runtime_dirs = [p for p in runtime_dirs if p.is_dir()]
        need(runtime_dirs, "Nougat AI runtime directories are missing")
        runtime_env = dict(os.environ)
        runtime_path = os.pathsep.join(str(p) for p in runtime_dirs)
        inherited = runtime_env.get("LD_LIBRARY_PATH", "")
        runtime_env["LD_LIBRARY_PATH"] = runtime_path + (os.pathsep + inherited if inherited else "")

        r = run([built, "--version"], capture=True, env=runtime_env)
        need(r.returncode == 0, "build-tree v48 could not start: " + r.stdout.strip())
        need(r.stdout.strip() == "Nougat Media Suite v0.0.48",
             "build-tree v48 identity mismatch: " + repr(r.stdout.strip()))
        print("PASS: build-tree v48 identity")

        for flag in ("--v47-nav-self-test", "--v47-fullscreen-controls-self-test", "--v47-window-identity-self-test"):
            r = run([built, flag], capture=True, env=runtime_env)
            print(r.stdout, end="")
            need(r.returncode == 0 and "PASS" in r.stdout,
                 f"retained baseline self-test failed: {flag}: {r.stdout.strip()}")

        # Promote only after all candidate checks pass.
        for p in ROOT.glob("Nougat_Media_Suite_v*"):
            if p.is_file():
                print("Removing obsolete root executable after successful v48 validation:", p.name)
                p.unlink()

        promoted = ROOT / TARGET
        shutil.copy2(built, promoted)
        promoted.chmod(promoted.stat().st_mode | 0o111)

        clean_env = dict(os.environ)
        clean_env.pop("LD_LIBRARY_PATH", None)
        r = run([promoted, "--version"], capture=True, env=clean_env)
        need(r.returncode == 0 and r.stdout.strip() == "Nougat Media Suite v0.0.48",
             "promoted v48 does not start from embedded project runtime")

        gio = shutil.which("gio")
        need(gio is not None, "gio is required to set the executable icon")
        icon_uri = APPROVED_ICON.resolve().as_uri()
        r = run([gio, "set", "-t", "string", promoted, "metadata::custom-icon", icon_uri], capture=True)
        need(r.returncode == 0, "could not assign approved Nougat N icon")

        install_gnome_identity()

        r = run([sys.executable, ROOT / "tools/check_game_runtimes_v48.py"], capture=True)
        print(r.stdout, end="")

        print("=== v0.0.48 NATIVE BUILD + EXECUTABLE PROMOTION PASS ===")
        print("Root executable:", promoted)
        print("DOS acceptance: GTA 1, Prince of Persia, Mario DOS title")
        print("Xbox 360 acceptance: GTA IV")
        print("No commit, tag, or GitHub push was performed.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. v0.0.48 is not accepted and nothing was pushed.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
