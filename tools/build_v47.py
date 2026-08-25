#!/usr/bin/env python3
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
TARGET = "Nougat_Media_Suite_v47"
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
    applications = home/".local/share/applications"
    applications.mkdir(parents=True, exist_ok=True)
    canonical_source = ROOT/"com.elderredsoftworks.NougatMediaSuite.desktop"
    legacy_source = ROOT/"NougatMediaSuite.desktop"
    need(canonical_source.is_file() and legacy_source.is_file(), "Nougat desktop launchers are missing")

    canonical_target = applications/canonical_source.name
    legacy_target = applications/legacy_source.name
    shutil.copy2(canonical_source, canonical_target)
    shutil.copy2(legacy_source, legacy_target)

    icon_root = home/".local/share/icons"
    icon_root.mkdir(parents=True, exist_ok=True)
    root_icon = icon_root/"nougat-media-suite-concept-sheet-v24.png"
    shutil.copy2(APPROVED_ICON, root_icon)

    width, height = png_dimensions(APPROVED_ICON)
    need(width > 0 and height > 0, "approved Nougat N is not a readable PNG")
    if width == height:
        themed_dir = icon_root/"hicolor"/(f"{width}x{height}")/"apps"
    else:
        themed_dir = icon_root/"hicolor"/"scalable"/"apps"
    themed_dir.mkdir(parents=True, exist_ok=True)
    themed_icon = themed_dir/"nougat-media-suite-concept-sheet-v24.png"
    shutil.copy2(APPROVED_ICON, themed_icon)

    desktop_db = shutil.which("update-desktop-database")
    if desktop_db:
        r = run([desktop_db, applications], capture=True)
        need(r.returncode == 0 and clean_build_output(r.stdout),
             "desktop database refresh failed or warned: " + r.stdout.strip())
    icon_cache = shutil.which("gtk-update-icon-cache")
    hicolor = icon_root/"hicolor"
    # A user-local hicolor directory does not always carry index.theme. GTK can
    # still find the unthemed XDG icon copy above; only refresh a real theme.
    if icon_cache and (hicolor/"index.theme").is_file():
        r = run([icon_cache, "-f", "-t", hicolor], capture=True)
        need(r.returncode == 0 and clean_build_output(r.stdout),
             "GNOME icon cache refresh failed or warned: " + r.stdout.strip())

    for launcher in (canonical_target, legacy_target):
        text = launcher.read_text()
        need("Nougat_Media_Suite_v47" in text, launcher.name + " does not launch v47")
        need("Icon=nougat-media-suite-concept-sheet-v24" in text,
             launcher.name + " does not use the approved N icon key")
        need("StartupWMClass=NougatMediaSuite" in text,
             launcher.name + " lost StartupWMClass")
        need("X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite" in text,
             launcher.name + " lost GNOME application ID")
        need("X-GNOME-WMClass=NougatMediaSuite" in text,
             launcher.name + " lost GNOME WM class")
        need("StartupNotify=true" in text,
             launcher.name + " lost StartupNotify")
    need(sha256(root_icon) == APPROVED_SHA and sha256(themed_icon) == APPROVED_SHA,
         "installed GNOME N icon bytes do not match approved master")
    print("PASS: GNOME launcher and approved Nougat N icon installed and verified")

def main():
    try:
        need((ROOT/"CMakeLists.txt").is_file(), "CMakeLists.txt missing")
        need(APPROVED_ICON.is_file(), "approved Nougat N icon missing")
        need(sha256(APPROVED_ICON) == APPROVED_SHA, "approved Nougat N master hash changed")
        need("/build/" in (ROOT/".gitignore").read_text(), "/build/ is not ignored")

        for base in (ROOT, BUILD):
            if base.is_dir():
                for p in base.glob("Nougat_Media_Suite_v*"):
                    if p.is_file():
                        print("Removing stale executable before build:", p)
                        p.unlink()

        r = run(["cmake", "-S", ROOT, "-B", BUILD], capture=True)
        print(r.stdout, end="")
        need(r.returncode == 0, "CMake configure failed")
        need(clean_build_output(r.stdout), "CMake configure emitted a warning")

        r = run(["cmake", "--build", BUILD, "-j2"], capture=True)
        print(r.stdout, end="")
        need(r.returncode == 0, "native v0.0.47 build failed")
        need(clean_build_output(r.stdout), "native build emitted a warning")

        built = BUILD/TARGET
        need(built.is_file() and os.access(built, os.X_OK), "built v47 executable missing")

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

        r = run([built, "--version"], capture=True, env=runtime_env)
        need(r.returncode == 0,
             "build-tree v47 could not start with Nougat AI runtime: " + r.stdout.strip())
        need(r.stdout.strip() == "Nougat Media Suite v0.0.47",
             "build-tree v47 identity mismatch: " + repr(r.stdout.strip()))
        print("PASS: build-tree v47 starts with project AI runtime")

        r = run([sys.executable, ROOT/"tools/test_nougat_media_suite_v47.py", ROOT, built],
                env=runtime_env)
        need(r.returncode == 0, "v0.0.47 contract validation failed")

        secure_v47 = ROOT/"tools/test_nougat_secure_search_v47.py"
        need(secure_v47.is_file(), "v47 Secure Search retention validator is missing")
        r = run([sys.executable, secure_v47, ROOT], env=runtime_env)
        need(r.returncode == 0, "v0.0.47 Secure Search retention validation failed")

        for p in ROOT.glob("Nougat_Media_Suite_v*"):
            if p.is_file():
                print("Removing obsolete root executable:", p.name)
                p.unlink()

        promoted = ROOT/TARGET
        shutil.copy2(built, promoted)
        promoted.chmod(promoted.stat().st_mode | 0o111)

        gio = shutil.which("gio")
        need(gio is not None, "gio is required to assign/verify the Files/Nautilus executable icon")
        icon_uri = APPROVED_ICON.resolve().as_uri()

        r = run([gio, "set", "-t", "string", promoted, "metadata::custom-icon", icon_uri],
                capture=True)
        need(r.returncode == 0, "gio could not assign approved Nougat N custom icon: " + r.stdout)

        r = run([gio, "info", "-a", "metadata::custom-icon", promoted], capture=True)
        need(r.returncode == 0 and icon_uri in r.stdout,
             "final executable custom-icon readback did not match approved Nougat N")

        roots = sorted(p.name for p in ROOT.glob("Nougat_Media_Suite_v*") if p.is_file())
        need(roots == [TARGET], "obsolete/multiple root executables survived promotion: " + repr(roots))
        built_versions = sorted(p.name for p in BUILD.glob("Nougat_Media_Suite_v*") if p.is_file())
        need(built_versions == [TARGET],
             "obsolete/multiple CMake-build executables survived: " + repr(built_versions))

        clean_env = dict(os.environ)
        clean_env.pop("LD_LIBRARY_PATH", None)
        r = run([promoted, "--version"], capture=True, env=clean_env)
        need(r.returncode == 0,
             "final root executable could not start from embedded $ORIGIN runtime: " + r.stdout.strip())
        need(r.stdout.strip() == "Nougat Media Suite v0.0.47",
             "final root executable identity failed: " + repr(r.stdout.strip()))
        print("PASS: final root v47 starts without LD_LIBRARY_PATH")

        r = run([promoted, "--v47-nav-self-test"], capture=True, env=clean_env)
        need(r.returncode == 0 and "PASS" in r.stdout,
             "System-tab reachability self-test failed: " + r.stdout.strip())
        print(r.stdout.strip())

        r = run([promoted, "--v47-fullscreen-controls-self-test"], capture=True, env=clean_env)
        need(r.returncode == 0 and "PASS" in r.stdout,
             "fullscreen transport self-test failed: " + r.stdout.strip())
        print(r.stdout.strip())

        install_gnome_identity()

        r = run([promoted, "--v47-window-identity-self-test"], capture=True, env=clean_env)
        need(r.returncode == 0 and "PASS" in r.stdout,
             "GNOME/X11 window identity self-test failed: " + r.stdout.strip())
        print(r.stdout.strip())

        for name in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
            text = (ROOT/name).read_text()
            need("Nougat_Media_Suite_v47" in text, f"{name} does not target v47")
            need("Icon=nougat-media-suite-concept-sheet-v24" in text,
                 f"{name} lost approved icon key")

        print("=== v0.0.47 NATIVE BUILD + EXECUTABLE PROMOTION PASS ===")
        print("Root executable:", promoted)
        print("Root executable set:", roots)
        print("Approved icon:", icon_uri)
        print("No commit/tag/push was performed. Owner acceptance is still required.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. v0.0.47 is not accepted or promoted as a release.")
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
