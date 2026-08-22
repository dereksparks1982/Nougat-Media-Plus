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
readme=(root/'README.md').read_text(encoding='utf-8')

need('VERSION 0.0.29' in cm and 'Nougat_Media_Suite_v29' in cm,'CMake v29 identity missing')
need('printf("Nougat Media Suite v0.0.29\\n")' in main,'runtime v29 identity missing')
need('const std::string versionLabel = "v0.0.29"' in main,'in-app v29 version missing')

# TV playback reliability implementation.
for token in [
    'parse_episode_code','natural_filename_less','build_same_folder_episode_queue',
    'request_local_playback','prepare_tv_autoplay(activeLibraryItem)',
    'show_up_next_overlay(bool draw_now=true)','upNextDeadlineMs = now_ms() + 10000',
    'Playing automatically in 10 seconds.','poll_up_next_overlay()','play_up_next_now()',
    'state == 6','lastLocalPlaybackLengthMs - 1500','Back to Series','back_to_series_from_up_next()',
    'resolve_catalog_series_for_episode_path','libraryParents.push_back(series)','start_library_task(5, {}, series)',
    'tvAutoplayRetryAttempts >= 3','Manual Stop',
]: need(token in main,'v29 TV reliability contract missing: '+token)
need(main.index('prepare_tv_autoplay(activeLibraryItem)') < main.index('ResumeRecord record;', main.index('void request_local_playback')),
     'next episode is not prepared before resume/open flow')

# Home wheel routing: header must win before Home page/shelf handling.
wheel=main[main.index('bool handle_wheel('):main.index('void handle_button(',main.index('bool handle_wheel('))]
need('if (target == win && y < 26) { scroll_top_navigation(delta); return true; }' in wheel,'Home header wheel repair missing')
need(wheel.index('y < 26') < wheel.index('currentView == ViewMode::Home'),'Home still swallows top-header wheel events')

# Vimeo directly follows YouTube and uses official current brand colors.
for token in [
    'enum class StreamPlatform { YouTube, Vimeo, Rumble, RuTube, VK, OK }',
    'case StreamPlatform::Vimeo:   r=23;  g=213; b=255; blendPercent=22',
    'case StreamPlatform::Vimeo:', 'rgb8(23,213,255)', 'rgb8(20,26,32)', 'rgb8(250,252,253)',
    'layout_button_row({&streamYoutubeTab,&streamVimeoTab,&streamRumbleTab',
    'source_button(streamVimeoTab,"Vimeo",StreamPlatform::Vimeo)',
    'app.stream_platform_home(StreamPlatform::Vimeo) == "https://vimeo.com/"',
]: need(token in main,'Vimeo Stream contract missing: '+token)

# Video background is uniform: v28 partial rail/matte is gone.
controls=main[main.index('void draw_controls('):main.index('const char* stream_platform_name',main.index('void draw_controls('))]
need('draw_quilted_background(target, {0, 34, W, std::max(1, H - 34)}, ViewMode::VideoPlayer);' in controls,
     'uniform Video Player background missing')
need('v0.0.28 theater surround' not in controls and 'outline_round(target, {7, 39' not in controls,
     'partial brown player rail remains')

# Home artwork: direct-path TV records can recover series; episode/season/series hierarchy; movie art fills region.
for token in [
    'Resolve the owning series even for resume records created from',
    'record.path.compare(0, candidate.path.size(), candidate.path) == 0',
    'client->load_library_children(season, episodes, episode_error)',
    'same_path = !record.path.empty() && episode.path == record.path',
    'At minimum a TV episode gets its matching season poster',
    'draw_cover_pixels_top_rounded(target, area, poster, radius);',
]: need(token in main,'v29 Home artwork repair missing: '+token)

for token in ['v0.0.29 candidate — TV Playback, Navigation, and Carry-Forward UI Repair',
              'v0.0.30 planned — P2P expansion']:
    need(token in road,'roadmap v29/v30 contract missing: '+token)
need('Vimeo is added immediately after YouTube' in readme and '10-second countdown' in readme,
     'README v29 scope missing')

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
'src/p2p_stream_server.hpp':'68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7'}
for rel,h in expected.items(): need(sha(rel)==h,rel+' changed outside v29 scope')
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.29','executable version mismatch')
    subprocess.check_call([str(exe),'--v29-tv-reliability-self-test'])
print('v29-contract=pass folder-up-next=pass countdown-10=pass autoplay-path=pass back-to-series=pass home-header-wheel=pass vimeo=pass player-surround=pass tv-artwork=pass movie-art-fill=pass protected-boundaries=pass')
