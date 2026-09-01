#!/usr/bin/env python3
"""Nougat Studio File Splitter / Reassembler v0.0.50.

Splits large files or folders into independently SHA-256 verified parts and
reconstructs them byte-for-byte. Uses only Python's standard library.
"""
from __future__ import annotations

import argparse
import datetime as _dt
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys
import tarfile
import tempfile
from dataclasses import dataclass
from typing import BinaryIO, Iterable

FORMAT = "nougat-parts-v1"
DEFAULT_PART_SIZE_MIB = 100
COPY_BLOCK = 4 * 1024 * 1024


class SplitterError(RuntimeError):
    pass


def _sha256_file(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(COPY_BLOCK), b""):
            h.update(block)
    return h.hexdigest()


def _utc_now() -> str:
    return _dt.datetime.now(_dt.timezone.utc).replace(microsecond=0).isoformat()


def _safe_name(name: str) -> str:
    # Preserve readable names while keeping generated transport filenames flat.
    cleaned = name.replace("/", "_").replace("\\", "_").strip()
    return cleaned or "payload"


def _manifest_name(payload_name: str) -> str:
    return _safe_name(payload_name) + ".parts.json"


def _part_name(payload_name: str, index: int, width: int) -> str:
    return f"{_safe_name(payload_name)}.part{index:0{width}d}"


def _ensure_output_dir(path: pathlib.Path) -> None:
    path.mkdir(parents=True, exist_ok=True)
    if not path.is_dir():
        raise SplitterError(f"Output path is not a directory: {path}")


def _refuse_collisions(paths: Iterable[pathlib.Path]) -> None:
    existing = [str(path) for path in paths if path.exists()]
    if existing:
        sample = "\n".join(existing[:6])
        raise SplitterError("Refusing to overwrite existing splitter output:\n" + sample)


def _split_payload(payload_path: pathlib.Path, output_dir: pathlib.Path, *,
                   mode: str, source_name: str, part_size_bytes: int,
                   folder_metadata: dict | None = None) -> pathlib.Path:
    if part_size_bytes <= 0:
        raise SplitterError("Part size must be greater than zero.")
    if not payload_path.is_file():
        raise SplitterError(f"Payload does not exist: {payload_path}")

    _ensure_output_dir(output_dir)
    payload_size = payload_path.stat().st_size
    part_count = max(1, (payload_size + part_size_bytes - 1) // part_size_bytes)
    width = max(3, len(str(part_count)))
    payload_name = payload_path.name
    manifest_path = output_dir / _manifest_name(payload_name)
    planned_parts = [output_dir / _part_name(payload_name, i + 1, width)
                     for i in range(part_count)]
    _refuse_collisions([manifest_path, *planned_parts])

    overall = hashlib.sha256()
    parts: list[dict] = []
    created: list[pathlib.Path] = []
    total_written = 0

    try:
        with payload_path.open("rb") as source:
            for index, final_path in enumerate(planned_parts, start=1):
                temp_path = final_path.with_name(final_path.name + ".tmp")
                if temp_path.exists():
                    temp_path.unlink()
                part_hash = hashlib.sha256()
                part_written = 0
                with temp_path.open("xb") as out:
                    remaining = part_size_bytes
                    while remaining > 0:
                        block = source.read(min(COPY_BLOCK, remaining))
                        if not block:
                            break
                        out.write(block)
                        overall.update(block)
                        part_hash.update(block)
                        part_written += len(block)
                        total_written += len(block)
                        remaining -= len(block)
                    out.flush()
                    os.fsync(out.fileno())
                os.replace(temp_path, final_path)
                created.append(final_path)
                parts.append({
                    "index": index,
                    "name": final_path.name,
                    "size": part_written,
                    "sha256": part_hash.hexdigest(),
                })
                percent = 100 if payload_size == 0 else int((total_written * 100) / payload_size)
                print(f"PROGRESS {min(100, percent)}", flush=True)

        if total_written != payload_size:
            raise SplitterError(
                f"Source size changed while splitting: expected {payload_size} bytes, read {total_written} bytes.")

        manifest = {
            "format": FORMAT,
            "created_utc": _utc_now(),
            "mode": mode,
            "source_name": source_name,
            "payload_name": payload_name,
            "payload_size": payload_size,
            "payload_sha256": overall.hexdigest(),
            "part_size_bytes": part_size_bytes,
            "part_count": len(parts),
            "parts": parts,
        }
        if folder_metadata is not None:
            manifest["folder"] = folder_metadata

        tmp_manifest = manifest_path.with_name(manifest_path.name + ".tmp")
        tmp_manifest.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        os.replace(tmp_manifest, manifest_path)
        print(f"MANIFEST {manifest_path}", flush=True)
        print(f"SHA256 {manifest['payload_sha256']}", flush=True)
        print(f"PARTS {len(parts)}", flush=True)
        return manifest_path
    except Exception:
        for path in created:
            try:
                path.unlink()
            except OSError:
                pass
        for path in planned_parts:
            tmp = path.with_name(path.name + ".tmp")
            try:
                tmp.unlink()
            except OSError:
                pass
        raise


def split_file(source: pathlib.Path, output_dir: pathlib.Path, part_size_bytes: int) -> pathlib.Path:
    source = source.expanduser().resolve()
    if not source.is_file():
        raise SplitterError(f"File does not exist: {source}")
    return _split_payload(source, output_dir.expanduser().resolve(), mode="file",
                          source_name=source.name, part_size_bytes=part_size_bytes)


def _build_folder_tar(source: pathlib.Path, tar_path: pathlib.Path) -> None:
    # Plain tar is intentional: fast, exact, stream-friendly, and no gzip timestamp.
    with tarfile.open(tar_path, mode="w", format=tarfile.PAX_FORMAT, dereference=False) as tf:
        tf.add(source, arcname=source.name, recursive=True)


def split_folder(source: pathlib.Path, output_dir: pathlib.Path, part_size_bytes: int) -> pathlib.Path:
    source = source.expanduser().resolve()
    if not source.is_dir():
        raise SplitterError(f"Folder does not exist: {source}")
    output_dir = output_dir.expanduser().resolve()
    _ensure_output_dir(output_dir)

    with tempfile.TemporaryDirectory(prefix="nougat-folder-split-") as temp_dir:
        tar_path = pathlib.Path(temp_dir) / f"{_safe_name(source.name)}.nougat-folder.tar"
        print("STATUS Packaging folder into a lossless tar payload...", flush=True)
        _build_folder_tar(source, tar_path)
        metadata = {
            "archive_format": "tar-pax",
            "root_name": source.name,
        }
        return _split_payload(tar_path, output_dir, mode="folder", source_name=source.name,
                              part_size_bytes=part_size_bytes, folder_metadata=metadata)


def _load_manifest(path: pathlib.Path) -> dict:
    path = path.expanduser().resolve()
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SplitterError(f"Could not read manifest {path}: {exc}") from exc
    if data.get("format") != FORMAT:
        raise SplitterError(f"Unsupported splitter manifest format: {data.get('format')!r}")
    required = ("payload_name", "payload_size", "payload_sha256", "part_count", "parts", "mode")
    for key in required:
        if key not in data:
            raise SplitterError(f"Manifest is missing required field: {key}")
    if not isinstance(data["parts"], list) or len(data["parts"]) != int(data["part_count"]):
        raise SplitterError("Manifest part count does not match the parts list.")
    return data


def verify(manifest_path: pathlib.Path) -> dict:
    manifest_path = manifest_path.expanduser().resolve()
    data = _load_manifest(manifest_path)
    base = manifest_path.parent
    total = 0
    for expected_index, part in enumerate(data["parts"], start=1):
        if int(part.get("index", -1)) != expected_index:
            raise SplitterError(f"Manifest part order is invalid at index {expected_index}.")
        name = str(part.get("name", ""))
        if not name or pathlib.Path(name).name != name:
            raise SplitterError(f"Unsafe part filename in manifest: {name!r}")
        part_path = base / name
        if not part_path.is_file():
            raise SplitterError(f"Missing part: {part_path}")
        actual_size = part_path.stat().st_size
        expected_size = int(part.get("size", -1))
        if actual_size != expected_size:
            raise SplitterError(
                f"Part size mismatch for {name}: expected {expected_size}, got {actual_size}.")
        actual_hash = _sha256_file(part_path)
        if actual_hash.lower() != str(part.get("sha256", "")).lower():
            raise SplitterError(f"SHA-256 mismatch for part: {name}")
        total += actual_size
        print(f"VERIFIED {expected_index}/{len(data['parts'])} {name}", flush=True)
    if total != int(data["payload_size"]):
        raise SplitterError(
            f"Combined part size mismatch: expected {data['payload_size']}, got {total}.")
    print("VERIFY PASS", flush=True)
    return data


def _safe_extract_tar(tar_path: pathlib.Path, destination: pathlib.Path) -> pathlib.Path:
    destination.mkdir(parents=True, exist_ok=True)
    dest_resolved = destination.resolve()
    with tarfile.open(tar_path, "r") as tf:
        members = tf.getmembers()
        for member in members:
            member_path = destination / member.name
            try:
                resolved_parent = member_path.parent.resolve()
            except OSError as exc:
                raise SplitterError(f"Unsafe tar member: {member.name}") from exc
            if os.path.commonpath([str(dest_resolved), str(resolved_parent)]) != str(dest_resolved):
                raise SplitterError(f"Archive path escapes destination: {member.name}")
            if member.name.startswith("/") or ".." in pathlib.PurePosixPath(member.name).parts:
                raise SplitterError(f"Unsafe archive member path: {member.name}")
        try:
            tf.extractall(destination, filter="data")  # Python 3.12+
        except TypeError:
            tf.extractall(destination)
    roots = [p for p in destination.iterdir()]
    return roots[0] if len(roots) == 1 else destination


def reassemble(manifest_path: pathlib.Path, output: pathlib.Path | None = None) -> pathlib.Path:
    manifest_path = manifest_path.expanduser().resolve()
    data = verify(manifest_path)
    base = manifest_path.parent
    mode = str(data["mode"])
    payload_name = str(data["payload_name"])

    if mode == "folder":
        if output is None:
            output_dir = base
        else:
            output_dir = output.expanduser().resolve()
        output_dir.mkdir(parents=True, exist_ok=True)
        root_name = str(data.get("folder", {}).get("root_name", data.get("source_name", "reassembled-folder")))
        final_root = output_dir / root_name
        if final_root.exists():
            raise SplitterError(f"Refusing to overwrite existing reconstructed folder: {final_root}")
        temp_dir = pathlib.Path(tempfile.mkdtemp(prefix="nougat-reassemble-", dir=str(output_dir)))
        temp_payload = temp_dir / payload_name
        extract_dir = temp_dir / "extract"
        try:
            _join_parts(data, base, temp_payload)
            actual_hash = _sha256_file(temp_payload)
            if actual_hash.lower() != str(data["payload_sha256"]).lower():
                raise SplitterError("Reassembled payload SHA-256 does not match the original.")
            extracted = _safe_extract_tar(temp_payload, extract_dir)
            candidate = extract_dir / root_name
            if not candidate.exists():
                if extracted == extract_dir:
                    raise SplitterError("Folder archive did not contain the expected root folder.")
                candidate = extracted
            os.replace(candidate, final_root)
            print(f"REASSEMBLED {final_root}", flush=True)
            print(f"SHA256 {actual_hash}", flush=True)
            return final_root
        finally:
            shutil.rmtree(temp_dir, ignore_errors=True)

    if mode != "file":
        raise SplitterError(f"Unsupported manifest mode: {mode}")
    if output is None:
        source_name = str(data.get("source_name") or payload_name)
        final_path = base / (source_name + ".reassembled")
    else:
        final_path = output.expanduser().resolve()
        if final_path.is_dir():
            final_path = final_path / str(data.get("source_name") or payload_name)
    if final_path.exists():
        raise SplitterError(f"Refusing to overwrite existing reconstructed file: {final_path}")
    final_path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = final_path.with_name(final_path.name + ".tmp")
    if temp_path.exists():
        temp_path.unlink()
    try:
        _join_parts(data, base, temp_path)
        actual_hash = _sha256_file(temp_path)
        if actual_hash.lower() != str(data["payload_sha256"]).lower():
            raise SplitterError("Reassembled file SHA-256 does not match the original.")
        os.replace(temp_path, final_path)
        print(f"REASSEMBLED {final_path}", flush=True)
        print(f"SHA256 {actual_hash}", flush=True)
        return final_path
    finally:
        try:
            temp_path.unlink()
        except OSError:
            pass


def _join_parts(data: dict, base: pathlib.Path, output_path: pathlib.Path) -> None:
    expected_total = int(data["payload_size"])
    written = 0
    with output_path.open("xb") as out:
        for part in data["parts"]:
            part_path = base / str(part["name"])
            with part_path.open("rb") as src:
                for block in iter(lambda: src.read(COPY_BLOCK), b""):
                    out.write(block)
                    written += len(block)
                    percent = 100 if expected_total == 0 else int((written * 100) / expected_total)
                    print(f"PROGRESS {min(100, percent)}", flush=True)
        out.flush()
        os.fsync(out.fileno())
    if written != expected_total:
        raise SplitterError(f"Reassembled size mismatch: expected {expected_total}, got {written}.")


def _part_size_bytes(mib: int) -> int:
    if mib < 1 or mib > 10240:
        raise SplitterError("Part size must be between 1 MiB and 10240 MiB.")
    return mib * 1024 * 1024



def _zenity(args: list[str], *, capture: bool = True) -> subprocess.CompletedProcess[str]:
    if shutil.which("zenity") is None:
        raise SplitterError("Zenity is required for the Studio File Splitter interface.")
    return subprocess.run(["zenity", *args], text=True,
                          stdout=subprocess.PIPE if capture else None,
                          stderr=subprocess.PIPE if capture else None)


def _zenity_path(args: list[str]) -> pathlib.Path | None:
    result = _zenity(args)
    if result.returncode != 0:
        return None
    value = result.stdout.strip()
    return pathlib.Path(value).expanduser().resolve() if value else None


def _zenity_part_size() -> int | None:
    result = _zenity([
        "--entry", "--title=Nougat Studio File Splitter",
        "--text=Part size in MiB:", f"--entry-text={DEFAULT_PART_SIZE_MIB}",
    ])
    if result.returncode != 0:
        return None
    try:
        return _part_size_bytes(int(result.stdout.strip()))
    except (ValueError, SplitterError) as exc:
        _zenity(["--error", "--title=Nougat Studio File Splitter", "--text=" + str(exc)], capture=False)
        return None


def _zenity_message(kind: str, text: str) -> None:
    _zenity([f"--{kind}", "--title=Nougat Studio File Splitter", "--width=520", "--text=" + text], capture=False)


def _gui_run_cli(arguments: list[str], title: str) -> bool:
    command = [sys.executable, str(pathlib.Path(__file__).resolve()), *arguments]
    child = subprocess.Popen(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    progress = subprocess.Popen([
        "zenity", "--progress", "--title=" + title,
        "--text=Nougat is working...", "--percentage=0", "--auto-close", "--no-cancel",
        "--width=520",
    ], text=True, stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    lines: list[str] = []
    assert child.stdout is not None
    for line in child.stdout:
        line = line.rstrip("\n")
        lines.append(line)
        if line.startswith("PROGRESS ") and progress.stdin is not None:
            try:
                percentage = int(line.split(None, 1)[1])
                progress.stdin.write(str(max(0, min(100, percentage))) + "\n")
                progress.stdin.flush()
            except (ValueError, BrokenPipeError):
                pass
        elif line.startswith("STATUS ") and progress.stdin is not None:
            try:
                progress.stdin.write("# " + line[7:] + "\n")
                progress.stdin.flush()
            except BrokenPipeError:
                pass
    rc = child.wait()
    if progress.stdin is not None:
        try:
            progress.stdin.close()
        except BrokenPipeError:
            pass
    try:
        progress.wait(timeout=2)
    except subprocess.TimeoutExpired:
        progress.terminate()
    tail = "\n".join(lines[-6:]) if lines else "No output was produced."
    if rc == 0:
        _zenity_message("info", "Operation completed successfully.\n\n" + tail)
        return True
    _zenity_message("error", "Operation failed.\n\n" + tail)
    return False


def studio_gui(action: str) -> int:
    if action == "split-file":
        source = _zenity_path(["--file-selection", "--title=Choose a file to split"])
        if source is None:
            return 0
        output = _zenity_path(["--file-selection", "--directory", "--title=Choose the output folder",
                               "--filename=" + str(source.parent) + "/"])
        if output is None:
            return 0
        size = _zenity_part_size()
        if size is None:
            return 0
        return 0 if _gui_run_cli(["split-file", str(source), "--output", str(output),
                                  "--part-size-mib", str(size // (1024 * 1024))],
                                 "Splitting file") else 2
    if action == "split-folder":
        source = _zenity_path(["--file-selection", "--directory", "--title=Choose a folder to split"])
        if source is None:
            return 0
        output = _zenity_path(["--file-selection", "--directory", "--title=Choose the output folder",
                               "--filename=" + str(source.parent) + "/"])
        if output is None:
            return 0
        size = _zenity_part_size()
        if size is None:
            return 0
        return 0 if _gui_run_cli(["split-folder", str(source), "--output", str(output),
                                  "--part-size-mib", str(size // (1024 * 1024))],
                                 "Packaging and splitting folder") else 2
    if action in {"reassemble", "verify"}:
        manifest = _zenity_path([
            "--file-selection", "--title=Choose a Nougat .parts.json manifest",
            "--file-filter=Nougat manifests | *.parts.json", "--file-filter=All files | *",
        ])
        if manifest is None:
            return 0
        arguments = [action, str(manifest)]
        if action == "reassemble":
            output = _zenity_path(["--file-selection", "--directory", "--title=Choose reconstruction destination",
                                   "--filename=" + str(manifest.parent) + "/"])
            if output is None:
                return 0
            arguments.extend(["--output", str(output)])
        return 0 if _gui_run_cli(arguments, "Reassembling" if action == "reassemble" else "Verifying parts") else 2
    raise SplitterError(f"Unknown Studio splitter action: {action}")

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Nougat Studio File Splitter / Reassembler")
    sub = parser.add_subparsers(dest="command", required=True)

    split_file_parser = sub.add_parser("split-file", help="Split one file")
    split_file_parser.add_argument("source", type=pathlib.Path)
    split_file_parser.add_argument("--output", type=pathlib.Path)
    split_file_parser.add_argument("--part-size-mib", type=int, default=DEFAULT_PART_SIZE_MIB)

    split_folder_parser = sub.add_parser("split-folder", help="Package and split one folder")
    split_folder_parser.add_argument("source", type=pathlib.Path)
    split_folder_parser.add_argument("--output", type=pathlib.Path)
    split_folder_parser.add_argument("--part-size-mib", type=int, default=DEFAULT_PART_SIZE_MIB)

    reassemble_parser = sub.add_parser("reassemble", help="Verify and reassemble a .parts.json manifest")
    reassemble_parser.add_argument("manifest", type=pathlib.Path)
    reassemble_parser.add_argument("--output", type=pathlib.Path)

    verify_parser = sub.add_parser("verify", help="Verify all parts without reconstructing")
    verify_parser.add_argument("manifest", type=pathlib.Path)

    gui_parser = sub.add_parser("studio-gui", help="Open a Studio File Splitter action with Zenity")
    gui_parser.add_argument("action", choices=("split-file", "split-folder", "reassemble", "verify"))
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.command == "split-file":
            source = args.source.expanduser().resolve()
            output = (args.output.expanduser().resolve() if args.output
                      else source.parent / f"{source.name}.nougat-parts")
            split_file(source, output, _part_size_bytes(args.part_size_mib))
        elif args.command == "split-folder":
            source = args.source.expanduser().resolve()
            output = (args.output.expanduser().resolve() if args.output
                      else source.parent / f"{source.name}.nougat-parts")
            split_folder(source, output, _part_size_bytes(args.part_size_mib))
        elif args.command == "reassemble":
            reassemble(args.manifest, args.output)
        elif args.command == "verify":
            verify(args.manifest)
        elif args.command == "studio-gui":
            return studio_gui(args.action)
        else:
            raise SplitterError(f"Unknown command: {args.command}")
        return 0
    except SplitterError as exc:
        print(f"ERROR {exc}", file=sys.stderr)
        return 2
    except KeyboardInterrupt:
        print("ERROR Operation cancelled.", file=sys.stderr)
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
