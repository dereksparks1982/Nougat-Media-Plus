#!/usr/bin/env python3
from pathlib import Path
import os
import shutil

ROOT = Path(__file__).resolve().parents[1]

def exe(candidate: str):
    p = Path(candidate)
    if "/" in candidate:
        return str(p) if p.is_file() and os.access(p, os.X_OK) else None
    return shutil.which(candidate)

print("=== NOUGAT v0.0.48 GAME RUNTIME CHECK ===")

dos_candidates = []
if os.environ.get("NOUGAT_DOSBOX"):
    dos_candidates.append(os.environ["NOUGAT_DOSBOX"])
dos_candidates += [
    str(ROOT / "components/games/runtime/dosbox-staging/dosbox"),
    "dosbox-staging",
    "dosbox",
]
dos = next((found for c in dos_candidates if (found := exe(c))), None)
print("DOS:", dos or "MISSING - install DOSBox Staging/DOSBox or set NOUGAT_DOSBOX")

native_candidates = []
if os.environ.get("NOUGAT_XENIA"):
    native_candidates.append(os.environ["NOUGAT_XENIA"])
native_candidates += [
    str(ROOT / "components/games/runtime/xenia/xenia_canary"),
    str(ROOT / "components/games/runtime/xenia/xenia"),
    "xenia_canary", "xenia-canary", "xenia",
]
native = next((found for c in native_candidates if not c.lower().endswith(".exe") and (found := exe(c))), None)

win_candidates = []
env_xenia = os.environ.get("NOUGAT_XENIA")
if env_xenia and env_xenia.lower().endswith(".exe"):
    win_candidates.append(env_xenia)
win_candidates += [
    str(ROOT / "components/games/runtime/xenia/xenia_canary.exe"),
    str(ROOT / "components/games/runtime/xenia/xenia.exe"),
]
win = next((c for c in win_candidates if Path(c).is_file()), None)

runner_candidates = []
if os.environ.get("NOUGAT_XENIA_RUNNER"):
    runner_candidates.append(os.environ["NOUGAT_XENIA_RUNNER"])
runner_candidates += ["umu-run", "wine64", "wine"]
runner = next((found for c in runner_candidates if (found := exe(c))), None)

if native:
    print("Xbox 360:", native, "(native/experimental Xenia)")
elif win and runner:
    print("Xbox 360:", win, "via", runner)
elif win:
    print("Xbox 360: Xenia found, runner MISSING - set NOUGAT_XENIA_RUNNER")
else:
    print("Xbox 360: MISSING - add Xenia Canary runtime or set NOUGAT_XENIA")
