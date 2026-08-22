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

need('VERSION 0.0.28' in cm and 'Nougat_Media_Suite_v28' in cm,'CMake v28 identity missing')
need('printf("Nougat Media Suite v0.0.28\\n")' in main,'runtime v28 identity missing')
need('const std::string versionLabel = "v0.0.28"' in main,'in-app v28 version missing')

# Page backgrounds are the main identity, Search remains cream, Stream provider-reactive.
for token in [
    'case ViewMode::Home:        r=91;  g=58;  b=134; blendPercent=58',
    'case ViewMode::VideoPlayer: r=91;  g=52;  b=31;  blendPercent=62',
    'case ViewMode::Library:     r=77;  g=120; b=61;  blendPercent=56',
    'case ViewMode::Discover:    r=158; g=51;  b=68;  blendPercent=56',
    'case ViewMode::Nougat:      r=241; g=227; b=194; blendPercent=8',
    'case ViewMode::Debug:       r=41;  g=40;  b=48;  blendPercent=70',
    'stream_palette_for(streamPlatform)',
]: need(token in main,'v28 page palette contract missing: '+token)

# Home state persistence, poster-first artwork, season/series TV fallback, silent preview, clipping and responsive layout.
for token in [
    'if (already_loaded && !homeNeedsRefresh.exchange(false)) return;',
    'continue_artwork_nodes', 'load_tv_poster_path(series.tmdb_id, season.season_number',
    'v0.0.28 Home rests on poster art, never a backdrop/still by preference',
    'draw_contain_pixels_top_rounded', 'draw_cover_pixels_top_rounded',
    'rounded_top_inset_for_row', 'home_grid_columns_for_width',
    'if (width >= 600) return std::max(3, available / 184);',
    'section_text(target, 28', 'metadata_text(target', 'homeHoverStartedMs < 600',
]: need(token in main,'v28 Home contract missing: '+token)
need('home_artwork_key' in main and 'tmdb_poster_path' in main,'poster-first Home key missing')

# Encoding repair.
for token in ['x11_safe_text','0xE2U','0x80U','0xA2U','0xB7U']:
    need(token in main,'metadata encoding repair missing: '+token)

# Library poster quality path: exact-ID TMDb first, local fallback, portrait gate, 480x720, aspect containment.
need(main.index('if (!node.tmdb_poster_path.empty()) return "tmdb:"') < main.index('if (!node.poster_item_id.empty()) {', main.index('std::string library_poster_key')),
     'Library poster key does not prefer TMDb')
for token in ['poster_quality_ok','480, 720','draw_contain_poster_pixels','metrics.posterHeight = std::max(168, metrics.tileWidth * 3 / 2)',
              'load_movie_poster_path(node.tmdb_id','load_tv_poster_path(node.tmdb_id, 0']:
    need(token in main,'Library artwork contract missing: '+token)

# Search cleanup and video matte.
need('"SEARCH"' not in main[:main.index('"SEARCHING"')] if '"SEARCHING"' in main else True,'redundant standalone Search heading remains')
need('nougatSearchBtn = {' in main and 'nougatResultsBox = {28, 148' in main,'Search controls were not reclaimed upward')
for token in ['v0.0.28 theater surround','rgb8(47, 27, 19)','rgb8(68, 39, 24)','rgb8(154, 91, 42)']:
    need(token in main,'Video Player theater surround missing: '+token)

# Release split is explicit.
for token in ['v0.0.28 candidate — Candy Palette, Artwork, and UI State Polish',
              'v0.0.29 planned — TV Playback and Navigation Reliability',
              'v0.0.30 planned — P2P expansion']:
    need(token in road,'roadmap split missing: '+token)
need('TV Up Next/autoplay' in readme and 'v0.0.29' in readme,'TV reliability deferral missing from README')

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
for rel,h in expected.items(): need(sha(rel)==h,rel+' changed outside v28 scope')
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.28','executable version mismatch')
    subprocess.check_call([str(exe),'--v28-ui-state-self-test'])
print('v28-contract=pass palette=pass home-state=pass poster-first=pass tv-season-art=pass top-corner-clip=pass responsive-grid=pass utf8-metadata=pass library-posters=pass search-cleanup=pass player-matte=pass release-split=pass protected-boundaries=pass')
