#!/usr/bin/env python3
from __future__ import annotations
import hashlib, pathlib, subprocess, sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
def sha(rel): return hashlib.sha256((root/rel).read_bytes()).hexdigest()
def slice_hash(text,a,b):
    start=text.index(a); end=text.index(b,start); return hashlib.sha256(text[start:end].encode()).hexdigest()
def function_hash(text, signature):
    start=text.index(signature); brace=text.index('{',start); depth=0
    for i in range(brace,len(text)):
        if text[i]=='{': depth+=1
        elif text[i]=='}':
            depth-=1
            if depth==0: return hashlib.sha256(text[start:i+1].encode()).hexdigest()
    raise ValueError('unterminated function '+signature)
main=(root/'src/main.cpp').read_text(encoding='utf-8')
for token in [
    'enum class SheetControlState { Normal, Hover, Pressed, Disabled }',
    'void draw_sheet_button_surface(', 'void draw_sheet_tab_surface(', 'void draw_sheet_panel_surface(',
    'void draw_sheet_track(', 'void draw_sheet_track_fill(', 'void draw_sheet_knob(', 'void draw_sheet_checkbox(',
    'draw_sheet_reference_texture(', 'nougat_ui_sheet_texture::kButtonTexture', '--v31-ui-sheet-self-test',
    'StreamPlatform::Vimeo', '"Vimeo"',
]: need(token in main,'retained v31 UI/provider contract missing: '+token)
need(function_hash(main,'    ViewPalette stream_palette_for(') == 'c57a8939fe016b88c7f67792ff8ebae5a5354838b02ab47cd4de8b5fab8ea452','accepted Stream palette function changed')
need(function_hash(main,'    ViewPalette palette_for(') == 'f15f60f417320b8866eb6b5a44caee04e3855be1c64c7454a0d41a26a49768f8','accepted page palette function changed')
need(slice_hash(main,'    void draw_home_artwork(', '    void draw_home_card(') == '4f03c18da06697b338cd7aea5b6ce72d59f942654f377af1f0c69d914083783c','Home artwork function changed')
need(function_hash(main,'    void draw_home_card(') == '383fa4a6c9e63a33c0e7df9be668e9b8dd81f315c7654266c05e3ab3d5bfc3d3','accepted Home card function changed')
need(sha('docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png') == 'cd57f3840bf113f293d5fcfe9b34652098f629d40ffb7587da00e5c938bf2889','approved UI sheet asset changed')
need((root/'src/nougat_ui_sheet_texture_data.hpp').is_file(),'sheet-derived runtime texture header missing')
expected={
'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff',
'components/nougat/nougat_engine.py':'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993',
'src/nougat/nougat_bridge.cpp':'15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944',
'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e',
}
for rel,h in expected.items(): need(sha(rel)==h,rel+' changed outside v32 scope')
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.32','v32 executable version mismatch')
    subprocess.check_call([str(exe),'--v29-tv-reliability-self-test'])
    subprocess.check_call([str(exe),'--v30-ui-library-player-self-test'])
    subprocess.check_call([str(exe),'--v31-ui-sheet-self-test'])
print('retained-v31=pass sheet-components=pass exact-palettes-retained=pass home-cards-retained=pass vimeo-retained=pass license-search-boundaries=pass')
