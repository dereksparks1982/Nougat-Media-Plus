#!/usr/bin/env python3
from pathlib import Path
import hashlib
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()

def need(ok, msg):
    if not ok:
        raise SystemExit('FAIL: ' + msg)

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

expected_icons = {
    16: 'c65de25b652bf634b1c55596722ed34825a490322d7e338d6e9ef94ee902ba17',
    32: '45ff86ba51a2e0035bbb3c9dbb7924fc09bc3535e48fdc071a20ae50efa8f2b1',
    48: '5e11c947177afac6aec9e9a0797f9da6eb9a2182ec290ef378663f613fcb8194',
    64: '6a02d4d365f05d4c7be724a3c348a4fa0fabc314f0b341ed0a2b6bd2aad5ac56',
    128: '5c1195deedaf650514fddd11681f4b40f326256b9779fe3d6f69ef9400d03197',
    256: '89fbdf8c638d4c0c40834ee587c2b6f6a31e921acbaebd3066d03e9d10eb413e',
    512: '40ebef58a7cc86e3883de287b0ed887d4eb3a60b2c4550c84fbebcaa3310734f',
}
for size, expected in expected_icons.items():
    p = root / f'assets/icons/nougat-media-suite-v53-{size}.png'
    need(p.is_file(), f'accepted v53/v54 icon missing: {p.name}')
    need(sha(p) == expected, f'accepted v53/v54 icon changed: {p.name}')

master = root / 'assets/icons/nougat-media-suite-v53.png'
need(master.is_file(), 'accepted v53/v54 icon master missing')
need(sha(master) == expected_icons[512], 'accepted v53/v54 icon master changed')

exe = root / 'Nougat_Media_Suite_v55'
need(exe.is_file(), 'v55 executable missing')
need(sha(exe) == 'c5b8af9da5ec172aa791139e87e951079633c09ae96edb608d5b452f30c14784',
     'v55 executable bytes changed during identity/root replacement repair')

for rel in ('NougatMediaSuite.desktop', 'com.elderredsoftworks.NougatMediaSuite.desktop'):
    text = (root / rel).read_text(encoding='utf-8')
    need('Nougat_Media_Suite_v55' in text, rel + ' does not target v55')
    need('Nougat_Media_Suite_v54' not in text, rel + ' still targets v54')
    need('Icon=nougat-media-suite' in text, rel + ' lost canonical Nougat icon key')
    need('StartupWMClass=NougatMediaSuite' in text, rel + ' lost WM class')
    need('X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite' in text,
         rel + ' lost application ID')

main = (root / 'src/main.cpp').read_text(errors='replace')
need('nougat_media_suite_icon_data.hpp' in main, 'embedded X11 icon source include missing')

print('PASS: v55 identity uses accepted v53/v54 icon family and v55 launch target')
