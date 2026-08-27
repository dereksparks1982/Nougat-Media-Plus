#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
WORKSHOP_MARKER = "NOUGAT_V50_WORKSHOP_PROCESS_PLUGIN"
GAMES_MARKER = "NOUGAT_V50_GAMES_PROCESS_PLUGIN"


def run(script: str) -> None:
    result = subprocess.run([sys.executable, str(ROOT / "tools" / script)], text=True)
    if result.returncode != 0:
        raise RuntimeError(script + " failed")


def main() -> int:
    try:
        text = MAIN.read_text(encoding="utf-8")
        if GAMES_MARKER in text:
            run("apply_v50_workshop_process_plugin.py")
            run("apply_v50_games_process_plugin.py")
            print("PASS: complete v0.0.50 candidate migration already applied through Games Plugin #2")
            return 0

        if WORKSHOP_MARKER not in text:
            run("apply_v50_core.py")
            run("apply_v50_dialog_labels.py")
            run("apply_v50_plugin_foundation.py")
            run("apply_v50_workshop_process_plugin.py")

        run("apply_v50_games_process_plugin.py")
        print("PASS: complete v0.0.50 candidate migration applied through Games Plugin #2")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
