#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import tempfile
import zipfile

FORMAT = "nougat-split-zip-v2"
BLOCK = 4 * 1024 * 1024

class SplitterError(RuntimeError):
    pass

class PieceCountTooSmall(SplitterError):
    def __init__(self, requested: int, minimum: int, max_bytes: int):
        self.requested = requested
        self.minimum = minimum
        self.max_bytes = max_bytes
        super().__init__(f"{requested} pieces are too large; minimum recommended count is {minimum}.")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(BLOCK), b""):
            h.update(block)
    return h.hexdigest()


def clean_name(value: str) -> str:
    value = value.strip().replace("/", "_").replace("\\", "_")
    if value.lower().endswith(".zip"):
        value = value[:-4].rstrip()
    if not value:
        raise SplitterError("Output name cannot be empty.")
    return value


def zipinfo_for(path: Path, arcname: str) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(arcname)
    info.create_system = 3
    info.external_attr = (path.lstat().st_mode & 0xFFFF) << 16
    return info


def package_folder(source: Path, output_zip: Path) -> None:
    source = source.resolve()
    root_name = source.name
    with zipfile.ZipFile(output_zip, "w", compression=zipfile.ZIP_DEFLATED,
                         allowZip64=True, compresslevel=6) as zf:
        root_info = zipinfo_for(source, root_name + "/")
        zf.writestr(root_info, b"")
        for path in sorted(source.rglob("*"), key=lambda p: p.as_posix().lower()):
            relative = path.relative_to(source).as_posix()
            arc = f"{root_name}/{relative}"
            if path.is_symlink():
                info = zipinfo_for(path, arc)
                info.external_attr = ((stat.S_IFLNK | (path.lstat().st_mode & 0o7777)) << 16)
                zf.writestr(info, os.readlink(path).encode("utf-8"))
            elif path.is_dir():
                zf.writestr(zipinfo_for(path, arc.rstrip("/") + "/"), b"")
            elif path.is_file():
                zf.write(path, arc)


def package_file(source: Path, output_zip: Path) -> None:
    with zipfile.ZipFile(output_zip, "w", compression=zipfile.ZIP_DEFLATED,
                         allowZip64=True, compresslevel=6) as zf:
        zf.write(source.resolve(), source.name)


def create_payload(source: Path, temp_dir: Path, base: str):
    source = source.expanduser().resolve()
    if source.is_dir():
        payload = temp_dir / f"{base}.zip"
        print("STATUS Creating ZIP from selected folder...", flush=True)
        package_folder(source, payload)
        return payload, "folder", source.name
    if not source.is_file():
        raise SplitterError(f"Source does not exist: {source}")
    if source.suffix.lower() == ".zip":
        print("STATUS Using existing ZIP directly...", flush=True)
        return source, "existing_zip", source.name
    payload = temp_dir / f"{base}.zip"
    print("STATUS Creating ZIP from selected file...", flush=True)
    package_file(source, payload)
    return payload, "file", source.name


def minimum_count(size: int, max_piece_bytes: int) -> int:
    if max_piece_bytes <= 0:
        return 1
    return max(1, math.ceil(size / max_piece_bytes))


def split(source: Path, output_dir: Path, output_name: str, pieces: int,
          max_piece_bytes: int = 0) -> Path:
    if pieces < 1:
        raise SplitterError("Piece count must be at least 1.")
    output_dir = output_dir.expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    base = clean_name(output_name)

    with tempfile.TemporaryDirectory(prefix="nougat-split-v51-") as temp_name:
        payload, mode, source_name = create_payload(source, Path(temp_name), base)
        size = payload.stat().st_size
        minimum = minimum_count(size, max_piece_bytes)
        if pieces < minimum:
            raise PieceCountTooSmall(pieces, minimum, max_piece_bytes)

        width = max(3, len(str(pieces)))
        part_paths = [output_dir / f"{base}.zip.{index:0{width}d}"
                      for index in range(1, pieces + 1)]
        manifest = output_dir / f"{base}.zip.parts.json"
        for path in [manifest, *part_paths]:
            if path.exists():
                raise SplitterError(f"Refusing to overwrite existing output: {path}")

        overall = hashlib.sha256()
        records = []
        remaining = size
        with payload.open("rb") as src:
            for index, final in enumerate(part_paths, start=1):
                remaining_parts = pieces - index + 1
                target = 0 if remaining <= 0 else math.ceil(remaining / remaining_parts)
                tmp = final.with_suffix(final.suffix + ".tmp")
                part_hash = hashlib.sha256()
                written = 0
                with tmp.open("xb") as out:
                    left = target
                    while left > 0:
                        block = src.read(min(BLOCK, left))
                        if not block:
                            break
                        out.write(block)
                        overall.update(block)
                        part_hash.update(block)
                        written += len(block)
                        left -= len(block)
                    out.flush()
                    os.fsync(out.fileno())
                os.replace(tmp, final)
                remaining -= written
                records.append({
                    "index": index,
                    "name": final.name,
                    "size": written,
                    "sha256": part_hash.hexdigest(),
                })
                done = size - remaining
                percent = 100 if size == 0 else min(100, int(done * 100 / size))
                print(f"PROGRESS {percent}", flush=True)

        record = {
            "format": FORMAT,
            "mode": mode,
            "source_name": source_name,
            "output_base": base,
            "payload_name": f"{base}.zip",
            "payload_size": size,
            "payload_sha256": overall.hexdigest(),
            "piece_count": pieces,
            "approx_piece_size": math.ceil(size / pieces) if pieces else 0,
            "max_piece_bytes": max_piece_bytes,
            "parts": records,
        }
        manifest.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"MANIFEST {manifest}", flush=True)
        return manifest


def load_manifest(manifest: Path) -> dict:
    data = json.loads(manifest.read_text(encoding="utf-8"))
    if data.get("format") != FORMAT:
        raise SplitterError("Unsupported Nougat split-ZIP manifest.")
    if not isinstance(data.get("parts"), list):
        raise SplitterError("Manifest is missing its part list.")
    return data


def verify(manifest: Path) -> dict:
    manifest = manifest.expanduser().resolve()
    data = load_manifest(manifest)
    total = 0
    for expected, part in enumerate(data["parts"], start=1):
        name = str(part.get("name", ""))
        if Path(name).name != name or int(part.get("index", -1)) != expected:
            raise SplitterError("Manifest contains an invalid part entry.")
        path = manifest.parent / name
        if not path.is_file():
            raise SplitterError(f"Missing part: {path}")
        if path.stat().st_size != int(part.get("size", -1)):
            raise SplitterError(f"Part size mismatch: {path.name}")
        if sha256_file(path) != str(part.get("sha256", "")):
            raise SplitterError(f"Part SHA-256 mismatch: {path.name}")
        total += path.stat().st_size
    if total != int(data.get("payload_size", -1)):
        raise SplitterError("Combined part size does not match ZIP payload size.")
    print("VERIFY PASS", flush=True)
    return data


def join_zip(manifest: Path, output_zip: Path) -> dict:
    data = verify(manifest)
    if output_zip.exists():
        raise SplitterError(f"Refusing to overwrite: {output_zip}")
    h = hashlib.sha256()
    tmp = output_zip.with_suffix(output_zip.suffix + ".tmp")
    with tmp.open("xb") as out:
        for part in data["parts"]:
            with (manifest.parent / part["name"]).open("rb") as src:
                for block in iter(lambda: src.read(BLOCK), b""):
                    out.write(block)
                    h.update(block)
        out.flush()
        os.fsync(out.fileno())
    if h.hexdigest() != data["payload_sha256"]:
        tmp.unlink(missing_ok=True)
        raise SplitterError("Reassembled ZIP SHA-256 does not match the manifest.")
    os.replace(tmp, output_zip)
    return data


def safe_member(name: str) -> PurePosixPath:
    p = PurePosixPath(name)
    if p.is_absolute() or ".." in p.parts:
        raise SplitterError(f"Unsafe ZIP path: {name}")
    return p


def extract_packaged(zip_path: Path, destination: Path) -> None:
    with zipfile.ZipFile(zip_path, "r") as zf:
        for info in zf.infolist():
            rel = safe_member(info.filename)
            target = destination.joinpath(*rel.parts)
            mode = (info.external_attr >> 16) & 0xFFFF
            if info.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                if mode & 0o7777:
                    os.chmod(target, mode & 0o7777)
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists() or target.is_symlink():
                raise SplitterError(f"Refusing to overwrite extracted path: {target}")
            if stat.S_IFMT(mode) == stat.S_IFLNK:
                os.symlink(zf.read(info).decode("utf-8"), target)
            else:
                with zf.open(info) as src, target.open("xb") as out:
                    shutil.copyfileobj(src, out, BLOCK)
                if mode & 0o7777:
                    os.chmod(target, mode & 0o7777)


def reassemble(manifest: Path, output: Path | None = None) -> Path:
    manifest = manifest.expanduser().resolve()
    data = load_manifest(manifest)
    if data["mode"] == "existing_zip":
        final = output.expanduser().resolve() if output else manifest.parent / f"{data['output_base']}.reassembled.zip"
        final.parent.mkdir(parents=True, exist_ok=True)
        join_zip(manifest, final)
        print(f"REASSEMBLED {final}", flush=True)
        return final

    destination = output.expanduser().resolve() if output else manifest.parent
    destination.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="nougat-reassemble-v51-") as temp_name:
        joined = Path(temp_name) / f"{data['output_base']}.zip"
        join_zip(manifest, joined)
        extract_packaged(joined, destination)
    restored = destination / data["source_name"]
    if not restored.exists() and not restored.is_symlink():
        raise SplitterError("Expected original folder/file was not restored.")
    print(f"REASSEMBLED {restored}", flush=True)
    return restored


def zenity(args: list[str]) -> subprocess.CompletedProcess:
    if shutil.which("zenity") is None:
        raise SplitterError("Zenity is required for the Studio File Splitter interface.")
    return subprocess.run(["zenity", *args], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def zvalue(args: list[str]) -> str:
    result = zenity(args)
    return result.stdout.strip() if result.returncode == 0 else ""


def human_size(value: int) -> str:
    amount = float(value)
    for unit in ("B", "KiB", "MiB", "GiB", "TiB"):
        if amount < 1024.0 or unit == "TiB":
            return f"{amount:.2f} {unit}"
        amount /= 1024.0
    return str(value)


def gui_split() -> int:
    kind = zvalue(["--list", "--title=Nougat File Splitter", "--text=Choose the original input.",
                   "--column=Input", "Folder", "File or existing ZIP"])
    if not kind:
        return 0
    if kind == "Folder":
        source = zvalue(["--file-selection", "--directory", "--title=Choose Folder"])
    else:
        source = zvalue(["--file-selection", "--title=Choose File or Existing ZIP"])
    if not source:
        return 0
    source_path = Path(source)
    output_dir = zvalue(["--file-selection", "--directory", "--title=Choose Output Folder"])
    if not output_dir:
        return 0
    default = source_path.stem if source_path.is_file() else source_path.name
    name = zvalue(["--entry", "--title=Output Name", "--text=Name for the split ZIP pieces:",
                   f"--entry-text={default}"])
    if not name:
        return 0
    pieces_text = zvalue(["--entry", "--title=Number of Pieces", "--text=How many pieces do you want?",
                          "--entry-text=3"])
    if not pieces_text:
        return 0
    try:
        pieces = int(pieces_text)
        if pieces < 1:
            raise ValueError
    except ValueError as exc:
        raise SplitterError("Piece count must be a positive whole number.") from exc
    max_text = zvalue(["--entry", "--title=Maximum Piece Size",
                       "--text=Optional maximum size per piece in MiB. Enter 0 for no maximum.",
                       "--entry-text=0"])
    max_mib = int(max_text or "0")
    if max_mib < 0:
        raise SplitterError("Maximum piece size cannot be negative.")
    max_bytes = max_mib * 1024 * 1024

    base = clean_name(name)
    with tempfile.TemporaryDirectory(prefix="nougat-preview-v51-") as temp_name:
        payload, _, _ = create_payload(source_path, Path(temp_name), base)
        size = payload.stat().st_size
        minimum = minimum_count(size, max_bytes)
        if pieces < minimum:
            answer = zenity(["--question", "--title=Piece Count Suggestion",
                             "--text", f"{pieces} pieces are too large for the configured maximum.\n"
                                       f"Minimum recommended count: {minimum}\n\nUse {minimum} pieces?"])
            if answer.returncode != 0:
                return 0
            pieces = minimum
        approx = math.ceil(size / pieces) if pieces else 0
        confirm = zenity(["--question", "--title=Ready to Split",
                          "--text", f"ZIP payload: {human_size(size)}\nPieces: {pieces}\n"
                                    f"Approximate piece size: {human_size(approx)}\n"
                                    f"Output: {base}.zip.001 ...\n\nStart splitting?"])
        if confirm.returncode != 0:
            return 0

    manifest = split(source_path, Path(output_dir), base, pieces, max_bytes)
    zenity(["--info", "--title=Nougat File Splitter", "--text", f"Split complete.\n{manifest}"])
    return 0


def gui_reassemble() -> int:
    manifest = zvalue(["--file-selection", "--title=Choose Nougat Parts Manifest",
                       "--file-filter=Nougat manifests | *.zip.parts.json"])
    if not manifest:
        return 0
    data = load_manifest(Path(manifest))
    if data["mode"] == "existing_zip":
        default = str(Path(manifest).parent / f"{data['output_base']}.reassembled.zip")
        output = zvalue(["--file-selection", "--save", "--confirm-overwrite", f"--filename={default}",
                         "--title=Save Reassembled ZIP"])
    else:
        output = zvalue(["--file-selection", "--directory", "--title=Choose Reassembly Destination"])
    if not output:
        return 0
    result = reassemble(Path(manifest), Path(output))
    zenity(["--info", "--title=Nougat File Splitter", "--text", f"Reassembly verified.\n{result}"])
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    s = sub.add_parser("split")
    s.add_argument("source", type=Path)
    s.add_argument("output_dir", type=Path)
    s.add_argument("--name", required=True)
    s.add_argument("--pieces", type=int, required=True)
    s.add_argument("--max-piece-mib", type=int, default=0)
    v = sub.add_parser("verify")
    v.add_argument("manifest", type=Path)
    r = sub.add_parser("reassemble")
    r.add_argument("manifest", type=Path)
    r.add_argument("--output", type=Path)
    g = sub.add_parser("studio-gui")
    g.add_argument("action", choices=["split", "reassemble", "verify"])
    args = parser.parse_args()

    if args.command == "split":
        split(args.source, args.output_dir, args.name, args.pieces,
              max(0, args.max_piece_mib) * 1024 * 1024)
    elif args.command == "verify":
        verify(args.manifest)
    elif args.command == "reassemble":
        reassemble(args.manifest, args.output)
    elif args.command == "studio-gui":
        if args.action == "split":
            return gui_split()
        if args.action == "reassemble":
            return gui_reassemble()
        manifest = zvalue(["--file-selection", "--title=Choose Nougat Parts Manifest",
                           "--file-filter=Nougat manifests | *.zip.parts.json"])
        if manifest:
            verify(Path(manifest))
            zenity(["--info", "--title=Nougat File Splitter",
                    "--text=All split ZIP pieces passed SHA-256 verification."])
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except PieceCountTooSmall as exc:
        print(f"SUGGESTED_PIECES {exc.minimum}")
        print(f"ERROR {exc}")
        raise SystemExit(2)
    except (SplitterError, ValueError, json.JSONDecodeError, zipfile.BadZipFile) as exc:
        print(f"ERROR {exc}")
        raise SystemExit(1)
