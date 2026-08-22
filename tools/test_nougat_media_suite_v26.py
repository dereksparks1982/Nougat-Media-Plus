#!/usr/bin/env python3
from __future__ import annotations
import hashlib, pathlib, re, struct, sys, zlib
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
def sha(rel):
    return hashlib.sha256((root/rel).read_bytes()).hexdigest()
def png_rgba(path):
    data=path.read_bytes(); need(data.startswith(b'\x89PNG\r\n\x1a\n'),f'not PNG: {path}')
    pos=8; ihdr=None; raw=b''
    while pos+12<=len(data):
        n=struct.unpack('>I',data[pos:pos+4])[0]; typ=data[pos+4:pos+8]; body=data[pos+8:pos+8+n]; pos+=12+n
        if typ==b'IHDR': ihdr=body
        elif typ==b'IDAT': raw+=body
        elif typ==b'IEND': break
    w,h,depth,color,comp,filt,interlace=struct.unpack('>IIBBBBB',ihdr)
    need((depth,color,comp,filt,interlace)==(8,6,0,0,0),'expected RGBA8 noninterlaced PNG')
    scan=zlib.decompress(raw); bpp=4; stride=w*bpp; rows=[]; prev=bytearray(stride); off=0
    for _ in range(h):
        ft=scan[off]; off+=1; cur=bytearray(scan[off:off+stride]); off+=stride
        for i in range(stride):
            left=cur[i-bpp] if i>=bpp else 0; up=prev[i]; ul=prev[i-bpp] if i>=bpp else 0
            if ft==1: cur[i]=(cur[i]+left)&255
            elif ft==2: cur[i]=(cur[i]+up)&255
            elif ft==3: cur[i]=(cur[i]+((left+up)//2))&255
            elif ft==4:
                p=left+up-ul; pa=abs(p-left); pb=abs(p-up); pc=abs(p-ul); pr=left if pa<=pb and pa<=pc else (up if pb<=pc else ul); cur[i]=(cur[i]+pr)&255
            elif ft!=0: raise SystemExit('FAIL: unsupported PNG filter')
        rows.append(bytes(cur)); prev=cur
    return w,h,rows

cm=(root/'CMakeLists.txt').read_text()
main=(root/'src/main.cpp').read_text()
diag=(root/'src/diagnostics/diagnostic_engine.cpp').read_text()
road=(root/'ROADMAP.md').read_text()
need('VERSION 0.0.26' in cm and 'Nougat_Media_Suite_v26' in cm,'CMake v26 identity missing')
need('printf("Nougat Media Suite v0.0.26\\n")' in main,'runtime version missing')
# Mouse Back/Forward real navigation history
for token in ['e.xbutton.button == 8U','navigate_back()','e.xbutton.button == 9U','navigate_forward()','navigationBackStack','navigationForwardStack']:
    need(token in main,'mouse navigation contract missing: '+token)
# Library cleanup
need('if (!libraryParents.empty()) text(target,104,58' in main,'nested Library heading placement missing')
need('libraryListViewBtn = {libraryViewX, 38' in main and 'const int libraryViewX = 28' in main,'Library view controls not at far left')
need('"MEDIA LIBRARY"' not in main,'redundant MEDIA LIBRARY label remains in source')
# Volume exact intent
need('std::min(200,vol)' in main and '*200/std::max' in main,'0-200% volume behavior missing')
need('const int volumeGroupX = std::max(10, (W - volumeGroupW) / 2);' in main,'volume group is not centered')
need('draw_speaker_icon(target,housing' not in main,'rejected speaker square/triangle glyphs still drawn')
need('std::to_string(vol)+"%"' in main,'correct volume percentage readout missing')
need('text(target,normalX-14' not in main,'rogue duplicate 100% readout remains')
# Header layering: fixed identity/status drawn before tabs
need(main.index('text(target, versionX, 17, versionLabel') < main.index('draw_tab(videoPlayerTab'), 'version not beneath scrolling tabs')
need(main.index('fill_circle(target, serverX + text_width(serverLabel) + 7') < main.index('draw_tab(videoPlayerTab'), 'server status dot not beneath scrolling tabs')
# Up Next 10s
for token in ['upNextDeadlineMs = now_ms() + 10000','"Play Next"','"Back to Series"','"Replay"','poll_up_next_overlay();','show_up_next_overlay()']:
    need(token in main,'Up Next contract missing: '+token)
# Diagnostics
for token in ['Nougat Media Suite Diagnostic Report','report_json','write_support_bundle','[REDACTED SENSITIVE LINE]']:
    need(token in diag,'diagnostic contract missing: '+token)
for token in ['"DIAGNOSTIC CENTER"','"Export TXT"','"Export JSON"','"Support Bundle"','export_debug_report(3)']:
    need(token in main,'Diagnostic Center UI missing: '+token)
need('v0.0.27 planned' in road and 'Seek hover previews' in road,'v0.0.27 hover-preview roadmap missing')
need('v0.0.28 planned' in road and 'P2P expansion' in road,'v0.0.28 P2P roadmap missing')
# P2P/Search implementation unchanged from accepted v25
expected={
'src/p2p_engine.cpp':'1ad8dec1a454f5809c9afb1647b51d461110d01ce8647cfdabeeb57e9b3137a5',
'src/p2p_engine.hpp':'3d21670ffb49616c011d008efe292340ee1e6e55a004260d193c3e864c2a283f',
'src/p2p_stream_server.cpp':'110a9d7dd5036f8adcc45def2f1853c51a1ec928cca88a6132de6d5c205c4a29',
'src/p2p_stream_server.hpp':'68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7',
'components/nougat/nougat_engine.py':'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993',
'src/nougat/nougat_bridge.cpp':'15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944',
'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e',
'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff'}
for rel,h in expected.items(): need(sha(rel)==h,rel+' changed outside v26 scope')
# Clean N perimeter: master bottom 16 rows fully transparent and embedded header matches generated sizes
master=root/'assets/icons/nougat-media-suite.png'; w,h,rows=png_rgba(master); need((w,h)==(512,512),'N master size wrong')
need(all(row[x*4+3]==0 for row in rows[496:] for x in range(w)),'bottom-edge icon sliver remains')
iconhashes={'14':'96526b6d4087ff6211832f3c0b081fce25be44524e8059cdab265305bf46b378','16':'5f2070e753c3bc63eb206e6f6418d22e264923b3e7d8f76e398a8abf3f23ce82','32':'411c304db0b4b44fbf463d76dcd47f43c8bbf9fea9befc6a87c774e9aef622f6','48':'7f8c94ff6033f24382efdb4bd7f29cb9155d3ce6e7af948a4192da5ff153d725','64':'139f370ff95f93a101988b0336359cdbab7e9b293ea76ba96dd2f5255c730d6d','128':'ed0b04a915a9028a5aae8f7a2b7ef28393f1bb021c0a73a4e8184e0497a7c695','256':'d22756e70737e7e1e068fb036863ba7cbded48b3f85ea0971404a19ca07da102','512':'681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804'}
for size,hx in iconhashes.items(): need(sha(f'assets/icons/nougat-media-suite-{size}.png')==hx,f'clean N {size} hash mismatch')
need(sha('src/nougat_media_suite_icon_data.hpp')=='c626664598d57a3756a62a425875fd48567ae4eaa8c9b5a385ccf4630a0b22cb','embedded clean N data mismatch')
if exe is not None:
    import subprocess
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.26','executable version mismatch')
print('v26-contract=pass mouse-back-forward=pass library-header=pass clean-N-bottom=pass diagnostics=pass volume-center-200=pass header-layering=pass up-next-countdown=pass p2p-deferred=pass roadmap-v27-v28=pass licensing-search-preserved=pass')
