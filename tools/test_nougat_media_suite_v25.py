#!/usr/bin/env python3
from __future__ import annotations
import hashlib
import http.server
import json
import os
import pathlib
import socketserver
import subprocess
import sys
import tempfile
import threading
import urllib.parse

ROOT = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parents[1]
BINARY = pathlib.Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else ROOT / "Nougat_Media_Suite_v25"
main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")


def need(ok: bool, message: str) -> None:
    if not ok:
        raise SystemExit("FAIL: " + message)


need("VERSION 0.0.25" in cmake, "CMake version is not v0.0.25")
need("add_executable(Nougat_Media_Suite_v25" in cmake, "v25 executable target missing")
need("Nougat Media Suite v0.0.25" in main, "CLI v25 version marker missing")
need('const std::string versionLabel = "v0.0.25"' in main, "in-app v25 marker missing")

# Stream provider is a full-page state, not merely a colored provider button.
for marker in [
    "Pixmap streamQuiltTiles[5] = {};",
    "streamQuiltTiles[stream_platform_index(streamPlatform)]",
    "StreamPlatform::YouTube: r=205; g=76;  b=67;  blendPercent=22;",
    "StreamPlatform::Rumble:  r=128; g=154; b=79;  blendPercent=22;",
    "StreamPlatform::RuTube:  r=168; g=107; b=178; blendPercent=20;",
    "StreamPlatform::VK:      r=91;  g=142; b=174; blendPercent=20;",
    "StreamPlatform::OK:      r=211; g=135; b=48;  blendPercent=22;",
    "currentView == ViewMode::Stream\n            ? stream_palette_for(streamPlatform) : palette_for(currentView)",
]:
    need(marker in main, "provider-reactive Stream page contract missing: " + marker)
stream = main[main.index("void draw_stream_screen"):main.index("std::string format_bytes", main.index("void draw_stream_screen"))]
need("XFillPolygon" in stream and "if (selected)" in stream, "selected Stream provider downward notch missing")
need("fill_round(target,{visual.x+8,visual.y+visual.h-4" not in stream, "old Stream underline-only selected marker remains")
need('"Online video: "' not in stream, "redundant Stream provider status label remains")
need('text(target, 28, 58, "STREAM"' not in stream, "redundant Stream heading remains above provider selectors")
need('fill(target, {18,36,W-36,52}, palette.panel);' not in stream, "redundant Stream provider background panel remains")
need('outline(target, {18,36,W-36,52}, palette.border);' not in stream, "redundant Stream provider panel border remains")

# Discover has two simultaneous state groups. TMDb/service controls remain actions.
for marker in [
    "bool discover_mode_selected(reddmedia::RecommendationMode mode) const",
    "bool discover_target_selected(reddmedia::RecommendationSource source",
    'draw_discover_selector(target, discoverUsualTab, "Usual"',
    'draw_discover_selector(target, discoverRandomTab, "Random"',
    'draw_discover_selector(target, discoverLocalMovieBtn, "Local Movie"',
    'draw_discover_selector(target, discoverLocalTvBtn, "Local TV"',
    'draw_discover_selector(target, discoverExternalMovieBtn, "External Movie"',
    'draw_discover_selector(target, discoverExternalTvBtn, "External TV"',
]:
    need(marker in main, "Discover dual-selection contract missing: " + marker)
discover = main[main.index("void draw_discover_screen"):main.index("void start_p2p_magnet", main.index("void draw_discover_screen"))]
for action in ["discoverTmdbTestBtn", "discoverTmdbReplaceBtn", "discoverTmdbClearBtn", "discoverMyServicesBtn"]:
    need(f"button_on(target, {action}" in discover, f"Discover action was incorrectly converted to persistent selector: {action}")

# Local Discover play must resolve the Jellyfin catalog to a real playable node.
for marker in [
    "bool resolve_discover_local_play_target",
    "libraryClient->load_all_recommendation_items(roots,error)",
    "libraryClient->load_library_children(root,seasons,error)",
    "LibraryNodeKind::Episode",
    "recommendationEngine->recent_history",
    "playable = episodes.front();",
    "if (!open_media(playable.path, 0))",
    "switch_view(ViewMode::VideoPlayer);",
]:
    need(marker in main, "Discover native-play repair missing: " + marker)
need("open_media(result.item.local_path, 0);" not in main, "Discover still tries to play raw recommendation path directly")

# Protected boundaries stay byte-identical.
license_expected = {
    "LICENSE": "640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84",
    "COPYRIGHT.md": "f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31",
    "CONTRIBUTING.md": "7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb",
    "THIRD_PARTY_NOTICES.md": "9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f",
    "docs/LICENSING_POLICY.md": "e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff",
}
for rel, expected in license_expected.items():
    got = hashlib.sha256((ROOT / rel).read_bytes()).hexdigest()
    need(got == expected, f"protected licensing file changed: {rel}")
search_expected = {
    "components/nougat/nougat_engine.py": "ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993",
    "src/nougat/nougat_bridge.cpp": "15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944",
    "src/nougat/nougat_bridge.hpp": "46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e",
}
for rel, expected in search_expected.items():
    got = hashlib.sha256((ROOT / rel).read_bytes()).hexdigest()
    need(got == expected, f"Search engine behavior changed outside v25 scope: {rel}")

need(BINARY.is_file(), "v25 test binary missing")
subprocess.run([str(BINARY), "--v25-ui-state-self-test"], check=True, timeout=20)


class ThreadedServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True


class Handler(http.server.BaseHTTPRequestHandler):
    def send_json(self, value: object) -> None:
        data = json.dumps(value).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        parsed = urllib.parse.urlparse(self.path)
        query = urllib.parse.parse_qs(parsed.query)
        if parsed.path == "/Startup/User":
            self.send_response(403)
            self.end_headers()
            return
        if parsed.path == "/Users/Me":
            self.send_json({"Id": "user-one"})
            return
        if parsed.path == "/Items":
            parent = query.get("parentId", [""])[0]
            if parent == "series-one":
                self.send_json({"Items": [{
                    "Id": "season-one", "ParentId": "series-one", "SeriesId": "series-one",
                    "Name": "Season 1", "Type": "Season", "IndexNumber": 1,
                }]})
                return
            if parent == "season-one":
                self.send_json({"Items": [
                    {
                        "Id": "episode-one", "ParentId": "season-one", "SeriesId": "series-one",
                        "SeasonId": "season-one", "SeriesName": "Resolver Series", "Name": "Pilot",
                        "Type": "Episode", "IndexNumber": 1, "ParentIndexNumber": 1,
                        "Path": self.server.episode_one,
                    },
                    {
                        "Id": "episode-watched", "ParentId": "season-one", "SeriesId": "series-one",
                        "SeasonId": "season-one", "SeriesName": "Resolver Series", "Name": "Second",
                        "Type": "Episode", "IndexNumber": 2, "ParentIndexNumber": 1,
                        "Path": self.server.episode_watched,
                    },
                ]})
                return
            self.send_json({"Items": [{
                "Id": "series-one", "Name": "Resolver Series", "Type": "Series",
                "Path": str(pathlib.Path(self.server.episode_one).parent),
                "ProviderIds": {"Tmdb": "123"},
            }]})
            return
        self.send_response(404)
        self.end_headers()

    def log_message(self, *_args: object) -> None:
        pass


# Behavioral resolver gate: no history -> episode 1; watched history -> that episode.
with tempfile.TemporaryDirectory(prefix="nougat-v25-resolver-") as raw:
    temp = pathlib.Path(raw)
    show = temp / "show"
    show.mkdir()
    episode_one = show / "S01E01.mkv"
    episode_watched = show / "S01E02.mkv"
    episode_one.write_bytes(b"one")
    episode_watched.write_bytes(b"two")
    state = temp / "client.json"
    state.write_text('{"Username":"Nougat","AccessToken":"local-token","UserId":"user-one"}', encoding="utf-8")
    try:
        server = ThreadedServer(("127.0.0.1", 8096), Handler)
    except OSError as exc:
        raise SystemExit(f"FAIL: v25 resolver test requires free localhost port 8096: {exc}") from exc
    server.episode_one = str(episode_one)
    server.episode_watched = str(episode_watched)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    env = os.environ.copy()
    env["HOME"] = str(temp / "home")
    env["REDDMEDIA_SERVER_CLIENT_CONFIG"] = str(state)
    try:
        subprocess.run([
            str(BINARY), "--discover-local-resolver-self-test", "series-one", str(episode_one)
        ], env=env, check=True, timeout=20)
        subprocess.run([
            str(BINARY), "--discover-local-resolver-self-test", "series-one",
            str(episode_watched), str(episode_watched)
        ], env=env, check=True, timeout=20)
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=2)

print("v25-contract=pass stream-provider-theme=pass stream-provider-notch=pass stream-redundant-header-removed=pass stream-provider-panel-removed=pass discover-dual-notch=pass discover-local-play=pass history-resume=pass search-preserved=pass license-preserved=pass")
