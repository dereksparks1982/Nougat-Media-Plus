#!/usr/bin/env python3
from __future__ import annotations
import hashlib, pathlib, subprocess, sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None
main=(root/'src/main.cpp').read_text(encoding='utf-8')
diag=(root/'src/diagnostics/diagnostic_engine.cpp').read_text(encoding='utf-8')

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
def sha(rel): return hashlib.sha256((root/rel).read_bytes()).hexdigest()

# Accepted navigation/header/player foundations remain.
for token in ['e.xbutton.button == 8U','navigate_back()','e.xbutton.button == 9U','navigate_forward()',
              'navigationBackStack','navigationForwardStack','std::min(200,vol)','std::to_string(vol)+"%"']:
    need(token in main,'retained accepted behavior missing: '+token)
need('"MEDIA LIBRARY"' not in main,'redundant Library root heading returned')
need('draw_speaker_icon(target,housing' not in main,'rejected volume speaker glyph returned')
need(main.index('line(target, 0, 25, W, 25, divider);') < main.index('draw_tab(homeTab'),
     'selected-tab notch/divider layering regressed')

# v27/v28 Home/resume/player/seek/palette/artwork foundations survive v29 reliability work.
for token in [
    'enum class ViewMode { Home, VideoPlayer, Library, Discover, Nougat, Stream, P2P, Debug }',
    'ViewMode currentView = ViewMode::Home;', '"CONTINUE WATCHING"', 'homeContinueArea.contains(x,y)',
    'homeContinueScrollX = std::max(0, homeContinueScrollX +', 'homePageScroll = std::max(0, std::min(max_scroll, homePageScroll +',
    'poll_home_preview()', 'homeHoverStartedMs < 600', 'rgb8(194, 122, 48)',
    '/.config/reddmedia/playback_resume.tsv', 'CONTINUE WATCHING?', 'PLAYBACK STOPPED',
    '"Resume"','"Restart"','"Load Different"','"Back to Library"','persist_current_resume(false)',
    'current_media_identity()','lastFullscreenOverlayMotionMs','apply_video_corner_shape',
    'update_seek_preview_hover','extract_video_frame_bmp(path, target, 320, 180','seek_preview_chapter_name',
    'pointer_crossed_hover_target','v0.0.27 flicker repair',
    'if (already_loaded && !homeNeedsRefresh.exchange(false)) return;',
    'home_grid_columns_for_width','if (width >= 600) return std::max(3, available / 184);',
    'x11_safe_text','poster_quality_ok','480, 720','draw_contain_poster_pixels',
    'case ViewMode::Home:        r=91;  g=58;  b=134; blendPercent=58',
    'case ViewMode::Library:     r=77;  g=120; b=61;  blendPercent=56',
    'case ViewMode::Discover:    r=158; g=51;  b=68;  blendPercent=56',
    'case ViewMode::Nougat:      r=241; g=227; b=194; blendPercent=8',
    'case ViewMode::Debug:       r=41;  g=40;  b=48;  blendPercent=70',
]: need(token in main,'retained v28 contract missing: '+token)
need('pointerFullRedrawPending = true' not in main,'old raw-motion full redraw scheduler returned')
need('"DISCOVER USUAL"' not in main and '"DISCOVER RANDOM"' not in main,'redundant Discover heading returned')
need('"Direct Play URL"' not in main and '"DIAGNOSTIC CENTER"' not in main,'redundant heading returned')
need('Paste URL Then Press Direct Watch / Vimeo / Rumble / RuTube / VK / OK' in main,'Stream placeholder/provider order regressed')
need('nougatResultsBox = {28, 148' in main,'Search reclaimed layout regressed')

# Diagnostics remain functional.
for token in ['Nougat Media Suite Diagnostic Report','report_json','write_support_bundle','[REDACTED SENSITIVE LINE]']:
    need(token in diag,'diagnostic engine missing: '+token)
for token in ['"Export TXT"','"Export JSON"','"Support Bundle"','export_debug_report(3)']:
    need(token in main,'diagnostic UI/export missing: '+token)

expected={
'components/nougat/nougat_engine.py':'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993',
'src/nougat/nougat_bridge.cpp':'15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944',
'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e',
'src/p2p_engine.cpp':'1ad8dec1a454f5809c9afb1647b51d461110d01ce8647cfdabeeb57e9b3137a5',
'src/p2p_engine.hpp':'3d21670ffb49616c011d008efe292340ee1e6e55a004260d193c3e864c2a283f',
'src/p2p_stream_server.cpp':'110a9d7dd5036f8adcc45def2f1853c51a1ec928cca88a6132de6d5c205c4a29',
'src/p2p_stream_server.hpp':'68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7',
'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff'}
for rel,h in expected.items(): need(sha(rel)==h,'protected file changed: '+rel)
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.29','v29 executable version mismatch')
    subprocess.check_call([str(exe),'--v28-ui-state-self-test'])
print('retained-v28=pass home-resume=pass palette=pass library-posters=pass search-cleanup=pass seek-preview=pass back-forward=pass volume-200=pass diagnostics=pass search-p2p-license=pass')
