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
        raise SystemExit('FAIL: ' + msg)

def read(rel):
    p = root / rel
    need(p.is_file(), 'missing required file: ' + rel)
    return p.read_text(errors='replace')

main = read('src/main.cpp')
cmake = read('CMakeLists.txt')
readme = read('README.md')
changelog = read('CHANGELOG.md')
roadmap = read('ROADMAP.md')

# v43 identity and permanent no-exceptions icon continuity.
need('VERSION 0.0.43' in cmake and 'Nougat_Media_Suite_v43' in cmake, 'v43 CMake identity missing')
need('INSTALL_RPATH "\\$ORIGIN/components/ai/runtime/lib;\\$ORIGIN/components/ai/runtime/lib64"' in cmake, 'accepted root-relative AI runtime RPATH missing')
need('const std::string versionLabel = "v0.0.43";' in main, 'v43 top-bar identity missing')
need(readme.startswith('# Nougat Media Suite v0.0.43'), 'README v43 identity missing')
need('v0.0.43 - Games, World TV, and Responsive Grid Repair' in changelog, 'CHANGELOG v43 record missing')
need('v0.0.43 candidate — Games, World TV, and Responsive Grid Repair' in roadmap, 'ROADMAP v43 record missing')

def sha256(rel):
    p = root / rel
    need(p.is_file(), 'missing icon-gate file: ' + rel)
    return hashlib.sha256(p.read_bytes()).hexdigest()

need(sha256('docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png') ==
     'a37fb2dd309af0404c615c6f2519da748952d194e44d7b796bf7d46353e92e62',
     'approved UI sheet changed')
need(sha256('assets/icons/nougat-media-suite-concept-sheet-v24.png') ==
     '681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804',
     'approved Nougat N master changed')
need(sha256('src/nougat_media_suite_icon_data.hpp') ==
     'c626664598d57a3756a62a425875fd48567ae4eaa8c9b5a385ccf4630a0b22cb',
     'embedded X11 Nougat icon changed')
for rel in (
    'assets/icons/nougat-media-suite-16.png', 'assets/icons/nougat-media-suite-32.png',
    'assets/icons/nougat-media-suite-48.png', 'assets/icons/nougat-media-suite-64.png',
    'assets/icons/nougat-media-suite-128.png', 'assets/icons/nougat-media-suite-256.png',
    'assets/icons/nougat-media-suite-512.png',
):
    need((root/rel).is_file(), 'approved icon family member missing: ' + rel)

def git_blob_sha1(rel):
    data = (root / rel).read_bytes()
    return hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()

need(git_blob_sha1('components/games/bundled/2048.nes') ==
     'a6f341e925e6686b88a3f6df39c8fa2329e22c18', 'bundled 2048 NES blob changed')
need(git_blob_sha1('components/games/bundled/Waveforms.nes') ==
     '8e23c4bdbaf74218cbe21736c99ebb38a62d8bff', 'bundled NES Waveforms blob changed')

# Carried Library grid repair.
for token in ('prefer_two_rows', '(inner_width + metrics.gap + pitch - 1) / pitch',
              'metrics.posterHeight = std::max(1, metrics.tileWidth * 3 / 2)'):
    need(token in main, 'responsive Library grid repair missing: ' + token)

# My Services fix.
for token in ('const int row_height = 34;', 'const int scrollbar_gutter = 30;',
              'discoverServicesScrollTrack.w + 16', 'services_box.w - 12 - scrollbar_gutter'):
    need(token in main, 'My Services scrollbar/text repair missing: ' + token)

# Games v43.
for token in (
    'enum class GamesDisplayMode { Grid, List };', 'stable_game_cache_hash', 'safe_zip_game_entry', 'unzip -Z1', 'unzip -p',
    'Atari 2600', 'Atari 5200', 'Atari 7800', 'Atari 8-bit', 'Atari Lynx',
    'candidates = {"stella"}', 'candidates = {"atari800"}', 'gamesDisplayMode == GamesDisplayMode::Grid',
    'Sidecar box art:', 'cached_game_artwork', 'doubleClick', 'handle_games_click(x,y,eventTime)',
    'usb-081f_USB_gamepad-joystick', 'D-pad, A/B, Start and Select', '--v43-release-self-test',
):
    need(token in main, 'Games v43 contract missing: ' + token)
for rel in (
    'components/games/bundled/BUNDLED_GAMES_SOURCES.md',
    'components/games/bundled/licenses/2048-nes-UNLICENSE.txt',
    'components/games/bundled/licenses/NES-Waveforms-MIT.txt',
):
    need((root/rel).is_file(), 'bundled legal-game record missing: ' + rel)

# World TV: language agnostic, user-requested Arabic, and verified current additions.
for token in (
    '"TRT World", "Türkiye", "English"', '"France 24 English", "France", "English"',
    '"Al Jazeera English", "Qatar", "English"', '"Al Jazeera Arabic", "Qatar", "Arabic"',
    'https://www.youtube.com/watch?v=bNyUyrR0PHo', '"DW News", "Germany", "English"',
    'https://www.youtube.com/watch?v=LuKwFajn37U',
):
    need(token in main, 'World TV v43 contract missing: ' + token)

for desktop in ('NougatMediaSuite.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
    d = read(desktop)
    need('Nougat_Media_Suite_v43' in d, desktop + ' does not launch v43')
    need('Icon=nougat-media-suite-concept-sheet-v24' in d, desktop + ' lost approved N icon')

if exe is not None:
    need(exe.is_file() and os.access(exe, os.X_OK), 'v43 executable missing/not executable')
    ver = subprocess.run([str(exe),'--version'], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(ver.returncode == 0 and ver.stdout.strip() == 'Nougat Media Suite v0.0.43',
         '--version did not report exact v0.0.43: ' + ver.stdout[-400:])
    # v43 release proof plus retained native regression suite from accepted v42.
    for flag in (
        '--v43-release-self-test', '--v25-ui-state-self-test', '--v28-ui-state-self-test', '--v29-tv-reliability-self-test',
        '--v31-ui-sheet-self-test', '--discover-ai-self-test', '--v32-p2p-player-repair-self-test',
        '--v33-integration-self-test', '--v35-cleanup-self-test', '--v36-library-ui-player-self-test',
        '--v37-live-tv-system-self-test', '--v39-diagnostic-self-test',
    ):
        result = subprocess.run([str(exe), flag], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        need(result.returncode == 0, flag + ' regression failed: ' + result.stdout[-1200:])

print('Nougat Media Suite v0.0.43 contract PASS')
