#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TARGET = "Nougat_Media_Suite_v50"
GUI_TARGET = "Nougat_Installer_v50"
GUI_SOURCE = ROOT / "installer/nougat_installer_gui_v50.cpp"
APPROVED_ICON = ROOT / "assets/icons/nougat-media-suite-concept-sheet-v24.png"
APPROVED_SHA = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"
DEFAULT_BUILD = Path(tempfile.gettempdir()) / "nougat-v50-build"
INSTALLER_OUTPUT = Path.home() / "Downloads" / "Nougat_Media_Suite_v0.0.50_GRAPHICAL_INSTALLER.zip"


def run(args, *, capture=False, env=None):
    printable = " ".join(str(item) for item in args)
    print("+", printable)
    kwargs = {"text": True, "env": env}
    if capture:
        kwargs.update(stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return subprocess.run([str(item) for item in args], **kwargs)


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def clean_output(text: str) -> bool:
    lower = text.lower()
    return "warning:" not in lower and "clock skew" not in lower


def apply_patch(script: str) -> None:
    result = run([sys.executable, ROOT / "tools" / script], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, script + " failed")


def validate_source() -> None:
    result = run([sys.executable, ROOT / "tools/test_nougat_media_suite_v50.py", ROOT], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "v0.0.50 source contract failed")

    result = run([sys.executable, ROOT / "tests/v50/test_workshop_split_archive.py"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "Workshop split/reassemble regression failed")

    result = run([sys.executable, ROOT / "tests/v50/test_plugin_installer.py"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "v0.0.50 player/plugin installer regression failed")


def configure_and_build(build_dir: Path, p2p_mode: str, ai_mode: str) -> Path:
    build_dir = build_dir.resolve()
    need(build_dir != ROOT.resolve(), "build directory may not be the source root")
    need(ROOT.resolve() not in build_dir.parents, "v50 build output must stay outside the source tree")
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)

    result = run([
        "cmake", "-S", ROOT, "-B", build_dir,
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DNOUGAT_AI_MODE={ai_mode}",
        f"-DNOUGAT_P2P_MODE={p2p_mode}",
    ], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "CMake configure failed")
    need(clean_output(result.stdout), "CMake configure emitted a warning")

    result = run(["cmake", "--build", build_dir, "--target", TARGET, "-j2"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "native v0.0.50 build failed")
    need(clean_output(result.stdout), "native v0.0.50 build emitted a warning")

    built = build_dir / TARGET
    need(built.is_file() and os.access(built, os.X_OK), "built v50 executable is missing")
    return built


def build_graphical_installer(build_dir: Path) -> Path:
    need(GUI_SOURCE.is_file(), "native graphical installer source is missing")
    built = build_dir / GUI_TARGET
    result = run([
        "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-I" + str(ROOT / "src"),
        GUI_SOURCE,
        "-lX11",
        "-o", built,
    ], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "native graphical installer build failed")
    need(clean_output(result.stdout), "native graphical installer build emitted a warning")
    need(built.is_file() and os.access(built, os.X_OK), "built graphical installer executable is missing")

    result = run([built, "--version"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite Installer v0.0.50",
         "graphical installer identity mismatch")

    xvfb = shutil.which("xvfb-run")
    if xvfb:
        result = run([xvfb, "-a", built, "--window-self-test"], capture=True)
    elif os.environ.get("DISPLAY"):
        result = run([built, "--window-self-test"], capture=True)
    else:
        raise RuntimeError("graphical installer icon self-test requires xvfb-run or an active DISPLAY")
    print(result.stdout, end="")
    need(result.returncode == 0 and "PASS" in result.stdout,
         "graphical installer X11 identity/icon self-test failed")
    return built


def native_validation(built: Path) -> None:
    result = run([built, "--version"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite v0.0.50",
         "v50 executable identity mismatch: " + repr(result.stdout.strip()))

    for flag in (
        "--v49-games-self-test",
        "--v47-nav-self-test",
        "--v47-fullscreen-controls-self-test",
        "--v47-window-identity-self-test",
    ):
        result = run([built, flag], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0 and "PASS" in result.stdout,
             f"retained native self-test failed: {flag}: {result.stdout.strip()}")


def promote(built: Path) -> Path:
    promoted = ROOT / TARGET
    temporary = ROOT / (TARGET + ".candidate")
    if temporary.exists():
        temporary.unlink()
    shutil.copy2(built, temporary)
    temporary.chmod(temporary.stat().st_mode | 0o111)

    result = run([temporary, "--version"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite v0.0.50",
         "staged root executable identity mismatch")

    for candidate in ROOT.glob("Nougat_Media_Suite_v*"):
        if candidate.is_file() and candidate not in {temporary, promoted}:
            print("Removing obsolete root executable after v50 validation:", candidate.name)
            candidate.unlink()
    os.replace(temporary, promoted)

    result = run([promoted, "--version"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0 and result.stdout.strip() == "Nougat Media Suite v0.0.50",
         "promoted v50 executable identity mismatch")

    gio = shutil.which("gio")
    need(gio is not None, "gio is required for final raw-executable icon verification")
    need(APPROVED_ICON.is_file(), "approved Nougat icon is missing")
    icon_uri = APPROVED_ICON.resolve().as_uri()
    result = run([gio, "set", "-t", "string", promoted, "metadata::custom-icon", icon_uri], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "could not assign approved Nougat icon to promoted executable")
    result = run([gio, "info", "-a", "metadata::custom-icon", promoted], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0 and icon_uri in result.stdout,
         "approved Nougat icon metadata readback failed on promoted executable")
    print("PASS: final promoted executable approved-icon metadata readback")
    return promoted


def package_candidate(gui_built: Path) -> Path:
    result = run([sys.executable, ROOT / "tools/package_v50_source.py"], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "v50 source packaging failed")

    root_gui = ROOT / GUI_TARGET
    if root_gui.exists():
        root_gui.unlink()
    shutil.copy2(gui_built, root_gui)
    root_gui.chmod(root_gui.stat().st_mode | 0o111)
    try:
        result = run([
            sys.executable,
            ROOT / "tools/package_v50_installer.py",
            "--output", INSTALLER_OUTPUT,
        ], capture=True)
        print(result.stdout, end="")
        need(result.returncode == 0, "v50 graphical installer packaging failed")
    finally:
        try:
            root_gui.unlink()
        except FileNotFoundError:
            pass

    need(INSTALLER_OUTPUT.is_file(), "graphical installer ZIP was not produced")
    result = run([sys.executable, ROOT / "tests/v50/test_installer_bundle.py", INSTALLER_OUTPUT], capture=True)
    print(result.stdout, end="")
    need(result.returncode == 0, "exact graphical installer package lifecycle validation failed")
    return INSTALLER_OUTPUT


def main() -> int:
    parser = argparse.ArgumentParser(description="Build the Nougat Media Suite v0.0.50 candidate")
    parser.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD)
    parser.add_argument("--p2p", choices=["AUTO", "ENABLED", "STUB"], default="AUTO")
    parser.add_argument("--ai", choices=["AUTO", "ENABLED", "STUB"], default="AUTO")
    parser.add_argument("--package", action="store_true", help="create the canonical lean source ZIP and tested graphical installer ZIP")
    args = parser.parse_args()

    try:
        need((ROOT / "CMakeLists.txt").is_file(), "CMakeLists.txt missing")
        need(APPROVED_ICON.is_file(), "approved Nougat icon missing")
        need(sha256(APPROVED_ICON) == APPROVED_SHA, "approved Nougat icon SHA-256 changed")

        apply_patch("apply_v50_core.py")
        apply_patch("apply_v50_dialog_labels.py")
        apply_patch("apply_v50_plugin_foundation.py")
        validate_source()
        built = configure_and_build(args.build_dir, args.p2p, args.ai)
        gui_built = build_graphical_installer(args.build_dir)
        native_validation(built)
        promoted = promote(built)

        installer_package = package_candidate(gui_built) if args.package else None

        print("=== NOUGAT MEDIA SUITE v0.0.50 CANDIDATE BUILD PASS ===")
        print("Root executable:", promoted)
        print("Graphical installer executable:", gui_built)
        print("Build directory:", args.build_dir)
        print("P2P mode:", args.p2p, "| Local AI mode:", args.ai)
        if installer_package is not None:
            print("Graphical installer package:", installer_package)
            print("Graphical installer SHA-256:", sha256(installer_package))
        print("Core law: Video Player + plugin system are mandatory; only real available plugins are selectable.")
        print("This is a candidate only. No acceptance tag or main-branch merge was performed.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. v0.0.50 is not accepted.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
