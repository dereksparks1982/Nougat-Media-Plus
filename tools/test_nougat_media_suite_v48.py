#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]


def fail(msg: str) -> None:
    print("FAIL:", msg)
    raise SystemExit(1)


def ok(msg: str) -> None:
    print("PASS:", msg)


def need(path: Path) -> str:
    if not path.is_file():
        fail("missing " + str(path.relative_to(ROOT)))
    return path.read_text(encoding="utf-8")


def require(text: str, token: str, label: str) -> None:
    if token not in text:
        fail(f"{label}: missing {token!r}")


def function_body(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        fail("missing function " + signature)

    # Find the real function body, not a brace that appears inside a default
    # argument such as: const std::string& archive_path = {}.
    open_paren = text.find("(", start)
    if open_paren < 0:
        fail("missing function parameter list " + signature)

    paren_depth = 0
    state = "code"
    i = open_paren
    close_paren = -1

    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""

        if state == "code":
            if c == "/" and n == "/":
                state = "line"
                i += 2
                continue
            if c == "/" and n == "*":
                state = "block"
                i += 2
                continue
            if c == '"':
                state = "string"
                i += 1
                continue
            if c == "'":
                state = "char"
                i += 1
                continue
            if c == "(":
                paren_depth += 1
            elif c == ")":
                paren_depth -= 1
                if paren_depth == 0:
                    close_paren = i
                    break
            i += 1
            continue

        if state == "line":
            if c == "\n":
                state = "code"
            i += 1
            continue

        if state == "block":
            if c == "*" and n == "/":
                state = "code"
                i += 2
            else:
                i += 1
            continue

        if c == "\\":
            i += 2
            continue
        if (state == "string" and c == '"') or (state == "char" and c == "'"):
            state = "code"
        i += 1

    if close_paren < 0:
        fail("unterminated function parameter list " + signature)

    brace = text.find("{", close_paren + 1)
    if brace < 0:
        fail("missing function body " + signature)

    depth = 0
    state = "code"
    i = brace
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""

        if state == "code":
            if c == "/" and n == "/":
                state = "line"
                i += 2
                continue
            if c == "/" and n == "*":
                state = "block"
                i += 2
                continue
            if c == '"':
                state = "string"
                i += 1
                continue
            if c == "'":
                state = "char"
                i += 1
                continue
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    return text[start:i + 1]
            i += 1
            continue

        if state == "line":
            if c == "\n":
                state = "code"
            i += 1
            continue

        if state == "block":
            if c == "*" and n == "/":
                state = "code"
                i += 2
            else:
                i += 1
            continue

        if c == "\\":
            i += 2
            continue
        if (state == "string" and c == '"') or (state == "char" and c == "'"):
            state = "code"
        i += 1

    fail("unterminated function " + signature)

def compile_host() -> None:
    compiler = shutil.which("g++")
    if not compiler:
        fail("g++ is required")
    with tempfile.TemporaryDirectory(prefix="nougat-v48-host-") as td:
        out = Path(td) / "emulator_host.o"
        r = subprocess.run(
            [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Isrc",
             "-c", "src/games/emulator_host.cpp", "-o", str(out)],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )
        if r.returncode != 0:
            fail("embedded emulator host compile failed:\n" + r.stdout)
    ok("embedded X11 emulator host compiles warning-free")


def parser_self_test() -> None:
    sample = r"""
static bool safe_zip_game_entry(const std::string& entry,
                                const std::string& archive_path = {}) {
    const std::string system = "NES";
    return !system.empty() && system != "Xbox 360";
}
"""
    body = function_body(sample, "static bool safe_zip_game_entry(")
    require(body, 'system != "Xbox 360"', "validator parser self-test")
    if "archive_path = {}" not in body:
        fail("validator parser self-test did not retain default argument")


def main() -> None:
    parser_self_test()
    print("=== NOUGAT MEDIA SUITE v0.0.48 DOS + XBOX 360 CONTRACT ===")
    main_cpp = need(ROOT / "src/main.cpp")
    cmake = need(ROOT / "CMakeLists.txt")
    host_h = need(ROOT / "src/games/emulator_host.hpp")
    host_cpp = need(ROOT / "src/games/emulator_host.cpp")
    runtime_installer = need(ROOT / "tools/install_game_runtimes_v48.py")

    for token in [
        "project(NougatMediaSuite VERSION 0.0.48 LANGUAGES CXX)",
        "add_executable(Nougat_Media_Suite_v48",
        "src/games/emulator_host.cpp",
        "target_compile_options(Nougat_Media_Suite_v48 PRIVATE -Wall -Wextra -Werror)",
    ]:
        require(cmake, token, "CMake v48")
    ok("v48 target and emulator host are wired with warnings-as-errors")

    for token in [
        '#include "games/emulator_host.hpp"',
        'return "Xbox 360";',
        'game.system = "DOS";',
        "scan_dos_game_directories",
        "NOUGAT_DOSBOX",
        "dosbox-staging",
        "NOUGAT_XENIA",
        "NOUGAT_XENIA_RUNNER",
        "xenia_canary",
        "gameHost.start",
        "poll_game_session",
        "gameHost.resize",
        "Nougat Media Suite v0.0.48",
    ]:
        require(main_cpp, token, "main v48")
    ok("DOS directory discovery and Xbox 360 launch paths are present")

    for token in [
        'folder / "NOU_LAUNCH.BAT"',
        '"--noprimaryconf"',
        '"--nolocalconf"',
        '"nougat-dosbox.conf"',
        'dos_command = "call " + selected.entry_point',
        "is_preservation_container",
        '"fullscreen=off"',
        '"output=texture"',
    ]:
        require(main_cpp, token, "repaired DOS runtime")
    if "dosbox_quote(selected.entry_point)" in main_cpp:
        fail("DOS launcher still wraps the entire command in quotes")
    if '"-noprimaryconf"' in main_cpp or '"-nolocalconf"' in main_cpp:
        fail("DOSBox Staging still uses obsolete single-dash config switches")
    ok("DOSBox Staging launch contract and single-root DOS packaging are repaired")

    for token in [
        "XReparentWindow", "_NET_WM_PID", "WaitingForWindow", "Embedded",
        "refused to fall back to a separate desktop window",
    ]:
        require(host_cpp, token, "embedded emulator host")
    require(host_h, "class EmulatorHost", "embedded emulator host header")
    ok("host reparents emulator windows into Nougat and has no external-window fallback")

    for token in [
        "XErrorTrap",
        "safe_window_attributes",
        "XReparentWindow",
        "XMoveResizeWindow",
        "now - impl_->last_geometry_ms >= 150",
    ]:
        require(host_cpp, token, "hardened emulator host")

    for token in [
        'ends_with_lower(lower, ".bin")',
        'file == "default.xex" ? "Xbox 360" : "Atari 8-bit"',
        '{emulator, launchPath, "--fullscreen"}',
        'gameHost.resize(videoW, videoH);',
    ]:
        require(main_cpp, token, "Atari/Mesen/geometry repair")

    ok("X11 host hardening, Atari discovery, Mesen presentation, and geometry repair remain present")

    launch = function_body(main_cpp, "void launch_selected_game()")
    if "fork(" in launch or "execl" in launch:
        fail("launch_selected_game still directly spawns a separate emulator window")
    require(launch, "gameHost.start", "game launcher")
    ok("Games launcher delegates to Nougat embedded host")

    safe_zip = function_body(main_cpp, "static bool safe_zip_game_entry(")
    require(safe_zip, 'system != "Xbox 360"', "Xbox ZIP safety")
    ok("Xbox 360 images are not unpacked from ROM ZIP cache")

    for token in [
        "DOS_SHA256", "bc229df72ea103b7865cdca67324772dbffa8e58866477e69a79638b723a0442",
        "XENIA_SHA256", "91df919a912bd305a214c535e0ab8abee43c18eb1bab1ef5e35991d16738b05e",
        "xenia_canary_linux.AppImage", "dosbox-staging-linux-x86_64-v0.82.2.tar.xz",
    ]:
        require(runtime_installer, token, "pinned emulator runtime installer")
    ok("DOSBox and Linux Xenia runtimes are pinned and SHA-256 verified")

    for legacy in [
        "src/search/secure_search.cpp", "src/privacy/privacy_broker_client.cpp",
        "src/crawler/crawler_access_manager.cpp", "src/world_tv/world_tv_service.cpp",
        "src/live_tv/tuner_backend.cpp", "src/recommendations/recommendation_engine.cpp",
    ]:
        require(cmake, legacy, "v47 subsystem retention")
    ok("v47 Search, privacy, World TV, Live TV, and recommendation modules remain wired")

    compile_host()
    print("=== v0.0.48 STATIC CONTRACT PASS ===")


if __name__ == "__main__":
    main()
