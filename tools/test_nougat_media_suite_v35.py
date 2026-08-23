#!/usr/bin/env python3
import hashlib, pathlib, struct, subprocess, sys

root = pathlib.Path(sys.argv[1]).resolve()
exe = pathlib.Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None

def need(cond, msg):
    if not cond:
        raise SystemExit('FAIL: ' + msg)

cm = (root / 'CMakeLists.txt').read_text(errors='replace')
main = (root / 'src/main.cpp').read_text(errors='replace')
tuner_h = (root / 'src/live_tv/tuner_backend.hpp').read_text(errors='replace')
tuner_cpp = (root / 'src/live_tv/tuner_backend.cpp').read_text(errors='replace')
readme = (root / 'README.md').read_text(errors='replace')
roadmap = (root / 'ROADMAP.md').read_text(errors='replace')

need('VERSION 0.0.35' in cm and 'Nougat_Media_Suite_v35' in cm, 'v0.0.35 CMake identity missing')
need('enum class ViewMode { Home, VideoPlayer, Library, Discover, LiveTV, Nougat, Stream, Studio, P2P, Debug };' in main,
     'Studio ViewMode/order declaration missing')
need('draw_tab(studioTab,"Studio",ViewMode::Studio);' in main and
     'ytdlpTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;\n        studioTab = {topX,1,kTopTabW,kTopTabH};' in main,
     'Studio tab is not immediately after Stream')
need('case ViewMode::Studio:      r=221; g=176; b=70;  blendPercent=48;' in main and
     'GOLD STUDIO' in main and 'rgb8(105,59,24)' in main,
     'Gold Studio yellow/gold + brown identity missing')
need('view == ViewMode::Nougat || view == ViewMode::Stream' in main and
     'view == ViewMode::Studio || view == ViewMode::Debug' in main,
     'square Search/Studio page-frame contract missing')
need('static constexpr int kPageControlY = kTopBarH + 10' in main and
     'kPageControlY, streamSourceScrollX' in main and 'kPageControlY, nougatPanelButtonsScrollX' in main and
     'kPageControlY, liveTvButtonsScrollX' in main and 'kPageControlY, debugButtonsScrollX' in main and
     'kPageControlY, discoverButtonsScrollX' in main and
     '*tool = {libraryToolX, kPageControlY' in main,
     'app-wide Stream-baseline top inner controls missing')
need('libraryGridBtn = {libraryViewRight - 32, kPageControlY' in main and
     'libraryListViewBtn = {libraryGridBtn.x - 36, kPageControlY' in main,
     'Library List/Grid toggles are not fixed at the far right of the standard row')
need('scroll_button_row(debugButtonsScrollX,10,delta)' in main,
     'Debug action strip does not scroll all ten buttons')
need('seekRect = {seekInset, seekY, std::max(220, W - seekInset * 2), 20};' in main and
     'volumeHousingW = kSheetVolumeW' in main and 'volumeHousingH = kSheetVolumeH' in main and
     'volRect = {volumeHousingX + 50, volumeHousingY + 15, 229, 16};' in main,
     'exact sheet-derived v0.0.35 volume geometry missing')
asset = root / 'assets/ui/nougat_volume_sheet_frames.bin'
need(asset.is_file(), 'approved-sheet VOLUME sprite asset missing')
blob = asset.read_bytes()
need(len(blob) == 9494259 and hashlib.sha256(blob).hexdigest() == '38197798a97e9ecadf3934daca692446bea586b36e2038c533aa5c92f51077e2',
     'approved-sheet VOLUME sprite asset hash/size mismatch')
need(blob[:8] == b'NVOLSPR1', 'approved-sheet VOLUME sprite header missing')
w,h,frames,channels = struct.unpack('<IIII', blob[8:24])
need((w,h,frames,channels) == (335,47,201,3), 'approved-sheet VOLUME sprite dimensions/frame count mismatch')
frame_bytes = w*h*channels
frame100 = blob[24+100*frame_bytes:24+101*frame_bytes]
need(hashlib.sha256(frame100).hexdigest() == 'f48c5ffc898ad31fe9b81ebf87f114f95e739cd7b369d7b8d2699bdf90dd0ad0',
     '100% VOLUME frame is not the exact canonical sheet housing pixels')
need('sheetVolumeLoaded = load_sheet_volume_frames();' in main and 'draw_sheet_volume_frame(target, vol);' in main,
     'runtime does not use the exact approved-sheet VOLUME sprite asset')
need('text(target, seekRect.x + seekRect.w - text_width(totalText), timeY, totalText' in main,
     'seek timestamps are not aligned beneath the track ends')
need('scroll_button_row(controlsScrollX, 8, delta' in main and
     'controlTotalW <= controlViewportW ? std::max(10, (W - controlTotalW) / 2)' in main and
     'playerControlsCentered' in main,
     'Video Player bottom controls are not centered at full width and fully scrollable when narrow')
need('librarySingleRowReach' in main and 'libraryToolClip' in main,
     'Library narrow-width one-row scrolling/far-right view-toggle contract missing')
need('kTopTabPointerHalfW = 16' in main and 'kTopTabPointerH = 10' in main and
     'draw_active_top_tab_pointer(buffer);' in main,
     'enlarged selected top-tab pointer overlay missing')
need('black/uninitialized' in main and
     'draw_quilted_background(buffer, {0, y0, W, h}, ViewMode::VideoPlayer);' in main and
     'if (hover_changed) redraw();' in main and
     'raw X11 pointer motion no longer' in main,
     'player repaint/Up Next no-flash motion regression protection missing')
need('const int videoTop = kTopBarH + 6;' in main and 'const int videoBottom = std::max(videoTop + 100, seekRect.y - 10);' in main and
     'const int titleStripY = std::max(kTopBarH + 8, seekRect.y - 30);' in main,
     'player video/title/seek stack still uses overlapping old offsets')
need('if (nougatSearchWorker.joinable()) nougatSearchWorker.join();' in main and
     'if (nougatCrawlWorker.joinable()) nougatCrawlWorker.join();' in main,
     'Nougat worker lifetime repair missing')
need('nougatSearchWorker.detach()' not in main and 'nougatCrawlWorker.detach()' not in main,
     'unsafe Nougat worker detach remains')
need('scan_channels(const TunerDevice& tuner' in tuner_h and 'ChannelScanProgress' in tuner_h,
     'native Live TV scan API missing')
for token in ('FE_SET_PROPERTY', 'SYS_ATSC', 'VSB_8', 'DMX_SET_FILTER', '0x1ffb', 'first_channel = 2', 'last_channel = 36'):
    need(token in tuner_cpp, f'native ATSC scan implementation missing {token}')
need('Linux tuner foundation. First hardware target' not in main,
     'removed Live TV description is still rendered')
need('# Nougat Media Suite v0.0.35\n\nNougat Media Suite is the new official identity' in readme,
     'README application description is not directly under name/version')
need('## Studio / Nougat Media Processing Engine' in roadmap and 'Audio Lab' in roadmap and 'Full Studio timeline' in roadmap,
     'Studio/media-processing roadmap expansion missing')
for rel in ('NougatMediaSuite.desktop', 'NougatMediaSuite_v35.desktop', 'com.elderredsoftworks.NougatMediaSuite.desktop'):
    text = (root / rel).read_text(errors='replace')
    need('Nougat_Media_Suite_v35' in text, f'v35 launcher identity missing: {rel}')

if exe:
    need(exe.is_file(), 'v35 executable missing')
    out = subprocess.check_output([str(exe), '--version'], text=True, stderr=subprocess.STDOUT).strip()
    need(out == 'Nougat Media Suite v0.0.35', f'v35 version mismatch: {out!r}')
    subprocess.run([str(exe), '--v35-cleanup-self-test'], check=True)

print('v35-contract=pass studio-tab=pass gold-studio=pass studio-roadmap=pass square-search=pass app-wide-top-row-alignment=pass library-one-row=pass library-view-right=pass debug-full-scroll=pass player-centered=pass player-full-scroll=pass active-tab-pointer=pass exact-sheet-volume-sprite=pass player-repaint=pass up-next-no-flash=pass player-stack=pass live-tv-atsc-scan=pass nougat-lifetime=pass readme-description=pass')
