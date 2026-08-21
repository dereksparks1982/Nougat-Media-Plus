#!/usr/bin/env python3
from __future__ import annotations
import pathlib,re,shutil,sys,tempfile

ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
INSTALLER=ROOT/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_22.sh'

def arr(source,name):
    m=re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)',source)
    if not m: raise AssertionError(f'missing installer array: {name}')
    return re.findall(r'^\s*"([^"]+)"\s*$',m.group(1),flags=re.MULTILINE)

source=INSTALLER.read_text(encoding='utf-8')
modified=arr(source,'modified_paths'); added=arr(source,'added_paths'); deleted=arr(source,'deleted_on_success')

assert {'LICENSE','README.md','THIRD_PARTY_NOTICES.md','COMPANY_BIBLE.md','CMakeLists.txt','src/main.cpp','NougatMediaSuite.desktop'}.issubset(modified)
assert {'COPYRIGHT.md','CONTRIBUTING.md','docs/LICENSING_POLICY.md','.github/PULL_REQUEST_TEMPLATE.md','NougatMediaSuite_v22.desktop','tools/test_license_protection_v22.py','tools/test_nougat_media_suite_v22.py','tools/test_nougat_media_suite_retained_v22.py','tools/test_nougat_media_suite_ui_smoke_v22.py'}.issubset(added)
assert {'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_21.sh','NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v21.json','NougatMediaSuite_v21.desktop','tools/test_nougat_media_suite_v21.py'}.issubset(deleted)
assert 'project_root="$HOME/DKLab/Projects/Nougat Media Suite"' in source
assert 'expected_base_commit="b89b0fc187bebc05f3390a76c76cbdc980713600"' in source
assert 'expected_base_tag="v0.0.21"' in source
assert 'Nougat_Media_Suite_v21' in source and 'Nougat_Media_Suite_v22' in source
assert 'run_accepted_v21' in source and 'LD_LIBRARY_PATH="$ai_runtime_search_path' in source
assert 'verify_relative_ai_rpath' in source and 'readelf -d' in source
assert 'env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v22" --discover-ai-self-test' in source
assert 'components/ai/runtime/lib/libllama.so.0' in source
assert 'Nougat Media Suite v0.0.22' in source
assert 'FINAL PASS: Nougat Media Suite v0.0.22 license-protection/runtime-path-repair candidate installed and validated.' in source
assert 'OWNER CHECK REQUIRED' in source
assert 'pinned_model_bytes="84106624"' in source
assert 'pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"' in source
assert '"$project_root/Nougat_Media_Suite_v22" --discover-ai-self-test' in source
assert 'gio set -t string "$exe" metadata::custom-icon "file://$icon"' in source
assert 'rm -f -- "$project_root/Nougat_Media_Suite_v21"' in source
active='\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('#'))
assert 'set -e' not in active
assert re.search(r'(^|\s)exit(\s|$)',active) is None
assert '|| exit' not in active

with tempfile.TemporaryDirectory(prefix='nms-v22-rollback.') as raw:
    base=pathlib.Path(raw); project=base/'project'; snap=base/'snapshot'; project.mkdir(); snap.mkdir()
    for rel in modified+deleted:
        p=project/rel; p.parent.mkdir(parents=True,exist_ok=True); p.write_text(f'base:{rel}\n')
        q=snap/rel; q.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(p,q)
    old=project/'Nougat_Media_Suite_v21'; old.write_text('accepted-v21\n'); shutil.copy2(old,snap/'Nougat_Media_Suite_v21')
    runtime=project/'components/jellyfin/runtime/preserve.me'; runtime.parent.mkdir(parents=True); runtime.write_text('runtime\n')
    model=project/'components/ai/models/preserve.gguf'; model.parent.mkdir(parents=True); model.write_text('model\n')
    userdata=project/'userdata.keep'; userdata.write_text('userdata\n')
    for rel in modified: (project/rel).write_text(f'candidate:{rel}\n')
    for rel in added:
        p=project/rel; p.parent.mkdir(parents=True,exist_ok=True); p.write_text('candidate-only\n')
    for rel in deleted:
        p=project/rel
        if p.exists(): p.unlink()
    old.unlink(); (project/'Nougat_Media_Suite_v22').write_text('candidate-v22\n')
    for rel in added:
        p=project/rel
        if p.is_dir(): shutil.rmtree(p)
        elif p.exists(): p.unlink()
    new=project/'Nougat_Media_Suite_v22'
    if new.exists(): new.unlink()
    for rel in modified+deleted:
        p=project/rel; p.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(snap/rel,p)
    shutil.copy2(snap/'Nougat_Media_Suite_v21',project/'Nougat_Media_Suite_v21')
    for rel in modified+deleted: assert (project/rel).read_text()==f'base:{rel}\n'
    assert all(not (project/rel).exists() for rel in added)
    assert not (project/'Nougat_Media_Suite_v22').exists()
    assert (project/'Nougat_Media_Suite_v21').read_text()=='accepted-v21\n'
    assert runtime.read_text()=='runtime\n' and model.read_text()=='model\n' and userdata.read_text()=='userdata\n'

print('installer-v22-contract=pass rollback=pass base-v21=pass root-v22-gate=pass license-files=pass runtime-preserved=pass user-data-preserved=pass')
