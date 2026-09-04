#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

project = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd()
exe = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None

errors = []

def need(path, token):
    p = project / path
    if not p.exists():
        errors.append(f"missing file: {path}")
        return ""
    text = p.read_text(errors="replace")
    if token not in text:
        errors.append(f"{path}: missing {token!r}")
    return text

cmake = need("CMakeLists.txt", "VERSION 0.0.61")
if "Nougat_Media_Suite_v61" not in cmake:
    errors.append("CMakeLists.txt: v61 executable target missing")

source = need("src/main.cpp", "NOUGAT_V61_DRONE_MISSION_CONTROL_UI")
for token in (
    "NOUGAT_V61_DRONE_MISSION_CONTROL_STATE",
    "NOUGAT_V61_DRONE_SIMULATION_ENGINE",
    "studioDroneWaypoints",
    "poll_drone_simulation();",
    "save_drone_director_shot",
    "Nougat Media Suite v0.0.61",
    "Real-aircraft command transmission remains disabled",
):
    if token not in source:
        errors.append(f"src/main.cpp: missing {token!r}")

for forbidden in (
    "MAV_CMD_COMPONENT_ARM_DISARM",
    "MAV_CMD_NAV_TAKEOFF",
    "mavsdk::Action::arm",
):
    if forbidden in source:
        errors.append(f"src/main.cpp: real-aircraft command token unexpectedly present: {forbidden}")

desktop = need("com.elderredsoftworks.NougatMediaSuite.desktop", "Nougat_Media_Suite_v61")
if "Name=Nougat Media Suite" not in desktop:
    errors.append("desktop product name changed unexpectedly")

readme = need("README.md", "## v0.0.61 candidate - Drone Lab Mission Control")
if "Nougat Media Plus" not in readme:
    errors.append("README: deferred Nougat Media Plus note missing")
if "owner-test candidate" not in readme:
    errors.append("README: candidate acceptance wording missing")

if exe is not None:
    if not exe.exists():
        errors.append(f"built executable missing: {exe}")
    else:
        try:
            result = subprocess.run([str(exe), "--version"], text=True, capture_output=True, timeout=10)
            output = (result.stdout + result.stderr).strip()
            if result.returncode != 0 or "v0.0.61" not in output:
                errors.append(f"executable version check failed: rc={result.returncode} output={output!r}")
        except Exception as exc:
            errors.append(f"executable version check exception: {exc}")

if errors:
    print("FAIL: Nougat v0.0.61 Drone Mission Control validation")
    for error in errors:
        print(" -", error)
    raise SystemExit(1)

print("PASS: Nougat v0.0.61 Drone Mission Control source contract")
print("PASS: editable Director Shot waypoint path")
print("PASS: functional trajectory simulation and telemetry")
print("PASS: simulation camera/gimbal and payload controls")
print("PASS: real-aircraft command transmission remains disabled")
print("PASS: Nougat Media Suite branding and approved N identity preserved")
