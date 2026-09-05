#!/usr/bin/env python3
from pathlib import Path

p = Path('src/main.cpp')
s = p.read_text()

# Version advance.
s = s.replace('v0.0.64', 'v0.0.65')
s = s.replace('--v64-media-plus-ui-self-test', '--v65-media-plus-ui-self-test')

# Chat scaffold include.
needle = '#include "radio/radio_backend.hpp"\n'
if '#include "chat/nougat_chat.hpp"' not in s:
    if needle not in s:
        raise SystemExit('STOP: radio include anchor not found')
    s = s.replace(needle, needle + '#include "chat/nougat_chat.hpp"\n', 1)

# Radio service scrolling state.
old = '    std::vector<std::pair<Rect,std::string>> radioServiceTabs;\n'
new = old + '    Rect radioServiceStrip, radioServiceScrollTrack, radioServiceScrollThumb;\n    int radioServiceScrollX = 0;\n    int radioServiceMaxScrollX = 0;\n'
if 'radioServiceMaxScrollX' not in s:
    if old not in s:
        raise SystemExit('STOP: radio service state anchor not found')
    s = s.replace(old, new, 1)

# TV antenna scanning belongs to Live TV, not Radio.
s = s.replace(', "Favorites", "Recordings", "TV Antenna Scan", "ISS / Sat"', ', "Favorites", "Recordings", "ISS / Sat"')
s = s.replace('        radioAntennaScanBtn=radioTabRect("TV Antenna Scan");\n', '        radioAntennaScanBtn={}; // v65: TV antenna scanning lives in Live TV.\n')

# Replace the dense 7-column Radio wall with a two-row horizontally scrollable service lane.
old_layout = '''        const int serviceCols = 7;\n        const int serviceGap = 6;\n        const int serviceW = std::max(92, (usable - serviceGap * (serviceCols - 1)) / serviceCols);\n        const int presetY = kPageControlY;\n'''
new_layout = '''        // NOUGAT_V65_RADIO_SERVICE_LANE: two-row mode lane, horizontally scrollable.\n        const int serviceRows = 2;\n        const int serviceGap = 6;\n        const int serviceW = std::max(112, std::min(142, usable / 7));\n        const int presetY = kPageControlY;\n        radioServiceStrip = {left, presetY, usable, serviceRows * (kCompactButtonH + 7) - 7};\n        const int serviceCols = (static_cast<int>(sizeof(kNougatRadioServices) / sizeof(kNougatRadioServices[0])) + serviceRows - 1) / serviceRows;\n        const int serviceVirtualW = std::max(0, serviceCols * (serviceW + serviceGap) - serviceGap);\n        radioServiceMaxScrollX = std::max(0, serviceVirtualW - usable);\n        radioServiceScrollX = std::max(0, std::min(radioServiceMaxScrollX, radioServiceScrollX));\n'''
if old_layout not in s:
    raise SystemExit('STOP: radio layout anchor not found')
s = s.replace(old_layout, new_layout, 1)

old_loop = '''        for (int i=0; i<serviceCount; ++i) {\n            const int row=i/serviceCols;\n            const int colIndex=i%serviceCols;\n            const Rect r{left + colIndex*(serviceW+serviceGap),\n                         presetY + row*(kCompactButtonH+7), serviceW, kCompactButtonH};\n            radioServiceTabs.push_back({r,kNougatRadioServices[i]});\n        }\n'''
new_loop = '''        for (int i=0; i<serviceCount; ++i) {\n            const int row = i % serviceRows;\n            const int colIndex = i / serviceRows;\n            const Rect r{left + colIndex * (serviceW + serviceGap) - radioServiceScrollX,\n                         presetY + row * (kCompactButtonH + 7), serviceW, kCompactButtonH};\n            if (r.x + r.w > radioServiceStrip.x && r.x < radioServiceStrip.x + radioServiceStrip.w)\n                radioServiceTabs.push_back({r,kNougatRadioServices[i]});\n        }\n        radioServiceScrollTrack = {left, radioServiceStrip.y + radioServiceStrip.h + 4, usable, 6};\n        if (radioServiceMaxScrollX > 0) {\n            const int thumbW = std::max(36, usable * usable / std::max(1, serviceVirtualW));\n            const int travel = std::max(0, usable - thumbW);\n            const int thumbX = left + (radioServiceMaxScrollX > 0 ? radioServiceScrollX * travel / radioServiceMaxScrollX : 0);\n            radioServiceScrollThumb = {thumbX, radioServiceScrollTrack.y, thumbW, radioServiceScrollTrack.h};\n        } else {\n            radioServiceScrollThumb = radioServiceScrollTrack;\n        }\n'''
if old_loop not in s:
    raise SystemExit('STOP: radio service loop anchor not found')
s = s.replace(old_loop, new_loop, 1)

old_panel = '        const int serviceRows=(serviceCount + serviceCols - 1) / serviceCols;\n        const int radioPanelY = presetY + serviceRows*(kCompactButtonH+7) + 10; // NOUGAT_V58_RADIO_SERVICE_GRID\n'
new_panel = '        const int radioPanelY = radioServiceScrollTrack.y + radioServiceScrollTrack.h + 10; // NOUGAT_V65_RADIO_SERVICE_LANE\n'
if old_panel not in s:
    raise SystemExit('STOP: radio panel anchor not found')
s = s.replace(old_panel, new_panel, 1)

# Visible scrollbar for the Radio mode lane.
anchor = '''        for (const auto& entry : radioServiceTabs) {\n            bool active = entry.second == radioSelectedService;\n            if (entry.second=="Favorites") active = radioPanel==RadioPanel::Favorites;\n            else if (entry.second=="Recordings") active = radioPanel==RadioPanel::Recordings;\n            else if (entry.second=="Cellular Lab") active = radioPanel==RadioPanel::Cellular;\n            button_on_state(target,entry.first,entry.second,\n                            active?SheetControlState::Hover:SheetControlState::Normal);\n        }\n\n'''
insert = anchor + '''        if (radioServiceScrollTrack.w > 0) {\n            fill(target, radioServiceScrollTrack, rgb8(7,20,14));\n            outline(target, radioServiceScrollTrack, rgb8(45,92,64));\n            fill(target, radioServiceScrollThumb, rgb8(59,121,83));\n            outline(target, radioServiceScrollThumb, rgb8(102,156,119));\n        }\n\n'''
if anchor not in s:
    raise SystemExit('STOP: radio draw anchor not found')
s = s.replace(anchor, insert, 1)

# Mouse wheel scrolls the Radio mode lane horizontally, consistent with other Nougat scroll lanes.
wheel_anchor = '''        if (currentView == ViewMode::Studio && target == win && studio_browser_active() && studioBrowserListRect.contains(x,y)) {\n'''
wheel_insert = '''        if (currentView == ViewMode::Radio && target == win && radioServiceStrip.contains(x,y)) {\n            radioServiceScrollX = std::max(0, std::min(radioServiceMaxScrollX,\n                radioServiceScrollX + (button == Button4 ? -180 : 180)));\n            layout();\n            redraw();\n            return true;\n        }\n''' + wheel_anchor
if wheel_anchor not in s:
    raise SystemExit('STOP: wheel handler anchor not found')
s = s.replace(wheel_anchor, wheel_insert, 1)

# Military seek renderer. The same function is used by regular/half-screen and fullscreen playback.
def replace_function(text: str, signature: str, replacement: str) -> str:
    start = text.find(signature)
    if start < 0:
        raise SystemExit(f'STOP: function not found: {signature}')
    brace = text.find('{', start)
    depth = 0
    i = brace
    in_string = False
    escape = False
    while i < len(text):
        c = text[i]
        if in_string:
            if escape:
                escape = False
            elif c == '\\\\':
                escape = True
            elif c == '"':
                in_string = False
        else:
            if c == '"':
                in_string = True
            elif c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    return text[:start] + replacement + text[i+1:]
        i += 1
    raise SystemExit(f'STOP: unmatched braces for {signature}')

seek = r'''    void draw_sheet_seek_frame(Drawable target, int percent) {
        // NOUGAT_V65_APPROVED_MILITARY_SEEK: exact design family approved by owner.
        percent = std::max(0, std::min(100, percent));
        const int trackH = 8;
        const int centerY = seekRect.y + seekRect.h / 2;
        const Rect track{seekRect.x, centerY - trackH / 2, seekRect.w, trackH};
        const unsigned long border = rgb8(42, 86, 61);
        const unsigned long unplayed = rgb8(7, 20, 14);
        const unsigned long played = rgb8(36, 91, 61);
        const unsigned long playedHi = rgb8(55, 116, 79);
        const unsigned long knobDark = rgb8(30, 70, 50);
        const unsigned long knobMid = rgb8(57, 112, 80);
        const unsigned long knobHi = rgb8(103, 153, 118);

        fill(target, track, unplayed);
        outline(target, track, border);

        const int knobW = 24;
        const int knobH = 28;
        const int travelStart = seekRect.x + knobW / 2;
        const int travelEnd = std::max(travelStart, seekRect.x + seekRect.w - knobW / 2);
        const int centerX = travelStart + static_cast<int>((static_cast<long long>(travelEnd - travelStart) * percent) / 100LL);
        const int filledW = std::max(0, std::min(seekRect.w, centerX - seekRect.x));
        if (filledW > 0) {
            Rect filled{seekRect.x, track.y, filledW, track.h};
            fill(target, filled, played);
            line(target, filled.x, filled.y + 1, filled.x + filled.w - 1, filled.y + 1, playedHi);
        }

        Rect knob{centerX - knobW / 2, centerY - knobH / 2, knobW, knobH};
        fill(target, knob, knobDark);
        outline(target, knob, knobHi);
        Rect inner{knob.x + 3, knob.y + 3, knob.w - 6, knob.h - 6};
        fill(target, inner, knobMid);
        line(target, inner.x + 1, inner.y + 1, inner.x + inner.w - 2, inner.y + 1, knobHi);
        line(target, inner.x + 1, inner.y + 1, inner.x + 1, inner.y + inner.h - 2, knobHi);
        line(target, inner.x + 1, inner.y + inner.h - 2, inner.x + inner.w - 2, inner.y + inner.h - 2, rgb8(25, 58, 42));
    }'''
s = replace_function(s, '    void draw_sheet_seek_frame(Drawable target, int percent)', seek)

# New renderer no longer depends on the legacy sheet asset, so both full and partial redraw paths use it.
s = s.replace('if (sheetSeekLoaded) {', 'if (true) {')

# Approved reference uses pale military timing text rather than the old black/caramel family.
s = s.replace('const unsigned long timingText = rgb8(0, 0, 0);', 'const unsigned long timingText = rgb8(206, 224, 213);')

p.write_text(s)
print('PASS: v65 source patch applied')
