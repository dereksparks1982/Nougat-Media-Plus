#!/usr/bin/env python3
"""Apply the v0.0.50 optional-plugin foundation after the core v50 migration.

Workshop is the first reference plugin. This patch removes executable-relative
Workshop resource guessing, hides the Workshop tab when the plugin is absent,
and resolves Workshop resources through Nougat's managed plugin path API.
"""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
MARKER = "NOUGAT_V50_PLUGIN_FOUNDATION"


def fail(message: str) -> None:
    raise RuntimeError(message)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        fail(f"{label}: expected exactly 1 match, found {count}")
    return text.replace(old, new, 1)


def validate(text: str) -> None:
    required = [
        MARKER,
        'nougat::paths::plugin_installed("workshop")',
        'nougat::paths::plugin_resource("workshop", "nougat_split_archive.py")',
        'draw_tab(studioTab,"Workshop",ViewMode::Studio);',
    ]
    missing = [token for token in required if token not in text]
    if missing:
        fail("plugin foundation missing: " + ", ".join(missing))

    forbidden = [
        'std::filesystem::path(exe_dir()) /\n            "components" / "workshop" / "nougat_split_archive.py"',
        'std::filesystem::path(exe_dir()).parent_path() /\n            "components" / "workshop" / "nougat_split_archive.py"',
    ]
    present = [token for token in forbidden if token in text]
    if present:
        fail("plugin foundation retains executable-relative Workshop path guessing")


def main() -> int:
    try:
        text = MAIN.read_text(encoding="utf-8")
        if MARKER in text:
            validate(text)
            print("PASS: v0.0.50 plugin foundation already applied and valid")
            return 0

        old_nav = 'draw_tab(studioTab,"Workshop",ViewMode::Studio);'
        new_nav = (
            f'// {MARKER}\n'
            '        if (nougat::paths::plugin_installed("workshop")) {\n'
            '            draw_tab(studioTab,"Workshop",ViewMode::Studio);\n'
            '        } else {\n'
            '            studioTab = {};\n'
            '        }'
        )
        text = replace_once(text, old_nav, new_nav, "Workshop optional tab")

        start = text.find('    std::string workshop_worker_script() const {')
        end = text.find('    static std::string workshop_bytes_label', start)
        if start < 0 or end < 0:
            fail("Workshop worker resolver seam not found after v50 core patch")

        resolver = '''    std::string workshop_worker_script() const {\n        return nougat::paths::plugin_resource("workshop", "nougat_split_archive.py").string();\n    }\n\n'''
        text = text[:start] + resolver + text[end:]

        # A stale session must never expose an uninstalled plugin. If Workshop
        # somehow becomes the current view after removal, render the Player and
        # return the UI to Player state immediately.
        old_draw = '''    void draw_studio_screen(Drawable target) {\n        const ViewPalette palette = palette_for(ViewMode::Studio);'''
        new_draw = '''    void draw_studio_screen(Drawable target) {\n        if (!nougat::paths::plugin_installed("workshop")) {\n            currentView = ViewMode::Player;\n            draw_player_screen(target);\n            return;\n        }\n        const ViewPalette palette = palette_for(ViewMode::Studio);'''
        text = replace_once(text, old_draw, new_draw, "Workshop stale-session guard")

        MAIN.write_text(text, encoding="utf-8")
        validate(text)
        print("PASS: applied v0.0.50 player-core optional-plugin foundation")
        print("PASS: Workshop resource lookup is managed-plugin-only")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
