#!/usr/bin/env python3
from pathlib import Path
import subprocess, sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
exe=Path(sys.argv[2]).resolve() if len(sys.argv)>2 else root/'Nougat_Media_Suite_v33'
def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
main=(root/'src/main.cpp').read_text()
cm=(root/'CMakeLists.txt').read_text()
p2ph=(root/'src/p2p_engine.hpp').read_text(); p2pc=(root/'src/p2p_engine.cpp').read_text()
server=(root/'src/media_server/media_server_manager.cpp').read_text(); tuner=(root/'src/live_tv/tuner_backend.cpp').read_text(); worker=(root/'components/security/nougat_security_worker.py').read_text()
need('VERSION 0.0.33' in cm and 'Nougat_Media_Suite_v33' in cm,'v33 build identity missing')
need('printf("Nougat Media Suite v0.0.33\\n")' in main and 'const std::string versionLabel = "v0.0.33"' in main,'v33 runtime/UI identity missing')
need('homeTab = ' in main and 'liveTvTab = ' in main and 'discoverTab.x < app.liveTvTab.x' in main,'Live TV top-nav insertion/order missing')
need('page_content_frame(ViewMode view)' in main and 'if (view == ViewMode::VideoPlayer)' in main and 'draw_page_frame' in main,'page-frame law/Video Player exclusion missing')
for token in ['homeContinueArea','homeVerticalScrollTrack','homeContinueScrollTrack','libraryVerticalScrollTrack','libraryToolRows','topNavClipX','topNavClipRight']:
    need(token in main,'viewport/scroll containment missing: '+token)
for token in ['set_speed_limits','set_seed_rules','queue_up','queue_down','force_reannounce','force_recheck','set_file_priority','trackers','enforce_seed_rules']:
    need(token in p2ph and token in p2pc,'P2P Plus method missing: '+token)
need('entry.message' not in p2pc,'libtorrent 2.0.x tracker adapter still references nonexistent announce_entry::message')
need('entry.verified ? "verified" : "waiting"' in p2pc,'libtorrent 2.0.x tracker adapter compatibility mapping missing')
need('setsid()' in server and 'MediaServerManager::~MediaServerManager() = default' in server and 'persistent-enabled' in server,'persistent server lifecycle missing')
for token in ['NOUGAT_MEDIA_SERVER_OWNER','owned_processes','process_has_owner_token','process_matches_nougat_signature','persist_owned_record','SIGTERM','SIGKILL']:
    need(token in server,'owned server process-tree stop contract missing: '+token)
need('killall' not in server and 'pkill' not in server and 'pgrep' not in server,'server stop must never kill Jellyfin by process name')
need('PR_SET_PDEATHSIG' not in server,'persistent server still tied to parent death')
need('mediaServer.stop();' not in main.split('void shutdown()',1)[1].split('};',1)[0],'UI shutdown still stops server')
for token in ['/dev/dvb','/sys/class/video4linux','WinTV-HVR-955Q','begin_channel_scan']:
    need(token in tuner or token in main,'Live TV scaffold missing: '+token)
for token in ['ANALYSIS INCOMPLETE','urlhaus','Threat intelligence key not configured']:
    need(token in worker or token in main,'security hardening missing: '+token)
need('"Safe"' not in worker,'forbidden Safe verdict present')
if exe.is_file():
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.33','v33 executable version mismatch')
    subprocess.run([str(exe),'--v33-integration-self-test'],check=True,env={**__import__('os').environ,'HOME':'/tmp/nougat-v33-contract-home'})
print('v33-contract=pass page-viewports=pass nav-clip=pass library-scroll=pass home-clip=pass p2p-plus=pass persistent-server=pass live-tv=pass security-hardening=pass video-player-excluded=pass')
