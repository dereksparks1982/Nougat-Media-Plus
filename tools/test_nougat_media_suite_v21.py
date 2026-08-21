#!/usr/bin/env python3
from __future__ import annotations
import pathlib, sys

ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()

def require(c,m):
    if not c: raise SystemExit(m)

main=(ROOT/'src/main.cpp').read_text(encoding='utf-8')
cmake=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
desktop=(ROOT/'NougatMediaSuite.desktop').read_text(encoding='utf-8')

# Product/version/icon identity.
for marker in (
    '"v0.0.21"', 'const char* title = "Nougat Media Suite"', 'NougatMediaSuite',
    'nougat_media_suite_icon::kIcon16', 'nougat_media_suite_icon::kIcon32',
    'nougat_media_suite_icon::kIcon64', 'Nougat Media Suite | Just Nougat it.',
):
    require(marker in main, f'missing v0.0.21 identity marker: {marker}')

# Top-level navigation is exactly the owner-approved six areas. P2P is not top-level.
for marker in (
    'draw_tab(videoPlayerTab,"Video Player",ViewMode::VideoPlayer)',
    'draw_tab(libraryTab,"Library",ViewMode::Library)',
    'draw_tab(discoverTab,"Discover",ViewMode::Discover)',
    'draw_tab(nougatTab,"Search",ViewMode::Nougat)',
    'draw_tab(ytdlpTab,"Stream",ViewMode::Stream)',
    'draw_tab(debugTab,"Debug",ViewMode::Debug)',
    'clamp_button_scroll(topNavScrollX, 6, topNavViewportW)',
):
    require(marker in main, f'missing six-tab navigation marker: {marker}')
require('draw_tab(p2pTab' not in main, 'P2P leaked back into top-level navigation')
require('Rect videoPlayerTab, libraryTab, discoverTab, nougatTab, ytdlpTab, debugTab;' in main,
        'top-level rectangle inventory still contains P2P')

# Search owns Search/Crawler/P2P and hides peer administration behind Network/Advanced.
for marker in (
    'enum class NougatPanel { Search, Crawler, P2P };',
    'nougat_button(target,nougatSearchPanelTab,"Search"',
    'nougat_button(target,nougatCrawlerPanelTab,"Crawler"',
    'nougat_button(target,nougatP2PPanelTab,"P2P"',
    'nougat_button(target,nougatNetworkAdvancedBtn,nougatNetworkAdvanced?"Back":"Network..."',
    '"NETWORK / ADVANCED"',
    'if (nougatPanel == NougatPanel::P2P) {\n            draw_p2p_screen(target);',
    'handle_p2p_click(x,y)',
):
    require(marker in main, f'missing Search/P2P organization marker: {marker}')
require('NougatPanel::Peers' not in main and 'NougatPanel::About' not in main,
        'Peers/About remain ordinary Search panels')

# Candy palette: Video Player is chocolate, not the former ReddMedia red.
for marker in (
    'col(0x1b1b,0x0f0f,0x0808)', 'col(0x341a,0x1a1a,0x0d0d)',
    'col(0x6b35,0x3535,0x1a1a)', 'col(0xd2a5,0xa5a5,0x6d6d)',
    'const ViewPalette tabPalette = palette_for(view);',
    'fill(target, tab, active ? tabPalette.buttonLight : tabPalette.button);',
):
    require(marker in main, f'missing candy/top-tab palette marker: {marker}')

# Existing Stream set and dynamic service palettes only.
for marker in (
    'stream_palette_for(StreamPlatform platform)', 'StreamPlatform::YouTube',
    'StreamPlatform::Rumble','StreamPlatform::RuTube','StreamPlatform::VK','StreamPlatform::OK',
    'col(0xffff,0x0000,0x0000)','col(0x8585,0xc7c7,0x4242)','col(0xeded,0x1414,0x3b3b)',
    'col(0x1010,0x0909,0x4343)','col(0x1212,0x3a3a,0xeded)','col(0x9a9a,0x1313,0xeded)',
    'col(0x1212,0xcccc,0xeded)','col(0xeeee,0x8282,0x0808)',
):
    require(marker in main, f'missing Stream palette marker: {marker}')
for forbidden in ('StreamPlatform::Vimeo','StreamPlatform::Dailymotion','StreamPlatform::Twitch',
                  'StreamPlatform::Kick','StreamPlatform::TikTok','StreamPlatform::Bilibili',
                  'StreamPlatform::Niconico'):
    require(forbidden not in main, f'deferred service leaked into v0.0.21: {forbidden}')

# Library view controls are real icon buttons and still drive the persisted modes.
for marker in (
    'void draw_library_view_button(Drawable target, const Rect& r, LibraryDisplayMode mode, bool active)',
    'LibraryDisplayMode::List', 'LibraryDisplayMode::Grid',
    'for (int row = 0; row < 3; ++row)', 'for (int column = 0; column < 2; ++column)',
    'draw_library_view_button(target,libraryListViewBtn,LibraryDisplayMode::List',
    'draw_library_view_button(target,libraryGridBtn,LibraryDisplayMode::Grid',
    'if (libraryGridBtn.contains(x,y)) { set_library_display_mode(LibraryDisplayMode::Grid); return; }',
    'if (libraryListViewBtn.contains(x,y)) { set_library_display_mode(LibraryDisplayMode::List); return; }',
    'library_view_modes.cfg',
):
    require(marker in main, f'missing working Library view-icon marker: {marker}')
require('"Grid [x]"' not in main and '"List [x]"' not in main,
        'old Grid/List [x] text controls remain')

# Accepted v0.0.20 behavior markers retained.
for marker in (
    'std::min(200, vol + delta)','volRect.w*vol/200','"100%"',
    'prepare_tv_autoplay(','play_next_tv_episode()','poll_natural_playback_end()',
    'mark_active_episode_completed()','"Direct Watch"','"Open Webpage"',
    'draw_stream_screen(buffer)','copy_nougat_output_selection()','select_all_nougat_output()',
):
    require(marker in main, f'missing retained-v20 marker: {marker}')

require('project(NougatMediaSuite VERSION 0.0.21 LANGUAGES CXX)' in cmake,'CMake identity mismatch')
require('add_executable(Nougat_Media_Suite_v21' in cmake,'executable target mismatch')
require('Name=Nougat Media Suite' in desktop and 'Icon=nougat-media-suite' in desktop and
        'StartupWMClass=NougatMediaSuite' in desktop,'desktop identity mismatch')
require((ROOT/'src/nougat_media_suite_icon_data.hpp').is_file(),'icon header missing')
for size in (16,32,48,64,128,256,512):
    require((ROOT/f'assets/icons/nougat-media-suite-{size}.png').is_file(),f'icon size missing {size}')
require((ROOT/'assets/icons/nougat-media-suite.png').is_file(),'full icon missing')

print('v21-contract=pass identity=pass top-nav-6=pass Search-Crawler-P2P=pass network-advanced=pass video-chocolate=pass library-view-icons=pass stream-dynamic=pass icon-system=pass deferred-services=pass retained-v20-markers=pass')
