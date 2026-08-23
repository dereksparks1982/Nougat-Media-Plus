#!/usr/bin/env python3
"""Install/verify Nougat's generated one-shot security runtime.

Nothing created by this helper belongs in Git. The exact engine versions are pinned
for the v0.0.33 candidate. This helper never installs a daemon.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile

YARA_X = "1.19.0"
CAPA = "9.4.0"
MAGIKA = "1.0.3"
CAPA_RULES_URL = "https://github.com/mandiant/capa-rules/archive/refs/tags/v9.4.0.zip"


def run(cmd: list[str], **kw) -> subprocess.CompletedProcess:
    print("+", " ".join(cmd))
    return subprocess.run(cmd, check=True, **kw)


def verify(runtime: Path, verbose: bool = False) -> bool:
    py = runtime / "venv" / "bin" / "python"
    capa = runtime / "venv" / "bin" / "capa"
    rules = runtime / "capa-rules-v9.4.0"

    def fail(message: str) -> bool:
        if verbose:
            print(f"Nougat Security Analysis runtime verify FAIL: {message}")
        return False

    if not py.is_file():
        return fail(f"missing Python runtime: {py}")
    if not capa.is_file():
        return fail(f"missing capa executable: {capa}")
    if not rules.is_dir():
        return fail(f"missing capa-rules directory: {rules}")
    # A directory name alone is not enough. Require actual capa YAML rules.
    try:
        if not any(rules.rglob("*.yml")):
            return fail("capa-rules directory contains no .yml rules")
    except OSError as exc:
        return fail(f"could not inspect capa-rules directory: {exc}")

    # Verify distributions using Python package metadata, not optional module-level
    # __version__ attributes. The latter are not a stable packaging contract.
    metadata_code = (
        "from importlib.metadata import version; "
        "assert version('yara-x')=='1.19.0', version('yara-x'); "
        "assert version('flare-capa')=='9.4.0', version('flare-capa'); "
        "assert version('magika')=='1.0.3', version('magika'); "
        "import yara_x, magika"
    )
    try:
        probe = subprocess.run(
            [str(py), "-c", metadata_code],
            check=False, text=True, capture_output=True, timeout=30,
        )
    except Exception as exc:
        return fail(f"Python package metadata probe could not run: {exc}")
    if probe.returncode != 0:
        detail = (probe.stderr or probe.stdout or "unknown package metadata error").strip()
        return fail(f"pinned Python package verification failed: {detail}")

    try:
        capa_probe = subprocess.run(
            [str(capa), "--version"],
            check=False, text=True, capture_output=True, timeout=30,
        )
    except Exception as exc:
        return fail(f"capa --version could not run: {exc}")
    capa_text = ((capa_probe.stdout or "") + (capa_probe.stderr or "")).strip()
    if capa_probe.returncode != 0:
        return fail(f"capa --version exited {capa_probe.returncode}: {capa_text}")
    if "9.4.0" not in capa_text:
        return fail(f"unexpected capa version output: {capa_text!r}")

    if verbose:
        print("Nougat Security Analysis runtime verify PASS: package metadata, capa CLI, and capa-rules tree.")
    return True


def install(runtime: Path) -> None:
    if sys.version_info < (3, 10):
        raise SystemExit("Python 3.10+ is required by capa 9.4.0")
    if verify(runtime):
        print("Nougat Security Analysis runtime already verified.")
        return

    runtime.parent.mkdir(parents=True, exist_ok=True)
    backup = runtime.with_name(runtime.name + ".previous-v33")

    # Build the virtual environment at its FINAL path. Python console scripts use
    # absolute shebangs, so building a venv in /tmp and then moving/copying it is
    # both cross-device unsafe (EXDEV) and can leave broken entry-point paths.
    # The existing generated runtime is moved aside on the same filesystem and is
    # restored if any install or verification step fails.
    if backup.exists():
        if runtime.exists():
            shutil.rmtree(backup)
        else:
            backup.rename(runtime)

    if runtime.exists():
        runtime.rename(backup)

    try:
        runtime.mkdir(parents=True, exist_ok=False)
        venv = runtime / "venv"
        try:
            run([sys.executable, "-m", "venv", str(venv)])
        except subprocess.CalledProcessError as exc:
            raise RuntimeError(
                "Python venv creation failed. The v0.0.33 installer prerequisite gate "
                "must provide the matching Ubuntu pythonX.Y-venv package before security setup."
            ) from exc

        pip = venv / "bin" / "pip"
        run([str(pip), "install", "--disable-pip-version-check", "--no-input", "--upgrade", "pip"])
        run([str(pip), "install", "--disable-pip-version-check", "--no-input",
             f"yara-x=={YARA_X}", f"flare-capa=={CAPA}", f"magika=={MAGIKA}"])

        with tempfile.TemporaryDirectory(prefix="nougat-security-v33-download-") as td:
            archive = Path(td) / "capa-rules-v9.4.0.zip"
            print("+ download", CAPA_RULES_URL)
            req = urllib.request.Request(CAPA_RULES_URL, headers={"User-Agent": "Nougat-Media-Suite/0.0.33"})
            with urllib.request.urlopen(req, timeout=60) as response, archive.open("wb") as out:
                shutil.copyfileobj(response, out)
            if not zipfile.is_zipfile(archive):
                raise RuntimeError("capa-rules download was not a valid ZIP archive")
            with zipfile.ZipFile(archive) as zf:
                zf.extractall(runtime)

        rules = runtime / "capa-rules-9.4.0"
        tagged = runtime / "capa-rules-v9.4.0"
        if rules.is_dir() and not tagged.exists():
            rules.rename(tagged)
        if not tagged.is_dir():
            # GitHub currently normally expands to capa-rules-9.4.0; tolerate exact tag-style names too.
            candidates = [p for p in runtime.iterdir() if p.is_dir() and p.name.startswith("capa-rules-") and p.name != "venv"]
            if len(candidates) == 1:
                candidates[0].rename(tagged)
        if not tagged.is_dir():
            raise RuntimeError("matching capa v9.4.0 rule directory was not produced")

        (runtime / "VERSIONS.txt").write_text(
            "YARA-X 1.19.0\ncapa 9.4.0\ncapa-rules 9.4.0\nMagika 1.0.3\n",
            encoding="utf-8",
        )

        if not verify(runtime, verbose=True):
            raise RuntimeError("installed Nougat security runtime did not verify; see the failed sub-check above")

    except Exception:
        if runtime.exists():
            shutil.rmtree(runtime)
        if backup.exists():
            backup.rename(runtime)
        raise
    else:
        if backup.exists():
            shutil.rmtree(backup)

    if not verify(runtime, verbose=True):
        raise RuntimeError("installed Nougat security runtime failed final verification; see the failed sub-check above")
    print("Nougat Security Analysis runtime PASS: YARA-X 1.19.0, capa 9.4.0, Magika 1.0.3, capa-rules 9.4.0.")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("project_root")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    root = Path(args.project_root).resolve()
    runtime = root / "components" / "security" / "runtime"
    if args.check:
        if verify(runtime, verbose=True):
            print("Nougat Security Analysis runtime verified.")
            return 0
        print("Nougat Security Analysis runtime is missing or incomplete.")
        return 1
    install(runtime)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
