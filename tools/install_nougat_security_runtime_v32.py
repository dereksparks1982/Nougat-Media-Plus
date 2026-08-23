#!/usr/bin/env python3
"""Install/verify Nougat's generated one-shot security runtime.

Nothing created by this helper belongs in Git. The exact engine versions are pinned
for the v0.0.32 candidate. This helper never installs a daemon.
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


def verify(runtime: Path) -> bool:
    py = runtime / "venv" / "bin" / "python"
    capa = runtime / "venv" / "bin" / "capa"
    rules = runtime / "capa-rules-v9.4.0"
    if not py.is_file() or not capa.is_file() or not rules.is_dir():
        return False
    code = (
        "import yara_x,magika; "
        "assert getattr(yara_x,'__version__','')=='1.19.0'; "
        "assert getattr(magika,'__version__','')=='1.0.3'"
    )
    try:
        subprocess.run([str(py), "-c", code], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        ver = subprocess.check_output([str(capa), "--version"], text=True, stderr=subprocess.STDOUT).strip()
        return "9.4.0" in ver
    except Exception:
        return False


def install(runtime: Path) -> None:
    if sys.version_info < (3, 10):
        raise SystemExit("Python 3.10+ is required by capa 9.4.0")
    if verify(runtime):
        print("Nougat Security Analysis runtime already verified.")
        return
    runtime.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="nougat-security-v32-") as td:
        stage = Path(td) / "runtime"
        venv = stage / "venv"
        run([sys.executable, "-m", "venv", str(venv)])
        pip = venv / "bin" / "pip"
        run([str(pip), "install", "--disable-pip-version-check", "--no-input", "--upgrade", "pip"])
        run([str(pip), "install", "--disable-pip-version-check", "--no-input",
             f"yara-x=={YARA_X}", f"flare-capa=={CAPA}", f"magika=={MAGIKA}"])

        archive = Path(td) / "capa-rules-v9.4.0.zip"
        print("+ download", CAPA_RULES_URL)
        req = urllib.request.Request(CAPA_RULES_URL, headers={"User-Agent": "Nougat-Media-Suite/0.0.32"})
        with urllib.request.urlopen(req, timeout=60) as response, archive.open("wb") as out:
            shutil.copyfileobj(response, out)
        if not zipfile.is_zipfile(archive):
            raise RuntimeError("capa-rules download was not a valid ZIP archive")
        with zipfile.ZipFile(archive) as zf:
            zf.extractall(stage)
        rules = stage / "capa-rules-9.4.0"
        tagged = stage / "capa-rules-v9.4.0"
        if rules.is_dir() and not tagged.exists():
            rules.rename(tagged)
        if not tagged.is_dir():
            # GitHub currently normally expands to capa-rules-9.4.0; tolerate exact tag-style names too.
            candidates = [p for p in stage.iterdir() if p.is_dir() and p.name.startswith("capa-rules-") and p.name != "venv"]
            if len(candidates) == 1:
                candidates[0].rename(tagged)
        if not tagged.is_dir():
            raise RuntimeError("matching capa v9.4.0 rule directory was not produced")

        (stage / "VERSIONS.txt").write_text(
            "YARA-X 1.19.0\ncapa 9.4.0\ncapa-rules 9.4.0\nMagika 1.0.3\n",
            encoding="utf-8",
        )
        # Verify staged runtime before replacing an older generated runtime.
        if not verify(stage):
            raise RuntimeError("staged Nougat security runtime did not verify")
        backup = runtime.with_name(runtime.name + ".previous-v32")
        if backup.exists():
            shutil.rmtree(backup)
        if runtime.exists():
            runtime.rename(backup)
        try:
            stage.rename(runtime)
        except Exception:
            if runtime.exists():
                shutil.rmtree(runtime)
            if backup.exists():
                backup.rename(runtime)
            raise
        if backup.exists():
            shutil.rmtree(backup)
    if not verify(runtime):
        raise RuntimeError("installed Nougat security runtime failed final verification")
    print("Nougat Security Analysis runtime PASS: YARA-X 1.19.0, capa 9.4.0, Magika 1.0.3, capa-rules 9.4.0.")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("project_root")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    root = Path(args.project_root).resolve()
    runtime = root / "components" / "security" / "runtime"
    if args.check:
        if verify(runtime):
            print("Nougat Security Analysis runtime verified.")
            return 0
        print("Nougat Security Analysis runtime is missing or incomplete.")
        return 1
    install(runtime)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
