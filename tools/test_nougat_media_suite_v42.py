#!/usr/bin/env python3
from pathlib import Path
import hashlib
import os
import subprocess
import sys
import tempfile

root = Path(sys.argv[1]).resolve()
exe = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None

def need(ok, msg):
    if not ok:
        raise SystemExit('FAIL: ' + msg)

def read(rel):
    p = root / rel
    need(p.is_file(), 'missing required file: ' + rel)
    return p.read_text(errors='replace')

main = read('src/main.cpp')
cmake = read('CMakeLists.txt')
client = read('src/media_server/jellyfin_api_client.cpp')
client_h = read('src/media_server/jellyfin_api_client.hpp')
embed = read('src/recommendations/embedding_engine.cpp')
rec = read('src/recommendations/recommendation_engine.cpp')
tmdb = read('src/recommendations/tmdb_client.cpp')
security = read('components/security/nougat_security_worker.py')
readme = read('README.md')
changelog = read('CHANGELOG.md')
roadmap = read('ROADMAP.md')
company_bible = read('COMPANY_BIBLE.md')
icon_provenance = read('docs/design/NOUGAT_ICON_SOURCE_v42.md')

# Release identity.
need('VERSION 0.0.42' in cmake and 'Nougat_Media_Suite_v42' in cmake, 'v0.0.42 CMake identity missing')
need('const std::string versionLabel = "v0.0.42";' in main, 'v0.0.42 top-bar version missing')
need(readme.startswith('# Nougat Media Suite v0.0.42'), 'README version missing')
need('## v0.0.42 - Persistent Libraries, Live TV Maintenance, Security, Intelligence, and Games' in changelog,
     'CHANGELOG v0.0.42 record missing')
need('## v0.0.42 candidate — Persistence, Intelligence, Live TV, Security, and Games' in roadmap,
     'ROADMAP v0.0.42 owner scope missing')
for desktop in ('NougatMediaSuite.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
    desktop_text = read(desktop)
    need('Nougat_Media_Suite_v42' in desktop_text, desktop + ' does not launch v42')
    need('Icon=nougat-media-suite-concept-sheet-v24' in desktop_text, desktop + ' does not use approved N icon key')

# Permanent no-exceptions Nougat icon release gate.
def sha256(rel):
    p = root / rel
    need(p.is_file(), 'missing icon-gate file: ' + rel)
    return hashlib.sha256(p.read_bytes()).hexdigest()

need(sha256('docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png') ==
     'a37fb2dd309af0404c615c6f2519da748952d194e44d7b796bf7d46353e92e62',
     'approved UI sheet is not the exact owner-supplied icon source')
need(sha256('assets/icons/nougat-media-suite-concept-sheet-v24.png') ==
     '681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804',
     'approved concept-sheet N master changed')
need(sha256('src/nougat_media_suite_icon_data.hpp') ==
     'c626664598d57a3756a62a425875fd48567ae4eaa8c9b5a385ccf4630a0b22cb',
     'embedded X11 Nougat icon data changed')
for rel in (
    'assets/icons/nougat-media-suite-16.png', 'assets/icons/nougat-media-suite-32.png',
    'assets/icons/nougat-media-suite-48.png', 'assets/icons/nougat-media-suite-64.png',
    'assets/icons/nougat-media-suite-128.png', 'assets/icons/nougat-media-suite-256.png',
    'assets/icons/nougat-media-suite-512.png',
):
    need((root/rel).is_file(), 'approved icon family member missing: ' + rel)
need('NO EXCEPTIONS RELEASE GATE' in company_bible and
     'raw versioned root executable as shown by Files/Nautilus' in company_bible and
     'top center of `docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png`' in company_bible,
     'Company Bible permanent icon law missing')
need('exact owner-uploaded `NougatSpriteSheet(7).png` bytes' in icon_provenance,
     'icon-source provenance is not pinned to the owner sheet')
need('#include "nougat_media_suite_icon_data.hpp"' in main and 'append_net_wm_icon' in main,
     'embedded X11/dock icon contract missing')

# Persistent library mappings and compatibility.
for token in (
    'library_mappings.tsv',
    'restore_mapping_registry',
    'save_mapping_registry',
    'update_mapping_registry(path, media_type, true',
    'update_mapping_registry(path, media_type, false',
    'directory_available(saved.path)',
):
    need(token in client, 'persistent library mapping contract missing: ' + token)
need('constexpr const char* kMovieLibraryName = "ReddMedia Movies";' in client and
     'constexpr const char* kTelevisionLibraryName = "ReddMedia TV";' in client,
     'legacy Jellyfin library identities were renamed instead of preserved')
need('std::string mapping_state_file_' in client_h, 'mapping state field missing')
need('The hidden catalog was configured outside ReddMedia' not in client,
     'stale destructive ReddMedia recovery message remains')

# Intelligence / metadata / Discover performance.
for token in (
    'canonical_series_title_from_structure',
    'strip_trailing_tv_season_marker',
    'node.child_count > 1',
    'resolve_metadata_identity',
    'exact canonical-title match is required for auto-apply',
):
    need(token in main or token in rec or token in tmdb, 'metadata intelligence marker missing: ' + token)
for token in ('cache_dir_', 'load_cached_embedding', 'save_cached_embedding', '/reddmedia/intelligence/embeddings'):
    need(token in embed, 'persistent embedding cache missing: ' + token)
need('redraw();\n            return;\n        }' in main and 'RecommendationSource::LiveTV' in main,
     'Discover Live TV branch missing')
# The Live TV branch must leave the mutex before redraw to avoid self-deadlock.
live_start = main.find('if (source == reddmedia::RecommendationSource::LiveTV)')
need(live_start >= 0, 'Discover Live TV selection branch missing')
live_chunk = main[live_start:live_start+2300]
need('std::lock_guard<std::mutex> lock(discoverState->mutex);' in live_chunk, 'Discover Live TV state lock missing')
need(live_chunk.find('redraw();') > live_chunk.find('}'), 'Discover Live TV redraw appears before lock scope closes')

# Live TV guide persistence/maintenance and World TV.
need('config_dir_ + "/guide.tsv"' in read('src/live_tv/tuner_backend.cpp'), 'guide cache is not persistent outside build')
for token in (
    'kGuideStaleSeconds = 12LL * 3600LL',
    'liveTvFullGuideRefreshQueued = true',
    'start_live_tv_current_mux_guide_harvest(false)',
    'button_on_state(target, liveTvGuideRefreshBtn, "Refreshing"',
    '((now_ms() / 800LL) % 2LL)',
):
    need(token in main, 'guide maintenance/Refreshing contract missing: ' + token)
need('"Refreshing..."' not in main, 'Refresh Guide active label incorrectly contains dots')
for token in (
    'button_on(target, liveTvWorldBtn, "World TV")',
    'Internet broadcaster feeds. Nougat does not filter this catalog by your location.',
    'https://www.youtube.com/watch?v=ABfFhWzWs0s',
    'https://www.youtube.com/watch?v=Ap-UM1O9RBU',
    'https://www.youtube.com/watch?v=gCNeDWCI0vo',
):
    need(token in main, 'World TV contract missing: ' + token)

# Live TV -> local playback ownership handoff.
request_at = main.find('void request_local_playback(')
need(request_at >= 0, 'shared local playback entry point missing')
request_chunk = main[request_at:request_at+2600]
need('if (currentMediaIsLiveTv) cleanup_player();' in request_chunk,
     'local media does not release active Live TV before resume/playback')
need(request_chunk.find('cleanup_player();') < request_chunk.find('ResumeRecord record;'),
     'Live TV cleanup occurs too late, after resume prompt decision')

# Player control correction and exact right-click subtitle UX.
need('const int bottomY = H - 40;' in main, 'transport row was not lifted')
need('const int volumeHousingY = H - 91;' in main, 'VOLUME row was not lifted')
need('const int seekY = H - 140;' in main, 'seek row changed unexpectedly')
need('items.push_back({"Subtitles On / Off", MenuAction::SubtitleToggle, 0});' in main,
     'right-click subtitle row changed identity')
need('item.action == MenuAction::SubtitleToggle' in main and 'boldFontInfo' in main,
     'subtitle active-state bold renderer missing')
need('subtitlesOn ? bold : normal' in main and 'subtitlesOn ? normal : bold' in main,
     'subtitle On/Off active word is not selected by state')
need('void handle_context_menu_click(int, int y)' in main and '(y - 8) / 26' in main,
     'full-row context menu hit routing missing')

# Card geometry + My Services scrollbar/header.
need('owner geometry repair: choose the column count first' in main,
     'responsive Library fill repair missing')
need('discoverServicesScrollDragging' in main and 'update_discover_services_scroll_from_pointer' in main,
     'My Services draggable scrollbar missing')
need('MY STREAMING SERVICES - UNITED STATES' in main and 'title/help copy occupy separate lines' in main,
     'My Services header separation repair missing')

# Virus Scan expansion and safe traversal.
for token in ('Scan Movies','Scan TV','Quick Scan','System Scan','--mapped-library','--quick-scan','--system-scan'):
    need(token in main or token in security, 'Virus Scan expansion missing: ' + token)
for token in ('Path("/proc")','Path("/sys")','Path("/dev")','Path("/run")','NETWORK_FS_TYPES','followlinks=False'):
    need(token in security, 'System Scan safe traversal missing: ' + token)
need('library_mappings.tsv' in security, 'Movies/TV scan does not use persistent mapping registry')

# Games is a real top-level functional surface after Studio.
need('enum class ViewMode { Home, VideoPlayer, Library, Discover, LiveTV, Nougat, Stream, Studio, Games, P2P, Debug };' in main,
     'Games ViewMode missing')
need('studioTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;\n        gamesTab = {topX,1,kTopTabW,kTopTabH};' in main,
     'Games tab is not immediately after Studio')
for token in (
    'draw_games_screen', 'start_game_scan', 'rom-folders.txt', 'scan_game_directory',
    'installed_game_emulator', 'execlp(emulator.c_str()', 'User ROMs are indexed in place',
    'Library", GamesPanel::Library', 'Systems", GamesPanel::Systems',
    'Controllers", GamesPanel::Controllers', 'Settings", GamesPanel::Settings',
):
    need(token in main, 'functional Games contract missing: ' + token)
for rel in (
    'components/games/bundled/BUNDLED_GAMES_SOURCES.md',
    'components/games/bundled/licenses/2048-nes-UNLICENSE.txt',
    'components/games/bundled/licenses/NES-Waveforms-MIT.txt',
):
    need((root/rel).is_file(), 'bundled-game licensing/source record missing: ' + rel)
sources = read('components/games/bundled/BUNDLED_GAMES_SOURCES.md')
need('a6f341e925e6686b88a3f6df39c8fa2329e22c18' in sources and
     '8e23c4bdbaf74218cbe21736c99ebb38a62d8bff' in sources,
     'pinned starter ROM Git blob identities missing')

# Stale user-visible branding is gone from the intended modules while compatibility IDs remain.
for rel in ('src/media_server/library_poster.cpp','src/recommendations/watch_provider_preferences.cpp','src/recommendations/viewing_history.cpp'):
    need('ReddMedia' not in read(rel), 'stale user-visible branding remains in ' + rel)

# Python security worker is syntactically valid and traversal policy can be imported.
subprocess.run([sys.executable, '-m', 'py_compile', str(root/'components/security/nougat_security_worker.py')], check=True)
with tempfile.TemporaryDirectory(prefix='nougat-v42-security-') as td:
    env = dict(os.environ)
    env['HOME'] = td
    probe = subprocess.run([
        sys.executable, '-c',
        "import importlib.util,sys; p=sys.argv[1]; s=importlib.util.spec_from_file_location('w',p); m=importlib.util.module_from_spec(s); s.loader.exec_module(m); assert {str(x) for x in m.PSEUDO_ROOTS}=={'/proc','/sys','/dev','/run'}; print('security traversal policy ok')",
        str(root/'components/security/nougat_security_worker.py')
    ], env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(probe.returncode == 0, 'security traversal policy import failed: ' + probe.stdout)

if exe is not None:
    need(exe.is_file() and os.access(exe, os.X_OK), 'v42 executable missing/not executable')
    version = subprocess.run([str(exe), '--version'], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    need(version.returncode == 0 and version.stdout.strip() == 'Nougat Media Suite v0.0.42',
         '--version did not report exact v0.0.42')
    # Retained high-value headless self-tests that remain semantically valid after the v42 UI changes.
    for flag in (
        '--v25-ui-state-self-test', '--v28-ui-state-self-test', '--v29-tv-reliability-self-test',
        '--v31-ui-sheet-self-test', '--discover-ai-self-test', '--v32-p2p-player-repair-self-test',
        '--v33-integration-self-test', '--v35-cleanup-self-test', '--v36-library-ui-player-self-test',
        '--v37-live-tv-system-self-test', '--v39-diagnostic-self-test',
    ):
        result = subprocess.run([str(exe), flag], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        need(result.returncode == 0, flag + ' regression failed: ' + result.stdout[-1200:])

print('Nougat Media Suite v0.0.42 contract PASS')
