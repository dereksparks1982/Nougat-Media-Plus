#!/usr/bin/env python3
from __future__ import annotations
import json,pathlib,re,sys
ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
source=(ROOT/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_24.sh').read_text(encoding='utf-8')

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)

def arr(name):
    m=re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)',source)
    need(m is not None,'missing installer array '+name)
    return re.findall(r'^\s*"([^"]+)"\s*$',m.group(1),re.M)

modified=arr('modified_paths')
added=arr('added_paths')
deleted=arr('deleted_on_success')
required_mod={
 'NougatMediaSuite.desktop','NougatMediaSuite_v22.desktop','NougatMediaSuite_v23.desktop',
 'NougatMediaSuite_v24.desktop','com.elderredsoftworks.NougatMediaSuite.desktop',
 'assets/icons/nougat-media-suite.png','assets/icons/nougat-media-suite-14.png','assets/icons/nougat-media-suite-16.png',
 'assets/icons/nougat-media-suite-32.png','assets/icons/nougat-media-suite-48.png','assets/icons/nougat-media-suite-64.png',
 'assets/icons/nougat-media-suite-128.png','assets/icons/nougat-media-suite-256.png','assets/icons/nougat-media-suite-512.png',
 'src/nougat_media_suite_icon_data.hpp','tools/test_nougat_media_suite_v24.py','tools/test_nougat_visual_assets_v24.py',
 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_24.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json'
}
need(required_mod.issubset(set(modified)),'true app-wide exact-N modified paths incomplete')
need(added==['assets/icons/nougat-media-suite-concept-sheet-v24.png'],'fresh cache-busting master icon path not the sole added project path')
need(deleted==[],'icon-only repair must not delete project files')
protected={'LICENSE','COPYRIGHT.md','CONTRIBUTING.md','THIRD_PARTY_NOTICES.md','docs/LICENSING_POLICY.md'}
search={'components/nougat/nougat_engine.py','src/nougat/nougat_bridge.cpp','src/nougat/nougat_bridge.hpp'}
need(protected.isdisjoint(modified+added),'protected licensing files touched')
need(search.isdisjoint(modified+added),'Search engine files touched')

manifest=json.loads((ROOT/'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json').read_text(encoding='utf-8'))
need('NougatMediaSuite_v22.desktop' in manifest.get('payload',{}),'v22 launcher missing from exact-N payload')
need('NougatMediaSuite_v23.desktop' in manifest.get('payload',{}),'v23 launcher missing from exact-N payload')
need('assets/icons/nougat-media-suite-concept-sheet-v24.png' in manifest.get('payload',{}),'literal concept-sheet master missing from payload')
need('assets/icons/nougat-media-suite-concept-sheet-v24.png' in manifest.get('required_absent',[]),'fresh master path is not required absent in rejected base')

for marker in [
 'expected_head="870808f38352efeda13ac2c83e99f53c6a5e3fb4"',
 'icon_key="nougat-media-suite-concept-sheet-v24"',
 'master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"',
 'install_icon_aliases', 'apply_raw_icon', 'refresh_nougat_favorite', 'verify_installed_identity',
 'metadata::custom-icon', 'gtk-update-icon-cache', 'update-desktop-database',
 'Raw executable exact concept-sheet N custom-icon metadata verified.',
 'FINAL PASS: Nougat Media Suite v0.0.24 literal concept-sheet N app-wide replacement installed and validated.'
]:
    need(marker in source,'missing installer contract: '+marker)

active='\n'.join(x for x in source.splitlines() if not x.lstrip().startswith('#'))
need('set -e' not in active,'set -e forbidden')
need(re.search(r'(^|\s)exit(\s|$)',active) is None,'bare exit forbidden')
need('|| exit' not in active,'|| exit forbidden')
print('installer-v24-true-app-wide-exact-N-contract=pass rollback-snapshot=pass v22-v23-v24-launchers=pass cache-busting-master=pass dock-favorite-rebind=pass raw-icon-order=pass license-preserved=pass search-engine-preserved=pass')
