#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import py_compile
import re
import shutil
import subprocess
import sys
import tempfile

class Failure(RuntimeError):
    pass

def need(condition: bool, message: str) -> None:
    if not condition:
        raise Failure(message)

def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
    main_cpp = (root / 'src/main.cpp').read_text(encoding='utf-8')
    cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    hdhr = (root / 'src/live_tv/hdhomerun_provider.cpp').read_text(encoding='utf-8')
    world_cpp = (root / 'src/world_tv/world_tv_service.cpp').read_text(encoding='utf-8')
    world_worker = (root / 'components/world_tv/nougat_world_tv_worker.py').read_text(encoding='utf-8')
    art_worker = (root / 'components/games/artwork_cache_worker.py').read_text(encoding='utf-8')

    need('NOUGAT_V53_CANDIDATE' in main_cpp, 'v53 source marker missing')
    need('Nougat Media Suite v0.0.53' in main_cpp, 'main version string is not v0.0.53')
    need('VERSION 0.0.53' in cmake and 'Nougat_Media_Suite_v53' in cmake, 'CMake target/version not promoted')
    need('Icon=nougat-media-suite' in (root/'NougatMediaSuite.desktop').read_text(), 'desktop icon identity not promoted')

    # Navigation must remain geometry-derived, not a newly hardcoded final-tab count.
    need('top_nav_left_bound()' in main_cpp and 'topNavClipRight' in main_cpp,
         'top-navigation geometry authority missing')
    need('int top_navigation_max_scroll() const' in main_cpp and 'debugTab.x + debugTab.w + topNavScrollX' in main_cpp,
         'top-navigation scrolling is not derived from the rendered final tab')

    # World TV guide geometry and runtime verification/failure classes.
    need('const int channelW=166;' in main_cpp, 'World TV channel column did not adopt Live TV guide width')
    need('const int rowH=46;' in main_cpp, 'World TV row height did not adopt Live TV guide height')
    for token in ('ERROR_CLASS', 'startup_timeout', 'dependency', 'resolver', 'provider', 'stream'):
        need(token in world_worker or token in world_cpp, f'World TV failure class missing: {token}')
    need('ffprobe_candidate' in world_worker, 'World TV playable-source verification was removed')

    # Studio File Splitter is embedded; no normal GUI worker invocation is allowed.
    for token in ('studioSourceRect','studioOutputRect','studioNameRect','studioPiecesRect','studioMaxMiBRect',
                  'Split Folder / File / ZIP','Reassemble','Verify Parts'):
        need(token in main_cpp, f'embedded File Splitter control missing: {token}')
    need('launch_studio_splitter_action("gui")' not in main_cpp, 'Studio still launches the old external GUI')
    need('python3 ' in main_cpp and 'nougat_file_splitter.py' in main_cpp, 'Studio worker bridge missing')

    # HDHomeRun explicit ATSC traversal and lock-before-program parsing.
    for token in ('kFirstPhysical = 2','kLastPhysical = 51','atsc_physical_frequency_hz',
                  '/status','/streaminfo','program(s) found','State returned to Idle','rf_attempted','rf_locked','raw_service_rows','parsed_services','rejected_service_rows','phase.total = 100'):
        need(token in hdhr, f'HDHomeRun v53 scan evidence missing: {token}')
    status_pos = hdhr.find('get " + tuner_path + "/status')
    stream_pos = hdhr.find('get " + tuner_path + "/streaminfo')
    need(status_pos >= 0 and stream_pos > status_pos, 'HDHomeRun program metadata is requested before lock/status probing')

    # Search hover retains the same sheet-control hover path.
    need('const bool hover = target == win && r.contains(pointerWindowX, pointerWindowY);' in main_cpp,
         'sheet button hover path missing')
    need('nougat_button(target,nougatSearchBtn' in main_cpp, 'Search button no longer uses Nougat sheet button hover path')

    # Expanded practical Linux emulator backend inventory.
    for token in ('dolphin','duckstation','pcsx2','ppsspp','rpcs3','cemu','mame','ryujinx'):
        need(token in main_cpp.lower(), f'practical Linux emulator backend missing: {token}')
    for token in ('strip_release_noise','PlayStation 2','GameCube','Nintendo Switch'):
        need(token in art_worker, f'Games artwork expansion missing: {token}')

    # Process identity and transient-overlay contracts.
    need('PR_SET_NAME' in main_cpp and 'NougatMediaSuite' in main_cpp, 'Linux process identity repair missing')
    need('apply_transient_window_style' in main_cpp and 'set_transient_opacity' in main_cpp and 'apply_rounded_transient_shape' in main_cpp,
         'rounded/translucent transient overlay path missing')

    # LAN Viewer, Child Safe, advisories, alerts.
    lan = (root/'src/lan/lan_viewer_service.cpp').read_text()
    child = (root/'src/safety/child_safe_controls.cpp').read_text()
    advisory = (root/'src/security/security_advisory_service.cpp').read_text()
    alerts = (root/'src/alerts/public_safety_alerts.cpp').read_text()
    for token in ('Verified Clean','Blocked','Unknown','private LAN'):
        need(token in lan, f'LAN Viewer trust contract missing: {token}')
    need('120000' in child and '0600' in child, 'Child Safe password/config protection contract missing')
    need('OSV' in advisory or 'api.osv.dev' in (root/'src/security/security_advisory_service.hpp').read_text(),
         'security advisory source missing')
    need('api.weather.gov' in alerts or 'api.weather.gov' in (root/'src/alerts/public_safety_alerts.hpp').read_text(),
         'NOAA/NWS alert source missing')

    # Icon tool must clear only alpha outside the approved rounded badge and regenerate WM arrays.
    icon_tool = (root/'tools/nougat_icon_alpha_fix_v53.py').read_text()
    for token in ('replace_icon_array','master-N','premultiplied-alpha','clear_rounded_exterior'):
        need(token in icon_tool, f'icon alpha/WM regeneration proof missing: {token}')
    compact_icon_tool = ''.join(icon_tool.split())
    need('forsizein(16,32,48,64,128,256,512)' in compact_icon_tool,
         'icon generator does not enumerate all required v53 icon sizes')
    need('forsizein(16,32,64)' in compact_icon_tool,
         'embedded _NET_WM_ICON generator does not enumerate 16/32/64 sizes')

    # Python syntax.
    for rel in ('tools/apply_v53.py','tools/nougat_icon_alpha_fix_v53.py',
                'components/world_tv/nougat_world_tv_worker.py','components/games/artwork_cache_worker.py'):
        py_compile.compile(str(root/rel), doraise=True)

    # New C++ units independently compile under the repository warnings-as-errors rule.
    cxx = shutil.which('g++') or shutil.which('c++')
    if cxx:
        with tempfile.TemporaryDirectory(prefix='nougat-v53-static-') as temp:
            for rel in ('src/lan/lan_viewer_service.cpp','src/safety/child_safe_controls.cpp',
                        'src/security/security_advisory_service.cpp','src/alerts/public_safety_alerts.cpp'):
                obj = Path(temp) / (Path(rel).stem + '.o')
                result = subprocess.run([cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),'-c',str(root/rel),'-o',str(obj)],
                                        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                need(result.returncode == 0, f'{rel} failed -Werror compile:\n{result.stdout}')

    print('PASS: Nougat Media Suite v0.0.53 candidate static contracts verified.')
    return 0

if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except Failure as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        raise SystemExit(1)
