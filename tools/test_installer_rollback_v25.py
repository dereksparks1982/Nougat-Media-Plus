#!/usr/bin/env python3
from __future__ import annotations
import json,pathlib,re,sys
ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
source=(ROOT/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_25.sh').read_text(encoding='utf-8')

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)

def arr(name):
    m=re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)',source)
    need(m is not None,'missing installer array '+name)
    return re.findall(r'^\s*"([^"]+)"\s*$',m.group(1),re.M)

modified=arr('modified_paths')
required={
 'APPLY_COMMAND.txt','CHANGELOG.md','INSTALL_NOUGAT_MEDIA_SUITE_v0_0_25.sh',
 'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v25.json','src/main.cpp',
 'tools/test_installer_rollback_v25.py','tools/test_nougat_media_suite_v25.py',
 'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_25_STREAM_SELECTION_DISCOVER_PLAY_HANDSHAKE.md',
 'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_25_STREAM_SELECTION_DISCOVER_PLAY_VALIDATION.md',
}
need(required.issubset(set(modified)),'same-version repair path coverage incomplete')
protected={'LICENSE','COPYRIGHT.md','CONTRIBUTING.md','THIRD_PARTY_NOTICES.md','docs/LICENSING_POLICY.md'}
search={'components/nougat/nougat_engine.py','src/nougat/nougat_bridge.cpp','src/nougat/nougat_bridge.hpp'}
visual={
 'assets/icons/nougat-media-suite.png','assets/icons/nougat-media-suite-concept-sheet-v24.png',
 'assets/ui/nougat-quilt-source.png','src/nougat_media_suite_icon_data.hpp','src/nougat_quilt_texture_data.hpp'
}
need(protected.isdisjoint(modified),'protected licensing files touched')
need(search.isdisjoint(modified),'Search engine files touched')
need(visual.isdisjoint(modified),'accepted icon/quilt assets touched')

manifest=json.loads((ROOT/'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v25.json').read_text(encoding='utf-8'))
need(manifest.get('accepted_git_base',{}).get('commit')=='f66d35b671c9bceee6151dc63003dc3ec24578e8','manifest accepted Git anchor wrong')
need(manifest.get('version')=='0.0.25','manifest version wrong')
need(manifest.get('base_state')=='installed rejected v0.0.25 header-removal candidate','repair base state wrong')
need('src/main.cpp' in manifest.get('base_files',{}),'main.cpp exact rejected-candidate base verification missing')
need(not manifest.get('required_absent'),'same-version repair should not require v25 files absent')

for marker in [
 'expected_head="f66d35b671c9bceee6151dc63003dc3ec24578e8"',
 'installed rejected v0.0.25 header-removal candidate',
 'Nougat Media Suite v0.0.25',
 'verify_git_state', 'verify_manifest', 'save_rollback_snapshot', 'restore_rollback',
 'Nougat_Media_Suite_v25',
 'python3 "$project_root/tools/test_nougat_media_suite_v25.py"',
 'python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v25.py"',
 'metadata::custom-icon',
 'FINAL PASS: Nougat Media Suite v0.0.25 Stream provider panel removal repair installed and validated.'
]:
    need(marker in source,'missing repair installer contract: '+marker)

need('[[ -x "$project_root/Nougat_Media_Suite_v24" ]]' not in source,'same-version repair incorrectly requires v24 executable')
need('--version 2>/dev/null)" == "Nougat Media Suite v0.0.24"' not in source,'same-version repair incorrectly validates v24 runtime')
active='\n'.join(x for x in source.splitlines() if not x.lstrip().startswith('#'))
need('set -e' not in active,'set -e forbidden')
need(re.search(r'(^|\s)exit(\s|$)',active) is None,'bare exit forbidden')
need('|| exit' not in active,'|| exit forbidden')
print('installer-v25-repair-contract=pass rejected-v25-panel-base=pass rollback-to-pre-panel-removal-v25=pass root-v25-preserved=pass license-preserved=pass search-engine-preserved=pass accepted-visual-assets-preserved=pass')
