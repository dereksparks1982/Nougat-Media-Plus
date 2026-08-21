#!/usr/bin/env python3
"""Regression proof for the v0.0.17 installer's Git rollback contract."""

from __future__ import annotations

import pathlib
import re
import shutil
import subprocess
import sys
import tempfile


def run(*args: str, cwd: pathlib.Path, capture: bool = False) -> str:
    completed = subprocess.run(
        args,
        cwd=cwd,
        check=True,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )
    return completed.stdout if capture else ""


def shell_array(source: str, name: str) -> list[str]:
    match = re.search(rf"(?ms)^{re.escape(name)}=\(\n(.*?)^\)", source)
    if not match:
        raise AssertionError(f"missing installer array: {name}")
    return re.findall(r'^\s*"([^"]+)"\s*$', match.group(1), flags=re.MULTILINE)


def assert_installer_contract(project_root: pathlib.Path) -> None:
    source = (project_root / "INSTALL_REDDMEDIA_v0_0_17.sh").read_text(encoding="utf-8")
    added = shell_array(source, "added_paths")
    modified = shell_array(source, "modified_paths")

    assert ".gitignore" in added
    assert "COMPANY_BIBLE.md" in added
    assert "ReddMedia_v17.desktop" in added
    assert "REDDMEDIA_PATCH_MANIFEST.json" in modified
    assert not any(path.startswith("components/ai/runtime/") for path in added + modified)
    assert not any(path.startswith("components/jellyfin/runtime/") for path in added + modified)
    assert 'git restore --source "$required_commit" --staged --worktree -- .' in source
    assert '"${added_paths[@]}" "ReddMedia_v17"' in source
    assert "Exact v0.0.16 tracked state restored; generated runtimes and user data preserved" in source
    assert 'cp "$build_root/candidate/ReddMedia_v17" "$project_root/ReddMedia_v17"' in source
    copy_position = source.index(
        'cp "$build_root/candidate/ReddMedia_v17" "$project_root/ReddMedia_v17"'
    )
    icon_position = source.index('apply_raw_executable_icon "$project_root/ReddMedia_v17"')
    assert copy_position < icon_position
    assert 'gio info -a metadata::custom-icon "$executable"' in source


def prove_git_semantics() -> None:
    if shutil.which("git") is None:
        raise AssertionError("git is required for the rollback regression")

    with tempfile.TemporaryDirectory(prefix="reddmedia-v17-rollback-test.") as raw:
        root = pathlib.Path(raw)
        run("git", "init", "-q", cwd=root)
        run("git", "config", "user.name", "ReddMedia Test", cwd=root)
        run("git", "config", "user.email", "reddmedia-test@localhost", cwd=root)

        tracked = root / "tracked.txt"
        v16 = root / "ReddMedia_v16"
        desktop16 = root / "ReddMedia_v16.desktop"
        tracked.write_text("committed-v16\n", encoding="utf-8")
        v16.write_text("v16 executable\n", encoding="utf-8")
        desktop16.write_text("v16 desktop\n", encoding="utf-8")
        run("git", "add", "tracked.txt", "ReddMedia_v16", "ReddMedia_v16.desktop", cwd=root)
        run("git", "commit", "-q", "-m", "v16 checkpoint", cwd=root)
        baseline = run("git", "rev-parse", "HEAD", cwd=root, capture=True).strip()

        ai_runtime = root / "components/ai/runtime/keep.txt"
        jellyfin_runtime = root / "components/jellyfin/runtime/keep.txt"
        ai_runtime.parent.mkdir(parents=True)
        jellyfin_runtime.parent.mkdir(parents=True)
        ai_runtime.write_text("preserve ai runtime\n", encoding="utf-8")
        jellyfin_runtime.write_text("preserve jellyfin runtime\n", encoding="utf-8")

        tracked.write_text("candidate-v17\n", encoding="utf-8")
        v16.unlink()
        desktop16.unlink()
        gitignore = root / ".gitignore"
        gitignore.write_text("components/ai/runtime/\ncomponents/jellyfin/runtime/\n", encoding="utf-8")
        v17 = root / "ReddMedia_v17"
        desktop17 = root / "ReddMedia_v17.desktop"
        v17.write_text("v17 executable\n", encoding="utf-8")
        desktop17.write_text("v17 desktop\n", encoding="utf-8")

        run("git", "restore", "--source", baseline, "--staged", "--worktree", "--", ".", cwd=root)
        for added in (gitignore, v17, desktop17):
            added.unlink()

        assert tracked.read_text(encoding="utf-8") == "committed-v16\n"
        assert v16.read_text(encoding="utf-8") == "v16 executable\n"
        assert desktop16.read_text(encoding="utf-8") == "v16 desktop\n"
        assert ai_runtime.read_text(encoding="utf-8") == "preserve ai runtime\n"
        assert jellyfin_runtime.read_text(encoding="utf-8") == "preserve jellyfin runtime\n"
        run("git", "diff", "--quiet", cwd=root)
        run("git", "diff", "--cached", "--quiet", cwd=root)
        remaining = run(
            "git", "ls-files", "--others", "--exclude-standard", cwd=root, capture=True
        ).splitlines()
        assert remaining == [
            "components/ai/runtime/keep.txt",
            "components/jellyfin/runtime/keep.txt",
        ]


def main() -> int:
    project_root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    assert_installer_contract(project_root)
    prove_git_semantics()
    print(
        "rollback-contract=pass tracked-v16-restored=pass v17-additions-removed=pass "
        "ai-runtime-preserved=pass jellyfin-runtime-preserved=pass icon-order=pass"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
