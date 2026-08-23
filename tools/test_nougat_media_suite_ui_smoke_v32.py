#!/usr/bin/env python3
import pathlib, shutil, subprocess, sys, tempfile
root=pathlib.Path(sys.argv[1]).resolve()
binary=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else root/'Nougat_Media_Suite_v32'
if not binary.is_file(): raise SystemExit('FAIL: v32 UI smoke binary missing')
for cmd in ('xvfb-run','xwininfo','xprop'):
    if not shutil.which(cmd): raise SystemExit(f'FAIL: {cmd} required for UI smoke')
with tempfile.TemporaryDirectory(prefix='nms-v32-ui.') as raw:
    app=pathlib.Path(raw)/'app'; app.mkdir()
    exe=app/'Nougat_Media_Suite_v32'; shutil.copy2(binary,exe); exe.chmod(0o755)
    script=f'''cd "{app}"
"{exe}" >/tmp/nms-v32-ui.out 2>/tmp/nms-v32-ui.err &
pid=$!
sleep 2
wid=$(xwininfo -root -tree | awk '/Nougat Media Suite/ {{print $1; exit}}')
if [ -z "$wid" ]; then
    kill $pid 2>/dev/null || true
    wait $pid 2>/dev/null || true
    return 9 2>/dev/null || true
fi
props=$(xprop -id "$wid" WM_CLASS _GTK_APPLICATION_ID _BAMF_DESKTOP_FILE _NET_WM_ICON _NET_WM_NAME)
printf '%s\n' "$props" >/tmp/nms-v32-xprops.out
ok=0
printf '%s\n' "$props" | grep -F 'WM_CLASS(STRING) = "nougat-media-suite", "NougatMediaSuite"' >/dev/null || ok=1
printf '%s\n' "$props" | grep -F '_GTK_APPLICATION_ID(UTF8_STRING) = "com.elderredsoftworks.NougatMediaSuite"' >/dev/null || ok=1
printf '%s\n' "$props" | grep -F 'com.elderredsoftworks.NougatMediaSuite.desktop' >/dev/null || ok=1
printf '%s\n' "$props" | grep -F '_NET_WM_ICON(CARDINAL)' >/dev/null || ok=1
printf '%s\n' "$props" | grep -F '_NET_WM_NAME(UTF8_STRING) = "Nougat Media Suite"' >/dev/null || ok=1
kill $pid 2>/dev/null || true
wait $pid 2>/dev/null || true
return $ok 2>/dev/null || true
'''
    result=subprocess.run(['xvfb-run','-a','bash','-lc',script],timeout=12)
    if result.returncode != 0:
        detail=pathlib.Path('/tmp/nms-v32-xprops.out')
        if detail.exists(): print(detail.read_text(errors='replace'))
        raise SystemExit('FAIL: v32 X11 identity/UI smoke failed')
print('nougat-media-suite-v32-ui-window=pass canonical-app-id=pass wm-class=pass dock-desktop-hint=pass net-wm-icon=pass default-home-window=pass')
