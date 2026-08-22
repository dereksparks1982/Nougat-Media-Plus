#!/usr/bin/env python3
from __future__ import annotations
import hashlib,pathlib,subprocess,sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None
main=(root/'src/main.cpp').read_text(encoding='utf-8'); diag=(root/'src/diagnostics/diagnostic_engine.cpp').read_text(encoding='utf-8')
def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
def sha(rel): return hashlib.sha256((root/rel).read_bytes()).hexdigest()
# v26 mouse navigation survives.
for token in ['e.xbutton.button == 8U','navigate_back()','e.xbutton.button == 9U','navigate_forward()','navigationBackStack','navigationForwardStack']:
    need(token in main,'retained mouse navigation missing: '+token)
# Library root/header/list-grid behavior survives.
need('const int libraryViewX = 28' in main and 'libraryListViewBtn = {libraryViewX, 38' in main,'retained Library view controls missing')
need('"MEDIA LIBRARY"' not in main,'redundant Library root heading returned')
# Volume stays 0-200 and centered, no rejected glyphs.
for token in ['std::min(200,vol)','*200/std::max','const int volumeGroupX = std::max(10, (W - volumeGroupW) / 2);','std::to_string(vol)+"%"']:
    need(token in main,'retained volume contract missing: '+token)
need('draw_speaker_icon(target,housing' not in main,'rejected speaker glyph returned')
# Fixed header remains under scrolling tabs; v27 additionally fixes divider/notch layer.
need(main.index('text(target, versionX, 17, versionLabel') < main.index('draw_tab(homeTab'), 'header version layering regressed')
need(main.index('fill_circle(target, serverX + text_width(serverLabel) + 7') < main.index('draw_tab(homeTab'), 'server-dot layering regressed')
# v26 Up Next remains.
for token in ['upNextDeadlineMs = now_ms() + 10000','"Play Next"','"Back to Series"','"Replay"','poll_up_next_overlay();','show_up_next_overlay()']:
    need(token in main,'retained Up Next missing: '+token)
# Diagnostics functionality remains even though owner removed redundant title heading.
for token in ['Nougat Media Suite Diagnostic Report','report_json','write_support_bundle','[REDACTED SENSITIVE LINE]']:
    need(token in diag,'retained diagnostic engine missing: '+token)
for token in ['"Export TXT"','"Export JSON"','"Support Bundle"','export_debug_report(3)']:
    need(token in main,'retained diagnostic UI/export missing: '+token)
# protected Search/P2P/license implementation hashes remain accepted.
expected={
'components/nougat/nougat_engine.py':'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993',
'src/nougat/nougat_bridge.cpp':'15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944',
'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e',
'src/p2p_engine.cpp':'1ad8dec1a454f5809c9afb1647b51d461110d01ce8647cfdabeeb57e9b3137a5',
'src/p2p_engine.hpp':'3d21670ffb49616c011d008efe292340ee1e6e55a004260d193c3e864c2a283f',
'src/p2p_stream_server.cpp':'110a9d7dd5036f8adcc45def2f1853c51a1ec928cca88a6132de6d5c205c4a29',
'src/p2p_stream_server.hpp':'68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7',
'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff'}
for rel,h in expected.items(): need(sha(rel)==h,'retained protected file changed: '+rel)
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.27','v27 executable version mismatch')
print('retained-v26=pass back-forward=pass library=pass volume-200=pass header-layering=pass up-next=pass diagnostics=pass search-p2p-license=pass')
