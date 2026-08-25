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
    p = root / rel
    need(p.is_file(), "missing " + rel)
    return p.read_text(errors="replace")

def sha(rel):
    return hashlib.sha256((root / rel).read_bytes()).hexdigest()

main = read("src/main.cpp")
cmake = read("CMakeLists.txt")
worker = read("components/world_tv/nougat_world_tv_worker.py")
service_h = read("src/world_tv/world_tv_service.hpp")
service_cpp = read("src/world_tv/world_tv_service.cpp")
readme = read("README.md")
changelog = read("CHANGELOG.md")
gitignore = read(".gitignore")
scanner = read("src/security/scanner_process.cpp")
lan_h = read("src/lan/lan_media_service.hpp")
lan_cpp = read("src/lan/lan_media_service.cpp")

need("VERSION 0.0.47" in cmake and "Nougat_Media_Suite_v47" in cmake,
     "v47 CMake identity missing")
need('const std::string versionLabel = "v0.0.47";' in main,
     "v47 top-bar identity missing")
need('printf("Nougat Media Suite v0.0.47\\n");' in main,
     "v47 --version identity missing")

need('#include "world_tv/world_tv_service.hpp"' in main,
     "World TV service is not wired into main")
need("src/world_tv/world_tv_service.cpp" in cmake,
     "World TV service is not wired into CMake")
need("WorldTvService" in service_h and "WorldTvResolveResult" in service_h and
     "struct WorldTvStation" in service_h,
     "World TV service contract missing")
need("fork()" in service_cpp or "::fork()" in service_cpp,
     "World TV worker invocation is not process-isolated")
need("::setpgid(0, 0)" in service_cpp and "::kill(-child, SIGTERM)" in service_cpp and
     "::kill(-child, SIGKILL)" in service_cpp and "std::chrono::seconds(32)" in service_cpp,
     "World TV worker process group is not bounded/terminable")

# The old top-bar easter-egg path is gone. The Saudi channel is now ordinary World TV.
for forbidden in (
    "open_hidden_live()",
    "hiddenBadgeLastClickMs",
    "HiddenLiveResolveState",
    "open_kaaba_live()",
    "kaabaBadgeLastClickMs",
    "Cm1v4bteXbI",
):
    need(forbidden not in main, "obsolete hidden World TV path remains: " + forbidden)

need("AlQuranAlKareemTV.sa" in main and "aloula-quran" in main,
     "Saudi Quran channel is not in the normal World TV catalog")
need("Russia24.ru" in main and "Russia1.ru" in main and "RussiaK.ru" in main and
     "TVCentr.ru" in main and "RBKTV.ru" in main,
     "core Russian World TV expansion missing")
need("ChannelOne.ru" in main and "NTV.ru" in main and "Mir24.ru" in main,
     "Russian national channel expansion missing")
need("TRTWorld.tr" in main and "TRTHaber.tr" in main and "TRT1.tr" in main and "NTV.tr" in main,
     "Turkiye World TV expansion missing")
need("ArirangTV.kr" in main and "NHKWorldJapan.jp" in main and
     "CNAInternational.sg" in main,
     "Asia World TV expansion missing")

# World TV is its own orange visual identity, not the Live TV teal.
need("case ViewMode::WorldTV:     r=201; g=116; b=38;" in main,
     "World TV orange quilt tint missing")
need("if (view == ViewMode::WorldTV) return {" in main,
     "World TV orange palette missing")
need("case ViewMode::WorldTV: return 11;" in main,
     "World TV does not own a separate quilt tile")
need("Pixmap quiltTiles[12]" in main and "const ViewMode views[12]" in main,
     "World TV separate quilt allocation missing")

# World TV is treated as a channel surface with real station artwork and guide text.
need("cached_world_tv_logo" in main and "draw_world_tv_logo" in main,
     "World TV real-logo rendering path missing")
need("start_world_tv_artwork_prefetch" in main and "poll_world_tv_artwork" in main,
     "World TV artwork worker path missing")
need("start_world_tv_guide_refresh" in main and "poll_world_tv_guide" in main,
     "World TV guide refresh path missing")
need("current_world_tv_program_title" in main,
     "World TV player identity does not include guide-aware programme title")
need("currentMediaIsWorldTv && !liveTvPlayingLabel.empty()" in main,
     "World TV station identity is not exposed in the player")
need("draw_player_activity_overlay_window" in main and "draw_world_tv_logo" in main,
     "World TV station artwork is not available to player activity UI")
need("fullscreenTransportWindow" in main and "draw_fullscreen_transport_overlay" in main,
     "fullscreen mouse transport overlay missing")
need('draw_transport(fullscreenRewindRect,"<",0)' in main and
     'draw_transport(fullscreenPlayRect,"^",1)' in main and
     'draw_transport(fullscreenForwardRect,">",2)' in main,
     "fullscreen [<] [^] [>] controls missing")
need("draw_sheet_button_surface(fullscreenTransportWindow" in main,
     "fullscreen symbol squares are not using the approved sheet button surface")
need("fullscreen_transport_hit" in main and "seek_relative(-10000)" in main and
     "seek_relative(10000)" in main,
     "fullscreen transport hit wiring missing")
need("--v47-fullscreen-controls-self-test" in main,
     "fullscreen transport runtime self-test missing")
need("scroll_button_row(topNavScrollX, 11, delta, topNavViewportW)" in main,
     "System top-tab scroll extent still uses the wrong tab count")

# False-positive black screens are rejected before native playback and stalls reconnect.
for token in (
    "ffprobe_candidate",
    "No playable non-YouTube direct source passed the World TV probe.",
    "worldTvBufferingSinceMs",
    "worldTvLastProgressMs",
    "worldTvLastPlaybackTimeMs",
):
    need(token in worker or token in main, "World TV reliability token missing: " + token)
need("worldTvResolveWorker" in main and "poll_world_tv_resolver" in main,
     "World TV resolver is not asynchronous")
need("worldTvStartupDeadlineMs" in main and "worldTvPlaybackVerified" in main,
     "retained World TV startup verification missing")
need("cleanup_player();" in main and "switch_view(ViewMode::WorldTV)" in main,
     "failed World TV playback does not return cleanly to the channel list")

# Data directory is explicitly non-YouTube and uses current channel/logo/guide directories.
for token in (
    'STREAMS_API = f"{API_BASE}/streams.json"',
    'LOGOS_API = f"{API_BASE}/logos.json"',
    'GUIDES_API = f"{API_BASE}/guides.json"',
    "youtube_url",
    "aloula_resolve_quran",
):
    need(token in worker, "World TV worker contract missing: " + token)

# Existing owner-approved reliability/security/LAN contracts remain.
need('securityBusyForButton?"Stop Scan":"Scan Again"' in main,
     "Stop Scan UI state missing")
need("securityProcess->cancel()" in main, "scanner cancellation wiring missing")
need("::kill(-pgid, SIGTERM)" in scanner and "::kill(-pgid, SIGKILL)" in scanner,
     "scanner process-group termination missing")
need("lanMedia.prepare();" in main, "LAN foundation not wired")
for token in ("/nougat/v1/catalog", "/nougat/v1/history", "/nougat/v1/media",
              "/nougat/v1/hls", "/nougat/v1/pair", "nougat.local"):
    need(token in lan_h or token in lan_cpp, "LAN contract missing " + token)

need("/build/" in gitignore, "build directory is not ignored")
need(readme.startswith("# Nougat Media Suite\n\n**Nougat Media Suite** is a native Linux media center"),
     "README full suite introduction is not first")
p47 = readme.find("## v0.0.47 -")
p46 = readme.find("## v0.0.46 -")
need(p47 > 0 and p46 > p47, "README order must be intro -> v47 -> older versions")
need("Al Quran Al Kareem TV" in readme and "Russian" in readme,
     "v47 release notes do not describe the normal World TV additions")
need("## v0.0.47 -" in changelog, "CHANGELOG v47 entry missing")

need(sha("assets/icons/nougat-media-suite-concept-sheet-v24.png") ==
     "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804",
     "approved Nougat N master changed")
for desktop in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
    d = read(desktop)
    need("Nougat_Media_Suite_v47" in d, desktop + " does not target v47")
    need("Icon=nougat-media-suite-concept-sheet-v24" in d,
         desktop + " lost approved N icon key")
    need("StartupWMClass=NougatMediaSuite" in d, desktop + " lost StartupWMClass")
    need("X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite" in d,
         desktop + " lost GNOME application ID")
    need("X-GNOME-WMClass=NougatMediaSuite" in d, desktop + " lost GNOME WM class")
    need("StartupNotify=true" in d, desktop + " lost StartupNotify")

secure = read("src/search/secure_search.cpp")
policy = read("src/privacy/privacy_policy.cpp")
need("Secure remote search unavailable" in secure,
     "Secure Search fail-closed behavior missing")
need("fallback" in policy.lower(),
     "Secure Search privacy policy unexpectedly lost fallback controls")

r = subprocess.run([
    sys.executable, "-m", "py_compile",
    str(root / "components/security/nougat_security_worker.py"),
    str(root / "components/world_tv/nougat_world_tv_worker.py"),
])
need(r.returncode == 0, "Python worker syntax failed")

if exe is not None:
    need(exe.is_file() and os.access(exe, os.X_OK),
         "v47 executable missing/not executable")
    r = subprocess.run([str(exe), "--version"], text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(r.returncode == 0 and r.stdout.strip() == "Nougat Media Suite v0.0.47",
         "--version identity failed")
    r = subprocess.run([str(exe), "--v44-release-self-test"], text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(r.returncode == 0,
         "retained release self-test failed: " + r.stdout[-1200:])

print("Nougat Media Suite v0.0.47 contract PASS")
