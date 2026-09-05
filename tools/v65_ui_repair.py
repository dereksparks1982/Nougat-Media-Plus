#!/usr/bin/env python3
from pathlib import Path

p = Path('src/main.cpp')
s = p.read_text()

if 'NOUGAT_V65_APPROVED_MILITARY_SEEK' not in s:
    raise SystemExit('STOP: approved v65 seek bar is missing; refusing to touch player seek code')


def replace_function(text: str, signature: str, replacement: str) -> str:
    start = text.find(signature)
    if start < 0:
        raise SystemExit(f'STOP: function not found: {signature}')
    brace = text.find('{', start)
    if brace < 0:
        raise SystemExit(f'STOP: opening brace not found: {signature}')
    depth = 0
    in_string = False
    escape = False
    i = brace
    while i < len(text):
        c = text[i]
        if in_string:
            if escape:
                escape = False
            elif c == '\\':
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
                    return text[:start] + replacement + text[i + 1:]
        i += 1
    raise SystemExit(f'STOP: unmatched braces: {signature}')


# ---------------------------------------------------------------------------
# v65 hybrid UI grammar
# Filled green-glass = navigation/mode state.
# Angular outlined = command/action.
# `button_on_state` remains the filled/stateful control path.
# `button_on` becomes the system-wide outlined action path.
# ---------------------------------------------------------------------------
command_button = r'''    void button_on(Drawable target, const Rect& r, const std::string& label) {
        // NOUGAT_V65_HYBRID_COMMAND_BUTTON
        const ViewPalette palette = currentView == ViewMode::Stream
            ? stream_palette_for(streamPlatform) : palette_for(currentView);
        const bool hover = target == win && r.contains(pointerWindowX, pointerWindowY);
        const unsigned long face = hover ? rgb8(5,31,22) : rgb8(2,18,13);
        const unsigned long edge = hover ? rgb8(109,226,147) : rgb8(27,126,76);
        const unsigned long innerEdge = hover ? rgb8(60,178,105) : rgb8(11,77,47);
        const unsigned long ink = hover ? rgb8(232,247,238) : palette.text;
        if (r.w <= 4 || r.h <= 6) return;
        Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        draw_tactical_polygon(target, {visual.x + 1, visual.y + 3, visual.w, visual.h},
                              rgb8(0,8,6), rgb8(0,8,6), 8);
        draw_tactical_polygon(target, visual, face, edge, 8);
        Rect inner{visual.x + 4, visual.y + 4, std::max(1, visual.w - 8), std::max(1, visual.h - 8)};
        outline_tactical_polygon(target, inner, innerEdge, 5);
        line(target, visual.x + 10, visual.y + 2, visual.x + visual.w - 7, visual.y + 2,
             hover ? rgb8(104,220,142) : rgb8(39,137,83));
        const int label_x = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        const int label_y = visual.y + visual.h / 2 + 5;
        text(target, label_x, label_y, head_to_width(label, visual.w - 10), ink);
    }'''
s = replace_function(s, '    void button_on(Drawable target, const Rect& r, const std::string& label)', command_button)

# Stateful action variant for actions such as LISTEN/SCAN/RECORD. It deliberately
# keeps the outlined geometry while allowing a stronger active edge.
stateful_anchor = '''    void button_on_state(Drawable target, const Rect& r, const std::string& label,\n                         SheetControlState state) {'''
idx = s.find(stateful_anchor)
if idx < 0:
    raise SystemExit('STOP: button_on_state anchor not found')
# Insert after the complete existing function.
brace = s.find('{', idx)
depth = 0
i = brace
while i < len(s):
    if s[i] == '{': depth += 1
    elif s[i] == '}':
        depth -= 1
        if depth == 0:
            end = i + 1
            break
    i += 1
else:
    raise SystemExit('STOP: button_on_state braces malformed')

if 'NOUGAT_V65_STATEFUL_COMMAND_BUTTON' not in s:
    helper = r'''

    void command_button_state(Drawable target, const Rect& r, const std::string& label,
                              SheetControlState state) {
        // NOUGAT_V65_STATEFUL_COMMAND_BUTTON
        const ViewPalette palette = currentView == ViewMode::Stream
            ? stream_palette_for(streamPlatform) : palette_for(currentView);
        const bool pointerHover = target == win && r.contains(pointerWindowX, pointerWindowY);
        const bool active = state == SheetControlState::Hover || state == SheetControlState::Pressed;
        const bool hot = pointerHover || active;
        const unsigned long face = active ? rgb8(5,34,23) : (pointerHover ? rgb8(5,30,21) : rgb8(2,18,13));
        const unsigned long edge = active ? rgb8(118,238,156) : (pointerHover ? rgb8(99,211,137) : rgb8(27,126,76));
        const unsigned long innerEdge = active ? rgb8(55,171,100) : rgb8(11,77,47);
        const unsigned long ink = hot ? rgb8(235,249,240) : palette.text;
        if (r.w <= 4 || r.h <= 6) return;
        Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        draw_tactical_polygon(target, {visual.x + 1, visual.y + 3, visual.w, visual.h},
                              rgb8(0,8,6), rgb8(0,8,6), 8);
        draw_tactical_polygon(target, visual, face, edge, 8);
        Rect inner{visual.x + 4, visual.y + 4, std::max(1, visual.w - 8), std::max(1, visual.h - 8)};
        outline_tactical_polygon(target, inner, innerEdge, 5);
        if (active)
            line(target, visual.x + 9, visual.y + 2, visual.x + visual.w - 7, visual.y + 2, rgb8(103,225,144));
        const int label_x = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        const int label_y = visual.y + visual.h / 2 + 5;
        text(target, label_x, label_y, head_to_width(label, visual.w - 10), ink);
    }'''
    s = s[:end] + helper + s[end:]

# Radio services stay filled/stateful mode buttons, while its direct receiver
# actions use the outlined command grammar even while active.
s = s.replace('button_on_state(target,radioListenBtn,state.receiving?"LISTENING":"LISTEN",state.receiving?SheetControlState::Hover:SheetControlState::Normal);',
              'command_button_state(target,radioListenBtn,state.receiving?"LISTENING":"LISTEN",state.receiving?SheetControlState::Hover:SheetControlState::Normal);')
s = s.replace('button_on_state(target,radioScanBtn,state.scanning?"SCANNING":"SCAN",state.scanning?SheetControlState::Hover:SheetControlState::Normal);',
              'command_button_state(target,radioScanBtn,state.scanning?"SCANNING":"SCAN",state.scanning?SheetControlState::Hover:SheetControlState::Normal);')
s = s.replace('button_on_state(target,radioRecordBtn,state.recording?"RECORDING":"RECORD",state.recording?SheetControlState::Hover:SheetControlState::Normal);',
              'command_button_state(target,radioRecordBtn,state.recording?"RECORDING":"RECORD",state.recording?SheetControlState::Hover:SheetControlState::Normal);')

# Remove the TV antenna service from Radio for real. The earlier v65 patch moved
# the hitbox but its label could survive because of line formatting.
s = s.replace('"Favorites", "Recordings", "TV Antenna Scan", "ISS / Sat"',
              '"Favorites", "Recordings", "ISS / Sat"')
s = s.replace(' || label=="Recordings" || label=="TV Antenna Scan") break;',
              ' || label=="Recordings") break;')

# ---------------------------------------------------------------------------
# System command lane: preserve the long command set, make it visibly and
# actively horizontally scrollable instead of shrinking/cramming buttons.
# ---------------------------------------------------------------------------
state_anchor = '    Rect serverStartBtn, serverStopBtn, serverRefreshBtn, securitySystemBtn;\n'
if 'systemCommandScrollTrack' not in s:
    if state_anchor not in s:
        raise SystemExit('STOP: System state anchor not found')
    s = s.replace(state_anchor, state_anchor +
        '    Rect systemCommandStrip, systemCommandScrollTrack, systemCommandScrollThumb;\n'
        '    int systemCommandMaxScrollX = 0;\n', 1)

layout_old = '''        layout_button_row({&serverStartBtn,&serverStopBtn,&serverRefreshBtn,&securitySystemBtn,\n                           &debugRunBtn,&debugRetryBtn,&debugMetadataBtn,&debugTmdbBtn,&debugLogsBtn,&debugCopyBtn,\n                           &debugExportTextBtn,&debugExportJsonBtn,&debugBundleBtn},\n                          kPageControlY, debugButtonsScrollX);\n        debugListBox = {28, 126, std::max(240, W-56), std::max(150, H-154)};\n'''
layout_new = '''        // NOUGAT_V65_SYSTEM_COMMAND_LANE: outlined actions with visible horizontal scroll.\n        systemCommandStrip = {28, kPageControlY, std::max(1, W-56), kCompactButtonH};\n        constexpr int kSystemCommandCount = 13;\n        const int systemVirtualW = kSystemCommandCount * kCompactButtonW;\n        systemCommandMaxScrollX = std::max(0, systemVirtualW - systemCommandStrip.w);\n        debugButtonsScrollX = std::max(0, std::min(systemCommandMaxScrollX, debugButtonsScrollX));\n        layout_button_row({&serverStartBtn,&serverStopBtn,&serverRefreshBtn,&securitySystemBtn,\n                           &debugRunBtn,&debugRetryBtn,&debugMetadataBtn,&debugTmdbBtn,&debugLogsBtn,&debugCopyBtn,\n                           &debugExportTextBtn,&debugExportJsonBtn,&debugBundleBtn},\n                          kPageControlY, debugButtonsScrollX);\n        systemCommandScrollTrack = {systemCommandStrip.x, systemCommandStrip.y + systemCommandStrip.h + 4,\n                                    systemCommandStrip.w, 6};\n        if (systemCommandMaxScrollX > 0) {\n            const int thumbW = std::max(36, systemCommandStrip.w * systemCommandStrip.w / std::max(1, systemVirtualW));\n            const int travel = std::max(0, systemCommandStrip.w - thumbW);\n            const int thumbX = systemCommandStrip.x + debugButtonsScrollX * travel / systemCommandMaxScrollX;\n            systemCommandScrollThumb = {thumbX, systemCommandScrollTrack.y, thumbW, systemCommandScrollTrack.h};\n        } else {\n            systemCommandScrollThumb = systemCommandScrollTrack;\n        }\n        debugListBox = {28, 126, std::max(240, W-56), std::max(150, H-154)};\n'''
if 'NOUGAT_V65_SYSTEM_COMMAND_LANE' not in s:
    if layout_old not in s:
        raise SystemExit('STOP: System layout anchor not found')
    s = s.replace(layout_old, layout_new, 1)

draw_anchor = '        button_on(target, securitySystemBtn, systemVirusScanMode ? "Back to System" : "Virus Scan");\n'
if 'NOUGAT_V65_SYSTEM_SCROLL_DRAW' not in s:
    if draw_anchor not in s:
        raise SystemExit('STOP: System draw anchor not found')
    draw_insert = draw_anchor + '''        // NOUGAT_V65_SYSTEM_SCROLL_DRAW\n        if (!systemVirusScanMode && systemCommandMaxScrollX > 0) {\n            fill(target, systemCommandScrollTrack, rgb8(3,20,14));\n            outline(target, systemCommandScrollTrack, rgb8(29,102,64));\n            fill(target, systemCommandScrollThumb, rgb8(47,112,76));\n            outline(target, systemCommandScrollThumb, rgb8(97,158,116));\n        }\n'''
    s = s.replace(draw_anchor, draw_insert, 1)

wheel_anchor = '        if (currentView == ViewMode::Radio && target == win && radioServiceStrip.contains(x,y)) {\n'
if 'NOUGAT_V65_SYSTEM_SCROLL_WHEEL' not in s:
    if wheel_anchor not in s:
        raise SystemExit('STOP: wheel insertion anchor not found')
    wheel = '''        // NOUGAT_V65_SYSTEM_SCROLL_WHEEL\n        if (currentView == ViewMode::Debug && target == win && systemCommandStrip.contains(x,y)) {\n            scroll_button_row(debugButtonsScrollX, 13, delta, systemCommandStrip.w);\n            layout();\n            redraw();\n            return true;\n        }\n''' + wheel_anchor
    s = s.replace(wheel_anchor, wheel, 1)

# ---------------------------------------------------------------------------
# Dock/sidebar icon cleanup.
# Remove only edge-connected near-black pixels from the embedded WM icon. The
# green rim/strip remains intact and blocks the flood fill from eating the N.
# ---------------------------------------------------------------------------
icon_func = r'''    void append_net_wm_icon(std::vector<unsigned long>& data, int size, const std::uint32_t* pixels) {
        // NOUGAT_V65_DOCK_ICON_BORDER_CLEANUP
        data.push_back(static_cast<unsigned long>(size));
        data.push_back(static_cast<unsigned long>(size));
        const int count = size * size;
        std::vector<unsigned char> exterior(static_cast<std::size_t>(count), 0U);
        std::vector<int> stack;
        stack.reserve(static_cast<std::size_t>(count));
        const auto is_edge_black = [&](int idx) {
            const std::uint32_t argb = pixels[idx];
            const unsigned a = (argb >> 24) & 0xffU;
            const unsigned r = (argb >> 16) & 0xffU;
            const unsigned g = (argb >> 8) & 0xffU;
            const unsigned b = argb & 0xffU;
            return a != 0U && r <= 38U && g <= 38U && b <= 38U;
        };
        const auto seed = [&](int idx) {
            if (idx < 0 || idx >= count || exterior[static_cast<std::size_t>(idx)] != 0U || !is_edge_black(idx)) return;
            exterior[static_cast<std::size_t>(idx)] = 1U;
            stack.push_back(idx);
        };
        for (int x=0; x<size; ++x) { seed(x); seed((size-1)*size+x); }
        for (int y=0; y<size; ++y) { seed(y*size); seed(y*size+size-1); }
        while (!stack.empty()) {
            const int idx = stack.back(); stack.pop_back();
            const int x = idx % size, y = idx / size;
            if (x > 0) seed(idx-1);
            if (x+1 < size) seed(idx+1);
            if (y > 0) seed(idx-size);
            if (y+1 < size) seed(idx+size);
        }
        for (int i=0; i<count; ++i) {
            std::uint32_t argb = pixels[i];
            if (exterior[static_cast<std::size_t>(i)] != 0U) argb &= 0x00ffffffU;
            data.push_back(static_cast<unsigned long>(argb));
        }
    }'''
s = replace_function(s, '    void append_net_wm_icon(std::vector<unsigned long>& data, int size, const std::uint32_t* pixels)', icon_func)

# The top-bar brand alpha must composite against the actual military header,
# never the old tan Nougat background.
s = s.replace('draw_suite_brand(target, 4, brandY, 227, 204, 172);',
              'draw_suite_brand(target, 4, brandY, 2, 15, 11); // NOUGAT_V65_BRAND_GREEN_BACKGROUND')

p.write_text(s)
print('PASS: v65 approved hybrid UI repair applied; seek bar untouched')
