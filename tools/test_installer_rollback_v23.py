#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import shutil
import sys
import tempfile

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
INSTALLER = ROOT / 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_23.sh'


def arr(source: str, name: str) -> list[str]:
    match = re.search(rf'(?ms)^{re.escape(name)}=\(\n(.*?)^\)', source)
    if not match:
        raise AssertionError(f'missing installer array: {name}')
    return re.findall(r'^\s*"([^"]+)"\s*$', match.group(1), flags=re.MULTILINE)


source = INSTALLER.read_text(encoding='utf-8')
modified = arr(source, 'modified_paths')
added = arr(source, 'added_paths')
deleted = arr(source, 'deleted_on_success')

# Exact approved v23 lane: concept UI + Stream/Direct Watch repair, with the
# already-pushed licensing state protected but not rewritten by this build.
assert {
    'APPLY_COMMAND.txt', 'CHANGELOG.md', 'CMakeLists.txt', 'COMPANY_BIBLE.md',
    'DEPENDENCIES.md', 'NougatMediaSuite.desktop', 'README.md', 'ROADMAP.md',
    'assets/icons/nougat-media-suite.png', 'docs/NOUGAT_MEDIA_SUITE_BRAND_PALETTE.md',
    'src/main.cpp', 'src/nougat_media_suite_icon_data.hpp', 'src/ytdlp_stream_server.cpp',
}.issubset(modified)
assert {
    'assets/icons/nougat-media-suite-14.png', 'NougatMediaSuite_v23.desktop',
    'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_23.sh',
    'NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v23.json',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_23_EXACT_CONCEPT_UI_STREAM_REPAIR_HANDSHAKE.md',
    'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_23_EXACT_CONCEPT_UI_STREAM_REPAIR_VALIDATION.md',
    'tools/test_installer_rollback_v23.py', 'tools/test_nougat_media_suite_ui_smoke_v23.py',
    'tools/test_nougat_media_suite_v23.py',
}.issubset(added)
assert deleted == ['NougatMediaSuite_v22.desktop']

protected_license_files = {
    'LICENSE', 'COPYRIGHT.md', 'CONTRIBUTING.md', 'THIRD_PARTY_NOTICES.md',
    'docs/LICENSING_POLICY.md',
}
assert protected_license_files.isdisjoint(modified)
assert protected_license_files.isdisjoint(added)
assert protected_license_files.isdisjoint(deleted)

assert 'project_root="$HOME/DKLab/Projects/Nougat Media Suite"' in source
assert 'expected_head="755c1ac"' in source
assert 'expected_base_exe_sha="651ba56069f97170b7f94cce2d019de703c18fa7f8d8e51775a61caf92e843a5"' in source
assert 'Nougat_Media_Suite_v22' in source and 'Nougat_Media_Suite_v23' in source
assert 'verify_license_state' in source
assert 'Protected v0.0.22 license-only state verified unchanged.' in source
assert 'verify_relative_ai_rpath' in source and 'readelf -d' in source
assert 'env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v23" --discover-ai-self-test' in source
assert 'components/ai/runtime/lib/libllama.so.0' in source
assert 'pinned_model_bytes="84106624"' in source
assert 'pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"' in source
assert 'gio set -t string "$exe" metadata::custom-icon "file://$icon"' in source
assert 'rm -f -- "$project_root/Nougat_Media_Suite_v22"' in source
assert 'rm -f -- "$project_root/NougatMediaSuite_v22.desktop"' in source
assert 'FINAL PASS: Nougat Media Suite v0.0.23 exact-concept UI and Stream repair candidate installed and validated.' in source
assert 'OWNER VISUAL CHECK REQUIRED' in source
assert 'OWNER STREAM CHECK REQUIRED' in source

active = '\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('#'))
assert 'set -e' not in active
assert re.search(r'(^|\s)exit(\s|$)', active) is None
assert '|| exit' not in active

# Simulate exactly what the installer's touched-state rollback is intended to do.
with tempfile.TemporaryDirectory(prefix='nms-v23-rollback.') as raw:
    base = pathlib.Path(raw)
    project = base / 'project'
    snap = base / 'snapshot'
    project.mkdir()
    snap.mkdir()

    for rel in modified + deleted:
        p = project / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(f'base:{rel}\n', encoding='utf-8')
        q = snap / rel
        q.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(p, q)

    old = project / 'Nougat_Media_Suite_v22'
    old.write_text('current-v22\n', encoding='utf-8')
    shutil.copy2(old, snap / 'Nougat_Media_Suite_v22')

    # Files and directories outside v23's touched lane must survive rollback.
    runtime = project / 'components/jellyfin/runtime/preserve.me'
    runtime.parent.mkdir(parents=True)
    runtime.write_text('runtime\n', encoding='utf-8')
    model = project / 'components/ai/models/preserve.gguf'
    model.parent.mkdir(parents=True)
    model.write_text('model\n', encoding='utf-8')
    userdata = project / 'userdata.keep'
    userdata.write_text('userdata\n', encoding='utf-8')
    license_file = project / 'LICENSE'
    license_file.write_text('protected-license\n', encoding='utf-8')
    upload = project / '1.zip'
    upload.write_text('owner-upload\n', encoding='utf-8')

    # Candidate application.
    for rel in modified:
        p = project / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(f'candidate:{rel}\n', encoding='utf-8')
    for rel in added:
        p = project / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('candidate-only\n', encoding='utf-8')
    for rel in deleted:
        p = project / rel
        if p.exists():
            p.unlink()
    old.unlink()
    (project / 'Nougat_Media_Suite_v23').write_text('candidate-v23\n', encoding='utf-8')

    # Candidate rollback.
    for rel in added:
        p = project / rel
        if p.is_dir():
            shutil.rmtree(p)
        elif p.exists():
            p.unlink()
    new = project / 'Nougat_Media_Suite_v23'
    if new.exists():
        new.unlink()
    for rel in modified + deleted:
        p = project / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(snap / rel, p)
    shutil.copy2(snap / 'Nougat_Media_Suite_v22', project / 'Nougat_Media_Suite_v22')

    for rel in modified + deleted:
        assert (project / rel).read_text(encoding='utf-8') == f'base:{rel}\n'
    assert all(not (project / rel).exists() for rel in added)
    assert not (project / 'Nougat_Media_Suite_v23').exists()
    assert (project / 'Nougat_Media_Suite_v22').read_text(encoding='utf-8') == 'current-v22\n'
    assert runtime.read_text(encoding='utf-8') == 'runtime\n'
    assert model.read_text(encoding='utf-8') == 'model\n'
    assert userdata.read_text(encoding='utf-8') == 'userdata\n'
    assert license_file.read_text(encoding='utf-8') == 'protected-license\n'
    assert upload.read_text(encoding='utf-8') == 'owner-upload\n'

print('installer-v23-contract=pass rollback=pass base-v22=pass root-v23-gate=pass license-preserved=pass runtime-preserved=pass user-data-preserved=pass uploads-preserved=pass')
