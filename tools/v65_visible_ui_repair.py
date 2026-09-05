#!/usr/bin/env python3
from pathlib import Path

p = Path('src/main.cpp')
s = p.read_text()

if 'NOUGAT_V65_APPROVED_MILITARY_SEEK' not in s:
    raise SystemExit('STOP: approved v65 seek bar marker missing')


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
# OWNER-APPROVED HYBRID GRAMMAR, made visually unmistakable.
# Filled green-glass remains the navigation/mode path. Direct commands are
# genuinely hollow angular outlines with no filled center.
# ---------------------------------------------------------------------------
command_button = r'''    void button_on(Drawable target, const Rect& r, const std::string& label) {
        // NOUGAT_V65_VISIBLE_OUTLINED_COMMAND
        const bool hover = target == win && r.contains(pointerWindowX, pointerWindowY);
        if (r.w <= 4 || r.h <= 6) return;

        const unsigned long outer = hover ? rgb8(101,184,129) : rgb8(35,105,69);
        const unsigned long inner = hover ? rgb8(58,137,91) : rgb8(15,65,43);
        const unsigned long rail  = hover ? rgb8(79,159,108) : rgb8(25,86,57);
        const unsigned long ink   = hover ? rgb8(219,239,226) : rgb8(166,202,179);

        const Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        outline_tactical_polygon(target, visual, outer, 8);
        const Rect inset{visual.x + 4, visual.y + 4,
                         std::max(1, visual.w - 8), std::max(1, visual.h - 8)};
        outline_tactical_polygon(target, inset, inner, 5);
        line(target, visual.x + 10, visual.y + 2,
             visual.x + std::max(11, visual.w / 3), visual.y + 2, rail);
        line(target, visual.x + visual.w - std::max(11, visual.w / 3), visual.y + visual.h - 3,
             visual.x + visual.w - 10, visual.y + visual.h - 3, rail);

        const int label_x = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        const int label_y = visual.y + visual.h / 2 + 5;
        text(target, label_x, label_y, head_to_width(label, visual.w - 10), ink);
    }'''
s = replace_function(s,
    '    void button_on(Drawable target, const Rect& r, const std::string& label)',
    command_button)

stateful_button = r'''    void command_button_state(Drawable target, const Rect& r, const std::string& label,
                              SheetControlState state) {
        // NOUGAT_V65_VISIBLE_STATEFUL_OUTLINED_COMMAND
        const bool pointerHover = target == win && r.contains(pointerWindowX, pointerWindowY);
        const bool active = state == SheetControlState::Hover || state == SheetControlState::Pressed;
        const bool hot = pointerHover || active;
        if (r.w <= 4 || r.h <= 6) return;

        const unsigned long outer = active ? rgb8(123,203,148)
            : (pointerHover ? rgb8(101,184,129) : rgb8(35,105,69));
        const unsigned long inner = active ? rgb8(70,151,101)
            : (pointerHover ? rgb8(58,137,91) : rgb8(15,65,43));
        const unsigned long rail = active ? rgb8(102,184,128)
            : (pointerHover ? rgb8(79,159,108) : rgb8(25,86,57));
        const unsigned long ink = hot ? rgb8(226,243,232) : rgb8(166,202,179);

        const Rect visual{r.x + 2, r.y + 1, std::max(1, r.w - 4), std::max(1, r.h - 4)};
        outline_tactical_polygon(target, visual, outer, 8);
        const Rect inset{visual.x + 4, visual.y + 4,
                         std::max(1, visual.w - 8), std::max(1, visual.h - 8)};
        outline_tactical_polygon(target, inset, inner, 5);
        line(target, visual.x + 10, visual.y + 2,
             visual.x + std::max(11, visual.w / 3), visual.y + 2, rail);
        if (active) {
            line(target, visual.x + visual.w - std::max(11, visual.w / 3), visual.y + visual.h - 3,
                 visual.x + visual.w - 10, visual.y + visual.h - 3, rail);
        }

        const int label_x = visual.x + std::max(5, (visual.w - text_width(label)) / 2);
        const int label_y = visual.y + visual.h / 2 + 5;
        text(target, label_x, label_y, head_to_width(label, visual.w - 10), ink);
    }'''
s = replace_function(s,
    '    void command_button_state(Drawable target, const Rect& r, const std::string& label,',
    stateful_button)

# Radio services are one continuous filled green-glass selector lane.
if 'const int serviceRows = 2;' in s:
    s = s.replace('const int serviceRows = 2;', 'const int serviceRows = 1; // NOUGAT_V65_VISIBLE_ONE_ROW_RADIO_MODES', 1)
elif 'const int serviceRows = 1;' not in s and 'NOUGAT_V65_VISIBLE_ONE_ROW_RADIO_MODES' not in s:
    raise SystemExit('STOP: Radio service row anchor not found')
s = s.replace('// NOUGAT_V65_RADIO_SERVICE_LANE: two-row mode lane, horizontally scrollable.',
              '// NOUGAT_V65_RADIO_SERVICE_LANE: one continuous filled mode lane, horizontally scrollable.')

# Direct Radio actions stay outlined even while active.
radio_replacements = {
    'button_on_state(target,radioListenBtn,state.receiving?"LISTENING":"LISTEN",state.receiving?SheetControlState::Hover:SheetControlState::Normal);':
        'command_button_state(target,radioListenBtn,state.receiving?"LISTENING":"LISTEN",state.receiving?SheetControlState::Hover:SheetControlState::Normal);',
    'button_on_state(target,radioScanBtn,state.scanning?"SCANNING":"SCAN",state.scanning?SheetControlState::Hover:SheetControlState::Normal);':
        'command_button_state(target,radioScanBtn,state.scanning?"SCANNING":"SCAN",state.scanning?SheetControlState::Hover:SheetControlState::Normal);',
    'button_on_state(target,radioRecordBtn,state.recording?"RECORDING":"RECORD",state.recording?SheetControlState::Hover:SheetControlState::Normal);':
        'command_button_state(target,radioRecordBtn,state.recording?"RECORDING":"RECORD",state.recording?SheetControlState::Hover:SheetControlState::Normal);',
}
for old, new in radio_replacements.items():
    s = s.replace(old, new)

# TV antenna scanning belongs to Live TV only.
s = s.replace('"Favorites", "Recordings", "TV Antenna Scan", "ISS / Sat"',
              '"Favorites", "Recordings", "ISS / Sat"')
s = s.replace(' || label=="TV Antenna Scan"', '')

# Composite the in-app brand against the tactical header, not the old tan backing.
s = s.replace('draw_suite_brand(target,4,brandY,227,204,172);',
              'draw_suite_brand(target,4,brandY,2,15,11); // NOUGAT_V65_VISIBLE_BRAND_BACKGROUND_CLEANUP')

required = [
    'NOUGAT_V65_VISIBLE_OUTLINED_COMMAND',
    'NOUGAT_V65_VISIBLE_STATEFUL_OUTLINED_COMMAND',
    'NOUGAT_V65_VISIBLE_ONE_ROW_RADIO_MODES',
    'NOUGAT_V65_SYSTEM_COMMAND_LANE',
    'command_button_state(target,radioListenBtn',
    'command_button_state(target,radioScanBtn',
    'command_button_state(target,radioRecordBtn',
]
for marker in required:
    if marker not in s:
        raise SystemExit(f'STOP: visible v65 UI requirement missing: {marker}')

# Refuse to package if either command renderer contains a filled face.
for sig in (
    '    void button_on(Drawable target, const Rect& r, const std::string& label)',
    '    void command_button_state(Drawable target, const Rect& r, const std::string& label,',
):
    start = s.find(sig)
    brace = s.find('{', start)
    depth = 0
    i = brace
    while i < len(s):
        if s[i] == '{': depth += 1
        elif s[i] == '}':
            depth -= 1
            if depth == 0:
                body = s[start:i+1]
                break
        i += 1
    if 'draw_tactical_polygon(target' in body or 'fill(target' in body or 'fill_round(target' in body:
        raise SystemExit(f'STOP: outlined command renderer still contains a filled face: {sig}')

p.write_text(s)
print('PASS: v65 visible hybrid UI repair applied; seek bar untouched')
