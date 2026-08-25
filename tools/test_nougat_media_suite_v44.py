#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import os
import re
import subprocess
import sys
from pathlib import Path

BASE = "32028db74d0a2c2ea80850b0f86e7f40e72183b0"
EXPECTED_CONCEPT_SHA = "a37fb2dd309af0404c615c6f2519da748952d194e44d7b796bf7d46353e92e62"
EXPECTED_ICON_SHA = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"
EXPECTED_X11_SHA = "c626664598d57a3756a62a425875fd48567ae4eaa8c9b5a385ccf4630a0b22cb"
MESEN_SHA = "c88ff4d251b407515c43d3332d641927655cd69fb538996b6a21da4509dbb58f"
RMG_SHA = "43ce15e11404aaff313ec44ca03601e5e753bba3b355c38a7c67a4344d517aca"
ATARI800_SHA = "f4e11dcc6706591b630ba1647dcced52986507b55021a9ee24cad3f1e81c65f7"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}")
    raise SystemExit(1)


def ok(msg: str) -> None:
    print(f"PASS: {msg}")


def read(root: Path, rel: str) -> str:
    p = root / rel
    if not p.is_file():
        fail(f"missing required file: {rel}")
    return p.read_text(errors="strict")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def require(text: str, token: str, label: str) -> None:
    if token not in text:
        fail(f"{label}: missing {token!r}")


def forbid(text: str, token: str, label: str) -> None:
    if token in text:
        fail(f"{label}: forbidden token present: {token!r}")


def function_text(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        fail(f"function not found: {signature}")
    brace = text.find("{", start)
    if brace < 0:
        fail(f"function opening brace not found: {signature}")
    depth = 0
    i = brace
    in_string = in_char = escape = line_comment = block_comment = False
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if line_comment:
            if c == "\n":
                line_comment = False
        elif block_comment:
            if c == "*" and n == "/":
                block_comment = False
                i += 1
        elif in_string:
            if escape:
                escape = False
            elif c == "\\":
                escape = True
            elif c == '"':
                in_string = False
        elif in_char:
            if escape:
                escape = False
            elif c == "\\":
                escape = True
            elif c == "'":
                in_char = False
        else:
            if c == "/" and n == "/":
                line_comment = True
                i += 1
            elif c == "/" and n == "*":
                block_comment = True
                i += 1
            elif c == '"':
                in_string = True
            elif c == "'":
                in_char = True
            elif c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    return text[start:i + 1]
        i += 1
    fail(f"unterminated function: {signature}")
    return ""


def git_quiet(root: Path, *args: str) -> bool:
    return subprocess.run(["git", *args], cwd=root, stdout=subprocess.DEVNULL,
                          stderr=subprocess.DEVNULL).returncode == 0


def main() -> None:
    root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()
    print(f"=== NOUGAT MEDIA SUITE v0.0.44 REGRESSION ===\nProject: {root}")

    cmake = read(root, "CMakeLists.txt")
    require(cmake, "project(NougatMediaSuite VERSION 0.0.44 LANGUAGES CXX)", "CMake version")
    require(cmake, "add_executable(Nougat_Media_Suite_v44", "CMake target")
    require(cmake, "target_compile_options(Nougat_Media_Suite_v44 PRIVATE -Wall -Wextra -Werror)", "warnings-as-errors")
    forbid(cmake, "Nougat_Media_Suite_v43", "CMake stale target")
    ok("v0.0.44 CMake identity and warnings-as-errors")

    for rel in ["NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"]:
        desktop = read(root, rel)
        require(desktop, "Nougat_Media_Suite_v44", rel)
        require(desktop, "Icon=nougat-media-suite-concept-sheet-v24", rel)
        forbid(desktop, "Nougat_Media_Suite_v43", rel)
    ok("desktop launchers target v44 and preserve approved icon key")

    main_cpp = read(root, "src/main.cpp")
    require(main_cpp, "enum class ViewMode { Home, VideoPlayer, Library, Discover, LiveTV, WorldTV, Nougat, Stream, Studio, Games, P2P, Debug };", "World TV top-level view")
    require(main_cpp, 'draw_tab(liveTvTab,"Live TV",ViewMode::LiveTV);\n        draw_tab(worldTvTab,"World TV",ViewMode::WorldTV);', "World TV tab order")
    require(main_cpp, "const int topControlCount = 11;", "top nav count")
    require(main_cpp, "if (currentView == ViewMode::WorldTV) draw_world_tv_screen(buffer);", "World TV rendering")
    require(main_cpp, "handle_world_tv_click(x,y,eventTime);", "World TV input")
    ok("World TV is a top-level tab immediately after Live TV")

    catalog = function_text(main_cpp, "static const std::vector<WorldTvStation>& world_tv_catalog()")
    if catalog.count(".m3u8") < 8:
        fail("World TV catalog has fewer than eight direct HLS television sources")
    for bad in ["youtube.com", "youtu.be", "watch?v="]:
        forbid(catalog.lower(), bad, "World TV direct-source contract")
    for required in ["France 24 Français", "Al Jazeera Arabic", "Al Jazeera English", "VTV1", "TRANS7", "TVK", "EBS Kids", "CMC TV", "dwstream102"]:
        require(catalog, required, "World TV international catalog")
    forbid(catalog, "dwstream104", "DW English feed identity")
    forbid(catalog, "Al Jazeera Balkans", "retired Al Jazeera Balkans service")
    forbid(catalog, "live-hls-web-ajb", "retired AJB stream")
    forbid(catalog, "live-hls-web-aje", "stale Al Jazeera English endpoint")
    forbid(catalog, "live-hls-web-aja", "stale Al Jazeera Arabic endpoint")
    require(catalog, "live-hls-apps-aje-fa.getaj.net", "current Al Jazeera English endpoint")
    require(catalog, "live-hls-apps-aja-fa.getaj.net", "current Al Jazeera Arabic endpoint")
    ok("World TV catalog contains direct foreign linear feeds and no YouTube URLs")

    world_open = function_text(main_cpp, "bool open_world_tv_location(")
    for option in [":network-caching=1200", ":live-caching=1200", ":http-reconnect=true",
                   ":adaptive-maxheight=1080", ":preferred-resolution=1080"]:
        require(world_open, option, "World TV playback option")
    require(main_cpp, "void poll_world_tv_reconnect()", "World TV reconnect")
    require(main_cpp, "worldTvReconnectAttempts >= 3", "World TV bounded reconnect")
    ok("World TV playback has 1080p ceiling, low live cache, HTTP reconnect and bounded recovery")

    require(main_cpp, "const auto& stations=app.world_tv_catalog()", "v44 native self-test World TV member scope")
    forbid(main_cpp, "const auto stations=world_tv_catalog()", "v44 native self-test unqualified World TV catalog")
    ok("v44 native self-test references the World TV catalog through App scope")

    require(main_cpp, 'if (ends_with_lower(lower, ".gbc")) return "Game Boy Color";', "GBC identity")
    emu = function_text(main_cpp, "std::string installed_game_emulator(")
    require(emu, 'components/games/runtime/mesen2/Mesen', "bundled MesenCE runtime")
    require(emu, 'components/games/runtime/rmg/AppRun', "bundled RMG runtime")
    require(emu, 'components/games/runtime/atari800/AppRun', "bundled Atari800 runtime")
    require(emu, 'system == "Nintendo 64"', "bundled N64 runtime routing")
    for system in ["NES", "SNES", "Game Boy", "Game Boy Color", "Game Boy Advance"]:
        require(emu, system, "Mesen-supported system")
    launch = function_text(main_cpp, "void launch_selected_game()")
    require(launch, "execl(emulator.c_str()", "absolute emulator launch")
    require(launch, "execlp(emulator.c_str()", "PATH emulator launch")
    require(launch, "-5200", "Atari800 5200 launch mode")
    ok("Games prefers bundled MesenCE/RMG/Atari800 and launches bundled-path and PATH backends correctly")

    game_metrics = function_text(main_cpp, "LibraryGridMetrics games_grid_metrics() const")
    lib_metrics = function_text(main_cpp, "LibraryGridMetrics library_grid_metrics() const")
    # Both must carry the defining Library geometry equations, rather than a custom carousel.
    metric_tokens = [
        "metrics.gap=8", "target_width=150", "inner_height>=430",
        "two_row_tile_height", "two_row_poster_height", "two_row_poster_height*2/3",
        "metrics.posterHeight=std::max(1,metrics.tileWidth*3/2)",
        "metrics.tileHeight=metrics.posterHeight+50", "metrics.visibleItems=metrics.columns*metrics.rows",
    ]
    for token in metric_tokens:
        require(game_metrics.replace(" ", ""), token.replace(" ", ""), "Games Library-identical geometry")
    for token in ["target_width = 150", "prefer_two_rows", "metrics.posterHeight", "metrics.tileHeight"]:
        require(lib_metrics, token, "Library geometry authority")
    require(main_cpp, "gamesVerticalScrollTrack", "Games scrollbar")
    require(main_cpp, "handle_games_scrollbar_motion", "Games scrollbar drag")
    require(main_cpp, "draw_home_scrollbar_component(target,gamesVerticalScrollTrack,gamesVerticalScrollThumb,palette);", "Games exact scrollbar family")
    ok("Games uses the Library multi-row grid equations and vertical scrollbar behavior")

    for token in ["game_remote_artwork_url", "thumbnails.libretro.com", "Named_Boxarts",
                  "bundled_game_artwork_path", "request_game_remote_artwork", "poll_game_artwork_downloads"]:
        require(main_cpp, token, "Games artwork")
    ok("Games supports sidecar, bundled and cached remote artwork")

    for kind in ["Home", "Library", "Discover", "LiveTV", "WorldTV", "Games"]:
        require(main_cpp, f"CardContextKind::{kind}", "global card context menus")
    for action in ["CardPlay", "CardOpenSource", "CardInfo", "CardRefresh", "CardOpenOfficial",
                   "CardRefreshArtwork", "CardOpenArtwork"]:
        require(main_cpp, f"MenuAction::{action}", "card menu actions")
    require(main_cpp, "if (button == Button3 && target == win && show_card_context_menu(x,y)) return;", "global right-click dispatch")
    ok("all approved card surfaces have contextual right-click actions")

    readme = read(root, "README.md")
    if not readme.startswith("# Nougat Media Suite\n\n"):
        fail("README title must remain exactly '# Nougat Media Suite' with no version in title")
    v44 = readme.find("## v0.0.44 - World TV, Playable Games, Artwork, and Global Card Actions")
    v43 = readme.find("## v0.0.43 - Games, World TV, and Responsive Grid Repair")
    if not (v44 > 0 and v43 > v44):
        fail("README v44 section must be directly ahead of v43 history")
    require(readme[:v44], "World TV is now a dedicated top-level international television area", "README current feature intro")
    ok("README keeps approved title/structure, extends current features, and places v44 above v43")

    changelog = read(root, "CHANGELOG.md")
    if not changelog.startswith("# Changelog\n\n## v0.0.44"):
        fail("CHANGELOG v44 section is not first")
    roadmap = read(root, "ROADMAP.md")
    if not roadmap.startswith("## v0.0.44 candidate"):
        fail("ROADMAP v44 candidate section is not first")
    ok("README/CHANGELOG/ROADMAP agree on v0.0.44")

    gitignore = read(root, ".gitignore")
    require(gitignore, "components/games/runtime/", "Git runtime exclusion")
    mesen_note = read(root, "components/games/emulators/MESENCE_RUNTIME_SOURCE.md")
    require(mesen_note, "MesenCE 2.2.1", "MesenCE source record")
    require(mesen_note, MESEN_SHA, "MesenCE source record checksum")
    rmg_note = read(root, "components/games/emulators/RMG_RUNTIME_SOURCE.md")
    require(rmg_note, "RMG 0.9.0", "RMG source record")
    require(rmg_note, RMG_SHA, "RMG source record checksum")
    atari_note = read(root, "components/games/emulators/ATARI800_RUNTIME_SOURCE.md")
    require(atari_note, "Atari800 7.1.2", "Atari800 source record")
    require(atari_note, ATARI800_SHA, "Atari800 source record checksum")
    for rel in ["licenses/emulators/MESENCE_GPL-3.0.txt", "licenses/emulators/RMG_GPL-3.0.txt", "licenses/emulators/ATARI800_GPL-2.0.txt"]:
        if not (root / rel).is_file():
            fail(f"third-party emulator GPL-3.0 license copy missing: {rel}")
    ok("MesenCE, RMG, and Atari800 are scoped as separate generated third-party runtimes with license/source records")

    for rel, label in [("components/games/runtime/mesen2/Mesen", "MesenCE"),
                       ("components/games/runtime/rmg/AppRun", "RMG"),
                       ("components/games/runtime/atari800/AppRun", "Atari800")]:
        runtime = root / rel
        if not runtime.is_file() or not os.access(runtime, os.X_OK):
            fail(f"installed {label} runtime launcher is missing or not executable: {runtime}")
    ok("actual MesenCE, RMG, and Atari800 runtime launchers exist and are executable")

    artwork_dir = root / "components/games/bundled/artwork"
    for name in ["2048.png", "Waveforms.png"]:
        art = artwork_dir / name
        if not art.is_file():
            fail(f"bundled test-game artwork missing: {art}")
        data = art.read_bytes()
        if len(data) <= 8 or data[:8] != b"\x89PNG\r\n\x1a\n":
            fail(f"bundled test-game artwork is not a non-empty PNG: {art}")
    ok("both legally redistributable bundled NES test titles have real packaged artwork")

    concept = root / "docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png"
    icon = root / "assets/icons/nougat-media-suite-concept-sheet-v24.png"
    x11 = root / "src/nougat_media_suite_icon_data.hpp"
    for p, expected, label in [(concept, EXPECTED_CONCEPT_SHA, "approved concept sheet"),
                               (icon, EXPECTED_ICON_SHA, "approved N master"),
                               (x11, EXPECTED_X11_SHA, "embedded X11 icon")]:
        if not p.is_file():
            fail(f"{label} missing: {p}")
        got = sha256(p)
        if got != expected:
            fail(f"{label} SHA-256 changed: {got}")
    ok("permanent exact Nougat N/icon authority hashes are unchanged")

    protected = ["LICENSE", "COPYRIGHT.md", "CONTRIBUTING.md", "THIRD_PARTY_NOTICES.md", "docs/LICENSING_POLICY.md"]
    if not git_quiet(root, "diff", "--quiet", BASE, "--", *protected):
        fail("protected project licensing files changed without owner approval")
    ok("protected project licensing boundary is unchanged")

    if len(sys.argv) > 2:
        exe = Path(sys.argv[2]).resolve()
        if not exe.is_file() or not os.access(exe, os.X_OK):
            fail(f"native executable missing or not executable: {exe}")
        try:
            version = subprocess.run([str(exe), "--version"], capture_output=True, text=True, timeout=15)
        except subprocess.TimeoutExpired:
            fail("native --version timed out")
        if version.returncode != 0 or (version.stdout + version.stderr).strip() != "Nougat Media Suite v0.0.44":
            fail(f"native version identity failed: rc={version.returncode} output={(version.stdout + version.stderr).strip()!r}")
        try:
            selftest = subprocess.run([str(exe), "--v44-release-self-test"], capture_output=True, text=True, timeout=45)
        except subprocess.TimeoutExpired:
            fail("native v44 release self-test timed out")
        if selftest.returncode != 0:
            fail(f"native v44 release self-test failed: rc={selftest.returncode} output={(selftest.stdout + selftest.stderr)[-3000:]!r}")
        ok("native executable identity and v44 release self-test")

    print("=== v0.0.44 REGRESSION PASS ===")


if __name__ == "__main__":
    main()
