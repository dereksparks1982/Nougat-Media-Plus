#!/usr/bin/env python3
"""Bounded Xvfb startup proof for ReddMedia v0.0.20."""
from __future__ import annotations
import os, pathlib, shutil, subprocess, sys, tempfile, textwrap
ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
BINARY = pathlib.Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else ROOT / 'ReddMedia_v20'
if not BINARY.is_file():
    raise SystemExit(f'missing candidate binary: {BINARY}')
if not shutil.which('xvfb-run') or not shutil.which('xwininfo'):
    raise SystemExit('xvfb-run and xwininfo are required for UI smoke test')
with tempfile.TemporaryDirectory(prefix='reddmedia-v20-ui-smoke.') as raw:
    work = pathlib.Path(raw)
    app = work/'app'; app.mkdir()
    home = work/'home'; home.mkdir()
    shutil.copy2(BINARY, app/'ReddMedia_v20')
    (app/'components/nougat').mkdir(parents=True)
    shutil.copy2(ROOT/'components/nougat/nougat_engine.py', app/'components/nougat/nougat_engine.py')
    script = textwrap.dedent('''
        ./ReddMedia_v20 &
        pid=$!
        sleep 2
        if ! kill -0 "$pid" 2>/dev/null; then
            echo 'ReddMedia exited before UI smoke observation'
            wait "$pid"
            return 20
        fi
        if ! xwininfo -root -tree 2>/dev/null | grep -F 'ReddMedia' >/dev/null; then
            echo 'ReddMedia window was not found'
            kill "$pid" 2>/dev/null || true
            wait "$pid" 2>/dev/null || true
            return 21
        fi
        kill "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
        echo 'reddmedia-v20-ui-window=pass startup-alive=pass'
    ''')
    env=os.environ.copy(); env['HOME']=str(home)
    result=subprocess.run(['xvfb-run','-a','bash','-c',script], cwd=app, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
    if result.returncode != 0:
        raise SystemExit(result.stdout.strip() or f'UI smoke failed rc={result.returncode}')
    print(result.stdout.strip())
