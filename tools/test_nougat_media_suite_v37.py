#!/usr/bin/env python3
import hashlib, pathlib, subprocess, sys

root = pathlib.Path(sys.argv[1]).resolve()
exe = pathlib.Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None

def need(cond, msg):
    if not cond:
        raise SystemExit('FAIL: ' + msg)

def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()

cm = (root / 'CMakeLists.txt').read_text(errors='replace')
main = (root / 'src/main.cpp').read_text(errors='replace')
tuner_h = (root / 'src/live_tv/tuner_backend.hpp').read_text(errors='replace')
tuner_cpp = (root / 'src/live_tv/tuner_backend.cpp').read_text(errors='replace')
readme = (root / 'README.md').read_text(errors='replace')
changelog = (root / 'CHANGELOG.md').read_text(errors='replace')

need('VERSION 0.0.37' in cm and 'Nougat_Media_Suite_v37' in cm and 'Nougat_Media_Suite_v36' not in cm,
     'v0.0.37 CMake/target identity missing')
protected = ('# Nougat Media Suite v0.0.37\n\n'
             'Nougat Media Suite is the new official identity of the Linux media application previously released as ReddMedia through accepted v0.0.20. '
             'It combines native local playback, a hidden local Jellyfin catalog foundation, local recommendation AI, optional TMDb discovery, decentralized Search, '
             'multi-platform Stream URL handling, and built-in P2P transfer/streaming in one desktop application.')
need(protected in readme, 'README application description moved or changed')
need('## v0.0.37 - Native Live TV Watch + Classic Guide + System/Visual Repair' in changelog,
     'v37 changelog scope missing')

# Library Search is functional and owns a real button.
for token in (
    'Rect libraryRefreshBtn, libraryBackBtn, librarySearchRect, librarySearchBtn, libraryListBox;',
    'librarySearchBtn = {libraryViewRight - kCompactButtonW, librarySearchY, kCompactButtonW, 30};',
    'button_on(target, librarySearchBtn, "Search");',
    'text(target, librarySearchRect.x+10, searchBaseline, "Search", palette.text);',
    'if (librarySearchBtn.contains(x,y))',
    'library_visible_indices()',
    'library_node_matches_search'):
    need(token in main, 'Library Search/button contract missing token: ' + token)

# Home card sizing correction.
need('(void)continue_card;' in main and
     'return std::max(168, card_width * 3 / 2);' in main and
     'home_card_height(true,180)==app.home_card_height(false,180)' in main,
     'Continue Watching is not on the shared Home card geometry')

# Wide seek retains exact source frames, side timestamps, and suppresses the halo.
seek = root / 'assets/ui/nougat_seek_sheet_frames.bin'
vol = root / 'assets/ui/nougat_volume_sheet_frames.bin'
need(seek.is_file() and sha256(seek) == 'edc27c16675e1114d64be3e233f20361f21a167666bf4257943705d8aeab9b16',
     'accepted exact seek sprite asset changed/missing')
need(vol.is_file() and sha256(vol) == '38197798a97e9ecadf3934daca692446bea586b36e2038c533aa5c92f51077e2',
     'accepted exact VOLUME sprite asset changed/missing')
for token in (
    'const int currentTimeReserve = 62;',
    'const int totalTimeReserve = 62;',
    'const int seekAvailable = std::max(220, W - sideMargin * 2 - currentTimeReserve - totalTimeReserve - seekGap * 2);',
    'const int sideGap = 12;',
    'a>=220U',
    'boundary-white',
    'draw_sheet_volume_frame(target, vol);'):
    need(token in main, 'seek/volume visual repair missing token: ' + token)

# Whole-face stitched status circle and System rename/server relocation.
need('static constexpr int kServerStatusDiameter = 20;' in main,
     'server status size is not the v37 sheet-circle geometry')
for token in (
    'The entire face changes state color',
    'XSetLineAttributes(d, gc, 1, LineOnOffDash',
    'case ViewMode::Debug: return "System";',
    'draw_tab(debugTab,"System",ViewMode::Debug);',
    'layout_button_row({&serverStartBtn,&serverStopBtn,&serverRefreshBtn,'):
    need(token in main, 'System/server circle contract missing token: ' + token)

# Live TV: logical tuner, safe tuner ownership, Watch Live, guide persistence and grid.
for token in (
    'enum class LiveTvTunerUse { Idle, Scanning, GuideRefreshing, Watching };',
    'bool live_playback_input(const TunerDevice& tuner,',
    'bool refresh_guide(const TunerDevice& tuner,',
    'std::vector<LiveTvProgram> load_guide() const;',
    'bool save_guide(const std::vector<LiveTvProgram>& programs',
    'mrl = "atsc://";',
    '":dvb-frequency="',
    'vlc_frequency_khz = frequency >= 10000000ULL ? frequency / 1000ULL : frequency;',
    '":dvb-modulation=8VSB"',
    '":program="',
    'liveTvTunerUse=LiveTvTunerUse::Watching;',
    'doubleClick',
    'watch_live_tv_channel(hit.channel_index)',
    'button_on(target,liveTvGuideBtn,"Guide")',
    'button_on(target,liveTvGuideRefreshBtn,guideBusy ? "Guide..." : "Refresh Guide")',
    'button_on(target,liveTvNowBtn,"Now")',
    'const int channelW=132;',
    'const int slotCount=',
    'program.start_unix<=now && now<eventEnd'):
    source = main if token not in tuner_h and token not in tuner_cpp else (tuner_h + tuner_cpp + main)
    need(token in (tuner_h + tuner_cpp + main), 'Live TV v37 contract missing token: ' + token)
need('const bool have_dvb_frontend = !result.empty();' in tuner_cpp and
     'if (!have_dvb_frontend && fs::exists(v4l_root, ec))' in tuner_cpp and
     'logical tuner' in tuner_cpp,
     'physical tuner deduplication missing')
for token in ('0x1ffb', 'collect_eit_pids', 'collect_eit_from_pid', 'source_id', 'guide.tsv'):
    need(token in tuner_cpp or token in tuner_h, 'PSIP EIT/guide cache missing token: ' + token)

# Accepted ATSC scanner remains intact.
for token in ('FE_SET_PROPERTY','SYS_ATSC','VSB_8','DMX_SET_FILTER','first_channel = 2','last_channel = 36'):
    need(token in tuner_cpp, 'ATSC scanner regression: ' + token)

for rel in ('NougatMediaSuite.desktop','NougatMediaSuite_v37.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
    text=(root/rel).read_text(errors='replace')
    need('Nougat_Media_Suite_v37' in text, 'v37 launcher identity missing: '+rel)

if exe:
    need(exe.is_file(), 'v37 executable missing')
    out=subprocess.check_output([str(exe),'--version'], text=True, stderr=subprocess.STDOUT).strip()
    need(out=='Nougat Media Suite v0.0.37', 'v37 version mismatch: '+repr(out))
    for test in ('--v35-cleanup-self-test','--v36-library-ui-player-self-test','--v37-live-tv-system-self-test'):
        subprocess.run([str(exe),test],check=True)

print('v37-contract=pass system=pass server-controls=pass library-search-button=pass home-card-size=pass wide-seek=pass halo-repair=pass stitched-server-circle=pass logical-tuner=pass watch-live=pass double-click=pass psip-eit=pass guide-cache=pass guide-grid=pass atsc-scan-retained=pass readme-description=pass')
