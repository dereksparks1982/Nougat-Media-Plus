#!/usr/bin/env python3
from __future__ import annotations
import pathlib,re,sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
installer=root/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_31.sh'
manifest=root/'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v31.json'

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
need(installer.is_file(),'v31 installer missing')
text=installer.read_text(encoding='utf-8')
need('expected_head="86cba5361ab67dac834e34709c1cc3d532e8da93"' in text,'accepted v30 commit gate missing')
need('Nougat_Media_Suite_v30' in text and 'Nougat Media Suite v0.0.30' in text,'accepted v30 executable gate/rollback missing')
need('Nougat_Media_Suite_v31' in text and 'Nougat Media Suite v0.0.31' in text,'v31 final executable/version gate missing')
need('verify_manifest' in text and 'required_absent' in text and 'base_files' in text,'manifest exact-base gate missing')
need('save exact accepted v0.0.30 rollback snapshot' in text,'rollback snapshot phase missing')
need('ROLLBACK PASS: accepted v0.0.30 touched state restored.' in text,'rollback proof missing')
need('apply_raw_icon "$project_root/Nougat_Media_Suite_v31"' in text,'final raw icon application missing')
need(text.index('cp "$build_root/full/Nougat_Media_Suite_v31"') < text.index('apply_raw_icon "$project_root/Nougat_Media_Suite_v31"'),'raw icon must follow final executable write')
need('gio info -a metadata::custom-icon' in text,'raw icon readback missing')
for token in [
    'tools/test_nougat_media_suite_retained_v30.py','tools/test_nougat_media_suite_v31.py',
    'tools/test_nougat_media_suite_ui_smoke_v31.py','tools/test_nougat_diagnostics_v26.py',
    'tools/test_license_protection_v22.py','tools/test_nougat_v19.py','tools/test_nougat_bridge_v19.py',
    'tools/test_media_server_lifecycle_v17.py','REDDMEDIA_P2P_STUB=ON','REDDMEDIA_AI_STUB=ON',
    'pkg-config --exists libtorrent-rasterbar','--discover-ai-self-test','--v25-ui-state-self-test',
    '--v28-ui-state-self-test','--v29-tv-reliability-self-test','--v30-ui-library-player-self-test',
    '--v31-ui-sheet-self-test','ffmpeg','xvfb-run',
]: need(token in text,'required installer gate missing: '+token)
for rel in [
    'APPLY_COMMAND.txt','CHANGELOG.md','CMakeLists.txt','README.md','ROADMAP.md',
    'NougatMediaSuite.desktop','NougatMediaSuite_v31.desktop','com.elderredsoftworks.NougatMediaSuite.desktop',
    'src/main.cpp','src/nougat_ui_sheet_texture_data.hpp','docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png','tools/test_nougat_media_suite_retained_v30.py','tools/test_nougat_media_suite_v31.py',
    'tools/test_nougat_media_suite_ui_smoke_v31.py','tools/test_installer_rollback_v31.py',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_31_EXACT_UI_SHEET_COMPONENTS_HANDSHAKE.md',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_31_EXACT_UI_SHEET_COMPONENTS_VALIDATION.md',
    'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_31.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v31.json',
]: need(f'"{rel}"' in text,'modified path absent from installer: '+rel)
modified_block=text.split('modified_paths=(',1)[1].split(')\n',1)[0]
for rel in [
    'LICENSE','COPYRIGHT.md','CONTRIBUTING.md','THIRD_PARTY_NOTICES.md','docs/LICENSING_POLICY.md',
    'components/nougat/nougat_engine.py','src/nougat/nougat_bridge.cpp','src/nougat/nougat_bridge.hpp',
    'src/p2p_engine.cpp','src/p2p_engine.hpp','src/p2p_stream_server.cpp','src/p2p_stream_server.hpp',
]: need(f'"{rel}"' not in modified_block,'protected file incorrectly included in changed paths: '+rel)
for pattern,label in [
    (r'(^|[;\s])exit(?:\s+[0-9]+)?(?:[;\s]|$)','shell exit'),
    (r'\|\|\s*exit\b','|| exit'),(r'\bset\s+-e\b','set -e'),
]: need(re.search(pattern,text,re.M) is None,label+' forbidden by terminal-safety rule')
need('rm -f -- "$project_root/Nougat_Media_Suite_v30"' in text,'accepted v30 root replacement cleanup missing')
for rel in ['INSTALL_NOUGAT_MEDIA_SUITE_v0_0_30.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v30.json','NougatMediaSuite_v30.desktop']:
    need(f'"{rel}"' in text,'superseded v30 cleanup/rollback path missing: '+rel)
need('accepted v0.0.30 page palettes and behavior are preserved' in text,'UI-only/palette preservation marker missing')
need('v0.0.32' not in modified_block,'future P2P work leaked into modified paths')
# During package assembly this test is permitted to run once before manifest creation.
if manifest.exists():
    need(manifest.is_file(),'v31 manifest is not a regular file')
print('installer-rollback-v31=pass accepted-v30-gate=pass manifest-gate=pass rollback=pass no-terminal-exit=pass final-root-executable=pass raw-icon-after-final-write=pass protected-boundaries=pass ui-only=pass')
