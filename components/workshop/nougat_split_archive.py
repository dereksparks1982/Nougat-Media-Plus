#!/usr/bin/env python3
"""Nougat Workshop Split Archive v1.

Stdlib-only reference engine for NOUGAT_SPLIT_ARCHIVE. Parts are ordinary ZIP
files using ZIP_STORED chunk entries so a maximum part size is predictable.
The external JSON manifest records paths, metadata, chunk order, and SHA-256
integrity for source files, chunks, and completed part ZIPs.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import sys
import tempfile
import time
import zipfile

FORMAT = "NOUGAT_SPLIT_ARCHIVE"
FORMAT_VERSION = 1
DEFAULT_MAX_PART_BYTES = 450 * 1024 * 1024
MIN_PART_BYTES = 2 * 1024 * 1024
ZIP_SAFETY_BYTES = 1024 * 1024
IO_BLOCK = 1024 * 1024


class SplitArchiveError(RuntimeError):
    pass


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(IO_BLOCK), b""):
            h.update(block)
    return h.hexdigest()


def sha256_range(path: Path, offset: int, size: int) -> str:
    h = hashlib.sha256()
    remaining = size
    with path.open("rb") as f:
        f.seek(offset)
        while remaining:
            block = f.read(min(IO_BLOCK, remaining))
            if not block:
                raise SplitArchiveError(f"Unexpected EOF while hashing {path}")
            h.update(block)
            remaining -= len(block)
    return h.hexdigest()


def safe_relative(value: str) -> Path:
    p = PurePosixPath(value)
    if p.is_absolute() or any(part in ("", ".", "..") for part in p.parts):
        raise SplitArchiveError(f"Unsafe archive path: {value!r}")
    return Path(*p.parts)


def stat_record(path: Path, rel: str, kind: str) -> dict:
    st = path.lstat()
    out = {
        "path": rel,
        "type": kind,
        "mode": stat.S_IMODE(st.st_mode),
        "mtime_ns": st.st_mtime_ns,
    }
    if kind == "file":
        out["size"] = st.st_size
    elif kind == "symlink":
        out["target"] = os.readlink(path)
    return out


def scan_source(source: Path) -> tuple[str, list[dict], int, int, int]:
    source = source.expanduser().resolve(strict=True)
    entries: list[dict] = []
    file_count = 0
    dir_count = 0
    total_bytes = 0

    if source.is_file():
        rec = stat_record(source, source.name, "file")
        entries.append(rec)
        file_count = 1
        total_bytes = rec["size"]
        return "file", entries, total_bytes, file_count, dir_count

    if not source.is_dir():
        raise SplitArchiveError("Source must be a regular file or directory")

    root_name = source.name
    root = stat_record(source, root_name, "directory")
    entries.append(root)
    dir_count += 1

    for current, dirs, files in os.walk(source, followlinks=False):
        current_path = Path(current)
        dirs.sort()
        files.sort()

        # os.walk lists symlinked directories in dirs. Record them as links and
        # remove them so we never traverse outside the selected tree.
        for name in list(dirs):
            p = current_path / name
            rel = (Path(root_name) / p.relative_to(source)).as_posix()
            if p.is_symlink():
                entries.append(stat_record(p, rel, "symlink"))
                dirs.remove(name)
            else:
                entries.append(stat_record(p, rel, "directory"))
                dir_count += 1

        for name in files:
            p = current_path / name
            rel = (Path(root_name) / p.relative_to(source)).as_posix()
            if p.is_symlink():
                entries.append(stat_record(p, rel, "symlink"))
                continue
            if not p.is_file():
                continue
            rec = stat_record(p, rel, "file")
            entries.append(rec)
            file_count += 1
            total_bytes += rec["size"]

    return "directory", entries, total_bytes, file_count, dir_count


def source_path_for_record(source: Path, source_type: str, record: dict) -> Path:
    if source_type == "file":
        return source
    rel = safe_relative(record["path"])
    # First path component is the archived root directory name.
    subparts = rel.parts[1:]
    return source.joinpath(*subparts)


def inspect_source(source: Path) -> dict:
    source = source.expanduser().resolve(strict=True)
    source_type, entries, total, files, dirs = scan_source(source)
    largest = max((e.get("size", 0) for e in entries), default=0)
    suggested_parts = max(1, math.ceil(total / max(1, DEFAULT_MAX_PART_BYTES - ZIP_SAFETY_BYTES)))
    return {
        "format": FORMAT,
        "operation": "inspect",
        "source": str(source),
        "source_type": source_type,
        "root_name": source.name,
        "total_bytes": total,
        "file_count": files,
        "directory_count": dirs,
        "entry_count": len(entries),
        "largest_file_bytes": largest,
        "default_max_part_bytes": DEFAULT_MAX_PART_BYTES,
        "suggested_parts": suggested_parts,
    }


def normalize_archive_name(name: str) -> str:
    cleaned = name.strip().replace("/", "_").replace("\\", "_")
    for suffix in (".manifest.json", ".zip"):
        if cleaned.endswith(suffix):
            cleaned = cleaned[: -len(suffix)]
    if not cleaned:
        raise SplitArchiveError("Archive name is empty")
    return cleaned


def plan_max_bytes(total_bytes: int, max_part_bytes: int | None, requested_parts: int | None) -> int:
    if requested_parts is not None:
        if requested_parts < 1:
            raise SplitArchiveError("Part count must be at least 1")
        payload = math.ceil(total_bytes / requested_parts) if total_bytes else 1
        # A small fixed allowance keeps ZIP metadata below the requested ceiling.
        computed = payload + ZIP_SAFETY_BYTES
        return max(MIN_PART_BYTES, computed)
    value = max_part_bytes if max_part_bytes is not None else DEFAULT_MAX_PART_BYTES
    if value < MIN_PART_BYTES:
        raise SplitArchiveError(f"Maximum part size must be at least {MIN_PART_BYTES} bytes")
    return value


def copy_range_into_zip(zf: zipfile.ZipFile, entry_name: str, source: Path, offset: int, size: int) -> str:
    h = hashlib.sha256()
    remaining = size
    info = zipfile.ZipInfo(entry_name)
    info.compress_type = zipfile.ZIP_STORED
    info.date_time = (1980, 1, 1, 0, 0, 0)
    info.external_attr = 0o600 << 16
    with source.open("rb") as src, zf.open(info, "w", force_zip64=True) as dst:
        src.seek(offset)
        while remaining:
            block = src.read(min(IO_BLOCK, remaining))
            if not block:
                raise SplitArchiveError(f"Unexpected EOF while reading {source}")
            dst.write(block)
            h.update(block)
            remaining -= len(block)
    return h.hexdigest()


def split_source(source: Path, output: Path, max_part_bytes: int | None, requested_parts: int | None, name: str | None) -> dict:
    source = source.expanduser().resolve(strict=True)
    output = output.expanduser().resolve()
    output.mkdir(parents=True, exist_ok=True)

    source_type, entries, total_bytes, file_count, dir_count = scan_source(source)
    archive_name = normalize_archive_name(name or source.name)
    ceiling = plan_max_bytes(total_bytes, max_part_bytes, requested_parts)
    payload_limit = ceiling - ZIP_SAFETY_BYTES
    if payload_limit <= 0:
        raise SplitArchiveError("Part ceiling leaves no room for payload")

    manifest_path = output / f"{archive_name}.manifest.json"
    existing_parts = sorted(output.glob(f"{archive_name}.part*.zip"))
    if manifest_path.exists() or existing_parts:
        raise SplitArchiveError(
            f"Output already contains a {archive_name!r} split set; choose another output directory or name"
        )

    manifest = {
        "format": FORMAT,
        "format_version": FORMAT_VERSION,
        "created_unix": int(time.time()),
        "archive_name": archive_name,
        "source_type": source_type,
        "root_name": source.name,
        "total_source_bytes": total_bytes,
        "file_count": file_count,
        "directory_count": dir_count,
        "max_part_bytes": ceiling,
        "entries": [],
        "parts": [],
        "chunks": [],
    }

    chunk_index = 0
    part_index = 0
    current_payload = 0
    zf: zipfile.ZipFile | None = None
    current_part_path: Path | None = None

    def open_part() -> None:
        nonlocal zf, current_payload, part_index, current_part_path
        part_index += 1
        current_payload = 0
        current_part_path = output / f"{archive_name}.part{part_index:03d}.zip"
        zf = zipfile.ZipFile(current_part_path, "w", compression=zipfile.ZIP_STORED, allowZip64=True)

    def close_part() -> None:
        nonlocal zf, current_part_path
        if zf is None or current_part_path is None:
            return
        zf.close()
        size = current_part_path.stat().st_size
        if size > ceiling:
            raise SplitArchiveError(
                f"Generated part exceeded ceiling ({size} > {ceiling}): {current_part_path.name}"
            )
        manifest["parts"].append({
            "index": part_index,
            "file": current_part_path.name,
            "size": size,
            "sha256": sha256_file(current_part_path),
        })
        zf = None
        current_part_path = None

    try:
        for record in entries:
            stored = dict(record)
            if record["type"] != "file":
                manifest["entries"].append(stored)
                continue

            src_file = source_path_for_record(source, source_type, record)
            stored["sha256"] = sha256_file(src_file)
            stored["chunks"] = []
            file_size = record["size"]
            offset = 0

            if file_size == 0:
                manifest["entries"].append(stored)
                continue

            while offset < file_size:
                if zf is None:
                    open_part()
                assert zf is not None
                assert current_part_path is not None

                remaining_part = payload_limit - current_payload
                if remaining_part <= 0:
                    close_part()
                    continue

                amount = min(file_size - offset, remaining_part)
                chunk_index += 1
                entry_name = f"chunks/{chunk_index:09d}.bin"
                digest = copy_range_into_zip(zf, entry_name, src_file, offset, amount)
                chunk = {
                    "index": chunk_index,
                    "path": record["path"],
                    "offset": offset,
                    "size": amount,
                    "sha256": digest,
                    "part": part_index,
                    "entry": entry_name,
                }
                manifest["chunks"].append(chunk)
                stored["chunks"].append(chunk_index)
                offset += amount
                current_payload += amount

                # Avoid another entry if it cannot contain useful payload.
                if payload_limit - current_payload < 4096:
                    close_part()

            manifest["entries"].append(stored)

        if zf is not None:
            close_part()
        if not manifest["parts"]:
            # An all-empty directory still gets one ordinary ZIP part.
            open_part()
            close_part()

        if requested_parts is not None and len(manifest["parts"]) > requested_parts:
            raise SplitArchiveError(
                f"Requested {requested_parts} parts is not achievable at the calculated safe size; "
                f"generated plan needs {len(manifest['parts'])}. Use a larger part count or max-size mode."
            )

        temp_manifest = manifest_path.with_suffix(manifest_path.suffix + ".tmp")
        temp_manifest.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        os.replace(temp_manifest, manifest_path)
        manifest["manifest"] = str(manifest_path)
        return manifest
    except Exception:
        if zf is not None:
            zf.close()
        for p in output.glob(f"{archive_name}.part*.zip"):
            try:
                p.unlink()
            except OSError:
                pass
        try:
            manifest_path.unlink()
        except OSError:
            pass
        raise


def locate_manifest(selected: Path) -> Path:
    selected = selected.expanduser().resolve(strict=True)
    if selected.name.endswith(".manifest.json"):
        return selected
    name = selected.name
    marker = ".part"
    if marker in name and name.endswith(".zip"):
        base = name.split(marker, 1)[0]
        candidate = selected.parent / f"{base}.manifest.json"
        if candidate.is_file():
            return candidate
    raise SplitArchiveError("Select a NOUGAT_SPLIT_ARCHIVE manifest or one of its .partNNN.zip files")


def load_manifest(selected: Path) -> tuple[Path, dict]:
    path = locate_manifest(selected)
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("format") != FORMAT or data.get("format_version") != FORMAT_VERSION:
        raise SplitArchiveError("Unsupported or invalid Nougat split archive manifest")
    return path, data


def verify_set(selected: Path) -> dict:
    manifest_path, manifest = load_manifest(selected)
    base = manifest_path.parent
    problems: list[str] = []

    part_map: dict[int, Path] = {}
    for part in manifest.get("parts", []):
        path = base / part["file"]
        part_map[int(part["index"])] = path
        if not path.is_file():
            problems.append(f"Missing part: {part['file']}")
            continue
        size = path.stat().st_size
        if size != int(part["size"]):
            problems.append(f"Wrong size: {part['file']} ({size} != {part['size']})")
            continue
        digest = sha256_file(path)
        if digest.lower() != str(part["sha256"]).lower():
            problems.append(f"SHA-256 mismatch: {part['file']}")

    if not problems:
        for chunk in manifest.get("chunks", []):
            part_path = part_map.get(int(chunk["part"]))
            if part_path is None or not part_path.is_file():
                continue
            try:
                with zipfile.ZipFile(part_path, "r") as zf:
                    with zf.open(chunk["entry"], "r") as src:
                        h = hashlib.sha256()
                        size = 0
                        for block in iter(lambda: src.read(IO_BLOCK), b""):
                            h.update(block)
                            size += len(block)
                if size != int(chunk["size"]):
                    problems.append(f"Chunk size mismatch: {chunk['entry']} in {part_path.name}")
                elif h.hexdigest().lower() != str(chunk["sha256"]).lower():
                    problems.append(f"Chunk SHA-256 mismatch: {chunk['entry']} in {part_path.name}")
            except (KeyError, zipfile.BadZipFile) as exc:
                problems.append(f"Unreadable chunk {chunk['entry']} in {part_path.name}: {exc}")

    return {
        "format": FORMAT,
        "operation": "verify",
        "manifest": str(manifest_path),
        "ok": not problems,
        "problems": problems,
        "part_count": len(manifest.get("parts", [])),
        "chunk_count": len(manifest.get("chunks", [])),
    }


def safe_symlink_target(root: Path, link_path: Path, target: str) -> bool:
    target_path = Path(target)
    if target_path.is_absolute():
        return False
    try:
        resolved_parent = link_path.parent.resolve()
        resolved_target = (resolved_parent / target_path).resolve(strict=False)
        resolved_target.relative_to(root.resolve())
        return True
    except (OSError, ValueError):
        return False


def reassemble(selected: Path, output: Path) -> dict:
    verification = verify_set(selected)
    if not verification["ok"]:
        raise SplitArchiveError("Cannot reassemble: " + "; ".join(verification["problems"]))

    manifest_path, manifest = load_manifest(selected)
    base = manifest_path.parent
    output = output.expanduser().resolve()
    output.mkdir(parents=True, exist_ok=True)
    root_name = str(manifest.get("root_name", "reassembled"))
    final_root = output / root_name
    if final_root.exists() or final_root.is_symlink():
        raise SplitArchiveError(f"Destination already exists: {final_root}")

    staging_parent = Path(tempfile.mkdtemp(prefix="nougat-reassemble-", dir=str(output)))
    staging_root = staging_parent / root_name
    part_map = {int(p["index"]): base / p["file"] for p in manifest.get("parts", [])}
    chunks_by_index = {int(c["index"]): c for c in manifest.get("chunks", [])}

    try:
        source_type = manifest.get("source_type")
        if source_type == "directory":
            staging_root.mkdir(parents=True, exist_ok=False)

        # Directories first, shallowest to deepest.
        dirs = [e for e in manifest.get("entries", []) if e.get("type") == "directory"]
        for entry in sorted(dirs, key=lambda e: len(safe_relative(e["path"]).parts)):
            rel = safe_relative(entry["path"])
            target = staging_parent / rel
            target.mkdir(parents=True, exist_ok=True)

        files = [e for e in manifest.get("entries", []) if e.get("type") == "file"]
        for entry in files:
            rel = safe_relative(entry["path"])
            target = staging_parent / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            h = hashlib.sha256()
            with target.open("wb") as dst:
                for chunk_index in entry.get("chunks", []):
                    chunk = chunks_by_index[int(chunk_index)]
                    part_path = part_map[int(chunk["part"])]
                    with zipfile.ZipFile(part_path, "r") as zf, zf.open(chunk["entry"], "r") as src:
                        for block in iter(lambda: src.read(IO_BLOCK), b""):
                            dst.write(block)
                            h.update(block)
            if target.stat().st_size != int(entry.get("size", 0)):
                raise SplitArchiveError(f"Reassembled size mismatch: {entry['path']}")
            if h.hexdigest().lower() != str(entry.get("sha256", h.hexdigest())).lower():
                raise SplitArchiveError(f"Reassembled SHA-256 mismatch: {entry['path']}")
            os.chmod(target, int(entry.get("mode", 0o644)))
            mtime_ns = int(entry.get("mtime_ns", 0))
            if mtime_ns > 0:
                os.utime(target, ns=(mtime_ns, mtime_ns), follow_symlinks=False)

        links = [e for e in manifest.get("entries", []) if e.get("type") == "symlink"]
        for entry in links:
            rel = safe_relative(entry["path"])
            target_path = staging_parent / rel
            target_path.parent.mkdir(parents=True, exist_ok=True)
            link_target = str(entry.get("target", ""))
            archive_root = staging_root if source_type == "directory" else staging_parent
            if not safe_symlink_target(archive_root, target_path, link_target):
                raise SplitArchiveError(f"Unsafe symlink target in archive: {entry['path']} -> {link_target}")
            os.symlink(link_target, target_path)

        # Directory metadata last so creating children does not overwrite times.
        for entry in sorted(dirs, key=lambda e: len(safe_relative(e["path"]).parts), reverse=True):
            target = staging_parent / safe_relative(entry["path"])
            os.chmod(target, int(entry.get("mode", 0o755)))
            mtime_ns = int(entry.get("mtime_ns", 0))
            if mtime_ns > 0:
                os.utime(target, ns=(mtime_ns, mtime_ns), follow_symlinks=False)

        if source_type == "file":
            staged_file = staging_parent / safe_relative(files[0]["path"])
            os.replace(staged_file, final_root)
        else:
            os.replace(staging_root, final_root)

        return {
            "format": FORMAT,
            "operation": "reassemble",
            "manifest": str(manifest_path),
            "output": str(final_root),
            "ok": True,
            "verified_source_bytes": int(manifest.get("total_source_bytes", 0)),
        }
    except Exception:
        if final_root.exists() or final_root.is_symlink():
            if final_root.is_dir() and not final_root.is_symlink():
                shutil.rmtree(final_root, ignore_errors=True)
            else:
                try:
                    final_root.unlink()
                except OSError:
                    pass
        raise
    finally:
        shutil.rmtree(staging_parent, ignore_errors=True)


def print_result(data: dict, as_json: bool) -> None:
    if as_json:
        print(json.dumps(data, indent=2, sort_keys=True))
        return
    for key, value in data.items():
        if key in ("entries", "chunks", "parts"):
            continue
        if isinstance(value, list):
            for item in value:
                print(f"{key}: {item}")
        else:
            print(f"{key}: {value}")


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Nougat Workshop NOUGAT_SPLIT_ARCHIVE engine")
    sub = p.add_subparsers(dest="command", required=True)

    inspect_p = sub.add_parser("inspect", help="Inspect a file/folder and suggest a split")
    inspect_p.add_argument("source", type=Path)
    inspect_p.add_argument("--json", action="store_true")

    split_p = sub.add_parser("split", help="Create a verified split archive set")
    split_p.add_argument("source", type=Path)
    split_p.add_argument("--output", type=Path, required=True)
    sizing = split_p.add_mutually_exclusive_group()
    sizing.add_argument("--max-part-bytes", type=int)
    sizing.add_argument("--parts", type=int)
    split_p.add_argument("--name")
    split_p.add_argument("--json", action="store_true")

    verify_p = sub.add_parser("verify", help="Verify manifest, part ZIPs, and chunks")
    verify_p.add_argument("selected", type=Path)
    verify_p.add_argument("--json", action="store_true")

    reassemble_p = sub.add_parser("reassemble", help="Verify and reconstruct the original tree")
    reassemble_p.add_argument("selected", type=Path)
    reassemble_p.add_argument("--output", type=Path, required=True)
    reassemble_p.add_argument("--json", action="store_true")
    return p


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        if args.command == "inspect":
            result = inspect_source(args.source)
        elif args.command == "split":
            result = split_source(args.source, args.output, args.max_part_bytes, args.parts, args.name)
        elif args.command == "verify":
            result = verify_set(args.selected)
            if not result["ok"]:
                print_result(result, args.json)
                return 2
        elif args.command == "reassemble":
            result = reassemble(args.selected, args.output)
        else:
            raise SplitArchiveError("Unknown operation")
        print_result(result, args.json)
        return 0
    except (OSError, ValueError, json.JSONDecodeError, SplitArchiveError, zipfile.BadZipFile) as exc:
        error = {"format": FORMAT, "ok": False, "error": str(exc)}
        print(json.dumps(error, indent=2, sort_keys=True), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
