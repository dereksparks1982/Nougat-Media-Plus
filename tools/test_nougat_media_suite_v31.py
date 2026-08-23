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
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
road=(root/'ROADMAP.md').read_text(encoding='utf-8')
readme=(root/'README.md').read_text(encoding='utf-8')

need('VERSION 0.0.31' in cm and 'Nougat_Media_Suite_v31' in cm,'CMake v31 identity missing')
need('printf("Nougat Media Suite v0.0.31\\n")' in main,'runtime v31 identity missing')
need('const std::string versionLabel = "v0.0.31"' in main,'in-app v31 version missing')
for token in [
    'enum class SheetControlState { Normal, Hover, Pressed, Disabled }',
    'void draw_sheet_button_surface(', 'void draw_sheet_tab_surface(', 'void draw_sheet_panel_surface(',
    'void draw_sheet_track(', 'void draw_sheet_track_segment(', 'void draw_sheet_track_fill(',
    'void draw_sheet_knob(', 'void draw_sheet_checkbox(', 'outline_round_dashed(',
    'draw_sheet_reference_texture(', 'nougat_ui_sheet_texture::kButtonTexture',
    'draw_sheet_tab_surface(target, surface, tabPalette, active, hover)',
    'draw_sheet_panel_surface(target, r, palette)',
    'draw_sheet_button_surface(target, r, palette, state)',
    'draw_sheet_checkbox(target, check, selected, palette)',
    '--v31-ui-sheet-self-test',
]: need(token in main,'approved sheet component contract missing: '+token)
need(main.count('draw_sheet_tab_surface(') >= 5,'shared sheet tab component is not reused broadly')
need(main.count('draw_sheet_button_surface(') >= 7,'shared sheet button component is not reused broadly')
need('const int knobD = 24;' in main and 'const int knobD=24;' in main,'sheet seek/volume knob geometry missing')
need('volRect = {volumeHousingX + volumeHousingPad, volumeY + 3, volumeTrackW, 12};' in main,'sheet volume track geometry missing')

# Owner rule: page/service palettes are exactly the accepted v0.0.30 palette code.
need(function_hash(main,'    ViewPalette stream_palette_for(') == 'c57a8939fe016b88c7f67792ff8ebae5a5354838b02ab47cd4de8b5fab8ea452','accepted Stream palette function changed')
need(function_hash(main,'    ViewPalette palette_for(') == 'f15f60f417320b8866eb6b5a44caee04e3855be1c64c7454a0d41a26a49768f8','accepted page palette function changed')
# Owner specifically accepted the redesigned Home cards. These function bodies must stay unchanged.
need(slice_hash(main,'    void draw_home_artwork(', '    void draw_home_card(') == '4f03c18da06697b338cd7aea5b6ce72d59f942654f377af1f0c69d914083783c',
     'Home artwork function changed')
need(slice_hash(main,'    void draw_home_card(', '    void draw_home_screen(') == '6f8e99a37ef6f35aecf8265dec5378f76fcd4336974312ff2fae15ebecacec45',
     'accepted Home card function changed')

need('v0.0.31 candidate — Exact Approved UI Sheet Components' in road,'v31 UI-only roadmap identity missing')
need('v0.0.32 planned — Focused P2P streaming expansion' in road,'P2P deferral missing')
need('Hauppauge WinTV-HVR-955Q' in road and 'HDHomeRun' in road and 'ATSC 3.0' in road,'Live TV roadmap additions missing')
need('Radio / SDR reception' in road and 'CB radio reception' in road,'radio/SDR roadmap additions missing')
need('The accepted page palettes do **not** change.' in readme,'README palette-preservation contract missing')
need(sha('docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png') == 'cd57f3840bf113f293d5fcfe9b34652098f629d40ffb7587da00e5c938bf2889','approved UI sheet asset changed')
need((root/'src/nougat_ui_sheet_texture_data.hpp').is_file(),'sheet-derived runtime texture header missing')

expected={'LICENSE': '640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84', 'COPYRIGHT.md': 'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31', 'CONTRIBUTING.md': '7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb', 'THIRD_PARTY_NOTICES.md': '9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f', 'docs/LICENSING_POLICY.md': 'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff', 'components/nougat/nougat_engine.py': 'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993', 'src/nougat/nougat_bridge.cpp': '15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944', 'src/nougat/nougat_bridge.hpp': '46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e', 'src/p2p_engine.cpp': '1ad8dec1a454f5809c9afb1647b51d461110d01ce8647cfdabeeb57e9b3137a5', 'src/p2p_engine.hpp': '3d21670ffb49616c011d008efe292340ee1e6e55a004260d193c3e864c2a283f', 'src/p2p_stream_server.cpp': '110a9d7dd5036f8adcc45def2f1853c51a1ec928cca88a6132de6d5c205c4a29', 'src/p2p_stream_server.hpp': '68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7'}
for rel,h in expected.items(): need(sha(rel)==h,rel+' changed outside v31 scope')
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.31','executable version mismatch')
    subprocess.check_call([str(exe),'--v29-tv-reliability-self-test'])
    subprocess.check_call([str(exe),'--v30-ui-library-player-self-test'])
    subprocess.check_call([str(exe),'--v31-ui-sheet-self-test'])
print('v31-contract=pass sheet-components=pass exact-palettes-retained=pass home-cards-retained=pass p2p-deferred=pass live-tv-roadmap=pass radio-sdr-roadmap=pass protected-boundaries=pass')
