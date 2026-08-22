#!/usr/bin/env python3
from __future__ import annotations
import json,pathlib,re,sys
ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
source=(ROOT/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_26.sh').read_text(encoding='utf-8')

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)

def arr(name):
    m=re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)',source)
    need(m is not None,'missing installer array '+name)
    return re.findall(r'^\s*"([^"]+)"\s*$',m.group(1),re.M)

modified=arr('modified_paths')
required={
 'APPLY_COMMAND.txt','CHANGELOG.md','CMakeLists.txt','COMPANY_BIBLE.md','README.md','ROADMAP.md',
 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_26.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v26.json',
 'NougatMediaSuite_v26.desktop','src/main.cpp','src/nougat_media_suite_icon_data.hpp',
 'src/diagnostics/diagnostic_engine.cpp','src/diagnostics/diagnostic_engine.hpp','src/diagnostics/diagnostic_types.hpp',
 'tools/test_nougat_diagnostics_v26.py','tools/test_nougat_media_suite_retained_v25.py',
 'tools/test_nougat_media_suite_v26.py','tools/test_nougat_media_suite_ui_smoke_v26.py','tools/test_installer_rollback_v26.py',
 'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_26_SYSTEMS_NAV_DIAGNOSTICS_UP_NEXT_HANDSHAKE.md',
 'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_26_SYSTEMS_NAV_DIAGNOSTICS_UP_NEXT_VALIDATION.md',
}
need(required.issubset(set(modified)),'v26 touched-path coverage incomplete')
protected={'LICENSE','COPYRIGHT.md','CONTRIBUTING.md','THIRD_PARTY_NOTICES.md','docs/LICENSING_POLICY.md'}
search={'components/nougat/nougat_engine.py','src/nougat/nougat_bridge.cpp','src/nougat/nougat_bridge.hpp'}
p2p={'src/p2p_engine.cpp','src/p2p_engine.hpp','src/p2p_stream_server.cpp','src/p2p_stream_server.hpp'}
need(protected.isdisjoint(modified),'protected licensing files touched')
need(search.isdisjoint(modified),'Search engine implementation touched')
need(p2p.isdisjoint(modified),'deferred P2P implementation touched')
for icon in ['assets/icons/nougat-media-suite.png','assets/icons/nougat-media-suite-14.png','assets/icons/nougat-media-suite-512.png','assets/icons/nougat-media-suite-concept-sheet-v24.png']:
    need(icon in modified,'cleaned app-wide N path missing: '+icon)

manifest=json.loads((ROOT/'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v26.json').read_text(encoding='utf-8'))
need(manifest.get('version')=='0.0.26','manifest version wrong')
need(manifest.get('accepted_git_base',{}).get('commit')=='c4d174466c2bb30c4eda8f04f09105e5d583040c','manifest accepted v25 commit wrong')
need(manifest.get('base_state')=='accepted v0.0.25 clean worktree','manifest base state wrong')
need('src/main.cpp' in manifest.get('base_files',{}),'main.cpp accepted base hash missing')
need('src/diagnostics/diagnostic_engine.cpp' in manifest.get('base_files',{}),'diagnostic accepted base hash missing')
for rel in ['NougatMediaSuite_v26.desktop','INSTALL_NOUGAT_MEDIA_SUITE_v0_0_26.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v26.json','Nougat_Media_Suite_v26']:
    if rel != 'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v26.json':
        need(rel in manifest.get('required_absent',[]) or rel in manifest.get('payload',{}),'new-v26 path not covered: '+rel)

for marker in [
 'expected_head="c4d174466c2bb30c4eda8f04f09105e5d583040c"',
 'accepted v0.0.25', 'Nougat Media Suite v0.0.26', 'verify_git_state', 'verify_manifest',
 'save_rollback_snapshot', 'restore_rollback', 'Nougat_Media_Suite_v25', 'Nougat_Media_Suite_v26',
 'python3 "$project_root/tools/test_nougat_diagnostics_v26.py"',
 'python3 "$project_root/tools/test_nougat_media_suite_retained_v25.py"',
 'python3 "$project_root/tools/test_nougat_media_suite_v26.py"',
 'python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v26.py"',
 'metadata::custom-icon',
 'P2P expansion remains roadmap v0.0.28',
 'FINAL PASS: Nougat Media Suite v0.0.26 Systems, Navigation, Diagnostics, and TV Up Next installed and validated.'
]:
    need(marker in source,'missing v26 installer contract: '+marker)
active='\n'.join(x for x in source.splitlines() if not x.lstrip().startswith('#'))
need('set -e' not in active,'set -e forbidden')
need(re.search(r'(^|\s)exit(\s|$)',active) is None,'bare exit forbidden')
need('|| exit' not in active,'|| exit forbidden')
print('installer-v26-contract=pass accepted-v25-base=pass rollback=pass final-root-v26=pass license-preserved=pass search-preserved=pass p2p-deferred=pass launcher-icon-refresh=pass')
