#!/usr/bin/env python3
import pathlib, shutil, subprocess, sys, tempfile
root=pathlib.Path(sys.argv[1]).resolve()
binary=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else root/'Nougat_Media_Suite_v23'
if not binary.is_file(): raise SystemExit('FAIL: v23 UI smoke binary missing')
if not shutil.which('xvfb-run') or not shutil.which('xwininfo'):
    raise SystemExit('FAIL: xvfb-run/xwininfo required for UI smoke')
with tempfile.TemporaryDirectory(prefix='nms-v23-ui.') as raw:
    app=pathlib.Path(raw)/'app'; app.mkdir()
    exe=app/'Nougat_Media_Suite_v23'; shutil.copy2(binary,exe); exe.chmod(0o755)
    # The executable only needs a visible X11 window for this deterministic smoke.
    script=f'''cd "{app}"\n"{exe}" >/tmp/nms-v23-ui.out 2>/tmp/nms-v23-ui.err &\npid=$!\nsleep 2\nxwininfo -root -tree | grep -F 'Nougat Media Suite' >/dev/null\nrc=$?\nkill $pid 2>/dev/null || true\nwait $pid 2>/dev/null || true\nreturn $rc 2>/dev/null || true\n'''
    result=subprocess.run(['xvfb-run','-a','bash','-lc',script],timeout=12)
    if result.returncode != 0: raise SystemExit('FAIL: v23 X11 window smoke failed')
print('nougat-media-suite-v23-ui-window=pass')
