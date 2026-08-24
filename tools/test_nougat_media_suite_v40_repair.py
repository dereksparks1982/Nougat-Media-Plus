#!/usr/bin/env python3
from pathlib import Path
import importlib.util
import os
import subprocess
import sys
import tempfile

root = Path(sys.argv[1]).resolve()
exe = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None

def need(ok, msg):
    if not ok:
        raise SystemExit("FAIL: " + msg)

main = (root/"src/main.cpp").read_text(errors="replace")
cmake = (root/"CMakeLists.txt").read_text(errors="replace")
engine_text = (root/"components/nougat/nougat_engine.py").read_text(errors="replace")

need("VERSION 0.0.40" in cmake and "Nougat_Media_Suite_v40" in cmake,
     "v0.0.40 CMake identity missing")
need('const std::string versionLabel = "v0.0.40";' in main,
     "v0.0.40 top version missing")

for token in (
    "const int sliverY = kTopBarH + 1;",
    "const int sliverH = 3;",
    "rgb8(170, 91, 24)",
    "XFillRectangle",
    "liveTvScanState->busy",
    "liveTvGuideState->busy",
    "nougatState->search_busy",
    "nougatState->crawl_busy",
    "ytdlpJob == YtDlpJob::Download",
):
    need(token in main, "loading contract missing: " + token)
need("draw_sheet_progress_frame(target,bar,renderPercent)" not in main,
     "old giant percentage loading renderer remains active")

final_row = ("layout_button_row({&liveTvGuideBtn,&liveTvDetectBtn,&liveTvRefreshBtn,"
             "&liveTvScanBtn,&liveTvWatchBtn,&liveTvGuideRefreshBtn,&liveTvRecordBtn}")
need(final_row in main, "Live TV final toolbar order missing")
need("bool liveTvTunersMode = false;" in main, "Tuners page state missing")
need('"LIVE TV TUNERS"' in main, "Tuners page title missing")
need("liveTvTunerHitboxes.push_back(card);" in main,
     "Tuners page tuner cards missing")
need("Open System and press Detect Tuner" not in main,
     "obsolete System tuner instruction remains")
need("tunerAdminInSystem" not in main,
     "retained self-test still expects tuner controls in System")

need("nougatSearchBtn = {nougatSearchRect.x + nougatSearchRect.w + nougatSearchGap" in main,
     "SEARCH is not immediately after search field")
need("nougatRawBtn = {nougatSearchBtn.x + kCompactButtonW + nougatSearchGap" in main,
     "RAW is not after SEARCH")
for token in (
    'return " OR ".join(tokens)',
    "INSERT INTO pages_fts(pages_fts) VALUES('rebuild')",
    "def live_discovery_search(",
    "LIVE-DISCOVERY",
    "html.duckduckgo.com/html/",
    "lite.duckduckgo.com/lite/",
):
    need(token in engine_text, "Search repair missing: " + token)

need('"Max Pages",searchPalette.text' in main,
     "Max Pages label is not intentionally visible")
need("const Rect maxPagesValue{" in main,
     "Max Pages visible value control missing")
need('"Max pages: "+std::to_string(nougatMaxPages),nougat_cream()' not in main,
     "old clipped Max pages text remains")

with tempfile.TemporaryDirectory(prefix="nougat-v40-search-test-") as td:
    os.environ["NOUGAT_HOME"] = td
    spec = importlib.util.spec_from_file_location(
        "nougat_engine_test", root/"components/nougat/nougat_engine.py")
    mod = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = mod
    spec.loader.exec_module(mod)

    need(" OR " in mod.make_match_query("alpha beta"),
         "search query still forces AND between all terms")

    mod.index_document(
        "https://example.test/alpha",
        "Alpha document",
        "This page contains alpha only.",
        "CLEARNET",
        "TEST"
    )
    total, rows = mod.local_search("alpha beta", limit=20, offset=0, raw=False)
    need(total >= 1 and rows, "partial-term local search still returns nothing")

    sample = """
    <div class="result">
      <a class="result__a" href="//duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.org%2Fstory">Example Story</a>
      <a class="result__snippet">A useful example snippet.</a>
    </div>
    """
    parser = mod._LiveSearchParser()
    parser.feed(sample)
    need(parser.results and parser.results[0]["title"] == "Example Story",
         "live discovery parser failed synthetic result")
    need(mod._unwrap_live_result_url(parser.results[0]["url"]).startswith("https://example.org/story"),
         "live discovery URL unwrap failed")

if exe:
    need(exe.is_file() and os.access(exe, os.X_OK), "v40 executable missing")
    out = subprocess.check_output(
        [str(exe), "--version"], text=True, stderr=subprocess.STDOUT).strip()
    need(out == "Nougat Media Suite v0.0.40",
         "v40 executable version mismatch: " + repr(out))

print("v40-repair-contract=pass loading=pass live-tv-tuners=pass search-engine=pass crawler-max-pages=pass")
