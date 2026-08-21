#!/usr/bin/env python3
"""Regression proof for the v0.0.18 snapshot rollback contract."""

from __future__ import annotations

import pathlib
import re
import shutil
import subprocess
import sys
import tempfile


def shell_array(source: str, name: str) -> list[str]:
    match = re.search(rf"(?ms)^{re.escape(name)}=\(\n(.*?)^\)", source)
    if not match:
        raise AssertionError(f"missing installer array: {name}")
    return re.findall(r'^\s*"([^"]+)"\s*$', match.group(1), flags=re.MULTILINE)


def assert_installer_contract(project_root: pathlib.Path) -> None:
    source = (project_root / "INSTALL_REDDMEDIA_v0_0_18.sh").read_text(
        encoding="utf-8"
    )
    added = shell_array(source, "added_paths")
    modified = shell_array(source, "modified_paths")

    assert "COMPANY_BIBLE.md" in modified
    assert "src/diagnostics/diagnostic_engine.cpp" in added
    assert "src/recommendations/watch_provider_preferences.cpp" in added
    assert "ReddMedia_v18.desktop" in added
    assert "REDDMEDIA_PATCH_MANIFEST.json" in modified
    assert not any(path.startswith("components/ai/runtime/") for path in added + modified)
    assert not any(
        path.startswith("components/jellyfin/runtime/") for path in added + modified
    )

    snapshot_marker = "PHASE START: Save exact pre-v0.0.18 rollback snapshot"
    apply_marker = "PHASE START: Apply v0.0.18 source and records"
    assert source.index(snapshot_marker) < source.index(apply_marker)
    assert "--exclude='components/ai/runtime/'" in source
    assert "--exclude='components/jellyfin/runtime/'" in source
    assert "rsync -aX --checksum --delete" in source
    assert "generated runtimes and user data preserved" in source
    assert 'rm -f -- "$project_root/ReddMedia_v17"' in source

    copy = 'cp "$build_root/candidate/ReddMedia_v18" "$project_root/ReddMedia_v18"'
    icon = 'apply_raw_executable_icon "$project_root/ReddMedia_v18"'
    assert source.index(copy) < source.index(icon)
    assert 'gio info -a metadata::custom-icon "$executable"' in source
    assert "nautilus -q" in source
    assert "OWNER CHECK REQUIRED" in source


def prove_snapshot_restore_semantics() -> None:
    if shutil.which("rsync") is None:
        raise AssertionError("rsync is required for the rollback regression")

    with tempfile.TemporaryDirectory(prefix="reddmedia-v18-rollback-test.") as raw:
        root = pathlib.Path(raw)
        project = root / "project"
        snapshot = root / "snapshot"
        project.mkdir()
        snapshot.mkdir()

        (project / "src").mkdir()
        (project / "src/main.cpp").write_text("v17 source\n", encoding="utf-8")
        (project / "ReddMedia_v17").write_text("v17 executable\n", encoding="utf-8")
        (project / "ReddMedia_v17.desktop").write_text("v17 desktop\n", encoding="utf-8")
        (project / ".git").mkdir()
        (project / ".git/keep").write_text("git metadata\n", encoding="utf-8")

        ai_runtime = project / "components/ai/runtime/keep.txt"
        jellyfin_runtime = project / "components/jellyfin/runtime/keep.txt"
        ai_runtime.parent.mkdir(parents=True)
        jellyfin_runtime.parent.mkdir(parents=True)
        ai_runtime.write_text("preserve ai runtime\n", encoding="utf-8")
        jellyfin_runtime.write_text("preserve jellyfin runtime\n", encoding="utf-8")

        subprocess.run(
            [
                "rsync",
                "-a",
                "--exclude=.git/",
                "--exclude=components/ai/runtime/",
                "--exclude=components/jellyfin/runtime/",
                f"{project}/",
                f"{snapshot}/",
            ],
            check=True,
        )

        (project / "src/main.cpp").write_text("v18 source\n", encoding="utf-8")
        (project / "ReddMedia_v17").unlink()
        (project / "ReddMedia_v17.desktop").unlink()
        (project / "ReddMedia_v18").write_text("v18 executable\n", encoding="utf-8")
        (project / "new-v18.txt").write_text("new\n", encoding="utf-8")

        subprocess.run(
            [
                "rsync",
                "-a",
                "--checksum",
                "--delete",
                "--exclude=.git/",
                "--exclude=components/ai/runtime/",
                "--exclude=components/jellyfin/runtime/",
                f"{snapshot}/",
                f"{project}/",
            ],
            check=True,
        )

        assert (project / "src/main.cpp").read_text(encoding="utf-8") == "v17 source\n"
        assert (project / "ReddMedia_v17").read_text(encoding="utf-8") == "v17 executable\n"
        assert (project / "ReddMedia_v17.desktop").is_file()
        assert not (project / "ReddMedia_v18").exists()
        assert not (project / "new-v18.txt").exists()
        assert ai_runtime.read_text(encoding="utf-8") == "preserve ai runtime\n"
        assert jellyfin_runtime.read_text(encoding="utf-8") == "preserve jellyfin runtime\n"
        assert (project / ".git/keep").read_text(encoding="utf-8") == "git metadata\n"


def main() -> int:
    project_root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    assert_installer_contract(project_root)
    prove_snapshot_restore_semantics()
    print(
        "rollback-contract=pass v17-snapshot-restored=pass v18-additions-removed=pass "
        "git-preserved=pass ai-runtime-preserved=pass jellyfin-runtime-preserved=pass "
        "icon-order=pass"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
