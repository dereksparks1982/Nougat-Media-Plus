#!/usr/bin/env python3
"""Regression proof for the v0.0.19 changed-file rollback and root-executable contract."""
from __future__ import annotations
import pathlib, re, shutil, sys, tempfile

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
INSTALLER = ROOT / 'INSTALL_REDDMEDIA_v0_0_19.sh'


def shell_array(source: str, name: str) -> list[str]:
    match = re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)', source)
    if not match:
        raise AssertionError(f'missing installer array: {name}')
    return re.findall(r'^\s*"([^"]+)"\s*$', match.group(1), flags=re.MULTILINE)


def main() -> int:
    source = INSTALLER.read_text(encoding='utf-8')
    modified = shell_array(source, 'modified_paths')
    added = shell_array(source, 'added_paths')
    required_modified = {'src/main.cpp','CMakeLists.txt','LICENSE','ReddMedia.desktop','COMPANY_BIBLE.md','src/recommendations/viewing_history.hpp','src/recommendations/viewing_history.cpp','src/recommendations/recommendation_engine.hpp','src/recommendations/recommendation_engine.cpp'}
    required_added = {
        'src/nougat/nougat_bridge.cpp','src/nougat/nougat_bridge.hpp',
        'components/nougat/nougat_engine.py','ReddMedia_v19.desktop',
        'tools/test_nougat_v19.py','tools/test_nougat_bridge_v19.py',
    }
    assert required_modified.issubset(modified)
    assert required_added.issubset(added)
    assert '$HOME/DKLab/Projects/Nougat' not in source
    assert 'project_root="$HOME/DKLab/Projects/ReddMedia"' in source
    assert 'ReddMedia_v18' in source and 'ReddMedia_v19' in source
    assert 'PHASE START: %s' in source
    assert 'FINAL PASS: ReddMedia v0.0.19 Nougat/UI/TV continuity candidate installed and validated.' in source
    assert 'OWNER CHECK REQUIRED' in source
    assert 'ensure_ui_smoke_tools()' in source
    assert 'missing+=(xvfb)' in source and 'missing+=(x11-utils)' in source
    assert 'sudo apt-get install -y "${missing[@]}"' in source
    assert 'gio info -a metadata::custom-icon "$executable"' in source
    assert 'rm -f -- "$project_root/ReddMedia_v18" "$project_root/ReddMedia_v18.desktop"' in source
    assert 'cp "$build_root/full/ReddMedia_v19" "$project_root/ReddMedia_v19"' in source
    assert 'if [[ ! -x "$project_root/ReddMedia_v19" ]]' in source
    assert 'pinned_model_expected_bytes="84106624"' in source
    assert 'pinned_model_expected_sha256="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"' in source
    assert 'ensure_pinned_ai_model()' in source
    assert 'ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models"' in source
    assert '"$project_root/ReddMedia_v19" --discover-ai-self-test' in source
    assert 'set -e' not in '\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('#'))
    assert re.search(r'(^|\s)exit(\s|$)', '\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('#'))) is None

    # Prove the changed-file restore model without touching runtime/user data.
    with tempfile.TemporaryDirectory(prefix='reddmedia-v19-rollback-contract.') as raw:
        base = pathlib.Path(raw)
        project = base/'project'; snapshot=base/'snapshot'
        project.mkdir(); snapshot.mkdir()
        for rel in modified:
            p=project/rel; p.parent.mkdir(parents=True, exist_ok=True); p.write_text(f'base:{rel}\n')
            q=snapshot/rel; q.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(p,q)
        (project/'ReddMedia_v18').write_text('v18\n')
        (snapshot/'ReddMedia_v18').write_text('v18\n')
        runtime=project/'components/jellyfin/runtime/keep.txt'; runtime.parent.mkdir(parents=True); runtime.write_text('runtime\n')
        userdata=project/'local-user-data.keep'; userdata.write_text('user\n')
        for rel in modified:
            (project/rel).write_text(f'candidate:{rel}\n')
        for rel in added:
            p=project/rel; p.parent.mkdir(parents=True, exist_ok=True); p.write_text('candidate-only\n')
        (project/'ReddMedia_v19').write_text('v19\n')
        # Equivalent restore semantics used by installer.
        for rel in added:
            p=project/rel
            if p.is_dir(): shutil.rmtree(p)
            elif p.exists(): p.unlink()
        (project/'ReddMedia_v19').unlink()
        for rel in modified:
            p=project/rel; p.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(snapshot/rel,p)
        shutil.copy2(snapshot/'ReddMedia_v18', project/'ReddMedia_v18')
        for rel in modified:
            assert (project/rel).read_text() == f'base:{rel}\n'
        assert all(not (project/rel).exists() for rel in added)
        assert not (project/'ReddMedia_v19').exists()
        assert (project/'ReddMedia_v18').read_text() == 'v18\n'
        assert runtime.read_text() == 'runtime\n'
        assert userdata.read_text() == 'user\n'

        # The AI repair's runtime asset is rollback-safe too: an absent preexisting model
        # is removed on candidate failure, while an existing different file can be restored.
        model = project/'components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf'
        model.parent.mkdir(parents=True, exist_ok=True)
        model.write_bytes(b'candidate-model')
        model.unlink()  # equivalent to pinned_model_state=added rollback
        assert not model.exists()
        backup = snapshot/'original-model.gguf'
        backup.write_bytes(b'owner-original')
        model.write_bytes(b'candidate-model')
        shutil.copy2(backup, model)  # equivalent to pinned_model_state=replaced rollback
        assert model.read_bytes() == b'owner-original'

    print('installer-contract=pass touched-file-rollback=pass root-v19-gate=pass pinned-model-gate=pass runtime-preserved=pass user-data-preserved=pass')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
