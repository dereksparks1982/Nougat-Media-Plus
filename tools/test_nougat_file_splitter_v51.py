#!/usr/bin/env python3
from pathlib import Path
import hashlib
import importlib.util
import os
import stat
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/nougat_file_splitter.py"
spec = importlib.util.spec_from_file_location("nougat_splitter_v51", TOOL)
mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(mod)

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

with tempfile.TemporaryDirectory() as td:
    t = Path(td)
    folder = t / "Movie Folder"
    (folder / "Sub").mkdir(parents=True)
    (folder / "movie.txt").write_text("movie-data" * 5000, encoding="utf-8")
    (folder / "Sub" / "note.txt").write_text("note", encoding="utf-8")
    os.symlink("../movie.txt", folder / "Sub" / "movie-link")
    os.chmod(folder / "movie.txt", 0o640)

    out = t / "folder-parts"
    manifest = mod.split(folder, out, "Night of the Living Dead", 3, 0)
    parts = sorted(out.glob("Night of the Living Dead.zip.[0-9][0-9][0-9]"))
    assert [p.name for p in parts] == [
        "Night of the Living Dead.zip.001",
        "Night of the Living Dead.zip.002",
        "Night of the Living Dead.zip.003",
    ]
    mod.verify(manifest)
    restored = mod.reassemble(manifest, t / "restore")
    assert restored.name == "Movie Folder"
    assert (restored / "movie.txt").read_bytes() == (folder / "movie.txt").read_bytes()
    assert stat.S_IMODE((restored / "movie.txt").stat().st_mode) == 0o640
    assert (restored / "Sub" / "movie-link").is_symlink()
    assert os.readlink(restored / "Sub" / "movie-link") == "../movie.txt"

    original_zip = t / "already.zip"
    with zipfile.ZipFile(original_zip, "w") as zf:
        zf.writestr("one.txt", "one")
        zf.writestr("two.txt", "two")
    original_sha = sha(original_zip)
    direct = t / "direct-parts"
    direct_manifest = mod.split(original_zip, direct, "Movie Backup", 2, 0)
    rebuilt = mod.reassemble(direct_manifest, t / "Movie Backup.reassembled.zip")
    assert sha(rebuilt) == original_sha

    ordinary = t / "single.bin"
    ordinary.write_bytes(os.urandom(2 * 1024 * 1024 + 1))
    try:
        mod.split(ordinary, t / "limited", "Limited", 1, 1 * 1024 * 1024)
    except mod.PieceCountTooSmall as exc:
        assert exc.minimum >= 2
    else:
        raise AssertionError("expected mathematically derived minimum-piece recommendation")

print("PASS: v0.0.51 File Splitter folder/file/existing-ZIP contracts")
