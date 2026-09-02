#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path
import re
import sys

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
MAIN = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
HOST_PATH = ROOT / "src/games/emulator_host.cpp"
HOST = HOST_PATH.read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
WORKER = (ROOT / "tools/nougat_file_splitter.py").read_text(encoding="utf-8")


def need(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def git_blob_sha(path: Path) -> str:
    data = path.read_bytes()
    header = f"blob {len(data)}\0".encode()
    return hashlib.sha1(header + data).hexdigest()


need("// NOUGAT_V54_FILE_SPLITTER_PROFESSIONAL" in MAIN, "v54 source marker missing")
need('printf("Nougat Media Suite v0.0.54\\n")' in MAIN, "v54 --version identity missing")
need("project(NougatMediaSuite VERSION 0.0.54" in CMAKE, "CMake project version is not v54")
need("add_executable(Nougat_Media_Suite_v54" in CMAKE, "v54 executable target missing")
need("enum class StudioPanel { Tools, FileSplitter };" in MAIN, "Studio Tools/FileSplitter panel model missing")
need('button_on(target,studioFileSplitterToolBtn,"File Splitter")' in MAIN,
     "File Splitter must be a Tools button")
need('button_on(target,studioToolsTab,"Tools")' in MAIN, "Tools tab missing")
need('"Download Location"' in MAIN, "in-app Download Location field missing")
need('"Use Suggestion"' in MAIN, "piece-count suggestion control missing")
need('"Split"' in MAIN and '"Reassemble"' in MAIN and '"Verify"' in MAIN and '"Stop"' in MAIN,
     "professional splitter action row incomplete")
need("spawn_studio_worker" in MAIN and "poll_studio_worker" in MAIN,
     "splitter work must run asynchronously")
need("pipe(pipes)" in MAIN and "O_NONBLOCK" in MAIN,
     "nonblocking progress channel missing")
need("maybe_auto_analyze_studio_source" in MAIN and '"analyze"' in MAIN,
     "automatic piece recommendation missing")
need("studioJobGroup" in MAIN and "SIGTERM" in MAIN and "SIGKILL" in MAIN,
     "safe Stop/cancellation process-group handling missing")
need("zenity" not in WORKER.lower(), "popup-based Zenity code must be removed")
need("studio-gui" not in WORKER, "legacy popup GUI command must be removed")
need("ProgressReporter" in WORKER and "VERIFY PASS" in WORKER,
     "worker live progress/integrity contract missing")
need("sha256" in WORKER.lower(), "SHA-256 verification missing")
need('FORMAT = "nougat-split-zip-v3"' in WORKER, "v54 splitter manifest format missing")
need('"nougat-split-zip-v2"' in WORKER, "legacy v2 manifest compatibility missing")

# Closed v0.0.53 Xbox embedding source is a hard regression gate. v54 is not
# allowed to alter it while implementing File Splitter.
need(git_blob_sha(HOST_PATH) == "22f03639525a37997ef01a049b35641876399afc",
     "closed v0.0.53 emulator_host.cpp changed")
need("NOUGAT_V53_XENIA_EDGE_EMBED_REPAIR" in HOST, "v53 Xenia Edge host repair missing")
need("NOUGAT_V53_XENIA_PRE_VULKAN_EMBED" in HOST, "v53 pre-Vulkan embed marker missing")
need("NOUGAT_EMBED_XID" in HOST, "v53 Xbox embed XID contract missing")

# The v54 source patch must not touch the accepted player-control renderer.
backup = ROOT / "src/main.cpp.v53final"
if backup.is_file():
    old = backup.read_text(encoding="utf-8")
    def function_body(text: str, signature: str) -> str:
        pos = text.find(signature)
        need(pos >= 0, f"player regression anchor missing: {signature}")
        brace = text.find("{", pos)
        need(brace >= 0, f"player regression function malformed: {signature}")
        depth = 0
        for i in range(brace, len(text)):
            if text[i] == "{": depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    return text[pos:i + 1]
        raise AssertionError(f"unterminated function: {signature}")
    for signature in (
        "void draw_player_controls_only()",
        "void draw_fullscreen_transport_overlay()",
        "void draw_player_activity_overlay_window()",
    ):
        need(function_body(MAIN, signature) == function_body(old, signature),
             f"v54 altered closed-v53 player geometry/rendering: {signature}")

print("PASS: v0.0.54 static File Splitter + closed-v53 Xbox regression gates")
