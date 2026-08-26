#!/usr/bin/env python3
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
TARGET = "Nougat_Media_Suite_v49"
APPROVED_ICON = ROOT / "assets/icons/nougat-media-suite-concept-sheet-v24.png"
APPROVED_SHA = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"


def run(args, capture=False, **kwargs):
    print("+", " ".join(str(item) for item in args))
    if capture:
        return subprocess.run([str(item) for item in args], text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs)
    return subprocess.run([str(item) for item in args], **kwargs)


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
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
        text = source.read_text(encoding="utf-8")
        need(TARGET in text, source.name + " does not target v49")
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
        result = run([desktop_db, applications], capture=True)
        need(result.returncode == 0 and clean_build_output(result.stdout),
             "desktop database refresh failed or warned: " + result.stdout.strip())


def runtime_environment():
    runtime_dirs = [
        ROOT / "components/ai/runtime/lib",
        ROOT / "components/ai/runtime/lib64",
    ]
    runtime_dirs = [path for path in runtime_dirs if path.is_dir()]
    need(runtime_dirs, "Nougat AI runtime directories are missing")
    env = dict(os.environ)
    runtime_path = os.pathsep.join(str(path) for path in runtime_dirs)
    inherited = env.get("LD_LIBRARY_PATH", "")
    env["LD_LIBRARY_PATH"] = runtime_path + (os.pathsep + inherited if inherited else "")
    return env


def main():
    try:
        need((ROOT / "CMakeLists.txt").is_file(), "CMakeLists.txt missing")
        need(APPROVED_ICON.is_file(), "approved Nougat N icon missing")
        need(sha256(APPROVED_ICON) == APPROVED_SHA, "approved Nougat N master hash changed")

        result = run([sys.executable, ROOT / "tools/apply_v49_games_repair.py"], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "v0.0.49 repair application failed")

        result = run([sys.executable, ROOT / "tools/install_game_runtimes_v49.py"], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "v0.0.49 game runtime installation failed")

        result = run([sys.executable, ROOT / "tools/test_nougat_media_suite_v49.py", ROOT], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "v0.0.49 pre-build contract failed")

        result = run(["cmake", "-S", ROOT, "-B", BUILD], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "CMake configure failed")
        need(clean_build_output(result.stdout), "CMake configure emitted a warning")

        result = run(["cmake", "--build", BUILD, "--target", TARGET, "-j2"], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "native v0.0.49 build failed")
        need(clean_build_output(result.stdout), "native build emitted a warning")

        built = BUILD / TARGET
        need(built.is_file() and os.access(built, os.X_OK), "built v49 executable missing")
        env = runtime_environment()

        result = run([built, "--version"], capture=True, env=env)
        print(result.stdout, end="")
        need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite v0.0.49",
             "build-tree v49 identity mismatch: " + repr(result.stdout.strip()))
        print("PASS: build-tree v49 identity")

        result = run([built, "--v49-games-self-test"], capture=True, env=env)
        print(result.stdout, end="")
        need(result.returncode == 0 and "PASS" in result.stdout,
             "v0.0.49 Games preference native self-test failed")

        for flag in ("--v47-nav-self-test", "--v47-fullscreen-controls-self-test", "--v47-window-identity-self-test"):
            result = run([built, flag], capture=True, env=env)
            print(result.stdout, end="")
            need(result.returncode == 0 and "PASS" in result.stdout,
                 f"retained baseline self-test failed: {flag}: {result.stdout.strip()}")

        # Promote only after every compile/static/native contract has passed.
        for candidate in ROOT.glob("Nougat_Media_Suite_v*"):
            if candidate.is_file():
                print("Removing obsolete root executable after successful v49 validation:", candidate.name)
                candidate.unlink()

        promoted = ROOT / TARGET
        shutil.copy2(built, promoted)
        promoted.chmod(promoted.stat().st_mode | 0o111)

        clean_env = dict(os.environ)
        clean_env.pop("LD_LIBRARY_PATH", None)
        result = run([promoted, "--version"], capture=True, env=clean_env)
        print(result.stdout, end="")
        need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite v0.0.49",
             "promoted v49 does not start from embedded project runtime")

        gio = shutil.which("gio")
        need(gio is not None, "gio is required to set the executable icon")
        icon_uri = APPROVED_ICON.resolve().as_uri()
        result = run([gio, "set", "-t", "string", promoted, "metadata::custom-icon", icon_uri], capture=True)
        need(result.returncode == 0, "could not assign approved Nougat N icon")

        install_gnome_identity()

        result = run([sys.executable, ROOT / "tools/check_game_runtimes_v49.py"], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "v0.0.49 required game runtime check failed")

        print("=== v0.0.49 NATIVE BUILD + EXECUTABLE PROMOTION PASS ===")
        print("Root executable:", promoted)
        print("Owner runtime tests requested: Atari 2600 embedded play/options, Atari artwork, dedupe preference, Games wheel/drag responsiveness.")
        print("NES and SNES Mesen behavior remains the presentation baseline.")
        print("NO GIT COMMIT, TAG, OR GITHUB PUSH WAS PERFORMED.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. v0.0.49 is not accepted and nothing was pushed.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
