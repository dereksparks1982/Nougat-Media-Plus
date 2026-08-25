#!/usr/bin/env python3
from pathlib import Path
import hashlib
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

def read(rel):
    p = root / rel
    need(p.is_file(), "missing required file: " + rel)
    return p.read_text(errors="replace")

main = read("src/main.cpp")
cmake = read("CMakeLists.txt")
engine_text = read("components/nougat/nougat_engine.py")
header = read("src/media_server/jellyfin_api_client.hpp")
client = read("src/media_server/jellyfin_api_client.cpp")
cache = read("src/media_server/library_metadata_cache.cpp")
roadmap = read("ROADMAP.md")
readme = read("README.md")
changelog = read("CHANGELOG.md")
bible = read("COMPANY_BIBLE.md")

# v0.0.41 release integrity.
need("VERSION 0.0.41" in cmake and "Nougat_Media_Suite_v41" in cmake,
     "v0.0.41 CMake identity missing")
need('const std::string versionLabel = "v0.0.41";' in main,
     "v0.0.41 top version missing")
need('const std::string versionLabel = "v0.0.40";' not in main,
     "stale v0.0.40 top version remains")
need(readme.startswith("# Nougat Media Suite v0.0.41"),
     "README version is not v0.0.41")
need("## v0.0.41 - Housekeeping, Archives, IMDb, Live TV and Player Activity Repair" in changelog,
     "CHANGELOG v0.0.41 record missing")
need("## v0.0.42 planned — Home LAN Web Viewer / Streaming" in roadmap,
     "v0.0.42 Home LAN web viewer roadmap entry missing")
need("## v0.0.43 planned — Games / Unified Emulation" in roadmap,
     "Games roadmap was not moved behind the web viewer project")

# Permanent owner workflow rule added during the final v0.0.41 stabilization pass.
for token in (
    "install, repair, build, validation, package-apply, or acceptance-preflight",
    "verified Nougat-owned integrated Jellyfin",
    "never kill Jellyfin blindly by process name",
    "terminal command blocks must contain commands only",
    "do not put `exit`, `exit 1`, or a command that closes the owner's terminal",
):
    need(token in bible, "safe Nougat/Jellyfin terminal workflow rule missing: " + token)

canonical = (
    "Nougat Media Suite is the new official identity of the Linux media application previously released as ReddMedia through accepted v0.0.20. "
    "It combines native local playback, a hidden local Jellyfin catalog foundation, local recommendation AI, optional TMDb discovery, decentralized Search, "
    "multi-platform Stream URL handling, and built-in P2P transfer/streaming in one desktop application."
)
need(canonical in readme, "canonical README introduction changed or missing")

# Retained v0.0.40 loading behavior.
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
    need(token in main, "retained loading contract missing: " + token)
need("draw_sheet_progress_frame(target,bar,renderPercent)" not in main,
     "old giant percentage loading renderer returned")

# Live TV toolbar + stop/guide state.
final_row = (
    "layout_button_row({&liveTvGuideBtn,&liveTvDetectBtn,&liveTvRefreshBtn,"
    "&liveTvScanBtn,&liveTvWatchBtn,&liveTvStopBtn,&liveTvGuideRefreshBtn,&liveTvRecordBtn}"
)
need(final_row in main, "v0.0.41 Live TV toolbar order missing")
need('button_on(target,liveTvStopBtn,"Stop Live")' in main, "Stop Live control missing")
need("if (hadLive) stop_media();" in main, "Stop Live does not use native player stop path")
need("liveTvTunerUse=LiveTvTunerUse::Idle;" in main, "Stop Live does not release tuner")
need("if (queued)" in main and "start_live_tv_guide_refresh();" in main,
     "queued guide sweep is not continued after Stop Live")
need("if (liveTvTunerUse != LiveTvTunerUse::Watching) liveTvGuideRefreshQueued=false;" in main,
     "idle manual guide refresh does not reset stale queue state")

# Owner-supplied real artwork for the three remaining local channels.
for token in (
    'if (channel.id == "36.3") return "charge";',
    'if (channel.id == "36.4") return "tcn";',
    'if (channel.id == "46.2") return "grit";',
):
    need(token in main, "Live TV exact channel-art mapping missing: " + token)
logo_hashes = {
    "assets/channel_logos/builtin/charge.bmp": "7292fdab8792925633b1ba13fba5598606827c7c8b067abcb2031ab2fd20783c",
    "assets/channel_logos/builtin/tcn.bmp": "60cb4523aca35c30eee54f62e42d3729c02aadb33afa087f1c510500a8e7d387",
    "assets/channel_logos/builtin/grit.bmp": "8e0b9068cd2fa2a39e4deae2b0495a70cf66f956f02c062d84233abd939f6844",
    "assets/channel_logos/builtin/36.3_Charge_.bmp": "7292fdab8792925633b1ba13fba5598606827c7c8b067abcb2031ab2fd20783c",
    "assets/channel_logos/builtin/36.4_TCN.bmp": "60cb4523aca35c30eee54f62e42d3729c02aadb33afa087f1c510500a8e7d387",
    "assets/channel_logos/builtin/46.2_Grit.bmp": "8e0b9068cd2fa2a39e4deae2b0495a70cf66f956f02c062d84233abd939f6844",
}
for rel, want in logo_hashes.items():
    logo = root / rel
    need(logo.is_file(), "owner-supplied Live TV artwork missing: " + rel)
    data = logo.read_bytes()
    need(hashlib.sha256(data).hexdigest() == want, "Live TV artwork changed unexpectedly: " + rel)
    need(len(set(data[54:])) > 8, "Live TV artwork is blank/placeholder-like: " + rel)

# Search v40 retained behavior + v41 Archive.
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
    need(token in engine_text, "retained Search repair missing: " + token)
need('"Max Pages",searchPalette.text' in main, "Crawler Max Pages label missing")
need("const Rect maxPagesValue{" in main, "Crawler Max Pages value control missing")

need("enum class NougatPanel { Search, Crawler, P2P, VirusScan, Archive };" in main,
     "Archive panel enum missing")
need('nougat_tab_button(target,nougatArchivePanelTab,"Archive"' in main,
     "Archive tab missing")
for token in (
    '{"Internet Archive / Archive.org","https://archive.org/","https://archivep75mbjunhxc6x4j5mwjmomyxb573v42baldlqu56ruil2oiad.onion/"}',
    '{"Minerva Archive","https://minerva-archive.org/",nullptr}',
    '{"Project Gutenberg","https://www.gutenberg.org/",nullptr}',
    '{"Software Heritage","https://www.softwareheritage.org/",nullptr}',
):
    need(token in main, "Archive directory entry missing: " + token)
need('execlp("xdg-open","xdg-open",target.c_str()' in main,
     "Archive external-browser action missing")
need('nougatArchiveTorRows' in main and 'nougat_button(target,torSite,"Tor")' in main,
     "Archive official-onion Tor button path missing")
need('nougat.open_url(link.second,true,error)' in main,
     "Archive Tor action does not open through Tor")

# Search result card bottom-gap housekeeping.
need("const int base_card_h=98;" in main, "dynamic Search-card base height missing")
need("std::max(base_card_h,((int)nougatResultsBox.h-12)/visible)" in main,
     "Search cards do not distribute spare scroll viewport height")

# Exact IMDb ProviderId and local cache.
need("std::string imdb_id;" in header, "IMDb field missing from LibraryNode")
need('json_string_value(object.substr(provider_ids), "Imdb")' in client,
     "Jellyfin IMDb ProviderId is not parsed")
need("node.imdb_id = reserved;" in cache and "hex_encode(node.imdb_id)" in cache,
     "IMDb ID is not round-tripped through local metadata cache")
need('open_external_url("https://www.imdb.com/title/"+imdb+"/")' in main,
     "Library IMDb link does not open exact title URL")
need('open_external_url("https://www.imdb.com/title/"+hit.second+"/")' in main,
     "Home IMDb link does not open exact title URL")
need('static bool is_exact_imdb_title_id' in main and 'library_node_has_imdb_link' in main,
     "shared exact IMDb title-ID gate missing")
need('homeImdbHitboxes' in main and 'metadata_text(target,card.x+card.w-42' in main,
     "Home IMDb text/hitbox path missing")
need('item.kind==reddmedia::LibraryNodeKind::Movie &&' not in main,
     "IMDb link is still restricted to Movies only")

# Shared Movies/TV Library grid repair: height-aware Library-only geometry.
need("int target_width = 150;" in main and "two_row_tile_height" in main,
     "Library adaptive two-row geometry missing")
need("two_row_poster_height * 2 / 3" in main,
     "Library card width is not capped from available two-row height")
need("Home card geometry is deliberately separate" in main,
     "Library repair no longer documents the Home-card isolation contract")
need("const int gridStartX = libraryListBox.x + 6" in main,
     "Library multi-row grid is not centered within its viewport")

# Library media-card hover popup must repaint on entry, exit and card-to-card transitions.
need("if (currentView == ViewMode::Library)" in main and
     "if (oldRow != newRow) return true;" in main,
     "Library card hover transitions do not force a deterministic repaint")

# Player tab shell + repaint stability.
need("return view == ViewMode::Home || view == ViewMode::VideoPlayer" in main,
     "Video Player is not using the common stitched page frame")
need("draw_page_frame(buffer, ViewMode::VideoPlayer);" in main,
     "partial player repaint can erase the stitched page frame")
need("resumePromptVisible || stoppedPlaybackVisible || needResumePrompt" in main and
     "Pixmap promptBuffer" in main and "XCopyArea(d, promptBuffer, video" in main,
     "player prompt is not atomically composed")
need("!resumePromptVisible && !stoppedPlaybackVisible" in main and
     "!upNextVisible && !needResumePrompt" in main,
     "video pointer motion still repaints prompt screens continuously")

# Discover + Search/Network bottom-edge clearance.
need("discoverFrame.y + discoverFrame.h - 16" in main,
     "Discover JustWatch attribution still sits on the bottom stitch")
need('"NETWORK / ADVANCED"' not in main,
     "Search Network stray floating heading remains")
need("nougatPeerListBox.y+nougatPeerListBox.h-10" in main,
     "Search Network status is still outside its stitched panel")
need("nougatNetworkActionsViewport" in main and "nougatNetworkButtonsScrollX" in main,
     "Search Network action row has no independent overflow viewport")
need("scroll_button_row(nougatPanelButtonsScrollX,6,delta)" in main,
     "Search/Crawler/P2P/Virus Scan/Network/Archive row does not scroll all six controls")
need("scroll_button_row(nougatNetworkButtonsScrollX,4,delta,nougatNetworkActionsViewport.w)" in main,
     "Search Network Add Peer/Remove/Start Node/Search peers row is not mouse-wheel scrollable")
need("const int networkInputWidth = std::max(200, W - 552);" in main,
     "Search Network peer field was not shortened for narrow-window action space")
need("const int networkActionsX = nougatPeerEntryRect.x + nougatPeerEntryRect.w + 12;" in main,
     "Search Network action row is not anchored directly after the peer field")

# Live TV inner action strip has eight buttons after Stop Live. Global top tabs are unrelated.
need("scroll_button_row(liveTvButtonsScrollX,8,delta)" in main,
     "Live TV inner action-row scroll still does not count all eight controls")
need("scroll_button_row(liveTvButtonsScrollX,7,delta)" not in main,
     "stale seven-button Live TV inner action-row extent remains")

# Player identity overlay: real child window, one authoritative three-second timer, no 16 ms repaint fight.
need("videoActivityOverlayWindow" in main and "XCreateSimpleWindow(d, video" in main,
     "video identity is not isolated in a real child overlay window")
need("XMapRaised(d, videoActivityOverlayWindow)" in main and
     "draw_player_activity_overlay_window" in main,
     "video identity overlay is not raised/drawn independently of libVLC frames")
need("kPlayerActivityVisibleMs = 3000" in main and "player_activity_is_active" in main,
     "authoritative three-second player activity timer missing")
need("lastPlayerActivityOverlayRedrawMs" not in main and
     "activityNow - lastPlayerActivityOverlayRedrawMs >= 16" not in main,
     "old 16 ms player-description repaint loop remains")
need("lastPlayerActivityMotionMs = activityNow;" in main and
     "if (wasHidden)" in main,
     "video motion does not reset the shared timer without repainting every motion event")
need("playerActivityOverlayVisible = false;" in main and
     "hide_player_activity_overlay_window();" in main and "hide_pointer();" in main,
     "pointer/description timeout hide path missing")
need("e.xcrossing.detail != NotifyInferior" in main,
     "child activity overlay incorrectly triggers parent-video leave handling")

# Retained Search behavior is exercised, not only token checked.
with tempfile.TemporaryDirectory(prefix="nougat-v41-search-test-") as td:
    old_home = os.environ.get("NOUGAT_HOME")
    os.environ["NOUGAT_HOME"] = td
    try:
        spec = importlib.util.spec_from_file_location(
            "nougat_engine_v41_test", root/"components/nougat/nougat_engine.py")
        mod = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = mod
        spec.loader.exec_module(mod)

        need(" OR " in mod.make_match_query("alpha beta"),
             "Search query still forces AND between all terms")

        mod.index_document(
            "https://example.test/alpha",
            "Alpha document",
            "This page contains alpha only.",
            "CLEARNET",
            "TEST"
        )
        total, rows = mod.local_search("alpha beta", limit=20, offset=0, raw=False)
        need(total >= 1 and rows, "partial-term local Search returned nothing")

        sample = """
        <div class="result">
          <a class="result__a" href="//duckduckgo.com/l/?uddg=https%3A%2F%2Fexample.org%2Fstory">Example Story</a>
          <a class="result__snippet">A useful example snippet.</a>
        </div>
        """
        parser = mod._LiveSearchParser()
        parser.feed(sample)
        need(parser.results and parser.results[0]["title"] == "Example Story",
             "live-discovery parser failed synthetic result")
        need(mod._unwrap_live_result_url(parser.results[0]["url"]).startswith("https://example.org/story"),
             "live-discovery URL unwrap failed")
    finally:
        if old_home is None:
            os.environ.pop("NOUGAT_HOME", None)
        else:
            os.environ["NOUGAT_HOME"] = old_home

if exe:
    need(exe.is_file() and os.access(exe, os.X_OK), "v0.0.41 executable missing")
    out = subprocess.check_output([str(exe), "--version"], text=True,
                                  stderr=subprocess.STDOUT).strip()
    need(out == "Nougat Media Suite v0.0.41",
         "v0.0.41 executable version mismatch: " + repr(out))
    repair = subprocess.check_output([str(exe), "--v41-library-imdb-repair-self-test"], text=True,
                                     stderr=subprocess.STDOUT).strip()
    need("Library/IMDb repair PASS" in repair,
         "v0.0.41 Library/IMDb runtime regression failed: " + repr(repair))

print("v41-contract=pass retained-v40=pass archive-tor=pass library-grid=pass library-hover=pass imdb-library-tv-home=pass live-tv-scroll=pass player-frame=pass player-repaint=pass discover-footer=pass network-layout=pass network-scroll=pass player-activity=pass live-tv-owner-art=pass roadmap=pass safe-shutdown-doc=pass")
