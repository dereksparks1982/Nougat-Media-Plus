#!/usr/bin/env python3
"""Regression proof for the v0.0.16 installer's exact Git rollback contract."""

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
    installer = project_root / "INSTALL_REDDMEDIA_v0_0_16.sh"
    source = installer.read_text(encoding="utf-8")
    added = shell_array(source, "added_paths")
    modified = shell_array(source, "modified_paths")

    assert "REDDMEDIA_PATCH_MANIFEST.json" in added
    assert "REDDMEDIA_PATCH_MANIFEST.json" not in modified
    assert 'git restore --source "$required_commit" --staged --worktree -- .' in source
    assert 'git restore --source "$required_commit" --staged --worktree -- "${modified_paths[@]}"' not in source
    assert "recover_known_failed_original_candidate" in source
    assert "4684d45952ecc2c1e87424cc7fd16333ad92295a745546a32ee6aa465e78f6c4" in source
    assert 'git diff --binary --output="$recovery_backup/tracked-files.patch"' in source
    assert 'cp "$manifest" "$recovery_backup/$manifest"' in source


def prove_git_semantics() -> None:
    if shutil.which("git") is None:
        raise AssertionError("git is required for the rollback regression")

    with tempfile.TemporaryDirectory(prefix="reddmedia-v16-rollback-test.") as raw:
        root = pathlib.Path(raw)
        run("git", "init", "-q", cwd=root)
        run("git", "config", "user.name", "ReddMedia Test", cwd=root)
        run("git", "config", "user.email", "reddmedia-test@localhost", cwd=root)

        tracked = root / "tracked.txt"
        tracked.write_text("committed-v15\n", encoding="utf-8")
        run("git", "add", "tracked.txt", cwd=root)
        run("git", "commit", "-q", "-m", "v15 baseline", cwd=root)
        baseline = run("git", "rev-parse", "HEAD", cwd=root, capture=True).strip()

        runtime_file = root / "components/jellyfin/runtime/keep.txt"
        runtime_file.parent.mkdir(parents=True)
        runtime_file.write_text("preserve accepted runtime\n", encoding="utf-8")

        tracked.write_text("staged-v16\n", encoding="utf-8")
        run("git", "add", "tracked.txt", cwd=root)
        tracked.write_text("unstaged-v16\n", encoding="utf-8")

        manifest = root / "REDDMEDIA_PATCH_MANIFEST.json"
        manifest.write_text("{}\n", encoding="utf-8")
        added_file = root / "v16-added.txt"
        added_file.write_text("v16\n", encoding="utf-8")

        run(
            "git",
            "restore",
            "--source",
            baseline,
            "--staged",
            "--worktree",
            "--",
            ".",
            cwd=root,
        )
        manifest.unlink()
        added_file.unlink()

        assert tracked.read_text(encoding="utf-8") == "committed-v15\n"
        run("git", "diff", "--quiet", cwd=root)
        run("git", "diff", "--cached", "--quiet", cwd=root)
        remaining = run(
            "git", "ls-files", "--others", "--exclude-standard", cwd=root, capture=True
        ).splitlines()
        assert remaining == ["components/jellyfin/runtime/keep.txt"]
        assert runtime_file.read_text(encoding="utf-8") == "preserve accepted runtime\n"


def main() -> int:
    project_root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    assert_installer_contract(project_root)
    prove_git_semantics()
    print("recovery-contract=pass tracked-rollback=pass added-cleanup=pass runtime-preserved=pass")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
