#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
from pathlib import Path, PurePosixPath
import shutil
import signal
import stat
import tempfile
import zipfile

FORMAT = "nougat-split-zip-v3"
LEGACY_FORMATS = {"nougat-split-zip-v2"}
BLOCK = 4 * 1024 * 1024
DEFAULT_TARGET_PIECE_BYTES = 450 * 1024 * 1024  # NOUGAT_V58_SAFE_UPLOAD_TARGET
MAX_UPLOAD_SAFE_PIECE_BYTES = 476 * 1024 * 1024  # below 500,000,000 bytes


class SplitterError(RuntimeError):
    pass


class SplitterCancelled(SplitterError):
    pass


class PieceCountTooSmall(SplitterError):
    def __init__(self, requested: int, minimum: int, max_bytes: int):
        self.requested = requested
        self.minimum = minimum
        self.max_bytes = max_bytes
        super().__init__(f"{requested} pieces are too large; minimum recommended count is {minimum}.")


_cancel_requested = False


def _on_signal(signum, _frame):
    del signum
    global _cancel_requested
    _cancel_requested = True


def install_signal_handlers() -> None:
    signal.signal(signal.SIGTERM, _on_signal)
    signal.signal(signal.SIGINT, _on_signal)


def check_cancelled() -> None:
    if _cancel_requested:
        raise SplitterCancelled("Operation cancelled.")


class ProgressReporter:
    def __init__(self, start: int = 0, end: int = 100):
        self.start = max(0, min(100, start))
        self.end = max(self.start, min(100, end))
        self.last = -1

    def update(self, done: int, total: int) -> None:
        check_cancelled()
        if total <= 0:
            pct = self.end
        else:
            fraction = max(0.0, min(1.0, float(done) / float(total)))
            pct = self.start + int(round((self.end - self.start) * fraction))
        if pct != self.last:
            self.last = pct
            print(f"PROGRESS {pct}", flush=True)

    def complete(self) -> None:
        if self.last != self.end:
            self.last = self.end
            print(f"PROGRESS {self.end}", flush=True)


def status(message: str) -> None:
    print(f"STATUS {message}", flush=True)


def human_size(value: int) -> str:
    amount = float(max(0, value))
    for unit in ("B", "KiB", "MiB", "GiB", "TiB", "PiB"):
        if amount < 1024.0 or unit == "PiB":
            return f"{amount:.2f} {unit}"
        amount /= 1024.0
    return str(value)


def sha256_file(path: Path, reporter: ProgressReporter | None = None,
                done_base: int = 0, total_all: int = 0) -> str:
    h = hashlib.sha256()
    local = 0
    with path.open("rb") as handle:
        while True:
            check_cancelled()
            block = handle.read(BLOCK)
            if not block:
                break
            h.update(block)
            local += len(block)
            if reporter is not None:
                reporter.update(done_base + local, total_all)
    return h.hexdigest()


def clean_name(value: str) -> str:
    value = value.strip().replace("/", "_").replace("\\", "_")
    if value.lower().endswith(".zip"):
        value = value[:-4].rstrip()
    if not value:
        raise SplitterError("Output name cannot be empty.")
    if value in {".", ".."}:
        raise SplitterError("Output name is invalid.")
    return value


def source_default_name(source: Path) -> str:
    name = source.name
    if source.is_file() and source.suffix.lower() == ".zip":
        name = source.stem
    elif source.is_file() and source.suffix:
        name = source.stem
    return clean_name(name or "Nougat Split")


def source_mode(source: Path) -> str:
    if source.is_dir():
        return "folder"
    if source.is_file() and source.suffix.lower() == ".zip":
        return "existing_zip"
    if source.is_file():
        return "file"
    raise SplitterError(f"Source does not exist: {source}")


def source_size(source: Path) -> int:
    source = source.expanduser().resolve()
    mode = source_mode(source)
    if mode != "folder":
        return source.stat().st_size
    total = 0
    status("Analyzing folder size...")
    for path in source.rglob("*"):
        check_cancelled()
        try:
            if path.is_file() and not path.is_symlink():
                total += path.stat().st_size
        except OSError:
            continue
    return total


def recommend_piece_count(size: int, target_piece_bytes: int = DEFAULT_TARGET_PIECE_BYTES) -> int:
    # NOUGAT_V59_EXACT_TARGET_FORMULA
    if size <= 0:
        return 1
    target = max(1, target_piece_bytes)
    return max(1, math.ceil(size / target))


def analyze(source: Path, target_piece_bytes: int = DEFAULT_TARGET_PIECE_BYTES) -> dict:
    # Analysis follows any positive owner-entered target. Split still blocks >476 MiB.
    if target_piece_bytes <= 0:
        raise SplitterError("Target piece size must be a positive value.")
    source = source.expanduser().resolve()
    mode = source_mode(source)
    size = source_size(source)
    # For files/folders that Nougat packages into ZIP first, source bytes are a
    # conservative estimate. Compression may make the final parts smaller.
    estimated = size
    pieces = recommend_piece_count(estimated, target_piece_bytes)
    approx = math.ceil(estimated / pieces) if pieces else 0
    result = {
        "mode": mode,
        "source_bytes": size,
        "estimated_payload_bytes": estimated,
        "recommended_pieces": pieces,
        "approx_piece_bytes": approx,
        "default_name": source_default_name(source),
        "target_piece_bytes": target_piece_bytes,
    }
    print(f"SOURCE_MODE {mode}", flush=True)
    print(f"SOURCE_BYTES {size}", flush=True)
    print(f"ESTIMATED_PAYLOAD_BYTES {estimated}", flush=True)
    print(f"SUGGESTED_PIECES {pieces}", flush=True)
    print(f"APPROX_PIECE_BYTES {approx}", flush=True)
    print(f"DEFAULT_NAME {result['default_name']}", flush=True)
    status(f"Suggested {pieces} piece{'s' if pieces != 1 else ''}, about {human_size(approx)} each.")
    return result


def zipinfo_for(path: Path, arcname: str) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(arcname)
    info.create_system = 3
    info.external_attr = (path.lstat().st_mode & 0xFFFF) << 16
    return info


def _write_file_to_zip(zf: zipfile.ZipFile, path: Path, arcname: str,
                       reporter: ProgressReporter, progress_state: list[int],
                       total_bytes: int) -> None:
    info = zipinfo_for(path, arcname)
    info.compress_type = zipfile.ZIP_DEFLATED
    with path.open("rb") as src, zf.open(info, "w", force_zip64=True) as out:
        while True:
            check_cancelled()
            block = src.read(BLOCK)
            if not block:
                break
            out.write(block)
            progress_state[0] += len(block)
            reporter.update(progress_state[0], total_bytes)


def package_folder(source: Path, output_zip: Path, reporter: ProgressReporter | None = None,
                   known_size: int | None = None) -> None:
    source = source.resolve()
    total = source_size(source) if known_size is None else known_size
    reporter = reporter or ProgressReporter(0, 100)
    done = [0]
    root_name = source.name
    with zipfile.ZipFile(output_zip, "w", compression=zipfile.ZIP_DEFLATED,
                         allowZip64=True, compresslevel=6) as zf:
        root_info = zipinfo_for(source, root_name + "/")
        zf.writestr(root_info, b"")
        for path in sorted(source.rglob("*"), key=lambda p: p.as_posix().lower()):
            check_cancelled()
            relative = path.relative_to(source).as_posix()
            arc = f"{root_name}/{relative}"
            if path.is_symlink():
                info = zipinfo_for(path, arc)
                info.external_attr = ((stat.S_IFLNK | (path.lstat().st_mode & 0o7777)) << 16)
                zf.writestr(info, os.readlink(path).encode("utf-8"))
            elif path.is_dir():
                zf.writestr(zipinfo_for(path, arc.rstrip("/") + "/"), b"")
            elif path.is_file():
                _write_file_to_zip(zf, path, arc, reporter, done, total)
    reporter.complete()


def package_file(source: Path, output_zip: Path, reporter: ProgressReporter | None = None) -> None:
    total = source.stat().st_size
    reporter = reporter or ProgressReporter(0, 100)
    done = [0]
    with zipfile.ZipFile(output_zip, "w", compression=zipfile.ZIP_DEFLATED,
                         allowZip64=True, compresslevel=6) as zf:
        _write_file_to_zip(zf, source.resolve(), source.name, reporter, done, total)
    reporter.complete()


def create_payload(source: Path, temp_dir: Path, base: str,
                   reporter: ProgressReporter | None = None):
    source = source.expanduser().resolve()
    mode = source_mode(source)
    if mode == "folder":
        payload = temp_dir / f"{base}.zip"
        status("Packaging folder into a streaming ZIP payload...")
        size = source_size(source)
        package_folder(source, payload, reporter, size)
        return payload, mode, source.name
    if mode == "existing_zip":
        status("Using existing ZIP directly...")
        if reporter is not None:
            reporter.complete()
        return source, mode, source.name
    payload = temp_dir / f"{base}.zip"
    status("Packaging file into a streaming ZIP payload...")
    package_file(source, payload, reporter)
    return payload, mode, source.name


def minimum_count(size: int, max_piece_bytes: int) -> int:
    if max_piece_bytes <= 0:
        return 1
    return max(1, math.ceil(size / max_piece_bytes))


def resolve_manifest_input(value: Path) -> Path:
    # NOUGAT_V59_FILE_ASSEMBLER_DISCOVERY
    candidate = value.expanduser().resolve()
    if candidate.name.lower().endswith(".zip.parts.json"):
        if not candidate.is_file():
            raise SplitterError(f"Manifest does not exist: {candidate}")
        return candidate

    match = re.match(r"^(?P<base>.+\.zip)\.(?P<index>[0-9]+)$", candidate.name, re.IGNORECASE)
    if not match:
        raise SplitterError("Choose a Nougat .zip.parts.json manifest or one numbered .zip.### part.")

    manifest = candidate.parent / f"{match.group('base')}.parts.json"
    if not manifest.is_file():
        raise SplitterError(
            f"Could not discover the matching parts manifest beside {candidate.name}: {manifest.name}"
        )

    data = json.loads(manifest.read_text(encoding="utf-8"))
    names = {str(part.get("name", "")) for part in data.get("parts", [])}
    if candidate.name not in names:
        raise SplitterError(f"The selected split part is not listed by {manifest.name}.")

    status(f"Discovered manifest from split part: {manifest.name}")
    return manifest


def load_manifest(manifest: Path) -> dict:
    data = json.loads(manifest.read_text(encoding="utf-8"))
    fmt = data.get("format")
    if fmt != FORMAT and fmt not in LEGACY_FORMATS:
        raise SplitterError("Unsupported Nougat split-ZIP manifest.")
    if not isinstance(data.get("parts"), list):
        raise SplitterError("Manifest is missing its part list.")
    return data


def verify(manifest: Path, reporter: ProgressReporter | None = None,
           quiet_final: bool = False) -> dict:
    manifest = resolve_manifest_input(manifest)
    data = load_manifest(manifest)
    total_expected = sum(max(0, int(part.get("size", 0))) for part in data["parts"])
    total = 0
    reporter = reporter or ProgressReporter(0, 100)
    for expected, part in enumerate(data["parts"], start=1):
        check_cancelled()
        name = str(part.get("name", ""))
        if Path(name).name != name or int(part.get("index", -1)) != expected:
            raise SplitterError("Manifest contains an invalid part entry.")
        path = manifest.parent / name
        if not path.is_file():
            raise SplitterError(f"Missing part: {path}")
        expected_size = int(part.get("size", -1))
        if path.stat().st_size != expected_size:
            raise SplitterError(f"Part size mismatch: {path.name}")
        actual_hash = sha256_file(path, reporter, total, total_expected)
        if actual_hash != str(part.get("sha256", "")):
            raise SplitterError(f"Part SHA-256 mismatch: {path.name}")
        total += expected_size
        reporter.update(total, total_expected)
    if total != int(data.get("payload_size", -1)):
        raise SplitterError("Combined part size does not match ZIP payload size.")
    reporter.complete()
    if not quiet_final:
        print("VERIFY PASS", flush=True)
    return data


def split(source: Path, output_dir: Path, output_name: str, pieces: int,
          max_piece_bytes: int = DEFAULT_TARGET_PIECE_BYTES) -> Path:
    if max_piece_bytes <= 0 or max_piece_bytes > MAX_UPLOAD_SAFE_PIECE_BYTES:
        raise SplitterError("Maximum piece size must be between 1 and 476 MiB so every part stays below 500 MB.")
    if pieces < 1:
        raise SplitterError("Piece count must be at least 1.")
    source = source.expanduser().resolve()
    output_dir = output_dir.expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    base = clean_name(output_name)
    created: list[Path] = []

    try:
        with tempfile.TemporaryDirectory(prefix="nougat-split-v54-") as temp_name:
            payload, mode, source_name = create_payload(
                source, Path(temp_name), base, ProgressReporter(0, 30))
            check_cancelled()
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

            status("Writing balanced split pieces...")
            split_reporter = ProgressReporter(30, 75)
            overall = hashlib.sha256()
            records = []
            remaining = size
            done_total = 0
            with payload.open("rb") as src:
                for index, final in enumerate(part_paths, start=1):
                    check_cancelled()
                    remaining_parts = pieces - index + 1
                    target = 0 if remaining <= 0 else math.ceil(remaining / remaining_parts)
                    tmp = final.with_suffix(final.suffix + ".tmp")
                    part_hash = hashlib.sha256()
                    written = 0
                    try:
                        with tmp.open("xb") as out:
                            left = target
                            while left > 0:
                                check_cancelled()
                                block = src.read(min(BLOCK, left))
                                if not block:
                                    break
                                out.write(block)
                                overall.update(block)
                                part_hash.update(block)
                                written += len(block)
                                left -= len(block)
                                done_total += len(block)
                                split_reporter.update(done_total, size)
                            out.flush()
                            os.fsync(out.fileno())
                        os.replace(tmp, final)
                        created.append(final)
                    except BaseException:
                        tmp.unlink(missing_ok=True)
                        raise
                    remaining -= written
                    records.append({
                        "index": index,
                        "name": final.name,
                        "size": written,
                        "sha256": part_hash.hexdigest(),
                    })
            split_reporter.complete()

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
            created.append(manifest)
            print(f"MANIFEST {manifest}", flush=True)

            status("Verifying every written piece with SHA-256...")
            verify(manifest, ProgressReporter(75, 100), quiet_final=True)
            print("VERIFY PASS", flush=True)
            status(f"Split complete: {pieces} verified pieces in {output_dir}")
            return manifest
    except BaseException:
        # Never leave a half-finished Nougat set that looks complete.
        for path in reversed(created):
            try:
                path.unlink(missing_ok=True)
            except OSError:
                pass
        raise


def join_zip(manifest: Path, output_zip: Path,
             reporter: ProgressReporter | None = None) -> dict:
    data = load_manifest(manifest)
    if output_zip.exists():
        raise SplitterError(f"Refusing to overwrite: {output_zip}")
    total = int(data.get("payload_size", 0))
    reporter = reporter or ProgressReporter(0, 100)
    h = hashlib.sha256()
    tmp = output_zip.with_suffix(output_zip.suffix + ".tmp")
    done = 0
    try:
        with tmp.open("xb") as out:
            for part in data["parts"]:
                check_cancelled()
                with (manifest.parent / part["name"]).open("rb") as src:
                    while True:
                        block = src.read(BLOCK)
                        if not block:
                            break
                        check_cancelled()
                        out.write(block)
                        h.update(block)
                        done += len(block)
                        reporter.update(done, total)
            out.flush()
            os.fsync(out.fileno())
        if h.hexdigest() != data["payload_sha256"]:
            raise SplitterError("Reassembled ZIP SHA-256 does not match the manifest.")
        os.replace(tmp, output_zip)
        reporter.complete()
        return data
    except BaseException:
        tmp.unlink(missing_ok=True)
        raise


def safe_member(name: str) -> PurePosixPath:
    p = PurePosixPath(name)
    if p.is_absolute() or ".." in p.parts:
        raise SplitterError(f"Unsafe ZIP path: {name}")
    return p


def extract_packaged(zip_path: Path, destination: Path,
                     reporter: ProgressReporter | None = None) -> None:
    reporter = reporter or ProgressReporter(0, 100)
    with zipfile.ZipFile(zip_path, "r") as zf:
        infos = zf.infolist()
        total = sum(max(0, info.file_size) for info in infos if not info.is_dir())
        done = 0
        for info in infos:
            check_cancelled()
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
                    while True:
                        block = src.read(BLOCK)
                        if not block:
                            break
                        check_cancelled()
                        out.write(block)
                        done += len(block)
                        reporter.update(done, total)
                if mode & 0o7777:
                    os.chmod(target, mode & 0o7777)
        reporter.complete()


def reassemble(manifest: Path, output: Path | None = None) -> Path:
    manifest = resolve_manifest_input(manifest)
    data = load_manifest(manifest)
    status("Verifying split pieces before reassembly...")
    verify(manifest, ProgressReporter(0, 30), quiet_final=True)

    if data["mode"] == "existing_zip":
        final = output.expanduser().resolve() if output else manifest.parent / f"{data['output_base']}.reassembled.zip"
        final.parent.mkdir(parents=True, exist_ok=True)
        status("Reassembling ZIP...")
        join_zip(manifest, final, ProgressReporter(30, 100))
        print(f"REASSEMBLED {final}", flush=True)
        print("VERIFY PASS", flush=True)
        status(f"Reassembly complete and verified: {final}")
        return final

    destination = output.expanduser().resolve() if output else manifest.parent
    destination.mkdir(parents=True, exist_ok=True)
    final = destination / data["source_name"]
    if final.exists() or final.is_symlink():
        raise SplitterError(f"Refusing to overwrite restored path: {final}")

    with tempfile.TemporaryDirectory(prefix="nougat-reassemble-v54-") as temp_name:
        joined = Path(temp_name) / f"{data['output_base']}.zip"
        status("Reassembling verified ZIP payload...")
        join_zip(manifest, joined, ProgressReporter(30, 65))
        extract_parent = Path(temp_name) / "restore"
        extract_parent.mkdir()
        status("Restoring original file/folder...")
        extract_packaged(joined, extract_parent, ProgressReporter(65, 100))
        restored_tmp = extract_parent / data["source_name"]
        if not restored_tmp.exists() and not restored_tmp.is_symlink():
            raise SplitterError("Expected original folder/file was not restored.")
        shutil.move(str(restored_tmp), str(final))

    print(f"REASSEMBLED {final}", flush=True)
    print("VERIFY PASS", flush=True)
    status(f"Reassembly complete and verified: {final}")
    return final


def main() -> int:
    install_signal_handlers()
    parser = argparse.ArgumentParser(description="Nougat Media Suite professional File Splitter worker")
    sub = parser.add_subparsers(dest="command", required=True)

    a = sub.add_parser("analyze")
    a.add_argument("source", type=Path)
    a.add_argument("--target-piece-mib", type=int, default=450)

    s = sub.add_parser("split")
    s.add_argument("source", type=Path)
    s.add_argument("output_dir", type=Path)
    s.add_argument("--name", required=True)
    s.add_argument("--pieces", type=int, required=True)
    s.add_argument("--max-piece-mib", type=int, default=450)

    v = sub.add_parser("verify")
    v.add_argument("manifest", type=Path)

    r = sub.add_parser("reassemble")
    r.add_argument("manifest", type=Path)
    r.add_argument("--output", type=Path)

    args = parser.parse_args()
    if args.command == "analyze":
        target = max(64, args.target_piece_mib) * 1024 * 1024
        analyze(args.source, target)
    elif args.command == "split":
        split(args.source, args.output_dir, args.name, args.pieces,
              max(0, args.max_piece_mib) * 1024 * 1024)
    elif args.command == "verify":
        status("Verifying every split piece with SHA-256...")
        verify(args.manifest)
        status("All split pieces passed SHA-256 verification.")
    elif args.command == "reassemble":
        reassemble(args.manifest, args.output)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except PieceCountTooSmall as exc:
        print(f"SUGGESTED_PIECES {exc.minimum}", flush=True)
        print(f"ERROR {exc}", flush=True)
        raise SystemExit(2)
    except SplitterCancelled as exc:
        print(f"CANCELLED {exc}", flush=True)
        raise SystemExit(130)
    except (SplitterError, ValueError, json.JSONDecodeError, zipfile.BadZipFile, OSError) as exc:
        print(f"ERROR {exc}", flush=True)
        raise SystemExit(1)
