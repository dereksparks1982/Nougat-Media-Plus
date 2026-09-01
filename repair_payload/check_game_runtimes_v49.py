#!/usr/bin/env python3
from pathlib import Path
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def exe(candidate: str):
    p = Path(candidate)
    if "/" in candidate:
        return str(p) if p.is_file() and os.access(p, os.X_OK) else None
    return shutil.which(candidate)


def missing_ldd(path: str, extra_library_path: str | None = None) -> list[str]:
    env = os.environ.copy()
    if extra_library_path:
        env["LD_LIBRARY_PATH"] = extra_library_path + (":" + env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else "")
    result = subprocess.run(["ldd", path], env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        return ["ldd failed: " + result.stdout.strip()]
    return [line.strip() for line in result.stdout.splitlines() if "not found" in line]


def blastem_identity(path: Path, package: Path) -> str:
    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = str(package / "lib") + (":" + env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else "")
    env["SDL_VIDEODRIVER"] = "x11"
    env["SDL_VIDEO_DRIVER"] = "x11"
    result = subprocess.run([str(path), "-v"], cwd=package, env=env,
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
    return result.stdout.strip() if result.returncode == 0 else ""


print("=== NOUGAT v0.0.49 GAME RUNTIME CHECK ===")

dos_candidates = []
if os.environ.get("NOUGAT_DOSBOX"):
    dos_candidates.append(os.environ["NOUGAT_DOSBOX"])
dos_candidates += [
    str(ROOT / "components/games/runtime/dosbox-staging/dosbox"),
    "dosbox-staging", "dosbox",
]
dos = next((found for candidate in dos_candidates if (found := exe(candidate))), None)
print("DOS:", dos or "MISSING")

stella_candidates = [
    str(ROOT / "components/games/runtime/stella/stella"),
    "stella",
]
stella = next((found for candidate in stella_candidates if (found := exe(candidate))), None)
print("Atari 2600:", stella or "MISSING")
stella_ok = bool(stella)
if stella and "/components/games/runtime/stella/stella" in stella:
    wrapper_path = Path(stella)
    wrapper_text = wrapper_path.read_text(encoding="utf-8", errors="replace") if wrapper_path.is_file() else ""
    if "SDL_VIDEO_DRIVER=x11" not in wrapper_text:
        stella_ok = False
        print("Atari 2600 SDL3 X11 wrapper: MISSING")
    else:
        print("Atari 2600 SDL3 X11 wrapper: PASS")
    native = ROOT / "components/games/runtime/stella/package/usr/bin/stella"
    if not native.is_file():
        stella_ok = False
        print("Atari 2600 runtime payload: MISSING usr/bin/stella")
    else:
        missing = missing_ldd(str(native))
        if missing:
            stella_ok = False
        print("Atari 2600 shared libraries:", "PASS" if not missing else "MISSING: " + " | ".join(missing))

blastem_candidates = [
    str(ROOT / "components/games/runtime/blastem/blastem"),
    "blastem",
]
blastem = next((found for candidate in blastem_candidates if (found := exe(candidate))), None)
print("Sega Genesis/Master System/Game Gear:", blastem or "MISSING")
blastem_ok = bool(blastem)
if blastem and "/components/games/runtime/blastem/blastem" in blastem:
    package = ROOT / "components/games/runtime/blastem/package"
    native = package / "blastem"
    if not native.is_file():
        blastem_ok = False
        print("Sega runtime payload: MISSING package/blastem")
    else:
        identity = blastem_identity(native, package)
        identity_ok = "0.6.3-pre" in identity and "8013468ed981" in identity
        missing = missing_ldd(str(native), str(package / "lib"))
        if not identity_ok or missing:
            blastem_ok = False
        print("Sega runtime identity:", identity if identity else "FAILED")
        print("Sega shared libraries:", "PASS" if not missing else "MISSING: " + " | ".join(missing))

native_candidates = []
if os.environ.get("NOUGAT_XENIA"):
    native_candidates.append(os.environ["NOUGAT_XENIA"])
native_candidates += [
    str(ROOT / "components/games/runtime/xenia/xenia_canary"),
    str(ROOT / "components/games/runtime/xenia/xenia"),
    "xenia_canary", "xenia-canary", "xenia",
]
native = next((found for candidate in native_candidates if not candidate.lower().endswith(".exe") and (found := exe(candidate))), None)
print("Xbox 360:", native or "MISSING")

atari800 = exe(str(ROOT / "components/games/runtime/atari800/AppRun")) or exe("atari800")
print("Atari 5200/8-bit:", atari800 or "MISSING")

if not dos or not stella_ok or not blastem_ok:
    raise SystemExit(1)
