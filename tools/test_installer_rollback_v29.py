#!/usr/bin/env python3
from __future__ import annotations
import pathlib,re,sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
installer=root/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_29.sh'
manifest=root/'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v29.json'

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
need(installer.is_file(),'v29 installer missing')
text=installer.read_text(encoding='utf-8')
need('expected_head="80b12f2b47959f4482e65238b9c0b6033f9785a2"' in text,'accepted v28 commit gate missing')
need('Nougat_Media_Suite_v28' in text and 'Nougat Media Suite v0.0.28' in text,'accepted v28 executable gate/rollback missing')
need('Nougat_Media_Suite_v29' in text and 'Nougat Media Suite v0.0.29' in text,'v29 final executable/version gate missing')
need('verify_manifest' in text and 'required_absent' in text and 'base_files' in text,'manifest exact-base gate missing')
need('save exact accepted v0.0.28 rollback snapshot' in text,'rollback snapshot phase missing')
need('ROLLBACK PASS: accepted v0.0.28 touched state restored.' in text,'rollback proof missing')
need('apply_raw_icon "$project_root/Nougat_Media_Suite_v29"' in text,'final raw icon application missing')
need(text.index('cp "$build_root/full/Nougat_Media_Suite_v29"') < text.index('apply_raw_icon "$project_root/Nougat_Media_Suite_v29"'),
     'raw icon is not applied after final executable write')
need('gio info -a metadata::custom-icon' in text,'raw icon readback missing')
for token in [
    'tools/test_nougat_media_suite_retained_v28.py','tools/test_nougat_media_suite_v29.py',
    'tools/test_nougat_media_suite_ui_smoke_v29.py','tools/test_nougat_diagnostics_v26.py',
    'tools/test_license_protection_v22.py','tools/test_nougat_v19.py','tools/test_nougat_bridge_v19.py',
    'tools/test_media_server_lifecycle_v17.py','REDDMEDIA_P2P_STUB=ON','REDDMEDIA_AI_STUB=ON',
    'pkg-config --exists libtorrent-rasterbar','--discover-ai-self-test','--v25-ui-state-self-test',
    '--v28-ui-state-self-test','--v29-tv-reliability-self-test','ffmpeg','xvfb-run',
]: need(token in text,'required installer gate missing: '+token)
for rel in [
    'APPLY_COMMAND.txt','CHANGELOG.md','CMakeLists.txt','README.md','ROADMAP.md',
    'NougatMediaSuite.desktop','NougatMediaSuite_v22.desktop','NougatMediaSuite_v23.desktop',
    'NougatMediaSuite_v24.desktop','NougatMediaSuite_v25.desktop','NougatMediaSuite_v26.desktop',
    'NougatMediaSuite_v27.desktop','NougatMediaSuite_v28.desktop','NougatMediaSuite_v29.desktop',
    'com.elderredsoftworks.NougatMediaSuite.desktop','src/main.cpp',
    'tools/test_nougat_media_suite_retained_v28.py','tools/test_nougat_media_suite_v29.py',
    'tools/test_nougat_media_suite_ui_smoke_v29.py','tools/test_installer_rollback_v29.py',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_29_TV_RELIABILITY_AND_UI_REPAIR_HANDSHAKE.md',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_29_TV_RELIABILITY_AND_UI_REPAIR_VALIDATION.md',
    'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_29.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v29.json',
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
source_block=text.split('run_source_tests() {',1)[1].split('\n}',1)[0]
need('tools/test_nougat_media_suite_retained_v28.py' in source_block,'retained-v28 gate missing')
need('tools/test_nougat_media_suite_v29.py' in source_block,'v29 contract gate missing')
need('tools/test_nougat_media_suite_v28.py' not in source_block,'obsolete v28 identity contract must not run after applying v29')
need('--v29-tv-reliability-self-test' in text,'runtime TV reliability self-test missing')
need('rm -f -- "$project_root/Nougat_Media_Suite_v28"' in text,'accepted v28 root replacement cleanup missing')
need('v0.0.30' in text and 'P2P expansion' in text,'P2P v30 defer marker missing')
need(manifest.exists(),'v29 manifest missing')
print('installer-rollback-v29=pass accepted-v28-gate=pass manifest=pass rollback=pass no-terminal-exit=pass final-root-executable=pass raw-icon-after-final-write=pass protected-boundaries=pass tv-reliability-runtime=pass')
