#!/usr/bin/env python3
from pathlib import Path
import hashlib

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src/main.cpp"
HOST = ROOT / "src/games/emulator_host.cpp"
BIN = ROOT / "Nougat_Media_Suite_v54"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("FAIL: " + message)


def git_blob_sha(path: Path) -> str:
    data = path.read_bytes()
    return hashlib.sha1(f"blob {len(data)}\0".encode() + data).hexdigest()

s = MAIN.read_text(encoding="utf-8")
need('case ViewMode::Studio:      r=186; g=190; b=196; blendPercent=68' in s,
     "Studio quilt is not Silver Screen silver")
need('rgb8(184,188,194), rgb8(207,211,216)' in s,
     "Studio palette is not silver")
need('draw_studio_film_strip' in s and 'draw_studio_film_strip(target,{0,kTopBarH,W,kTopBarH}' in s,
     "full-width straight film-strip Studio header is missing")
need('section_text(target, 28, kTopBarH*2+22, "STUDIO"' in s,
     "Studio heading is not below the full-width film-strip header")
need('"Add File"' in s and 'StudioBrowserPurpose::SourceFile' in s,
     "Add File in-page browser is missing")
need('"Add Folder"' in s and 'StudioBrowserPurpose::SourceFolder' in s,
     "Add Folder in-page browser is missing")
need('"Add ZIP / Manifest"' in s and 'StudioBrowserPurpose::SourceZipManifest' in s,
     "Add ZIP / Manifest in-page browser is missing")
need('"Choose Location"' in s and 'StudioBrowserPurpose::OutputFolder' in s,
     "Choose Location in-page browser is missing")
need('draw_studio_browser' in s and 'Browse entirely inside Nougat.' in s,
     "in-page browser surface is missing")
need('studioBrowserListRect.contains(x,y)' in s,
     "browser list wheel scrolling is missing")
need('if (studio_browser_active()) { handle_studio_browser_click(x,y); return; }' in s,
     "browser click routing is missing")
need('if (studio_browser_active()) {' in s and 'if (ks == XK_Escape) { close_studio_browser(); return; }' in s,
     "browser keyboard close path is missing")
need(git_blob_sha(HOST) == '22f03639525a37997ef01a049b35641876399afc',
     "closed v0.0.53 Xbox emulator host changed")
need(BIN.is_file() and BIN.stat().st_size > 1_000_000,
     "production v54 executable is missing")
print('PASS: v0.0.54 Studio Silver Screen + in-page browser repair gates')
