#!/usr/bin/env python3
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
TARGET = "Nougat_Media_Suite_v50"
PREVIOUS_TARGET = "Nougat_Media_Suite_v49"
APPROVED_ICON = ROOT / "assets/icons/nougat-media-suite-concept-sheet-v24.png"
APPROVED_SHA = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"


def run(args, capture=False, **kwargs):
    print("+", " ".join(str(x) for x in args))
    command = [str(x) for x in args]
    if capture:
        return subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs)
    return subprocess.run(command, **kwargs)


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def clean_output(text):
    lower = text.lower()
    return "warning:" not in lower and "clock skew" not in lower


def png_dimensions(path):
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        return (0, 0)
    return int.from_bytes(data[16:20], "big"), int.from_bytes(data[20:24], "big")


def install_gnome_identity():
    home = Path.home()
    apps = home / ".local/share/applications"
    apps.mkdir(parents=True, exist_ok=True)
    for source in (ROOT / "com.elderredsoftworks.NougatMediaSuite.desktop", ROOT / "NougatMediaSuite.desktop"):
        need(source.is_file(), source.name + " is missing")
        body = source.read_text(encoding="utf-8")
        need(TARGET in body, source.name + " does not target v50")
        installed = apps / source.name
        shutil.copy2(source, installed)
        need(TARGET in installed.read_text(encoding="utf-8"), installed.name + " installed launcher identity mismatch")

    icon_root = home / ".local/share/icons"
    icon_root.mkdir(parents=True, exist_ok=True)
    icon_copy = icon_root / "nougat-media-suite-concept-sheet-v24.png"
    shutil.copy2(APPROVED_ICON, icon_copy)
    need(sha256(icon_copy) == APPROVED_SHA, "user icon copy does not match approved Nougat N")

    width, height = png_dimensions(APPROVED_ICON)
    need(width > 0 and height > 0, "approved Nougat N is not a readable PNG")
    themed = icon_root / "hicolor" / (f"{width}x{height}" if width == height else "scalable") / "apps"
    themed.mkdir(parents=True, exist_ok=True)
    themed_copy = themed / "nougat-media-suite-concept-sheet-v24.png"
    shutil.copy2(APPROVED_ICON, themed_copy)
    need(sha256(themed_copy) == APPROVED_SHA, "themed icon copy does not match approved Nougat N")

    desktop_db = shutil.which("update-desktop-database")
    if desktop_db:
        result = run([desktop_db, apps], capture=True)
        need(result.returncode == 0 and clean_output(result.stdout),
             "desktop database refresh failed or warned: " + result.stdout.strip())


def runtime_environment():
    directories = [ROOT / "components/ai/runtime/lib", ROOT / "components/ai/runtime/lib64"]
    directories = [path for path in directories if path.is_dir()]
    need(directories, "Nougat AI runtime directories are missing")
    env = dict(os.environ)
    runtime_path = os.pathsep.join(str(path) for path in directories)
    inherited = env.get("LD_LIBRARY_PATH", "")
    env["LD_LIBRARY_PATH"] = runtime_path + (os.pathsep + inherited if inherited else "")
    return env


def run_python_test(relative):
    result = run([sys.executable, ROOT / relative, ROOT], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, relative + " failed")


def game_runtime_snapshot():
    checker = ROOT / "tools/check_game_runtimes_v49.py"
    if not checker.is_file():
        return None
    result = run([sys.executable, checker], capture=True)
    print(result.stdout, end="")
    return result.returncode, result.stdout


def verify_game_runtime_not_regressed(before):
    if before is None:
        return
    checker = ROOT / "tools/check_game_runtimes_v49.py"
    result = run([sys.executable, checker], capture=True)
    print(result.stdout, end="")
    before_rc, before_output = before
    if before_rc == 0:
        need(result.returncode == 0, "retained v49 game runtimes regressed during v50 build")
        return
    need(result.returncode == before_rc and result.stdout == before_output,
         "retained v49 game-runtime state changed during v50 build")
    print("INFO: Retained v49 emulator runtime gaps were already present before v50 and are unchanged.")
    print("INFO: Remaining emulator/runtime completion is assigned to v0.0.51 and does not invalidate v0.0.50.")


def main():
    try:
        need((ROOT / "CMakeLists.txt").is_file(), "CMakeLists.txt missing")
        need(APPROVED_ICON.is_file(), "approved Nougat N icon missing")
        need(sha256(APPROVED_ICON) == APPROVED_SHA, "approved Nougat N master hash changed")
        for command, package in (("hdhomerun_config", "hdhomerun-config"), ("curl", "curl"),
                                 ("zenity", "zenity"), ("python3", "python3")):
            need(shutil.which(command) is not None,
                 f"{command} is required for v0.0.50. Install Ubuntu package: {package}")

        previous = ROOT / PREVIOUS_TARGET
        previous_sha = sha256(previous) if previous.is_file() else None
        if previous_sha:
            print("Retaining accepted v0.0.49 executable until owner acceptance:", previous)
            print("v0.0.49 executable SHA-256:", previous_sha)
        else:
            print("INFO: accepted v0.0.49 root executable is not present; v50 build will not delete any prior executable.")

        print("=== RETAINED GAME RUNTIME BASELINE ===")
        game_runtime_before = game_runtime_snapshot()

        run_python_test("tools/test_nougat_file_splitter_v50.py")
        run_python_test("tools/test_hdhomerun_provider_v50.py")
        run_python_test("tools/test_nougat_media_suite_v50.py")
        run_python_test("tools/test_license_protection_v22.py")

        result = run(["cmake", "-S", ROOT, "-B", BUILD], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "CMake configure failed")
        need(clean_output(result.stdout), "CMake configure emitted a warning")

        result = run(["cmake", "--build", BUILD, "--target", TARGET, "-j2"], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "native v0.0.50 build failed")
        need(clean_output(result.stdout), "native build emitted a warning")

        built = BUILD / TARGET
        need(built.is_file() and os.access(built, os.X_OK), "built v50 executable missing")
        env = runtime_environment()
        result = run([built, "--version"], capture=True, env=env)
        print(result.stdout, end="")
        need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite v0.0.50",
             "build-tree v50 identity mismatch: " + repr(result.stdout.strip()))
        print("PASS: build-tree v50 identity")

        for flag in ("--v49-games-self-test", "--v47-nav-self-test", "--v47-fullscreen-controls-self-test",
                     "--v47-window-identity-self-test"):
            result = run([built, flag], capture=True, env=env)
            print(result.stdout, end="")
            need(result.returncode == 0 and "PASS" in result.stdout,
                 f"retained baseline self-test failed: {flag}: {result.stdout.strip()}")

        print("=== RETAINED GAME RUNTIME POST-BUILD COMPARISON ===")
        verify_game_runtime_not_regressed(game_runtime_before)

        # Promote only after every source/build/regression gate has passed. Never delete v49 here.
        promoted = ROOT / TARGET
        shutil.copy2(built, promoted)
        promoted.chmod(promoted.stat().st_mode | 0o111)
        need(promoted.is_file() and os.access(promoted, os.X_OK), "root v50 executable promotion failed")

        clean_env = dict(os.environ)
        clean_env.pop("LD_LIBRARY_PATH", None)
        result = run([promoted, "--version"], capture=True, env=clean_env)
        print(result.stdout, end="")
        need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite v0.0.50",
             "promoted v50 does not start from embedded project runtime")

        gio = shutil.which("gio")
        need(gio is not None, "gio is required to set and verify the executable icon")
        expected_icon_uri = APPROVED_ICON.resolve().as_uri()
        result = run([gio, "set", "-t", "string", promoted, "metadata::custom-icon", expected_icon_uri], capture=True)
        need(result.returncode == 0, "could not assign approved Nougat N icon")
        result = run([gio, "info", "-a", "metadata::custom-icon", promoted], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0 and expected_icon_uri in result.stdout,
             "approved Nougat N executable icon readback failed")
        print("PASS: root executable N icon metadata readback")

        install_gnome_identity()

        if previous_sha is not None:
            need(previous.is_file() and sha256(previous) == previous_sha,
                 "accepted v0.0.49 executable changed during v50 build")
            print("PASS: accepted v0.0.49 executable preserved byte-for-byte")

        print("=== v0.0.50 NATIVE BUILD + EXECUTABLE PROMOTION PASS ===")
        print("Root executable:", promoted)
        print("SHA-256:", sha256(promoted))
        print("Owner tests required: Studio split/reassemble; WinTV scan/watch; FLEX discovery; FLEX playback; FLEX watch + second-tuner scan.")
        print("v0.0.51 is assigned to remaining emulator support plus LAN Web Viewer scaffolding.")
        print("NO GIT COMMIT, TAG, OR GITHUB PUSH WAS PERFORMED.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. v0.0.50 is not accepted and nothing was pushed.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
