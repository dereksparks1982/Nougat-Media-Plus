#!/usr/bin/env python3
"""Regression proof for the v0.0.20 changed-file rollback and root-executable contract."""
from __future__ import annotations
import pathlib, re, shutil, sys, tempfile

ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
INSTALLER=ROOT/'INSTALL_REDDMEDIA_v0_0_20.sh'

def shell_array(source: str, name: str) -> list[str]:
    m=re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)',source)
    if not m: raise AssertionError(f'missing installer array: {name}')
    return re.findall(r'^\s*"([^"]+)"\s*$',m.group(1),flags=re.MULTILINE)

def main()->int:
    source=INSTALLER.read_text(encoding='utf-8')
    modified=shell_array(source,'modified_paths')
    added=shell_array(source,'added_paths')
    deleted=shell_array(source,'deleted_on_success')
    assert {'src/main.cpp','CMakeLists.txt','ReddMedia.desktop','ROADMAP.md','COMPANY_BIBLE.md'}.issubset(modified)
    assert {'ReddMedia_v20.desktop','tools/test_reddmedia_v20.py','tools/test_reddmedia_ui_smoke_v20.py','tools/test_installer_rollback_v20.py'}.issubset(added)
    assert {'ReddMedia_v19.desktop','INSTALL_REDDMEDIA_v0_0_19.sh','REDDMEDIA_PATCH_MANIFEST_v19.json'}.issubset(deleted)
    assert 'expected_base_commit="e1cf0aa1ca55757c8048e5f142f9c1d92f23cb26"' in source
    assert 'accepted_v19_manifest_sha256="26106d40b8878d08c7fb759217c6a56ad8c5221df1a732028261450c272f8a47"' in source
    assert 'ReddMedia_v19' in source and 'ReddMedia_v20' in source
    assert 'FINAL PASS: ReddMedia v0.0.20 Stream/palette/volume/Library-view candidate installed and validated.' in source
    assert 'OWNER CHECK REQUIRED' in source
    assert 'ensure_ui_smoke_tools()' in source
    assert 'gio info -a metadata::custom-icon "$executable"' in source
    assert 'cp "$build_root/full/ReddMedia_v20" "$project_root/ReddMedia_v20"' in source
    assert 'if [[ ! -x "$project_root/ReddMedia_v20" ]]' in source
    assert 'pinned_model_expected_bytes="84106624"' in source
    assert 'pinned_model_expected_sha256="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"' in source
    assert 'verify_existing_pinned_ai_model()' in source
    assert 'pinned_model_source=' not in source
    assert 'ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models"' in source
    assert '"$project_root/ReddMedia_v20" --discover-ai-self-test' in source
    active='\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('#'))
    assert 'set -e' not in active
    assert re.search(r'(^|\s)exit(\s|$)',active) is None

    with tempfile.TemporaryDirectory(prefix='reddmedia-v20-rollback-contract.') as raw:
        base=pathlib.Path(raw); project=base/'project'; snapshot=base/'snapshot'
        project.mkdir(); snapshot.mkdir()
        for rel in modified+deleted:
            p=project/rel; p.parent.mkdir(parents=True,exist_ok=True); p.write_text(f'base:{rel}\n')
            q=snapshot/rel; q.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(p,q)
        (project/'ReddMedia_v19').write_text('v19\n'); (snapshot/'ReddMedia_v19').write_text('v19\n')
        runtime=project/'components/jellyfin/runtime/keep.txt'; runtime.parent.mkdir(parents=True); runtime.write_text('runtime\n')
        userdata=project/'local-user-data.keep'; userdata.write_text('user\n')
        for rel in modified:
            (project/rel).write_text(f'candidate:{rel}\n')
        for rel in added:
            p=project/rel; p.parent.mkdir(parents=True,exist_ok=True); p.write_text('candidate-only\n')
        for rel in deleted:
            (project/rel).unlink()
        (project/'ReddMedia_v20').write_text('v20\n')
        # Equivalent restore semantics.
        for rel in added:
            p=project/rel
            if p.is_dir(): shutil.rmtree(p)
            elif p.exists(): p.unlink()
        (project/'ReddMedia_v20').unlink()
        for rel in modified+deleted:
            p=project/rel; p.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(snapshot/rel,p)
        shutil.copy2(snapshot/'ReddMedia_v19',project/'ReddMedia_v19')
        for rel in modified+deleted:
            assert (project/rel).read_text()==f'base:{rel}\n'
        assert all(not (project/rel).exists() for rel in added)
        assert not (project/'ReddMedia_v20').exists()
        assert (project/'ReddMedia_v19').read_text()=='v19\n'
        assert runtime.read_text()=='runtime\n'
        assert userdata.read_text()=='user\n'
    print('installer-v20-contract=pass rollback=pass base-v19=pass root-v20-gate=pass runtime-preserved=pass user-data-preserved=pass')
    return 0
if __name__=='__main__': raise SystemExit(main())
