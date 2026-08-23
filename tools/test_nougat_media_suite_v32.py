#!/usr/bin/env python3
from __future__ import annotations
import pathlib, subprocess, sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
main=(root/'src/main.cpp').read_text(encoding='utf-8')
p2ph=(root/'src/p2p_engine.hpp').read_text(encoding='utf-8')
p2p=(root/'src/p2p_engine.cpp').read_text(encoding='utf-8')
server=(root/'src/p2p_stream_server.cpp').read_text(encoding='utf-8')
diag=(root/'src/diagnostics/diagnostic_engine.cpp').read_text(encoding='utf-8')
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
road=(root/'ROADMAP.md').read_text(encoding='utf-8')
need('VERSION 0.0.32' in cm and 'Nougat_Media_Suite_v32' in cm,'CMake v32 identity missing')
need('printf("Nougat Media Suite v0.0.32\\n")' in main and 'const std::string versionLabel = "v0.0.32"' in main,'runtime/in-app v32 identity missing')
for token in ['bool remove_transfer(std::string& error);','void prioritize_playback_window(std::uint64_t offset);','selected_progress','selected_buffered_bytes']:
    need(token in p2ph,'P2P v32 header contract missing: '+token)
for token in ['session.remove_torrent(impl_->handle)','fs::remove(resume_file(), ec)','fs::remove(selected_file_state(), ec)','constexpr std::uint64_t immediate = 8ULL * 1024ULL * 1024ULL','constexpr std::uint64_t ahead = 48ULL * 1024ULL * 1024ULL','constexpr std::uint64_t rewind = 4ULL * 1024ULL * 1024ULL']:
    need(token in p2p,'P2P engine v32 behavior missing: '+token)
need('engine_.clear_stream_priority();' in server,'new range must clear stale P2P deadlines')
need('engine_.prioritize_playback_window(position);' in server,'range server does not drive moving P2P window')
for token in ['button_on(target,p2pPlayBtn,"Watch Now")','button_on(target,p2pRemoveBtn,"Remove")','Selected media: ','Start buffer: ','Magnet handed to P2P from Search.','magnetResult?"Open P2P":"Open Tor"']:
    need(token in main,'P2P UI integration missing: '+token)
for token in ['p2p_selected_progress','p2p_selected_buffered_bytes','p2p_stream_running']:
    need(token in main or token in diag,'P2P diagnostics evidence missing: '+token)
# Search cream seam is intentionally darker without changing palette functions.
need('rgb8(132, 78, 40)' in main and 'rgb8(126, 72, 35)' in main,'Search cream seam contrast repair missing')
# Volume deliberately reuses the Seek component and no longer has a separate oversized outer housing.
need('Rect openBtn, rewindBtn, previousBtn, playBtn, nextBtn, forwardBtn, stopBtn, fsBtn, seekRect, volRect, volumeHousingRect' in main,'volume state missing')
need('volRect = {volumeTrackX, volumeY - 4, volumeTrackW, seekRect.h};' in main,'seek-style volume geometry missing')
vol_start=main.index('    void draw_volume_bar('); vol_end=main.index('    bool episode_navigation_available',vol_start); volume=main[vol_start:vol_end]
need('draw_sheet_track(target, volRect, trackBorder, creamTrack' in volume,'volume is not using the Seek-style sheet track')
need('draw_concept_field(target, housing' not in volume,'rejected oversized volume housing returned')
# Up Next redraw must be composed offscreen; direct clear of live video window is forbidden in countdown block.
start=main.index('    void draw_video_message() {'); end=main.index('        if (resumePromptVisible) {',start)
upnext=main[start:end]
need('XCreatePixmap' in upnext and 'XCopyArea' in upnext,'Up Next offscreen composition repair missing')
need('XClearWindow(d,videoWin)' not in upnext,'Up Next still clears live video window during countdown')
# Stream provider panel must match Discover silhouette: no left-side accent stripe; Vimeo explicitly retained.
start=main.index('    void draw_stream_screen('); end=main.index('    std::string format_bytes(',start)
stream=main[start:end]
need('source_button(streamVimeoTab,"Vimeo",StreamPlatform::Vimeo);' in stream,'Vimeo Stream tab missing')
need('fill(target,{logBox.x,logBox.y,6,logBox.h},palette.accent);' not in stream,'Stream provider left accent strip still present')
need('draw_primary_panel(target, logBox, palette);' in stream,'Stream sheet-style lower panel treatment missing')
need('v0.0.32 candidate — Native P2P Media + Nougat Security Analysis' in road,'v32 roadmap identity missing')

# Same-version security replacement and owner-reported surgical repairs.
for token in ['NougatPanel { Search, Crawler, P2P, VirusScan }','"Virus Scan"','"Scan File"','"Scan Folder"','"Scan Again"','components/security/nougat_security_worker.py']:
    need(token in main,'v32 security UI/integration missing: '+token)
need('"Node "+node' not in main and '"Node ID: "+node' in main,'ordinary Search Node ID cleanup missing')
need('text(target,28,174,status,searchPalette.text);' in main and 'nougatCrawlLogBox = {28, 190,' in main,'Crawler status position-only repair missing')
for token in ['known_peers','known_seeds','swarm_availability','announcing_trackers','announcing_dht','announcing_lsd']:
    need(token in p2ph,'P2P seed/availability status missing: '+token)
for token in ['You: Seed ✓','Available, idle','your complete local copy = 1.00']:
    need(token in main,'P2P seeding clarity UI missing: '+token)
security=(root/'components/security/nougat_security_worker.py').read_text(encoding='utf-8')
need('WARN ME FIRST' in security and 'no daemon mode' in security,'security policy contract missing')
need('v0.0.33 planned — P2P Plus' in road,'P2P Plus roadmap assignment missing')
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.32','v32 executable version mismatch')
    subprocess.check_call([str(exe),'--v32-p2p-player-repair-self-test'])

# Home scrolling controls are explicit and sheet-styled.
need('XRectangle homeClip{0, static_cast<short>(viewport_top)' in main, 'Home fixed-header clip missing')
need('homeVerticalScrollTrack = {W - 18, viewport_top + 4, 12, verticalTrackH};' in main, 'Home vertical scrollbar missing')
need('homeContinueScrollTrack = {homeContinueArea.x, scrollY, homeContinueArea.w, 14};' in main, 'Continue Watching horizontal scrollbar missing')
need('handle_home_scrollbar_press' in main, 'Home scrollbar click/drag routing missing')
need('homeVerticalScrollDragging = false;' in main, 'Home scrollbar release cleanup missing')
print('v32-contract=pass p2p-native=pass selected-progress=pass playback-window=pass seed-availability=pass security-analysis=pass warn-first=pass virus-scan-tab=pass crawler-position-only=pass node-cleanup=pass autoplay-flicker=pass volume-seek-style=pass search-seams=pass stream-bottom-only=pass vimeo=pass')
