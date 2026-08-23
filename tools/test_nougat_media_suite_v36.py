#!/usr/bin/env python3
import hashlib, pathlib, struct, subprocess, sys

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
jelly = (root / 'src/media_server/jellyfin_api_client.cpp').read_text(errors='replace')
tuner_h = (root / 'src/live_tv/tuner_backend.hpp').read_text(errors='replace')
tuner_cpp = (root / 'src/live_tv/tuner_backend.cpp').read_text(errors='replace')
readme = (root / 'README.md').read_text(errors='replace')
roadmap = (root / 'ROADMAP.md').read_text(errors='replace')

need('VERSION 0.0.36' in cm and 'Nougat_Media_Suite_v36' in cm and 'Nougat_Media_Suite_v35' not in cm,
     'v0.0.36 CMake/target identity missing')

# Protected README description must remain verbatim directly under the current title.
protected = ('# Nougat Media Suite v0.0.36\n\n'
             'Nougat Media Suite is the new official identity of the Linux media application previously released as ReddMedia through accepted v0.0.20. '
             'It combines native local playback, a hidden local Jellyfin catalog foundation, local recommendation AI, optional TMDb discovery, decentralized Search, '
             'multi-platform Stream URL handling, and built-in P2P transfer/streaming in one desktop application.')
need(protected in readme, 'README application description moved or changed')

# Header repairs.
need('const unsigned long headerTan = rgb8(227, 204, 172);' in main and
     'fill(target, {0, 0, W, kTopBarH}, headerTan);' in main,
     'approved volume-housing tan header fill missing')
need('draw_sheet_status_circle(target, statusX, statusY, kServerStatusDiameter, light);' in main and
     'kServerStatusDiameter = 16' in main,
     'sheet-family circular Server indicator missing')
need('const int badgeY = (kTopBarH - nougat_media_suite_icon::kTopBar14Size) / 2;' in main and
     'const int headerBaseline = kTopBarH / 2 + 5;' in main and
     'const int statusY = (kTopBarH - kServerStatusDiameter) / 2;' in main,
     'header clusters are not vertically centered')

# Library search: own row, exact shared field renderer, exact placeholder, live filtering and original-index mapping.
need('librarySearchRect = {libraryInnerX, kPageControlBottom + 10' in main,
     'Library Search is not on its own row below green actions')
need('draw_concept_field(target, librarySearchRect' in main and
     'text(target, librarySearchRect.x+10, searchBaseline, "Search", palette.muted);' in main,
     'Library exact-sheet Search field/placeholder missing')
need('library_node_matches_search' in main and 'library_visible_indices()' in main and
     'libraryRowNodeIndices.push_back(nodeIndex)' in main,
     'Library live filtering/original node mapping missing')
need('libraryGridBtn = {libraryViewRight - 32, kPageControlY' in main and
     'libraryListViewBtn = {libraryGridBtn.x - 36, kPageControlY' in main,
     'Library List/Grid pair no longer fixed at far right')

# Collection hierarchy must not trust collapseBoxSetItems alone.
for token in ('std::set<std::string> collection_member_ids',
              'load_library_children(node, children, child_error)',
              'collection_member_ids.insert(child.id)',
              'loaded.erase(std::remove_if',
              'node.kind != LibraryNodeKind::MovieCollection'):
    need(token in jelly, 'client-side collection hierarchy missing token: ' + token)
need('"ProductionYear%2CSortName"' in jelly,
     'collection members are not sorted in production-year/name order')

# Home artwork repair: offscreen pixmaps must use their own unclipped GC before the page clip is reapplied.
need(main.count('GC imageGc = XCreateGC(d, tmp, 0, nullptr);') >= 2 and
     main.count('gc = imageGc;') >= 2 and main.count('gc = windowGc;') >= 2,
     'Home offscreen artwork still inherits the page/shelf X11 clip')
need('draw_cover_pixels_top_rounded(target, area, poster, radius);' in main and
     'card_width * 9 / 16' in main and 'card_width * 3 / 2' in main,
     'Home 16:9/portrait card geometry missing')

# Literal approved seek sprite family.
sheet = root / 'docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png'
need(sheet.is_file() and sha256(sheet) == 'cd57f3840bf113f293d5fcfe9b34652098f629d40ffb7587da00e5c938bf2889',
     'canonical approved UI sheet missing/changed')
seek = root / 'assets/ui/nougat_seek_sheet_frames.bin'
need(seek.is_file() and seek.stat().st_size == 5039520 and
     sha256(seek) == 'edc27c16675e1114d64be3e233f20361f21a167666bf4257943705d8aeab9b16',
     'exact-sheet seek sprite family missing/changed')
blob = seek.read_bytes()
need(blob[:8] == b'NSEEKSP1', 'seek sprite header missing')
w,h,frames,channels = struct.unpack('<IIII', blob[8:24])
need((w,h,frames,channels) == (378,33,101,4), 'seek sprite geometry/frame count mismatch')
need('sheetSeekLoaded = load_sheet_seek_frames();' in main and
     'draw_sheet_seek_frame(target, seekPercent);' in main,
     'runtime does not use exact-sheet seek sprite family')
need('const int sideGap = 12;' in main and
     'seekRect.x - sideGap - text_width(currentText)' in main and
     'seekRect.x + seekRect.w + sideGap' in main,
     'elapsed/total readouts are not on seek-bar sides')

# Existing accepted VOLUME asset stays exact, only geometry masking/repaint behavior changes.
vol = root / 'assets/ui/nougat_volume_sheet_frames.bin'
need(vol.is_file() and vol.stat().st_size == 9494259 and
     sha256(vol) == '38197798a97e9ecadf3934daca692446bea586b36e2038c533aa5c92f51077e2',
     'accepted exact VOLUME sprite changed')
need('for (int row=0; row<kSheetVolumeH; ++row)' in main and
     'rounded_top_inset_for_row(edgeRow, cornerRadius)' in main,
     'VOLUME rectangular sheet-background masking missing')

# Stable player controls repaint. Both legacy event entry points must delegate to one full-height repaint unit.
need('void draw_player_controls_only()' in main and
     'const int y0 = std::max(kTopBarH, seekRect.y - 10);' in main and
     'const int h = std::max(0, H - y0);' in main,
     'stable full-height player controls repaint region missing')
need('void draw_seek_time_only() {\n        draw_player_controls_only();\n    }' in main and
     'void draw_volume_only() {\n        draw_player_controls_only();\n    }' in main,
     'seek/volume updates still use competing clip rectangles')

# Working v35 ATSC scan remains present and v37 playback is roadmap only.
need('scan_channels(const TunerDevice& tuner' in tuner_h and 'ChannelScanProgress' in tuner_h,
     'accepted Live TV scan API missing')
for token in ('FE_SET_PROPERTY','SYS_ATSC','VSB_8','DMX_SET_FILTER','0x1ffb','first_channel = 2','last_channel = 36'):
    need(token in tuner_cpp, 'accepted ATSC scan regression: ' + token)
need('## v0.0.37 planned — Native Live TV Watch Playback' in roadmap and
     'MPEG transport-stream program/PIDs' in roadmap,
     'next-build Watch Live agenda missing')

for rel in ('NougatMediaSuite.desktop','NougatMediaSuite_v36.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
    text = (root / rel).read_text(errors='replace')
    need('Nougat_Media_Suite_v36' in text, 'v36 launcher identity missing: ' + rel)

if exe:
    need(exe.is_file(), 'v36 executable missing')
    out = subprocess.check_output([str(exe), '--version'], text=True, stderr=subprocess.STDOUT).strip()
    need(out == 'Nougat Media Suite v0.0.36', 'v36 version mismatch: ' + repr(out))
    subprocess.run([str(exe), '--v35-cleanup-self-test'], check=True)
    subprocess.run([str(exe), '--v36-library-ui-player-self-test'], check=True)

print('v36-contract=pass library-search=pass collection-hierarchy=pass home-artwork-clip=pass exact-sheet-seek-sprite=pass seek-side-times=pass exact-volume-preserved=pass player-stable-repaint=pass tan-header=pass server-circle=pass live-tv-scan-retained=pass v37-watch-live-agenda=pass readme-description=pass')
