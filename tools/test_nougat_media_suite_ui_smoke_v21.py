#!/usr/bin/env python3
from __future__ import annotations
import os,pathlib,shutil,subprocess,sys,tempfile,textwrap
ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
BINARY=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else ROOT/'Nougat_Media_Suite_v21'
if not BINARY.is_file(): raise SystemExit(f'missing candidate binary: {BINARY}')
if not shutil.which('xvfb-run') or not shutil.which('xwininfo'): raise SystemExit('xvfb-run and xwininfo required')
with tempfile.TemporaryDirectory(prefix='nms-v21-ui.') as raw:
    w=pathlib.Path(raw); app=w/'app'; home=w/'home'; app.mkdir(); home.mkdir()
    shutil.copy2(BINARY,app/'Nougat_Media_Suite_v21')
    (app/'components/nougat').mkdir(parents=True)
    shutil.copy2(ROOT/'components/nougat/nougat_engine.py',app/'components/nougat/nougat_engine.py')
    script=textwrap.dedent("""
    ./Nougat_Media_Suite_v21 &
    pid=$!
    sleep 2
    if ! kill -0 "$pid" 2>/dev/null; then echo early-exit; wait "$pid"; return 20; fi
    if ! xwininfo -root -tree 2>/dev/null | grep -F 'Nougat Media Suite' >/dev/null; then echo window-not-found; kill "$pid" 2>/dev/null || true; wait "$pid" 2>/dev/null || true; return 21; fi
    kill "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    echo nougat-media-suite-v21-ui-window=pass
    """)
    env=os.environ.copy(); env['HOME']=str(home)
    r=subprocess.run(['xvfb-run','-a','bash','-c',script],cwd=app,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20)
    if r.returncode: raise SystemExit(r.stdout.strip() or f'ui smoke rc={r.returncode}')
    print(r.stdout.strip())
