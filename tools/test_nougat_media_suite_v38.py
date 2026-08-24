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
readme = (root / 'README.md').read_text(errors='replace')
changelog = (root / 'CHANGELOG.md').read_text(errors='replace')

need('VERSION 0.0.38' in cm and 'Nougat_Media_Suite_v38' in cm and 'Nougat_Media_Suite_v37' not in cm,
     'v0.0.38 CMake/target identity missing')
need(readme.startswith('# Nougat Media Suite v0.0.38\n'), 'README v0.0.38 identity missing')
need('## v0.0.38 - Library, Live TV and Player Exact-Sheet Polish' in changelog,
     'v0.0.38 changelog scope missing')

# Canonical approved-sheet assets must remain byte-exact.
assets = {
    'docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png': 'cd57f3840bf113f293d5fcfe9b34652098f629d40ffb7587da00e5c938bf2889',
    'assets/ui/nougat_seek_sheet_frames.bin': 'edc27c16675e1114d64be3e233f20361f21a167666bf4257943705d8aeab9b16',
    'assets/ui/nougat_volume_sheet_frames.bin': '38197798a97e9ecadf3934daca692446bea586b36e2038c533aa5c92f51077e2',
    'assets/ui/nougat_progress_sheet_frames.bin': '3e1ab4aa3063e558f934cabcaeb8555f63da8b8c6f1b4c8024f4ff05a904c6ce',
}
for rel, expected in assets.items():
    p = root / rel
    need(p.is_file() and sha256(p) == expected, 'approved sheet asset changed/missing: ' + rel)

# Owner-approved local broadcaster/network artwork.
logos = {
    'abc.bmp':'2e0cd0679796f80262882f6027581829dfd0183f5c36e04c9833597d234b840c',
    'bounce.bmp':'14e18100c8909fb2453d15348959b34faee748f819f6dfccc838bc5b42ad61f5',
    'busted.bmp':'2b48f6bd84a2c0e62c94dcaeacc34e106f22e934b53cf1a029433db8d76d4dfa',
    'cbs.bmp':'0cf005ac4b7700b472aca7e8cf292aeeb0596845710e198a8ced49066414b9a8',
    'create.bmp':'18587ed5644b7440466c1b81fe0cf2ddc8b5d30b4787a4e217f6d96b9ad296f0',
    'cw.bmp':'a94a4dff897946eea92f8227e8d43504cec2eff67c3ea57074de3450f85113ab',
    'fox.bmp':'ae238e2d4704617ff3954305da65474b4df48fce13ce4127d73097a9319f2a52',
    'ion.bmp':'fd8a0c9a1ec9f65b341f8dd3ba5a3765cccebc03f0d190bdea434b52f24c37db',
    'ion_plus.bmp':'2313b5d6bd1bbe5c1501731b49db0ce5efb9c67cb43073c0a90f63677a4c1b8c',
    'metv.bmp':'9f53c3f979526232d66cf1451aa7c33c149d97040d272bf320e9968750168cbf',
    'nbc.bmp':'b901bdd567d2bf27d2af3a15294a20965c5f6da2d856dcc2b32d8cef1b1b2d36',
    'pbs.bmp':'85d5a200a0ee6dda51e554aed9ce7a38e9c29d32aa5f8e9ffa4bd17387b47fee',
    'pbs_kids.bmp':'e21f0bbbfe98a1da565b01932d865ebae1e668786fd589be72b6109f4b7b093c',
    'shoplc.bmp':'eeadf9103905bcba6698def2e926e5393a7c47141ed61e31d0433d3e02fb27ee',
    'telemundo.bmp':'210a4b6d1409cc507f8fe48f21d123ac4871fe7bf9d83bb686e8b62708379cdc',
}
for name, expected in logos.items():
    p = root / 'assets/channel_logos/builtin' / name
    need(p.is_file() and sha256(p) == expected, 'Live TV logo changed/missing: ' + name)

# Exact-sheet player repair. Artwork is retained; only rejected external underside
# crop-shadow rows are omitted. Volume geometry is not reinterpreted and percent is black.
for token in (
    'draw_sheet_seek_frame(target, seekPercent);',
    'const int compositeH = std::min(targetH, 26);',
    'draw_sheet_volume_frame(target, vol);',
    'for (int row=1; row<kSheetVolumeH-2; ++row)',
    'std::to_string(vol) + "%", rgb8(0, 0, 0)',
    'draw_quilted_background(target, {0, y0, W, volumeHousingRect.h + 27}, ViewMode::VideoPlayer);'):
    need(token in main, 'exact-sheet player repair contract missing token: ' + token)
need('text(target, volumeHousingRect.x + 8, volumeHousingRect.y - 5, "VOLUME"' not in main,
     'redundant VOLUME label returned')

# System-wide loading bar stays in its original top lane. It is only tall enough
# for the percent, and the percent stays inside the moving caramel fill.
for token in (
    'static constexpr int kSheetProgressW = 279;',
    'static constexpr int kSheetProgressH = 40;',
    'static constexpr int kSheetProgressFrames = 101;',
    'const int barH=22;',
    'const Rect bar{2,kTopBarH+1,std::max(1,W-4),barH};',
    'const int renderPercent=std::max(percent,minVisualPercent);',
    'draw_sheet_progress_frame(target,bar,renderPercent);',
    'const int leading=fillLeft+(fillRight-fillLeft)*renderPercent/100;',
    'text(target,pctX,pctY,pct,rgb8(255,244,224));',
    'if (discoverState->busy) { progress = discoverState->progress; determinate = true; label = "Working..."; return true; }'):
    need(token in main, 'approved loading-bar contract missing token: ' + token)

# Continue Watching threshold is ten seconds of actual playback.
need('if (record.position_ms < 10000) continue;' in main,
     'Continue Watching 10-second threshold missing')

# Library Search remains in place and typed text/caret are visible.
for token in (
    'draw_concept_field(target, librarySearchRect, palette.field, palette.border, librarySearchFocused);',
    'const unsigned long searchInputText = rgb8(54,36,28);',
    'text(target, librarySearchRect.x+10, searchBaseline, visibleSearch, searchInputText);',
    'librarySearchQuery.append(buf,static_cast<std::size_t>(n));',
    'lastLibrarySearchCaretRedrawMs >= 500'):
    need(token in main, 'Library Search visibility contract missing token: ' + token)

# Approved-sheet scrollbar treatment is visible wherever existing vertical wheel
# scrolling exists. Wheel routing itself is retained separately in handle_wheel().
for token in (
    'draw_home_scrollbar_component(target, libraryVerticalScrollTrack, libraryVerticalScrollThumb, palette);',
    'draw_visible_vertical_scrollbar(target,liveTvListBox,liveTvGuideChannelScroll',
    'draw_visible_vertical_scrollbar(target,nougatCrawlLogBox,nougatCrawlScroll',
    'draw_visible_vertical_scrollbar(target,nougatResultsBox,nougatResultScroll',
    'draw_visible_vertical_scrollbar(target,nougatPeerListBox,nougatPeerScroll',
    'draw_visible_vertical_scrollbar(target,discoverResultBox,discoverDetailsScroll',
    'draw_visible_vertical_scrollbar(target,services_box,discoverServicesScroll',
    'draw_visible_vertical_scrollbar(target,debugListBox,debugScroll'):
    need(token in main, 'visible scrollbar contract missing token: ' + token)
for token in (
    'liveTvGuideChannelScroll+(button==Button4?-1:1)',
    'libraryScroll + (button == Button4 ? -grid.columns : grid.columns)',
    'discoverDetailsScroll + (button == Button4 ? -1 : 1)',
    'nougatCrawlScroll + (button == Button4 ? -3 : 3)',
    'nougatResultScroll = std::max(0, nougatResultScroll + (button == Button4 ? -1 : 1))'):
    need(token in main, 'existing mouse-wheel scrolling changed/missing: ' + token)

# Live TV: Guide is the single guide destination; tuner administration moved to System.
need('liveTvNowBtn' not in main and 'liveTvChannelsBtn' not in main,
     'redundant Live TV Channels/Now control returned')
for token in (
    'layout_button_row({&liveTvScanBtn,&liveTvWatchBtn,&liveTvGuideBtn,&liveTvGuideRefreshBtn,&liveTvRecordBtn}',
    'layout_button_row({&serverStartBtn,&serverStopBtn,&serverRefreshBtn,&liveTvDetectBtn,&liveTvRefreshBtn,',
    'button_on(target,liveTvGuideBtn,"Guide");',
    'bool liveTvGuideMode = true;',
    'restore_live_tv_last_channel();',
    'save_live_tv_last_channel();',
    'void handle_live_tv_key(KeySym ks)',
    'ks==XK_Up || ks==XK_Down',
    'ks==XK_Return || ks==XK_KP_Enter'):
    need(token in main, 'Live TV navigation/system contract missing token: ' + token)

# Real network-logo mapping, never a duplicate numbered badge fallback.
for token in (
    'if (has("ksnw") || has(" nbc")) return "nbc";',
    'if (has("kpts") || has(" pbs")) return "pbs";',
    'if (has("create")) return "create";',
    'if (has("bounce")) return "bounce";',
    'if (has("shoplc") || has("shop lc")) return "shoplc";',
    'if (has("busted")) return "busted";',
    'if (has("ion plus") || has("ionplus") || has("ion plu")) return "ion_plus";',
    'draw_contain_pixels_rounded(target,slot,logo,5,rgb8(247,236,217));',
    'if (has("telemundo") || has("t\'mundo") || has("tmundo")) return "telemundo";',
    'load_bmp_file(exe_dir()+"/assets/channel_logos/builtin/"+network+".bmp"',
    'std::string call=channel.name.empty()?channel.id:channel.name;'):
    need(token in main, 'Live TV real-logo/fallback contract missing token: ' + token)

# Live TV Previous/Next transport buttons tune adjacent channels while Live TV is playing;
# non-Live-TV playback retains episode navigation behavior.
for token in (
    'int relative_live_tv_channel_index(int delta) const {',
    'bool live_tv_channel_navigation_available(int delta) const {',
    'void play_relative_live_tv_channel(int delta) {',
    'if (currentMediaIsLiveTv) play_relative_live_tv_channel(-1);',
    'if (currentMediaIsLiveTv) play_relative_live_tv_channel(1);',
    '(!currentMediaIsLiveTv && episode_navigation_available(-1))',
    '(!currentMediaIsLiveTv && episode_navigation_available(1))'):
    need(token in main, 'Live TV Previous/Next channel-navigation contract missing token: ' + token)

# Unified player activity and current-program timing remain in the build.
for token in (
    'long long lastPlayerActivityMotionMs = 0;',
    'now_ms() - lastPlayerActivityMotionMs >= 3000',
    'current_live_tv_program(',
    'identity += "  •  " + program->title;',
    'liveProgramStart=program->start_unix;',
    'liveProgramEnd=program->start_unix+program->duration_seconds;',
    'const unsigned long timingText = rgb8(0, 0, 0);',
    'text(target, leftTextX, timeY, currentText, timingText);',
    'text(target, rightEdge - text_width(totalText), timeY, totalText, timingText);',
    'text(target,leftTextX,clockY,startText,timingText);',
    'text(target,rightEdge-text_width(endText),clockY,endText,timingText);'):
    need(token in main, 'player/Live TV program contract missing token: ' + token)

# Native ATSC input remains intact.
tuner_cpp = (root / 'src/live_tv/tuner_backend.cpp').read_text(errors='replace')
for token in ('FE_SET_PROPERTY','SYS_ATSC','VSB_8','DMX_SET_FILTER','mrl = "atsc://";',':program='):
    need(token in (tuner_cpp + main), 'retained ATSC/native playback regression: ' + token)

for rel in ('NougatMediaSuite.desktop','NougatMediaSuite_v38.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
    text = (root / rel).read_text(errors='replace')
    need('Nougat_Media_Suite_v38' in text, 'v38 launcher identity missing: ' + rel)

if exe:
    need(exe.is_file(), 'v38 executable missing')
    out = subprocess.check_output([str(exe), '--version'], text=True, stderr=subprocess.STDOUT).strip()
    need(out == 'Nougat Media Suite v0.0.38', 'v38 version mismatch: ' + repr(out))
    for test in ('--v35-cleanup-self-test','--v36-library-ui-player-self-test',
                 '--v37-live-tv-system-self-test','--v38-library-live-tv-player-self-test'):
        subprocess.run([str(exe), test], check=True)

print('v38-contract=pass exact-sheet-art=pass player-shadow-repair=pass volume-black=pass loading-top-lane=pass library-search=pass scrollbars=pass mouse-wheel-retained=pass live-tv-guide-only=pass tuner-admin-system=pass real-network-logos=pass live-tv-prev-next=pass program-overlay=pass live-tv-program-clocks=pass timing-black=pass atsc-retained=pass')
