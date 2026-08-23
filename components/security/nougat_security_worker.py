#!/usr/bin/env python3
"""Nougat Security Analysis one-shot worker.

The worker intentionally has no daemon mode. It starts for one file/folder scan,
prints a human-readable report, then exits. It never moves, deletes, quarantines,
renames, or opens the scanned file.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import mimetypes
import os
from pathlib import Path
import shutil
import subprocess
import sys
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


def scan_folder(folder: Path, online: bool) -> str:
    files = [p for p in folder.rglob("*") if p.is_file() and not p.is_symlink()]
    lines = ["VERDICT=NO THREATS DETECTED", "NOUGAT SECURITY ANALYSIS — FOLDER", "", f"Folder: {folder}", f"Files: {len(files)}", ""]
    worst = 0
    details: list[str] = []
    for i, p in enumerate(files, 1):
        try:
            r = scan_one(p, online=online)
            severity = {"NO THREATS DETECTED": 0, "ANALYSIS INCOMPLETE": 1, "SUSPICIOUS": 2, "THREAT DETECTED": 3}.get(r["verdict"], 1)
            worst = max(worst, severity)
            lines.append(f"[{i}/{len(files)}] {render_one(r, compact=True)}")
            if severity:
                details.append(render_one(r))
        except Exception as exc:
            worst = max(worst, 1)
            lines.append(f"[{i}/{len(files)}] ERROR: {p}: {exc}")
    verdict = ["NO THREATS DETECTED", "ANALYSIS INCOMPLETE", "SUSPICIOUS", "THREAT DETECTED"][worst]
    lines[0] = "VERDICT=" + verdict
    if details:
        lines.extend(["", "FLAGGED FILE DETAILS", "====================", ""])
        lines.append("\n\n".join(details))
    lines.extend(["", "RESULT: " + verdict, "Policy: WARN ME FIRST. Nougat did not move, delete, quarantine, rename, or open any scanned file."])
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--file")
    mode.add_argument("--auto-file", help="Scan a completed Nougat download unless unchanged scan history proves it was already scanned")
    mode.add_argument("--folder")
    mode.add_argument("--history", action="store_true")
    ap.add_argument("--offline", action="store_true", help="Skip community reputation lookups")
    args = ap.parse_args()
    try:
        if args.history:
            print(render_history())
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
