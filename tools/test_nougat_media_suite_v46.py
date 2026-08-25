#!/usr/bin/env python3
from pathlib import Path
import hashlib
import os
import subprocess
import sys

root = Path(sys.argv[1]).resolve()
exe = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None

def need(ok, msg):
    if not ok:
        raise SystemExit("FAIL: " + msg)

def read(rel):
    p = root/rel
    need(p.is_file(), "missing " + rel)
    return p.read_text(errors="replace")

def sha(rel):
    return hashlib.sha256((root/rel).read_bytes()).hexdigest()

main = read("src/main.cpp")
cmake = read("CMakeLists.txt")
worker = read("components/security/nougat_security_worker.py")
readme = read("README.md")
gitignore = read(".gitignore")
scanner = read("src/security/scanner_process.cpp")
lan_h = read("src/lan/lan_media_service.hpp")
lan_cpp = read("src/lan/lan_media_service.cpp")

need("VERSION 0.0.46" in cmake and "Nougat_Media_Suite_v46" in cmake, "v46 CMake identity missing")
need('const std::string versionLabel = "v0.0.46";' in main, "v46 top-bar identity missing")

need("ViewMode::LiveTV || currentView == ViewMode::WorldTV" in main,
     "World TV video-window ownership repair missing")
need("worldTvStartupDeadlineMs" in main and "worldTvPlaybackVerified" in main,
     "World TV verified-start gate missing")

need("https://www.youtube.com/live/Cm1v4bteXbI" in main,
     "verified official KSA Qur'an TV Kaaba source missing")
need("kaabaBadgeLastClickMs" in main and "open_kaaba_live()" in main,
     "Nougat N Kaaba double-click wiring missing")

need('securityBusyForButton?"Stop Scan":"Scan Again"' in main, "Stop Scan UI state missing")
need("securityProcess->cancel()" in main, "scanner cancellation wiring missing")
need("::kill(-pgid, SIGTERM)" in scanner and "::kill(-pgid, SIGKILL)" in scanner,
     "scanner process-group termination missing")
need("::setpgid(0, 0)" in scanner and "::setpgid(child, child)" in scanner, "scanner worker does not own a dedicated process group")
need('"SCAN CANCELLED"' in main and "Partial results" in main,
     "cancelled partial-result reporting missing")
need('" files | Threats " << detections << " | Suspicious " << suspicious' in main,
     "separate live Threat/Suspicious counters missing")

need("bulk_clamav_scan" in worker and "--file-list=" in worker,
     "bulk ClamAV collection pass missing")
need("scan_one_bulk" in worker and "Skipped by bulk fast-pass policy" in worker,
     "tiered fast/deep scanner policy missing")
need('PROGRESS={scanned}|{total}|{threats}|{suspicious}|' in worker,
     "separate progress protocol missing")

need("lanMedia.prepare();" in main, "LAN foundation not wired")
for token in ("/nougat/v1/catalog", "/nougat/v1/history", "/nougat/v1/media",
              "/nougat/v1/hls", "/nougat/v1/pair", "nougat.local"):
    need(token in lan_h or token in lan_cpp, "LAN contract missing " + token)
need("automatic_upnp = false" in lan_h, "automatic UPnP is not fail-closed")
need("automatic_cloud_relay = false" in lan_h, "cloud relay is not fail-closed")
need("cloud_login_required = false" in lan_h, "LAN foundation incorrectly requires cloud login")

need("/build/" in gitignore, "build directory is not ignored")
need(readme.startswith("# Nougat Media Suite\n\n**Nougat Media Suite** is a native Linux media center"),
     "README full suite introduction is not first")
p46 = readme.find("## v0.0.46 -")
p45 = readme.find("## v0.0.45 -")
need(p46 > 0 and p45 > p46, "README order must be intro -> current version -> older versions")

need(sha("assets/icons/nougat-media-suite-concept-sheet-v24.png") ==
     "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804",
     "approved Nougat N master changed")
for desktop in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
    d = read(desktop)
    need("Nougat_Media_Suite_v46" in d, desktop + " does not target v46")
    need("Icon=nougat-media-suite-concept-sheet-v24" in d, desktop + " lost approved N icon key")

secure = read("src/search/secure_search.cpp")
policy = read("src/privacy/privacy_policy.cpp")
need("Secure remote search unavailable" in secure, "Secure Search fail-closed behavior missing")
need("fallback" in policy.lower(), "Secure Search privacy policy unexpectedly lost fallback controls")

r = subprocess.run([sys.executable, "-m", "py_compile",
                    str(root/"components/security/nougat_security_worker.py")])
need(r.returncode == 0, "security worker Python syntax failed")

if exe is not None:
    need(exe.is_file() and os.access(exe, os.X_OK), "v46 executable missing/not executable")
    r = subprocess.run([str(exe), "--version"], text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(r.returncode == 0 and r.stdout.strip() == "Nougat Media Suite v0.0.46",
         "--version identity failed")
    r = subprocess.run([str(exe), "--v44-release-self-test"], text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(r.returncode == 0, "retained release self-test failed: " + r.stdout[-1200:])

print("Nougat Media Suite v0.0.46 contract PASS")
