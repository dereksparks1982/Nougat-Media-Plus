#!/usr/bin/env python3
from __future__ import annotations
import hashlib,pathlib,re,subprocess,sys
root=pathlib.Path(sys.argv[1]).resolve(); exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None
def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
def sha(p):
    h=hashlib.sha256();
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
main=(root/'src/main.cpp').read_text(errors='replace'); cm=(root/'CMakeLists.txt').read_text(errors='replace'); tuner=(root/'src/live_tv/tuner_backend.cpp').read_text(errors='replace')
types=(root/'src/diagnostics/diagnostic_types.hpp').read_text(errors='replace'); engine_hpp=(root/'src/diagnostics/diagnostic_engine.hpp').read_text(errors='replace'); engine=(root/'src/diagnostics/diagnostic_engine.cpp').read_text(errors='replace')
need('VERSION 0.0.39' in cm and 'Nougat_Media_Suite_v39' in cm,'v0.0.39 CMake identity missing')
for tok in ('Passed','NeedsAttention','Problem','NotTested','Information'): need(tok in types,'new severity missing: '+tok)
# The rejected v0.0.39 working tree already consumes this richer diagnostic API.
# Keep the engine/header source-compatible with that exact candidate instead of
# replacing one side of the interface and discovering the mismatch at compile time.
for tok in (
 'playback_is_live_tv','live_tv_tuner_count','live_tv_channel_count',
 'live_tv_guide_channels_with_data','live_tv_guide_cache_mtime','live_tv_guide_refresh_busy',
 'live_tv_current_mux_harvest_active','live_tv_full_refresh_queued','live_tv_tuner_name',
 'live_tv_tuner_backend','live_tv_tuner_status','live_tv_frontend_path','live_tv_demux_path',
 'live_tv_dvr_path','live_tv_net_path','live_tv_frontend_accessible','live_tv_demux_accessible',
 'live_tv_dvr_accessible','live_tv_net_accessible','live_tv_signal_lock','live_tv_delivery_systems',
 'live_tv_current_station','live_tv_current_frequency','live_tv_current_program_number',
 'live_tv_current_program_title','live_tv_current_program_start','live_tv_current_program_end',
 'search_status','DiagnosticCheck','checks','attention_count','section','name'):
    need(tok in types,'rejected-v39 diagnostic API compatibility missing: '+tok)
need('write_history_snapshot' in engine_hpp and 'DiagnosticEngine::write_history_snapshot' in engine,
     'rejected-v39 diagnostic history API compatibility missing')
for tok in ('Expected:','Observed:','Evidence:','Repair:','SYSTEM HEALTH:','append_history','read_history'): need(tok in engine,'diagnostic evidence/history contract missing: '+tok)
need('Quick Diagnostic' in main and 'Deep Diagnostic' in main,'Quick/Deep Diagnostic controls missing')
need('--v39-diagnostic-self-test' in main,'v39 CLI diagnostic self-test missing')
need('--v39-channel-logo-audit' in main,'v39 channel-logo acceptance audit missing')
need('Stop Live TV playback before refreshing the broadcast guide.' not in main,'old stop-playback guide behavior returned')
need('liveTvGuideRefreshQueued' in main,'idle-tuner queued guide refresh missing')
need('harvest_current_multiplex_guide' in main,'current-multiplex PSIP playback integration missing')
need('current_mux_only' in main,'current-multiplex guide ownership state missing')
need('!currentMediaIsLiveTv' in main,'deferred full guide sweep is not gated on playback release')
logo_match=re.search(r'void\s+draw_live_tv_channel_logo\s*\([^)]*\)\s*\{([\s\S]*?)\n\s*\}\n\s*void\s+refresh_live_tv_tuners',main)
need(logo_match is not None,'Live TV channel-logo renderer not found')
logo_body=logo_match.group(1)
need('v0.0.39: real channel artwork only' in logo_body and
     'std::string call=channel.name.empty()?channel.id:channel.name;' not in logo_body and
     'call.size()>5U' not in logo_body and 'head_to_width(call' not in logo_body,
     'text channel-art fallback returned inside channel-logo renderer')
need('const std::vector<LiveTvProgram> preserved' in tuner or 'v0.0.39 merge/preserve' in tuner,'guide cache preservation missing')
need('type <= 0x0103U' in tuner,'expanded EIT table collection missing')
need('NougatTunerBackend::harvest_current_multiplex_guide' in tuner,'current-multiplex PSIP backend missing')
vol=root/'assets/ui/nougat_volume_sheet_frames.bin'
need(vol.is_file() and sha(vol)=='38197798a97e9ecadf3934daca692446bea586b36e2038c533aa5c92f51077e2','VOLUME protected asset changed')
# The owner's 2026-08-23 full component sheet is the sole approved UI authority.
# Screenshots are bug/layout evidence only and are never UI source art.
for rel,expected in {
 'docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png':'d9e57f0276877cecb69e0d8c23e3a955a78742c135221d9ff4cd902eacb1ad25',
 'assets/ui/nougat_seek_sheet_frames.bin':'edc27c16675e1114d64be3e233f20361f21a167666bf4257943705d8aeab9b16',
 'assets/ui/nougat_progress_sheet_frames.bin':'3e1ab4aa3063e558f934cabcaeb8555f63da8b8c6f1b4c8024f4ff05a904c6ce'}.items():
    p=root/rel; need(p.is_file() and sha(p)==expected,'approved sheet asset changed: '+rel)
if exe:
    need(exe.is_file(),'v39 executable missing')
    out=subprocess.check_output([str(exe),'--version'],text=True,stderr=subprocess.STDOUT).strip(); need(out=='Nougat Media Suite v0.0.39','v39 version mismatch: '+repr(out))
    # Retained executable behavior, then new diagnostic semantics.
    for test in ('--v35-cleanup-self-test','--v36-library-ui-player-self-test','--v37-live-tv-system-self-test','--v38-library-live-tv-player-self-test','--v39-diagnostic-self-test'):
        subprocess.run([str(exe),test],check=True,timeout=60)
print('v39-contract=pass diagnostics=pass guide-preserve=pass guide-queue=pass current-mux-psip=pass real-art-only=pass volume-untouched=pass retained-v38-runtime=pass')
