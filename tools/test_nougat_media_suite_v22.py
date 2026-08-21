#!/usr/bin/env python3
from __future__ import annotations
import pathlib, sys
ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()

def need(c,m):
    if not c: raise SystemExit(m)
main=(ROOT/'src/main.cpp').read_text(encoding='utf-8')
cmake=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
desktop=(ROOT/'NougatMediaSuite.desktop').read_text(encoding='utf-8')

need('"v0.0.22"' in main, 'v0.0.22 in-app version marker missing')
need('Nougat Media Suite v0.0.22' in main, 'v0.0.22 CLI version marker missing')
need('project(NougatMediaSuite VERSION 0.0.22 LANGUAGES CXX)' in cmake, 'CMake version mismatch')
need('add_executable(Nougat_Media_Suite_v22' in cmake, 'v22 executable target mismatch')
need('BUILD_WITH_INSTALL_RPATH TRUE' in cmake, 'v22 must build with its relocatable install RPATH')
need('INSTALL_RPATH "\\$ORIGIN/components/ai/runtime/lib;\\$ORIGIN/components/ai/runtime/lib64"' in cmake, 'relative llama runtime RPATH missing')
need('INSTALL_RPATH_USE_LINK_PATH FALSE' in cmake, 'absolute link paths must not leak into final RPATH')
need('-Wl,-rpath-link,${REDDMEDIA_AI_RUNTIME}/lib' in cmake, 'link-time llama dependency discovery missing')
need('BUILD_RPATH "${REDDMEDIA_AI_RUNTIME}' not in cmake, 'old absolute AI BUILD_RPATH leaked into v22')
need('Exec=/home/dereksparks1982/DKLab/Projects/Nougat Media Suite/Nougat_Media_Suite_v22' in desktop, 'desktop launcher does not use renamed project path/v22 executable')

for marker in (
    'draw_tab(videoPlayerTab,"Video Player",ViewMode::VideoPlayer)',
    'draw_tab(libraryTab,"Library",ViewMode::Library)',
    'draw_tab(discoverTab,"Discover",ViewMode::Discover)',
    'draw_tab(nougatTab,"Search",ViewMode::Nougat)',
    'draw_tab(ytdlpTab,"Stream",ViewMode::Stream)',
    'draw_tab(debugTab,"Debug",ViewMode::Debug)',
    'enum class NougatPanel { Search, Crawler, P2P };',
    'stream_palette_for(StreamPlatform platform)',
    'StreamPlatform::YouTube','StreamPlatform::Rumble','StreamPlatform::RuTube','StreamPlatform::VK','StreamPlatform::OK',
    'std::min(200, vol + delta)','volRect.w*vol/200','"100%"',
    'prepare_tv_autoplay(','play_next_tv_episode()','poll_natural_playback_end()',
    '"Direct Watch"','"Open Webpage"','draw_stream_screen(buffer)',
    'draw_library_view_button(target,libraryListViewBtn,LibraryDisplayMode::List',
    'draw_library_view_button(target,libraryGridBtn,LibraryDisplayMode::Grid',
    'copy_nougat_output_selection()','select_all_nougat_output()',
):
    need(marker in main, f'retained v0.0.21 behavior marker missing: {marker}')
need('draw_tab(p2pTab' not in main, 'P2P leaked back to top-level navigation')
for forbidden in ('StreamPlatform::Vimeo','StreamPlatform::Dailymotion','StreamPlatform::Twitch','StreamPlatform::Kick','StreamPlatform::TikTok','StreamPlatform::Bilibili','StreamPlatform::Niconico'):
    need(forbidden not in main, f'deferred Stream service leaked into license-only v22: {forbidden}')

need((ROOT/'COPYRIGHT.md').is_file(), 'COPYRIGHT.md missing')
need((ROOT/'CONTRIBUTING.md').is_file(), 'CONTRIBUTING.md missing')
need((ROOT/'docs/LICENSING_POLICY.md').is_file(), 'LICENSING_POLICY.md missing')
print('v22-contract=pass version=pass license-only=pass retained-v21=pass launcher-path=pass deferred-features=pass')
