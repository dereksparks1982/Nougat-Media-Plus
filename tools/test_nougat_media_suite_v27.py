#!/usr/bin/env python3
from __future__ import annotations
import hashlib, pathlib, subprocess, sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
def sha(rel): return hashlib.sha256((root/rel).read_bytes()).hexdigest()
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
road=(root/'ROADMAP.md').read_text(encoding='utf-8')
api_h=(root/'src/media_server/jellyfin_api_client.hpp').read_text(encoding='utf-8')
api_cpp=(root/'src/media_server/jellyfin_api_client.cpp').read_text(encoding='utf-8')
need('VERSION 0.0.27' in cm and 'Nougat_Media_Suite_v27' in cm,'CMake v27 identity missing')
need('printf("Nougat Media Suite v0.0.27\\n")' in main,'runtime v27 identity missing')
need('const std::string versionLabel = "v0.0.27"' in main,'in-app v27 version missing')
# Home first/default, real content, resume shelf and no arrow-button carousel.
for token in [
    'enum class ViewMode { Home, VideoPlayer, Library, Discover, Nougat, Stream, P2P, Debug }',
    'ViewMode currentView = ViewMode::Home;',
    'draw_tab(homeTab,"Home",ViewMode::Home);',
    'draw_tab(videoPlayerTab,"Video Player",ViewMode::VideoPlayer);',
    '"CONTINUE WATCHING"','"LOCAL"','homeContinueArea.contains(x,y)',
    'homeContinueScrollX = std::max(0, homeContinueScrollX +',
    'homePageScroll = std::max(0, std::min(max_scroll, homePageScroll +',
    '"Recommended For You"','"Action", "Adventure", "Comedy", "Drama"',
]: need(token in main,'Home contract missing: '+token)
need('homeLeftArrow' not in main and 'homeRightArrow' not in main,'rejected Home arrow controls present')
# Wide artwork + real silent frame hover preview.
for token in ['BackdropImageTags','load_backdrop_image_bmp','640, 360','draw_cover_pixels','poll_home_preview()','extract_video_frame_bmp(path, target, 384, 216','homeHoverStartedMs < 600']:
    need(token in (main+api_h+api_cpp),'Home artwork/preview contract missing: '+token)
# Caramel Continue Watching progress.
need('rgb8(194, 122, 48)' in main and 'static_cast<double>(resume_ms)' in main,'caramel Continue Watching progress missing')
# Persistent resume and stopped state.
for token in ['/.config/reddmedia/playback_resume.tsv','CONTINUE WATCHING?','Last watched at ','"Continue"','"Start Over"','"Cancel"',
              'PLAYBACK STOPPED','"Resume"','"Restart"','"Load Different"','"Back to Library"','persist_current_resume(false)','resumeStore.mark_completed']:
    need(token in main,'persistent resume/stop contract missing: '+token)
# Title identity below video + fullscreen activity identity.
for token in ['current_media_identity()','titleStripY','S%02dE%02d','lastFullscreenOverlayMotionMs','now_ms() - lastFullscreenOverlayMotionMs < 3000']:
    need(token in main,'now-playing identity contract missing: '+token)
# Rounded windowed video; square fullscreen through shape reset.
for token in ['apply_video_corner_shape','const int radius = 14','kShapeBounding','None, kShapeSet']:
    need(token in main,'rounded video viewport contract missing: '+token)
# Seek hover actual-frame preview, cache, timestamp and real chapter label.
for token in ['seekPreviewWindow','update_seek_preview_hover','extract_video_frame_bmp(path, target, 320, 180','seek_preview_chapter_name','state->cache[target]','format_time(seekPreviewTargetMs)']:
    need(token in main,'seek-hover preview contract missing: '+token)
# Flicker source repair: raw motion must not schedule repeated full redraws.
need('pointer_crossed_hover_target' in main,'hover transition repaint gate missing')
need('pointerFullRedrawPending = true' not in main,'old raw-motion full redraw scheduler remains')
need('v0.0.27 flicker repair' in main,'flicker repair marker missing')
# Active notch must be painted after divider.
need(main.index('line(target, 0, 25, W, 25, divider);') < main.index('draw_tab(homeTab'), 'header divider still paints across selected notch')
# Requested cleanup and exact Stream wording.
need('"DISCOVER USUAL"' not in main and '"DISCOVER RANDOM"' not in main,'redundant Discover state heading remains')
need('"Direct Play URL"' not in main,'Direct Play URL heading remains')
need('"DIAGNOSTIC CENTER"' not in main,'Diagnostic Center heading remains')
need('Paste URL Then Press Direct Watch / Rumble / RuTube / VK / OK' in main,'exact Stream placeholder missing')
# Roadmap boundaries + logged next visual proposal.
need('v0.0.28 planned — P2P expansion' in road,'focused v0.0.28 P2P lane missing')
for token in ['Owner-approved later visual proposal','Home:** purple main background','Video Player:** cocoa/chocolate/caramel','Library:** green main background','Discover:** red main background','Search:** the only native page whose main background stays cream','bright light frame around windowed video']:
    need(token in road,'deferred palette/video-surround proposal missing: '+token)
# Protected owner/license/Search/P2P boundaries.
expected={
'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff',
'components/nougat/nougat_engine.py':'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993',
'src/nougat/nougat_bridge.cpp':'15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944',
'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e',
'src/p2p_engine.cpp':'1ad8dec1a454f5809c9afb1647b51d461110d01ce8647cfdabeeb57e9b3137a5',
'src/p2p_engine.hpp':'3d21670ffb49616c011d008efe292340ee1e6e55a004260d193c3e864c2a283f',
'src/p2p_stream_server.cpp':'110a9d7dd5036f8adcc45def2f1853c51a1ec928cca88a6132de6d5c205c4a29',
'src/p2p_stream_server.hpp':'68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7',
}
for rel,h in expected.items(): need(sha(rel)==h,rel+' changed outside v27 scope')
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.27','executable version mismatch')
print('v27-contract=pass home=pass continue-watching=pass personalized-local=pass wide-artwork=pass hover-preview=pass resume=pass stopped-screen=pass title-strip=pass rounded-video=pass seek-preview=pass flicker-repair=pass cleanup=pass roadmap-proposal=pass protected-boundaries=pass')
