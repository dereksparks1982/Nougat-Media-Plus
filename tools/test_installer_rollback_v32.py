#!/usr/bin/env python3
from __future__ import annotations
import pathlib,re,sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
installer=root/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_32.sh'
manifest=root/'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v32.json'

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
need(installer.is_file(),'v32 installer missing')
text=installer.read_text(encoding='utf-8')
need('expected_head="b22d11d4989784dc6df56abbde08344720064790"' in text,'accepted v31 commit gate missing')
need('Nougat_Media_Suite_v31' in text and 'Nougat Media Suite v0.0.31' in text,'accepted v31 executable gate/rollback missing')
need('Nougat_Media_Suite_v32' in text and 'Nougat Media Suite v0.0.32' in text,'v32 final executable/version gate missing')
need("m.get('target_version') != '0.0.32'" in text,'manifest target v32 gate missing')
need("m.get('accepted_base_commit') != 'b22d11d4989784dc6df56abbde08344720064790'" in text,'manifest base commit gate missing')
need('save exact accepted v0.0.31 rollback snapshot' in text,'rollback snapshot phase missing')
need('ROLLBACK PASS: accepted v0.0.31 touched state restored.' in text,'rollback proof missing')
need('apply_raw_icon "$project_root/Nougat_Media_Suite_v32"' in text,'final raw icon application missing')
need(text.index('cp "$build_root/full/Nougat_Media_Suite_v32"') < text.index('apply_raw_icon "$project_root/Nougat_Media_Suite_v32"'),'raw icon must follow final executable write')
need('gio info -a metadata::custom-icon' in text,'raw icon readback missing')
for token in [
    'tools/test_nougat_media_suite_retained_v31.py','tools/test_nougat_media_suite_v32.py',
    'tools/test_nougat_media_suite_ui_smoke_v32.py','tools/test_p2p_stream_server_v32.py',
    'tools/test_nougat_diagnostics_v26.py','tools/test_license_protection_v22.py','tools/test_nougat_v19.py',
    'tools/test_nougat_bridge_v19.py','tools/test_media_server_lifecycle_v17.py',
    'REDDMEDIA_P2P_STUB=ON','REDDMEDIA_AI_STUB=ON','pkg-config --exists libtorrent-rasterbar',
    '--v29-tv-reliability-self-test','--v30-ui-library-player-self-test','--v31-ui-sheet-self-test',
    '--v32-p2p-player-repair-self-test','ffmpeg','xvfb-run',
]: need(token in text,'required installer gate missing: '+token)
for rel in [
    'APPLY_COMMAND.txt','CHANGELOG.md','CMakeLists.txt','README.md','ROADMAP.md',
    'NougatMediaSuite.desktop','NougatMediaSuite_v32.desktop','com.elderredsoftworks.NougatMediaSuite.desktop',
    'src/main.cpp','src/p2p_engine.cpp','src/p2p_engine.hpp','src/p2p_stream_server.cpp',
    'src/diagnostics/diagnostic_engine.cpp','src/diagnostics/diagnostic_types.hpp',
    'tools/test_nougat_media_suite_retained_v31.py','tools/test_nougat_media_suite_v32.py',
    'tools/test_nougat_media_suite_ui_smoke_v32.py','tools/test_p2p_stream_server_v32.py','tools/test_installer_rollback_v32.py',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_32_NATIVE_P2P_MEDIA_AND_PLAYER_UI_REPAIR_HANDSHAKE.md',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_32_NATIVE_P2P_MEDIA_AND_PLAYER_UI_REPAIR_VALIDATION.md',
    'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_32.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v32.json',
]: need(f'"{rel}"' in text,'modified path absent from installer: '+rel)
modified_block=text.split('modified_paths=(',1)[1].split(')\n',1)[0]
for rel in [
    'LICENSE','COPYRIGHT.md','CONTRIBUTING.md','THIRD_PARTY_NOTICES.md','docs/LICENSING_POLICY.md',
    'components/nougat/nougat_engine.py','src/nougat/nougat_bridge.cpp','src/nougat/nougat_bridge.hpp',
    'docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png','src/nougat_ui_sheet_texture_data.hpp',
]: need(f'"{rel}"' not in modified_block,'protected/retained file incorrectly included in changed paths: '+rel)
for rel in ['src/p2p_engine.cpp','src/p2p_engine.hpp','src/p2p_stream_server.cpp']:
    need(f'"{rel}"' in modified_block,'P2P implementation missing from v32 scope: '+rel)
for pattern,label in [
    (r'(^|[;\s])exit(?:\s+[0-9]+)?(?:[;\s]|$)','shell exit'),
    (r'\|\|\s*exit\b','|| exit'),(r'\bset\s+-e\b','set -e'),
]: need(re.search(pattern,text,re.M) is None,label+' forbidden by terminal-safety rule')
need('rm -f -- "$project_root/Nougat_Media_Suite_v31"' in text,'accepted v31 root replacement cleanup missing')
for rel in ['INSTALL_NOUGAT_MEDIA_SUITE_v0_0_31.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v31.json','NougatMediaSuite_v31.desktop']:
    need(f'"{rel}"' in text,'superseded v31 cleanup/rollback path missing: '+rel)
need('Search cream seam ink darkened only' in text,'palette-preservation/contrast status missing')
need('YouTube/Vimeo/Rumble/RuTube/VK/OK' in text,'Stream provider cleanup owner check missing')
if manifest.exists(): need(manifest.is_file(),'v32 manifest is not a regular file')
need('candidate-v32' in text and 'accepted_candidate_files' in text,'same-version replacement does not accept exact first-v32 candidate')
need('if [[ "$base_mode" == "v31" ]]' in text and 'elif [[ "$base_mode" == "candidate-v32" ]]' in text,'root executable prerequisite is not base-mode-aware')
need('rollback_root/project/Nougat_Media_Suite_v32' in text,'replacement rollback does not preserve first-v32 executable')
print('installer-rollback-v32=pass accepted-v31-or-first-v32-gate=pass manifest-gate=pass rollback=pass no-terminal-exit=pass final-root-executable=pass raw-icon-after-final-write=pass p2p-in-scope=pass ui-sheet-retained=pass')
