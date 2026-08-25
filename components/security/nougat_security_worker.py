#!/usr/bin/env python3
"""Nougat Security Analysis one-shot worker.

The worker intentionally has no daemon mode. It starts for one file/folder scan,
prints a human-readable report, then exits. It never moves, deletes, quarantines,
renames, or opens the scanned file.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import mimetypes
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Any

RUNTIME_ROOT = Path(__file__).resolve().parent / "runtime"
BUILTIN_RULES = Path(__file__).resolve().parent / "rules"
USER_CONFIG = Path.home() / ".config" / "nougat-media-suite" / "security"
USER_RULES = USER_CONFIG / "rules"
AUTH_KEY = USER_CONFIG / "abusech.key"
HISTORY = USER_CONFIG / "scan_history.jsonl"
CAPA_RULES = RUNTIME_ROOT / "capa-rules-v9.4.0"

EXECUTABLE_EXTS = {".exe", ".dll", ".sys", ".scr", ".com", ".msi", ".elf", ".so", ".bin"}
MEDIA_EXTS = {".mkv", ".mp4", ".avi", ".mov", ".webm", ".mp3", ".flac", ".wav", ".m4a", ".ts", ".m2ts"}
SCRIPT_EXTS = {".sh", ".bash", ".ps1", ".bat", ".cmd", ".vbs", ".js", ".py", ".pl", ".rb"}


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def human_size(n: int) -> str:
    value = float(max(0, n))
    units = ["B", "KiB", "MiB", "GiB", "TiB"]
    i = 0
    while value >= 1024.0 and i < len(units) - 1:
        value /= 1024.0
        i += 1
    return f"{value:.0f} {units[i]}" if i == 0 else f"{value:.1f} {units[i]}"


def fallback_file_type(path: Path) -> dict[str, Any]:
    out = {"engine": "file/mimetypes", "label": "unknown", "description": "Unknown", "mime": "application/octet-stream", "group": "unknown", "extensions": []}
    file_cmd = shutil.which("file")
    if file_cmd:
        try:
            desc = subprocess.check_output([file_cmd, "-b", str(path)], text=True, stderr=subprocess.DEVNULL, timeout=15).strip()
            mime = subprocess.check_output([file_cmd, "-b", "--mime-type", str(path)], text=True, stderr=subprocess.DEVNULL, timeout=15).strip()
            out.update(description=desc or "Unknown", mime=mime or out["mime"])
            low = desc.lower()
            if "elf" in low: out["label"] = "elf"
            elif "pe32" in low or "ms-dos executable" in low: out["label"] = "pebin"
            elif "matroska" in low: out["label"] = "mkv"
            elif "mpeg" in low and "video" in low: out["label"] = "mpeg_video"
            elif "zip archive" in low: out["label"] = "zip"
            elif "python" in low: out["label"] = "python"
            elif "shell script" in low: out["label"] = "shell"
        except Exception:
            pass
    guessed, _ = mimetypes.guess_type(str(path))
    if guessed and out["mime"] == "application/octet-stream":
        out["mime"] = guessed
    return out


def magika_type(path: Path) -> dict[str, Any]:
    try:
        from magika import Magika  # type: ignore
        result = Magika().identify_path(path)
        o = result.output
        return {
            "engine": "Magika",
            "label": str(o.label),
            "description": str(o.description),
            "mime": str(o.mime_type),
            "group": str(o.group),
            "extensions": [str(x).lower().lstrip(".") for x in (o.extensions or [])],
            "score": float(getattr(result, "score", 0.0) or 0.0),
        }
    except Exception as exc:
        data = fallback_file_type(path)
        data["magika_error"] = str(exc)
        return data


def extension_findings(path: Path, ftype: dict[str, Any]) -> list[str]:
    findings: list[str] = []
    suffixes = [s.lower() for s in path.suffixes]
    ext = path.suffix.lower()
    allowed = {"." + x for x in ftype.get("extensions", []) if x}
    label = str(ftype.get("label", "")).lower()
    desc = str(ftype.get("description", "")).lower()
    executable = label in {"pebin", "elf", "dotnet", "msi"} or "executable" in desc or "shared object" in desc
    if executable and ext not in EXECUTABLE_EXTS:
        findings.append(f"Extension/type mismatch: {ext or '(none)'} hides executable content")
    if len(suffixes) >= 2 and suffixes[-1] in EXECUTABLE_EXTS and suffixes[-2] in MEDIA_EXTS:
        findings.append(f"Suspicious double extension: {''.join(suffixes[-2:])}")
    if allowed and ext and ext not in allowed and ext not in EXECUTABLE_EXTS:
        # Informational mismatch. Keep conservative: only flag when claimed media conflicts with detected executable/code/archive.
        if ext in MEDIA_EXTS and (executable or label in {"python", "javascript", "shell", "zip", "rar", "7zip"}):
            findings.append(f"Media extension {ext} does not match detected {ftype.get('description', label)}")
    return findings


def load_yara_sources() -> tuple[list[tuple[str, str]], list[str]]:
    sources: list[tuple[str, str]] = []
    errors: list[str] = []
    for base, prefix in [(BUILTIN_RULES, "builtin"), (USER_RULES, "user")]:
        if not base.is_dir():
            continue
        for p in sorted(base.rglob("*.yar")) + sorted(base.rglob("*.yara")):
            try:
                sources.append((f"{prefix}_{p.stem}", p.read_text(encoding="utf-8", errors="replace")))
            except Exception as exc:
                errors.append(f"Could not read rule {p.name}: {exc}")
    return sources, errors


def yara_scan(path: Path) -> dict[str, Any]:
    result: dict[str, Any] = {"engine": "YARA-X", "available": False, "matches": [], "errors": []}
    try:
        import yara_x  # type: ignore
        result["available"] = True
        compiler = yara_x.Compiler()
        sources, errors = load_yara_sources()
        result["errors"].extend(errors)
        if not sources:
            result["errors"].append("No YARA-X rules are installed")
            return result
        for namespace, source in sources:
            try:
                compiler.new_namespace(namespace)
                compiler.add_source(source)
            except Exception as exc:
                result["errors"].append(f"Rule compile error in {namespace}: {exc}")
        rules = compiler.build()
        scanner = yara_x.Scanner(rules)
        scanner.set_timeout(120)
        scanned = scanner.scan_file(str(path))
        result["matches"] = [r.identifier for r in scanned.matching_rules]
    except Exception as exc:
        result["errors"].append(str(exc))
    return result


def looks_capa_compatible(ftype: dict[str, Any], path: Path) -> bool:
    label = str(ftype.get("label", "")).lower()
    desc = str(ftype.get("description", "")).lower()
    return label in {"pebin", "elf", "dotnet", "msi"} or path.suffix.lower() in EXECUTABLE_EXTS or "executable" in desc or "shared object" in desc


def capa_scan(path: Path, ftype: dict[str, Any]) -> dict[str, Any]:
    result: dict[str, Any] = {"engine": "capa", "applicable": looks_capa_compatible(ftype, path), "available": False, "summary": [], "error": ""}
    if not result["applicable"]:
        return result
    candidates = [RUNTIME_ROOT / "venv" / "bin" / "capa"]
    if shutil.which("capa"):
        candidates.append(Path(shutil.which("capa") or ""))
    capa = next((p for p in candidates if p and p.is_file() and os.access(p, os.X_OK)), None)
    if capa is None:
        result["error"] = "capa runtime is not installed"
        return result
    if not CAPA_RULES.is_dir():
        result["error"] = "capa v9.4.0 rules are not installed"
        return result
    result["available"] = True
    try:
        proc = subprocess.run([str(capa), "-r", str(CAPA_RULES), str(path)], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
        lines = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
        # Keep a compact top-level capability summary; full capa output is not needed in the normal UI.
        useful: list[str] = []
        for line in lines:
            low = line.lower()
            if low.startswith("warning:") or set(line) <= {"-", "="}:
                continue
            if line.startswith("ATT&CK") or line.startswith("MAEC") or line.startswith("MBC"):
                continue
            useful.append(line)
            if len(useful) >= 18:
                break
        result["summary"] = useful
        if proc.returncode not in (0,):
            result["error"] = f"capa exited with status {proc.returncode}"
    except subprocess.TimeoutExpired:
        result["error"] = "capa analysis timed out after 180 seconds"
    except Exception as exc:
        result["error"] = str(exc)
    return result


def clamav_scan(path: Path) -> dict[str, Any]:
    result: dict[str, Any] = {"engine": "ClamAV", "available": False, "detected": False, "message": "Not installed"}
    clamscan = shutil.which("clamscan")
    if not clamscan:
        return result
    result["available"] = True
    try:
        proc = subprocess.run([clamscan, "--no-summary", "--stdout", str(path)], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=300)
        text = proc.stdout.strip()
        result["detected"] = proc.returncode == 1
        result["message"] = text or ("No detection" if proc.returncode == 0 else f"Scanner error {proc.returncode}")
    except subprocess.TimeoutExpired:
        result["message"] = "ClamAV scan timed out"
    except Exception as exc:
        result["message"] = str(exc)
    return result


def _post(url: str, key: str, body: bytes, content_type: str) -> dict[str, Any]:
    req = urllib.request.Request(url, data=body, method="POST", headers={"Auth-Key": key, "Content-Type": content_type, "User-Agent": "Nougat-Media-Suite/0.0.33"})
    with urllib.request.urlopen(req, timeout=12) as resp:
        return json.loads(resp.read().decode("utf-8", "replace"))


def community_telemetry(sha256: str) -> dict[str, Any]:
    out: dict[str, Any] = {
        "configured": False,
        "malwarebazaar": "Community telemetry not configured",
        "threatfox": "Community telemetry not configured",
        "urlhaus": "Community telemetry not configured",
        "known_malicious": False,
    }
    try:
        key = AUTH_KEY.read_text(encoding="utf-8").strip()
    except Exception:
        key = ""
    if not key:
        return out
    out["configured"] = True
    try:
        form = urllib.parse.urlencode({"query": "get_info", "hash": sha256}).encode()
        data = _post("https://mb-api.abuse.ch/api/v1/", key, form, "application/x-www-form-urlencoded")
        status = str(data.get("query_status", "unknown"))
        if status == "ok" and data.get("data"):
            item = data["data"][0]
            signature = item.get("signature") or "known malware"
            out["malwarebazaar"] = f"KNOWN: {signature}"
            out["known_malicious"] = True
        elif status in {"hash_not_found", "file_not_found", "no_results"}:
            out["malwarebazaar"] = "No known hash match"
        else:
            out["malwarebazaar"] = f"Lookup: {status}"
    except Exception as exc:
        out["malwarebazaar"] = f"Lookup unavailable: {exc.__class__.__name__}"
    try:
        body = json.dumps({"query": "search_ioc", "search_term": sha256}).encode()
        data = _post("https://threatfox-api.abuse.ch/api/v1/", key, body, "application/json")
        status = str(data.get("query_status", "unknown"))
        if status == "ok" and data.get("data"):
            families = sorted({str(x.get("malware_printable") or x.get("malware") or "malware") for x in data["data"]})
            out["threatfox"] = "KNOWN IOC: " + ", ".join(families[:4])
            out["known_malicious"] = True
        elif status in {"no_result", "no_results"}:
            out["threatfox"] = "No known IOC match"
        else:
            out["threatfox"] = f"Lookup: {status}"
    except Exception as exc:
        out["threatfox"] = f"Lookup unavailable: {exc.__class__.__name__}"
    try:
        form = urllib.parse.urlencode({"sha256_hash": sha256}).encode()
        data = _post("https://urlhaus-api.abuse.ch/v1/payload/", key, form, "application/x-www-form-urlencoded")
        status = str(data.get("query_status", "unknown"))
        if status == "ok":
            signature = data.get("signature") or data.get("file_type") or "known URLhaus payload"
            out["urlhaus"] = f"KNOWN PAYLOAD: {signature}"
            out["known_malicious"] = True
        elif status in {"no_results", "hash_not_found", "invalid_hash"}:
            out["urlhaus"] = "No known payload hash match"
        else:
            out["urlhaus"] = f"Lookup: {status}"
    except Exception as exc:
        out["urlhaus"] = f"Lookup unavailable: {exc.__class__.__name__}"
    return out



def record_history(r: dict[str, Any]) -> None:
    try:
        USER_CONFIG.mkdir(parents=True, exist_ok=True)
        entry = {
            "path": r.get("path", ""),
            "sha256": r.get("sha256", ""),
            "verdict": r.get("verdict", ""),
            "type": r.get("type", {}).get("description", "Unknown"),
            "size": r.get("size", 0),
            "mtime_ns": r.get("mtime_ns", 0),
        }
        with HISTORY.open("a", encoding="utf-8") as f:
            f.write(json.dumps(entry, ensure_ascii=False) + "\n")
        os.chmod(HISTORY, 0o600)
    except Exception:
        pass


def already_scanned_unchanged(path: Path) -> dict[str, Any] | None:
    """Return the newest matching history record when size+mtime prove the file is unchanged.

    This is used only by automatic completed-download scans so restarting Nougat does not
    repeatedly run heavyweight analysis against the same completed file. Manual Scan Again
    always performs a fresh scan.
    """
    try:
        st = path.stat()
        lines = HISTORY.read_text(encoding="utf-8", errors="replace").splitlines()
        resolved = str(path.resolve())
        for line in reversed(lines):
            try:
                item = json.loads(line)
            except Exception:
                continue
            if item.get("path") != resolved:
                continue
            if int(item.get("size", -1)) == st.st_size and int(item.get("mtime_ns", -1)) == st.st_mtime_ns:
                return item
        return None
    except Exception:
        return None


def render_history(limit: int = 30) -> str:
    lines = ["VERDICT=HISTORY", "NOUGAT SECURITY ANALYSIS — RECENT SCANS", ""]
    try:
        raw = HISTORY.read_text(encoding="utf-8", errors="replace").splitlines()[-max(1, limit):]
        if not raw:
            lines.append("No scan history yet.")
        for line in reversed(raw):
            try:
                item = json.loads(line)
                lines.append(f"{item.get('verdict','UNKNOWN')}: {item.get('path','')}  [{str(item.get('sha256',''))[:12]}]")
            except Exception:
                continue
    except FileNotFoundError:
        lines.append("No scan history yet.")
    except Exception as exc:
        lines.append(f"History unavailable: {exc}")
    return "\n".join(lines)

def scan_one(path: Path, online: bool = True) -> dict[str, Any]:
    stat = path.stat()
    digest = sha256_file(path)
    ftype = magika_type(path)
    ext_findings = extension_findings(path, ftype)
    yara = yara_scan(path)
    capa = capa_scan(path, ftype)
    clam = clamav_scan(path)
    telemetry = community_telemetry(digest) if online else {"configured": False, "malwarebazaar": "Skipped", "threatfox": "Skipped", "urlhaus": "Skipped", "known_malicious": False}

    threat = bool(yara.get("matches")) or bool(clam.get("detected")) or bool(telemetry.get("known_malicious"))
    suspicious = bool(ext_findings)

    # Truthful verdict gate. "No threats detected" means every required/relevant
    # Nougat analysis lane actually ran. An unavailable engine is not a clean
    # result. ClamAV remains an optional second opinion and does not gate the
    # verdict; configured online threat intelligence does gate an online scan.
    incomplete_reasons: list[str] = []
    if ftype.get("engine") != "Magika":
        incomplete_reasons.append("Magika file-type runtime unavailable")
    if not yara.get("available"):
        incomplete_reasons.append("YARA-X runtime unavailable")
    elif yara.get("errors"):
        incomplete_reasons.append("YARA-X did not complete cleanly")
    if capa.get("applicable") and (not capa.get("available") or capa.get("error")):
        incomplete_reasons.append("capa executable analysis unavailable or incomplete")
    if online and not telemetry.get("configured"):
        incomplete_reasons.append("Threat intelligence key not configured")
    elif online and (str(telemetry.get("malwarebazaar", "")).startswith("Lookup unavailable") or
                     str(telemetry.get("threatfox", "")).startswith("Lookup unavailable") or
                     str(telemetry.get("urlhaus", "")).startswith("Lookup unavailable")):
        incomplete_reasons.append("Threat intelligence lookup incomplete")

    if threat:
        verdict = "THREAT DETECTED"
    elif suspicious:
        verdict = "SUSPICIOUS"
    elif incomplete_reasons:
        verdict = "ANALYSIS INCOMPLETE"
    else:
        verdict = "NO THREATS DETECTED"
    result = {
        "path": str(path), "size": stat.st_size, "mtime_ns": stat.st_mtime_ns, "sha256": digest, "type": ftype,
        "extension_findings": ext_findings, "yara": yara, "capa": capa, "clamav": clam,
        "telemetry": telemetry, "verdict": verdict, "incomplete_reasons": incomplete_reasons,
    }
    record_history(result)
    return result


def render_one(r: dict[str, Any], compact: bool = False) -> str:
    ftype = r["type"]
    if compact:
        return f"{r['verdict']}: {r['path']}"
    lines = [
        f"VERDICT={r['verdict']}",
        "NOUGAT SECURITY ANALYSIS",
        "",
        f"File: {r['path']}",
        f"Size: {human_size(r['size'])}",
        f"SHA-256: {r['sha256']}",
        f"Detected type: {ftype.get('description', ftype.get('label', 'Unknown'))}",
        f"Type engine: {ftype.get('engine', 'Unknown')}",
        f"MIME: {ftype.get('mime', 'Unknown')}",
    ]
    if r["extension_findings"]:
        lines.append("Extension checks: " + "; ".join(r["extension_findings"]))
    else:
        lines.append("Extension checks: No suspicious mismatch detected")
    y = r["yara"]
    if y.get("available"):
        lines.append("YARA-X: " + (", ".join(y.get("matches", [])) if y.get("matches") else "No rule matches"))
        if y.get("errors"):
            lines.append("YARA-X notes: " + "; ".join(y["errors"][:3]))
    else:
        lines.append("YARA-X: Runtime not installed")
    c = r["capa"]
    if not c.get("applicable"):
        lines.append("capa: Not applicable to this file type")
    elif not c.get("available"):
        lines.append("capa: " + (c.get("error") or "Runtime not installed"))
    else:
        lines.append("capa: Capability analysis completed")
        for entry in c.get("summary", [])[:8]:
            lines.append("  • " + entry)
        if c.get("error"):
            lines.append("capa note: " + c["error"])
    cl = r["clamav"]
    lines.append("ClamAV: " + (cl.get("message") or "No detection"))
    t = r["telemetry"]
    lines.append("MalwareBazaar: " + str(t.get("malwarebazaar")))
    lines.append("ThreatFox: " + str(t.get("threatfox")))
    lines.append("URLhaus: " + str(t.get("urlhaus")))
    if r.get("incomplete_reasons"):
        lines.append("")
        lines.append("Analysis completeness:")
        for reason in r["incomplete_reasons"]:
            lines.append("  • " + str(reason))
        lines.append("No suspicious indicators were found by the checks that ran, but this file has NOT received a complete security analysis.")
    lines.extend(["", "RESULT: " + r["verdict"], "Policy: WARN ME FIRST. Nougat did not move, delete, quarantine, rename, or open this file."])
    return "\n".join(lines)



NETWORK_FS_TYPES = {
    "nfs", "nfs4", "cifs", "smb3", "sshfs", "fuse.sshfs", "9p",
    "ceph", "glusterfs", "davfs", "fuse.rclone",
}
PSEUDO_ROOTS = (Path("/proc"), Path("/sys"), Path("/dev"), Path("/run"))
MAPPING_REGISTRY = Path.home() / ".config" / "reddmedia" / "server" / "library_mappings.tsv"


def _decode_mount_path(value: str) -> str:
    return (value.replace("\\040", " ").replace("\\011", "\\t")
                 .replace("\\012", "\\n").replace("\\134", "\\"))


def network_mount_points() -> set[Path]:
    mounts: set[Path] = set()
    try:
        for line in Path("/proc/self/mountinfo").read_text(encoding="utf-8", errors="replace").splitlines():
            fields = line.split()
            if "-" not in fields or len(fields) < 10:
                continue
            dash = fields.index("-")
            if dash + 1 >= len(fields):
                continue
            fs_type = fields[dash + 1].lower()
            if fs_type in NETWORK_FS_TYPES:
                mounts.add(Path(_decode_mount_path(fields[4])))
    except Exception:
        pass
    return mounts


def _is_under(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def _skip_path(path: Path, network_mounts: set[Path], allow_run_media: bool = False) -> bool:
    absolute = path if path.is_absolute() else path.resolve(strict=False)
    for pseudo in PSEUDO_ROOTS:
        if allow_run_media and pseudo == Path("/run") and _is_under(absolute, Path("/run/media")):
            continue
        if absolute == pseudo or _is_under(absolute, pseudo):
            return True
    for mount in network_mounts:
        if absolute == mount or _is_under(absolute, mount):
            return True
    return False


def iter_regular_files(roots: list[Path], *, recent_after: float | None = None,
                       allow_run_media: bool = False) -> tuple[list[Path], list[str]]:
    """Enumerate scan targets without following symlinks or entering unsafe mounts."""
    found: list[Path] = []
    notes: list[str] = []
    network_mounts = network_mount_points()
    seen: set[str] = set()
    for original in roots:
        root = original.expanduser()
        try:
            root = root.resolve(strict=False)
        except Exception:
            pass
        key = str(root)
        if key in seen:
            continue
        seen.add(key)
        if not root.exists():
            notes.append(f"Unavailable path skipped: {root}")
            continue
        if root.is_symlink() or _skip_path(root, network_mounts, allow_run_media=allow_run_media):
            notes.append(f"Unsafe/remote path skipped: {root}")
            continue
        if root.is_file():
            try:
                if recent_after is None or root.stat().st_mtime >= recent_after:
                    found.append(root)
            except (OSError, PermissionError) as exc:
                notes.append(f"Could not inspect {root}: {exc}")
            continue
        if not root.is_dir():
            continue

        def onerror(exc: OSError) -> None:
            name = getattr(exc, "filename", None) or str(root)
            notes.append(f"Could not read {name}: {exc.strerror or exc}")

        for dirpath, dirnames, filenames in os.walk(root, topdown=True, followlinks=False, onerror=onerror):
            current = Path(dirpath)
            kept: list[str] = []
            for name in dirnames:
                child = current / name
                try:
                    if child.is_symlink() or _skip_path(child, network_mounts, allow_run_media=allow_run_media):
                        continue
                except OSError:
                    continue
                kept.append(name)
            dirnames[:] = kept
            for name in filenames:
                candidate = current / name
                try:
                    if candidate.is_symlink() or not candidate.is_file():
                        continue
                    if recent_after is not None and candidate.stat().st_mtime < recent_after:
                        continue
                    found.append(candidate)
                except (OSError, PermissionError) as exc:
                    notes.append(f"Could not inspect {candidate}: {exc}")
    return found, notes


def load_mapped_library_roots(kind: str) -> list[Path]:
    wanted = kind.strip().lower()
    roots: list[Path] = []
    try:
        with MAPPING_REGISTRY.open("r", encoding="utf-8", newline="") as handle:
            for row in csv.reader(handle, delimiter="\t", quotechar='"'):
                if len(row) >= 2 and row[0].strip().lower() == wanted and row[1].strip():
                    roots.append(Path(row[1].strip()).expanduser())
    except FileNotFoundError:
        return []
    except Exception:
        return []
    return roots


def emit_progress(path: Path, scanned: int, total: int, threats: int, suspicious: int, started: float) -> None:
    elapsed_ms = int((time.monotonic() - started) * 1000.0)
    clean_path = str(path).replace("\n", " ").replace("\r", " ")
    print(f"PROGRESS={scanned}|{total}|{threats}|{suspicious}|{elapsed_ms}|{clean_path}", flush=True)


def bulk_clamav_scan(files: list[Path]) -> dict[str, dict[str, Any]]:
    # One ClamAV process handles the collection instead of one process per file.
    results: dict[str, dict[str, Any]] = {}
    clamscan = shutil.which("clamscan")
    if not clamscan or not files:
        return results
    list_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", prefix="nougat-clam-", suffix=".txt", delete=False) as handle:
            list_path = Path(handle.name)
            for path in files:
                handle.write(str(path) + "\n")
        proc = subprocess.run(
            [clamscan, "--no-summary", "--stdout", f"--file-list={list_path}"],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for line in proc.stdout.splitlines():
            path_text, sep, status = line.rpartition(": ")
            if not sep:
                continue
            results[path_text] = {
                "engine": "ClamAV", "available": True,
                "detected": status.endswith("FOUND"), "message": status,
            }
    except Exception as exc:
        results["__error__"] = {
            "engine": "ClamAV", "available": False, "detected": False,
            "message": f"Bulk ClamAV pass unavailable: {exc}",
        }
    finally:
        if list_path is not None:
            try:
                list_path.unlink()
            except OSError:
                pass
    return results


def scan_one_bulk(path: Path, online: bool, clam_result: dict[str, Any]) -> dict[str, Any]:
    stat = path.stat()
    ftype = magika_type(path)
    ext_findings = extension_findings(path, ftype)
    yara = yara_scan(path)
    clam = clam_result or {
        "engine": "ClamAV", "available": False, "detected": False,
        "message": "Bulk engine unavailable",
    }

    local_threat = bool(yara.get("matches")) or bool(clam.get("detected"))
    suspicious = bool(ext_findings)
    deep = local_threat or suspicious

    digest = ""
    capa: dict[str, Any] = {
        "engine": "capa", "applicable": looks_capa_compatible(ftype, path),
        "available": False, "summary": [], "error": "Skipped by bulk fast-pass policy",
    }
    telemetry: dict[str, Any] = {
        "configured": False,
        "malwarebazaar": "Skipped by bulk fast-pass policy",
        "threatfox": "Skipped by bulk fast-pass policy",
        "urlhaus": "Skipped by bulk fast-pass policy",
        "known_malicious": False,
    }
    if deep:
        digest = sha256_file(path)
        capa = capa_scan(path, ftype)
        if online:
            telemetry = community_telemetry(digest)

    threat = local_threat or bool(telemetry.get("known_malicious"))
    incomplete_reasons: list[str] = []
    if ftype.get("engine") != "Magika":
        incomplete_reasons.append("Magika file-type runtime unavailable")
    if not yara.get("available"):
        incomplete_reasons.append("YARA-X runtime unavailable")
    elif yara.get("errors"):
        incomplete_reasons.append("YARA-X did not complete cleanly")
    if deep and capa.get("applicable") and (not capa.get("available") or capa.get("error")):
        incomplete_reasons.append("capa deep analysis unavailable or incomplete")

    if threat:
        verdict = "THREAT DETECTED"
    elif suspicious:
        verdict = "SUSPICIOUS"
    elif incomplete_reasons:
        verdict = "ANALYSIS INCOMPLETE"
    else:
        verdict = "NO THREATS DETECTED"

    result = {
        "path": str(path), "size": stat.st_size, "mtime_ns": stat.st_mtime_ns,
        "sha256": digest or "(not computed in ordinary bulk fast pass)",
        "type": ftype, "extension_findings": ext_findings, "yara": yara,
        "capa": capa, "clamav": clam, "telemetry": telemetry,
        "verdict": verdict, "incomplete_reasons": incomplete_reasons,
    }
    if deep:
        record_history(result)
    return result



def scan_collection(roots: list[Path], label: str, online: bool, *,
                    recent_after: float | None = None, allow_run_media: bool = False) -> str:
    started = time.monotonic()
    files, notes = iter_regular_files(roots, recent_after=recent_after, allow_run_media=allow_run_media)
    lines = [
        "VERDICT=NO THREATS DETECTED",
        f"NOUGAT SECURITY ANALYSIS — {label}",
        "",
        "Roots: " + (", ".join(str(p) for p in roots) if roots else "(none)"),
        f"Files discovered: {len(files)}",
        "Bulk policy: one ClamAV collection pass; broad fast checks; capa/reputation only after an indicator justifies deep analysis.",
        "",
    ]

    clam_results = bulk_clamav_scan(files)
    worst = detections = suspicious = incomplete = errors = 0
    details: list[str] = []
    sample_lines: list[str] = []

    for i, path in enumerate(files, 1):
        try:
            clam = clam_results.get(str(path), clam_results.get("__error__", {
                "engine": "ClamAV", "available": False, "detected": False,
                "message": "No bulk result",
            }))
            result = scan_one_bulk(path, online=online, clam_result=clam)
            severity = {"NO THREATS DETECTED": 0, "ANALYSIS INCOMPLETE": 1,
                        "SUSPICIOUS": 2, "THREAT DETECTED": 3}.get(result["verdict"], 1)
            worst = max(worst, severity)
            if result["verdict"] == "THREAT DETECTED":
                detections += 1
            elif result["verdict"] == "SUSPICIOUS":
                suspicious += 1
            elif result["verdict"] == "ANALYSIS INCOMPLETE":
                incomplete += 1
            if len(sample_lines) < 180 or severity >= 2:
                sample_lines.append(f"[{i}/{len(files)}] {render_one(result, compact=True)}")
            if severity >= 2 and len(details) < 40:
                details.append(render_one(result))
        except (OSError, PermissionError) as exc:
            worst = max(worst, 1)
            errors += 1
            if len(sample_lines) < 180:
                sample_lines.append(f"[{i}/{len(files)}] UNREADABLE: {path}: {exc}")
        except Exception as exc:
            worst = max(worst, 1)
            errors += 1
            if len(sample_lines) < 180:
                sample_lines.append(f"[{i}/{len(files)}] ERROR: {path}: {exc}")
        emit_progress(path, i, len(files), detections, suspicious, started)

    verdict = ["NO THREATS DETECTED", "ANALYSIS INCOMPLETE", "SUSPICIOUS", "THREAT DETECTED"][worst]
    lines[0] = "VERDICT=" + verdict
    elapsed = time.monotonic() - started
    lines.extend([
        f"Files scanned: {len(files)}",
        f"Threat detections: {detections}",
        f"Suspicious files: {suspicious}",
        f"Incomplete analyses: {incomplete}",
        f"Unreadable/error files: {errors}",
        f"Elapsed: {elapsed:.1f} seconds",
        "",
    ])
    if notes:
        lines.append("Traversal notes:")
        for note in notes[:60]:
            lines.append("  • " + note)
        if len(notes) > 60:
            lines.append(f"  • ... {len(notes) - 60} additional traversal note(s) omitted")
        lines.append("")
    lines.extend(sample_lines)
    if len(files) > len(sample_lines):
        lines.append(f"... {len(files) - len(sample_lines)} additional clean/ordinary file result(s) omitted from the on-screen report")
    if details:
        lines.extend(["", "FLAGGED FILE DETAILS", "====================", "", "\n\n".join(details)])
    lines.extend(["", "RESULT: " + verdict,
                  "Policy: WARN ME FIRST. Nougat did not move, delete, quarantine, rename, or open any scanned file."])
    return "\n".join(lines)


def scan_folder(folder: Path, online: bool) -> str:
    return scan_collection([folder], "FOLDER", online)


def quick_scan_roots() -> list[Path]:
    home = Path.home()
    return [home / "Downloads", home / "Desktop", home / ".local" / "bin",
            home / ".config" / "autostart", Path("/tmp")]


def system_scan_roots(profile: str) -> tuple[list[Path], float | None, bool, str]:
    home = Path.home()
    p = profile.lower()
    if p == "full":
        return [Path("/")], None, False, "FULL SYSTEM"
    if p == "critical":
        return [Path("/bin"), Path("/sbin"), Path("/usr/bin"), Path("/usr/sbin"),
                Path("/usr/local/bin"), Path("/usr/local/sbin"), Path("/etc")], None, False, "CRITICAL SYSTEM AREAS"
    if p == "startup":
        roots = [home / ".config" / "autostart", home / ".config" / "systemd" / "user",
                 Path("/etc/xdg/autostart"), Path("/etc/systemd/system"), Path("/usr/lib/systemd/system"),
                 home / ".bashrc", home / ".profile", home / ".xprofile", home / ".xsessionrc"]
        return roots, None, False, "STARTUP LOCATIONS"
    if p == "downloads":
        return [home / "Downloads"], None, False, "DOWNLOADS"
    if p == "removable":
        user = os.environ.get("USER", "")
        roots = [Path("/media") / user, Path("/run/media") / user, Path("/mnt")]
        return roots, None, True, "REMOVABLE DRIVES"
    if p == "changed":
        recent_after = time.time() - 7 * 24 * 60 * 60
        roots = [home, Path("/etc"), Path("/usr/local/bin"), Path("/usr/local/sbin")]
        return roots, recent_after, False, "NEW/CHANGED FILES (7 DAYS)"
    raise ValueError(f"Unknown system scan profile: {profile}")


def main() -> int:
    ap = argparse.ArgumentParser()
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--file")
    mode.add_argument("--auto-file", help="Scan a completed Nougat download unless unchanged scan history proves it was already scanned")
    mode.add_argument("--folder")
    mode.add_argument("--mapped-library", choices=("movies", "tv"))
    mode.add_argument("--quick-scan", action="store_true")
    mode.add_argument("--system-scan", choices=("full", "critical", "startup", "downloads", "removable", "changed"))
    mode.add_argument("--history", action="store_true")
    ap.add_argument("--offline", action="store_true", help="Skip community reputation lookups")
    args = ap.parse_args()
    try:
        if args.history:
            print(render_history())
            return 0
        if args.mapped_library:
            roots = load_mapped_library_roots(args.mapped_library)
            if not roots:
                raise ValueError(f"No persistent {args.mapped_library.title()} library folders are mapped")
            print(scan_collection(roots, f"{args.mapped_library.upper()} LIBRARY", online=not args.offline))
            return 0
        if args.quick_scan:
            print(scan_collection(quick_scan_roots(), "QUICK SCAN", online=not args.offline))
            return 0
        if args.system_scan:
            roots, recent_after, allow_run_media, label = system_scan_roots(args.system_scan)
            print(scan_collection(roots, label, online=not args.offline,
                                  recent_after=recent_after, allow_run_media=allow_run_media))
            return 0

        target = Path(args.file or args.auto_file or args.folder).expanduser().resolve()
        if args.file or args.auto_file:
            if not target.is_file():
                raise ValueError("Selected path is not a regular file")
            if args.auto_file:
                prior = already_scanned_unchanged(target)
                if prior is not None:
                    print("VERDICT=ALREADY SCANNED")
                    print("NOUGAT SECURITY ANALYSIS")
                    print()
                    print(f"File: {target}")
                    print(f"Previous result: {prior.get('verdict','UNKNOWN')}")
                    print("Automatic scan skipped because file size and modification time are unchanged since the previous scan.")
                    print("Use Scan Again in Virus Scan to force a fresh analysis.")
                    return 0
            print(render_one(scan_one(target, online=not args.offline)))
        else:
            if not target.is_dir():
                raise ValueError("Selected path is not a directory")
            print(scan_folder(target, online=not args.offline))
        return 0
    except KeyboardInterrupt:
        print("VERDICT=SCAN CANCELLED\nScan cancelled.")
        return 130
    except Exception as exc:
        print(f"VERDICT=SCAN ERROR\nNougat Security Analysis error: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
