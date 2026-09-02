#!/usr/bin/env python3
from pathlib import Path
import hashlib
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
FAILURES = []

def expect(condition, message):
    if not condition:
        FAILURES.append(message)

cmake = (ROOT / 'CMakeLists.txt').read_text()
main = (ROOT / 'src/main.cpp').read_text()
service = (ROOT / 'src/lan/lan_media_service.cpp').read_text()
header = (ROOT / 'src/lan/lan_media_service.hpp').read_text()
manager = (ROOT / 'src/media_server/media_server_manager.cpp').read_text()
html = (ROOT / 'components/web_player/index.html').read_text()
js = (ROOT / 'components/web_player/app.js').read_text()
css = (ROOT / 'components/web_player/styles.css').read_text()

expect('VERSION 0.0.55' in cmake, 'CMake project version is not 0.0.55.')
expect('Nougat_Media_Suite_v55' in cmake, 'CMake target is not the v55 executable.')
expect('Nougat Media Suite v0.0.55' in main, 'main.cpp does not expose v0.0.55 identity.')
expect('fullscreenPreviousRect' in main and 'fullscreenNextRect' in main,
       'Five-button fullscreen transport geometry is missing.')
expect('draw_transport(fullscreenRewindRect,-2,0);' in main and
       'draw_transport(fullscreenPreviousRect,-1,1);' in main and
       'draw_transport(fullscreenPlayRect,0,2);' in main and
       'draw_transport(fullscreenNextRect,1,3);' in main and
       'draw_transport(fullscreenForwardRect,2,4);' in main,
       'Fullscreen transport does not expose << < ^ > >> in the approved order.')
expect('XSetLineAttributes(d,gc,4,LineSolid,CapRound,JoinRound);' in main,
       'Fullscreen transport glyphs are still undersized font characters.')
expect('--nowebclient' in manager, 'Hidden Jellyfin backend must keep --nowebclient; do not expose stock Jellyfin Web.')
expect('kJellyfinBackendPort = 8098' in manager, 'Hidden Jellyfin backend must move to loopback port 8098.')
expect('kNougatWebPort = 8096' in manager, 'Nougat Web Player public LAN endpoint must be port 8096.')
expect('nougat-web-service' in manager and 'web_runtime_path_' in manager, 'Background Nougat Web Player helper is not supervised with Jellyfin.')
expect('LocalNetworkAddresses' in manager and '127.0.0.1' in manager, 'Hidden Jellyfin backend is not constrained to loopback.')
expect('lanMedia.prepare();' not in main, 'Desktop GUI must not own the Web Player listener; it must survive GUI close.')
expect('preferred_port = 8096' in header, 'Nougat Web Player must own LAN port 8096.')
expect('getifaddrs' in service and 'is_private_lan_address' in service,
       'LAN binding/private-address gate is missing.')
expect('/nougat/v1/catalog' in service, 'Catalog endpoint is missing.')
expect('/nougat/v1/media' in service and 'Accept-Ranges: bytes' in service,
       'Direct byte-range media delivery is missing.')
expect('/nougat/v1/transcode' in service and '/usr/bin/ffmpeg' in service,
       'Browser compatibility transcoding endpoint is missing.')
expect('components/web_player/' in service, 'First-party web assets are not served by LanMediaService.')
expect('Nougat Web Player' in html, 'Nougat Web Player HTML identity is missing.')
expect("fetch('/nougat/v1/catalog'" in js, 'Web client does not use the Nougat catalog endpoint.')
expect('/nougat/v1/transcode' in js, 'Web client has no compatibility fallback.')
expect('No cloud login' in html, 'Web client does not state the LAN/no-cloud boundary.')
expect('--nowebclient' not in service, 'LAN service must not attempt to control Jellyfin Web.')
expect('http://' in service and ':" + std::to_string(status_.preferred_port)' in service,
       'Access URL construction is missing.')
expect('.empty-player[hidden] { display: none; }' in css, 'Playback placeholder can remain painted over active video.')
expect('@media (max-height: 760px)' in css and '820px' in css, '1366x768/short-viewport fit rule is missing.')
expect('setPlayerStatus' in js and 'clearAfterMs' in js, 'Transient player status clearing is missing.')
expect('nougat_web_service_v55' in cmake and 'web_player_daemon.cpp' in cmake, 'Background Web Player helper target is missing.')
expect(len(css) > 1000, 'Web player stylesheet is unexpectedly empty.')

# Permanent Xbox/Xenia regression gate. This is the exact source file from the
# uploaded v0.0.54 base and must remain byte-identical in this web-player build.
emulator_host = ROOT / 'src/games/emulator_host.cpp'
expected = 'ccb778246a9677a13dd9c71133b817b8d9138264fe1d8a0cd885edda4f271bef'
actual = hashlib.sha256(emulator_host.read_bytes()).hexdigest()
expect(actual == expected, f'Xbox emulator host changed: {actual}')

for launcher in ['NougatMediaSuite.desktop', 'com.elderredsoftworks.NougatMediaSuite.desktop']:
    text = (ROOT / launcher).read_text()
    expect('Nougat_Media_Suite_v55' in text, f'{launcher} does not launch v55.')

if FAILURES:
    for failure in FAILURES:
        print('FAIL:', failure)
    sys.exit(1)
print('PASS: Nougat v0.0.55 first-party LAN Web Player static/regression gate')
