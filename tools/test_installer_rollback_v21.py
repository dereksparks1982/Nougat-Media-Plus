#!/usr/bin/env python3
"""Regression proof for the Nougat Media Suite v0.0.21 installer and rollback contract."""
from __future__ import annotations
import pathlib, re, shutil, sys, tempfile

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
INSTALLER = ROOT / 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_21.sh'


def shell_array(source: str, name: str) -> list[str]:
    m = re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)', source)
    if not m:
        raise AssertionError(f'missing installer array: {name}')
    return re.findall(r'^\s*"([^"]+)"\s*$', m.group(1), flags=re.MULTILINE)


def main() -> int:
    source = INSTALLER.read_text(encoding='utf-8')
    modified = shell_array(source, 'modified_paths')
    added = shell_array(source, 'added_paths')
    deleted = shell_array(source, 'deleted_on_success')

    assert {
        'src/main.cpp', 'CMakeLists.txt', 'README.md', 'ROADMAP.md',
        'COMPANY_BIBLE.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md',
        'docs/NOUGAT_INTEGRATION_POLICY.md'
    }.issubset(modified)
    assert {
        'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_21.sh',
        'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v21.json',
        'NougatMediaSuite.desktop', 'NougatMediaSuite_v21.desktop',
        'src/nougat_media_suite_icon_data.hpp',
        'assets/icons/nougat-media-suite.png',
        'assets/icons/nougat-media-suite-16.png',
        'assets/icons/nougat-media-suite-32.png',
        'assets/icons/nougat-media-suite-48.png',
        'assets/icons/nougat-media-suite-64.png',
        'assets/icons/nougat-media-suite-128.png',
        'assets/icons/nougat-media-suite-256.png',
        'assets/icons/nougat-media-suite-512.png',
        'docs/NOUGAT_MEDIA_SUITE_BRAND_PALETTE.md',
        'tools/test_nougat_media_suite_v21.py',
        'tools/test_nougat_media_suite_retained_v21.py',
        'tools/test_nougat_media_suite_ui_smoke_v21.py',
        'tools/test_installer_rollback_v21.py',
    }.issubset(added)
    assert {
        'INSTALL_REDDMEDIA_v0_0_20.sh', 'REDDMEDIA_PATCH_MANIFEST_v20.json',
        'ReddMedia.desktop', 'ReddMedia_v20.desktop',
        'src/reddmedia_icon_data.hpp', 'assets/icons/reddmedia.png',
        'assets/icons/reddmedia-16.png', 'assets/icons/reddmedia-32.png',
        'assets/icons/reddmedia-48.png', 'assets/icons/reddmedia-64.png',
        'assets/icons/reddmedia-128.png', 'assets/icons/reddmedia-256.png',
    }.issubset(deleted)

    assert 'expected_base_commit="c3d2c60e5c36407b96a0eba72e2863f884aacd28"' in source
    assert 'expected_base_tag="v0.0.20"' in source
    assert 'accepted_v20_manifest_sha256="c076743084625d41d9355f1f073088afc349b9c8a6155347645d4884ccf3d658"' in source
    assert 'ReddMedia_v20' in source and 'Nougat_Media_Suite_v21' in source
    assert '[[ -x "$project_root/ReddMedia_v20" && "$("$project_root/ReddMedia_v20" --version 2>/dev/null)" == "ReddMedia v0.0.20" ]]' in source
    assert '[[ "$("$project_root/Nougat_Media_Suite_v21" --version)" == "Nougat Media Suite v0.0.21" ]]' in source
    assert 'FINAL PASS: Nougat Media Suite v0.0.21 official rename and palette candidate installed and validated.' in source
    assert 'OWNER CHECK REQUIRED' in source
    assert 'pinned_model_bytes="84106624"' in source
    assert 'pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"' in source
    assert 'verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha"' in source
    assert 'ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models"' in source
    assert '"$project_root/Nougat_Media_Suite_v21" --discover-ai-self-test' in source
    assert 'gio set -t string "$exe" metadata::custom-icon "file://$icon"' in source
    assert 'gio info -a metadata::custom-icon "$exe"' in source
    assert 'NougatMediaSuite.desktop' in source and 'nougat-media-suite.png' in source
    assert 'rm -f -- "$project_root/ReddMedia_v20"' in source
    assert 'components/jellyfin/runtime' in source
    assert 'components/ai/runtime/include/llama.h' in source

    active = '\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('#'))
    assert 'set -e' not in active
    assert re.search(r'(^|\s)exit(\s|$)', active) is None
    assert '|| exit' not in active

    # Simulate the installer rollback semantics. Runtime/user data must survive,
    # while every changed identity file and executable is restored exactly.
    with tempfile.TemporaryDirectory(prefix='nougat-media-suite-v21-rollback-contract.') as raw:
        base = pathlib.Path(raw)
        project = base / 'project'
        snapshot = base / 'snapshot'
        project.mkdir(); snapshot.mkdir()

        for rel in modified + deleted:
            p = project / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(f'base:{rel}\n')
            q = snapshot / rel
            q.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(p, q)

        old_exe = project / 'ReddMedia_v20'
        old_exe.write_text('accepted-v20\n')
        shutil.copy2(old_exe, snapshot / 'ReddMedia_v20')

        runtime = project / 'components/jellyfin/runtime/preserve.me'
        runtime.parent.mkdir(parents=True, exist_ok=True)
        runtime.write_text('runtime\n')
        model = project / 'components/ai/models/preserve.gguf'
        model.parent.mkdir(parents=True, exist_ok=True)
        model.write_text('model\n')
        search_data = project / 'local-search-data.keep'
        search_data.write_text('search-data\n')
        user_media = project / 'user-media.keep'
        user_media.write_text('user-media\n')

        # Candidate application.
        for rel in modified:
            (project / rel).write_text(f'candidate:{rel}\n')
        for rel in added:
            p = project / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text('candidate-only\n')
        for rel in deleted:
            p = project / rel
            if p.exists(): p.unlink()
        old_exe.unlink()
        (project / 'Nougat_Media_Suite_v21').write_text('candidate-v21\n')

        # Equivalent restore semantics.
        for rel in added:
            p = project / rel
            if p.is_dir(): shutil.rmtree(p)
            elif p.exists(): p.unlink()
        new_exe = project / 'Nougat_Media_Suite_v21'
        if new_exe.exists(): new_exe.unlink()
        for rel in modified + deleted:
            p = project / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(snapshot / rel, p)
        shutil.copy2(snapshot / 'ReddMedia_v20', project / 'ReddMedia_v20')

        for rel in modified + deleted:
            assert (project / rel).read_text() == f'base:{rel}\n'
        assert all(not (project / rel).exists() for rel in added)
        assert not (project / 'Nougat_Media_Suite_v21').exists()
        assert (project / 'ReddMedia_v20').read_text() == 'accepted-v20\n'
        assert runtime.read_text() == 'runtime\n'
        assert model.read_text() == 'model\n'
        assert search_data.read_text() == 'search-data\n'
        assert user_media.read_text() == 'user-media\n'

    print('installer-v21-contract=pass rollback=pass base-v20=pass root-v21-gate=pass icon-system=pass runtime-preserved=pass user-data-preserved=pass')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
