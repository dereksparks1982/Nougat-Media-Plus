#!/usr/bin/env python3
"""Deterministic v0.0.19 UI-contract plus retained behavior validation."""

from __future__ import annotations

import http.server
import json
import os
import pathlib
import shutil
import socketserver
import stat
import subprocess
import sys
import tempfile
import threading
import urllib.parse


ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
API_KEY = "0123456789abcdef0123456789abcdef"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


main_source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
for marker in (
    '"v0.0.19"',
    'draw_tab(videoPlayerTab,"Video Player"',
    'draw_tab(libraryTab,"Library"',
    'draw_tab(discoverTab,"Discover"',
    'draw_tab(nougatTab,"Nougat"',
    'draw_tab(ytdlpTab,"YouTube"',
    'draw_tab(p2pTab,"P2P"',
    'draw_tab(debugTab,"Debug"',
    'static constexpr int kCompactButtonW = 116;',
    'static constexpr int kCompactButtonH = 26;',
    'scroll_top_navigation(delta)',
    'scroll_bottom_controls(delta)',
    'libraryButtonsScrollX',
    'discoverButtonsScrollX',
    'ytdlpButtonsScrollX',
    'p2pButtonsScrollX',
    'debugButtonsScrollX',
    'const int volumeY = H - 64;',
    'const int seekY = H - 96;',
    '"Volume " + std::to_string(vol) + "%"',
    'volumeDragging=true',
    'prepare_tv_autoplay(',
    'play_next_tv_episode()',
    'poll_natural_playback_end()',
    'mark_active_episode_completed()',
    'tvAutoplayQueue',
    'libraryParents',
    'draw_nougat_screen(buffer)',
    '"RAW"',
    '"Copy"',
    '"Select All"',
    'nougat_cocoa()',
    'nougat_chocolate()',
    'nougat_tan()',
    'nougat_caramel()',
    'nougat_cream()',
):
    require(marker in main_source, f"missing v0.0.19 behavior marker: {marker}")

# Exact owner-approved current order. Stream is roadmap-only in this candidate.
order_markers = [
    'draw_tab(videoPlayerTab,"Video Player"',
    'draw_tab(libraryTab,"Library"',
    'draw_tab(discoverTab,"Discover"',
    'draw_tab(nougatTab,"Nougat"',
    'draw_tab(ytdlpTab,"YouTube"',
    'draw_tab(p2pTab,"P2P"',
    'draw_tab(debugTab,"Debug"',
]
positions = [main_source.index(marker) for marker in order_markers]
require(positions == sorted(positions), "top-level tab order is not owner-approved order")
require('draw_tab(stream' not in main_source.lower(), "Stream was pulled into v0.0.19 instead of remaining roadmap-only")

# Interior themes: Library green, Discover plum, P2P blue, Debug amber; YouTube remains red-family.
for rgb in (
    'col(0xe4e4,0xeeeE,0xe6e6)',
    'col(0xeeeE,0xe8e8,0xf2f2)',
    'col(0xe3e3,0xeaea,0xf2f2)',
    'col(0xf3f3,0xeaea,0xd0d0)',
    'col(0xeeee,0xe4e4,0xe4e4)',
    'col(0x2424,0x1818,0x1212)',
    'col(0x4949,0x3030,0x2525)',
    'col(0xd2d2,0xa5a5,0x6d6d)',
    'col(0xb9b9,0x6f6f,0x3636)',
    'col(0xf3f3,0xe5e5,0xcccc)',
):
    require(rgb in main_source, f"required v0.0.19 palette marker missing: {rgb}")

for selection_marker in (
    'nougatOutputSelectionStart',
    'nougatOutputSelectionEnd',
    'copy_nougat_output_selection()',
    'select_all_nougat_output()',
    'show_nougat_output_context_menu',
    'XK_c',
    'XK_a',
):
    require(selection_marker in main_source, f"Nougat selectable/copyable output marker missing: {selection_marker}")

require('tail_to_width(result.item.overview' not in main_source,
        'Discover still discards the beginning of long descriptions')

# All main Video Player action buttons are adjacent and exactly the compact size.
for chain in (
    'openBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;',
    'rewindBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;',
    'playBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;',
    'stopBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;',
    'forwardBtn = {x, bottomY, kCompactButtonW, kCompactButtonH}; x += kCompactButtonW;',
    'fsBtn = {x, bottomY, kCompactButtonW, kCompactButtonH};',
):
    require(chain in main_source, f"compact adjacent player-control contract missing: {chain}")

print('ui-contract=pass compact-buttons=pass wheel-scroll=pass footer=pass volume-percent=pass tv-autoplay=pass viewing-history-completion=pass tab-themes=pass')


HARNESS = r'''
#include "diagnostics/diagnostic_engine.hpp"
#include "media_server/jellyfin_api_client.hpp"
#include "recommendations/recommendation_engine.hpp"
#include "recommendations/viewing_history.hpp"
#include "recommendations/watch_provider_preferences.hpp"

#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <sys/stat.h>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 7) return 90;
    const std::string history = argv[1];
    const std::string credential = argv[2];
    const std::string preferences = argv[3];
    const std::string jellyfin_state = argv[4];
    const std::string model = argv[5];
    const std::string runtime = argv[6];
    std::string error;

    reddmedia::RecommendationEngine recommendations("stub.gguf", history, credential);
    if (!recommendations.save_external_credential("0123456789abcdef0123456789abcdef", error)) {
        std::fprintf(stderr, "credential failed: %s\n", error.c_str());
        return 1;
    }
    struct stat credential_information {};
    if (stat(credential.c_str(), &credential_information) != 0 ||
        (credential_information.st_mode & 0777) != 0600) return 20;
    if (recommendations.save_external_credential(
            "ffffffffffffffffffffffffffffffff", error) ||
        error.find("401") == std::string::npos) return 21;
    std::ifstream preserved_credential(credential);
    std::string preserved_value;
    std::getline(preserved_credential, preserved_value);
    if (preserved_value != "0123456789abcdef0123456789abcdef") return 22;

    reddmedia::MediaDescriptor local_movie;
    local_movie.id = "local-movie";
    local_movie.tmdb_id = "701";
    local_movie.title = "Owned Movie";
    local_movie.media_type = reddmedia::RecommendationMediaType::Movie;
    local_movie.local_path = "/media/owned-movie.mkv";
    reddmedia::MediaDescriptor local_tv = local_movie;
    local_tv.id = "local-tv";
    local_tv.tmdb_id = "702";
    local_tv.title = "Owned TV";
    local_tv.media_type = reddmedia::RecommendationMediaType::Television;
    local_tv.local_path.clear();
    const std::vector<reddmedia::MediaDescriptor> local_items = {local_movie, local_tv};
    if (!recommendations.record_started(local_movie, error) ||
        !recommendations.record_started(local_tv, error)) return 24;
    if (!recommendations.record_completed(local_tv, error)) return 25;
    {
        reddmedia::ViewingHistory verification(history);
        std::vector<reddmedia::ViewingRecord> movie_history;
        std::vector<reddmedia::ViewingRecord> tv_history;
        if (!verification.recent(reddmedia::RecommendationMediaType::Movie, movie_history, error, 10) ||
            !verification.recent(reddmedia::RecommendationMediaType::Television, tv_history, error, 10)) return 26;
        if (movie_history.empty() || movie_history.front().item.id != local_movie.id ||
            movie_history.front().completed) return 27;
        if (tv_history.empty() || tv_history.front().item.id != local_tv.id ||
            !tv_history.front().completed) return 28;
    }
    for (const auto mode : {reddmedia::RecommendationMode::Usual,
                            reddmedia::RecommendationMode::Random}) {
        for (const auto source : {reddmedia::RecommendationSource::Local,
                                  reddmedia::RecommendationSource::External}) {
            for (const auto type : {reddmedia::RecommendationMediaType::Movie,
                                    reddmedia::RecommendationMediaType::Television}) {
                for (int repeat = 0; repeat < (mode == reddmedia::RecommendationMode::Random ? 20 : 1);
                     ++repeat) {
                    reddmedia::RecommendationRequest request;
                    request.mode = mode;
                    request.source = source;
                    request.media_type = type;
                    reddmedia::RecommendationResult result;
                    if (!recommendations.recommend(request, local_items, result, error) ||
                        result.item.media_type != type) {
                        std::fprintf(stderr, "type gate failed mode=%d source=%d type=%d: %s\n",
                            static_cast<int>(mode), static_cast<int>(source),
                            static_cast<int>(type), error.c_str());
                        return 23;
                    }
                }
            }
        }
    }

    reddmedia::WatchAvailability movie;
    if (!recommendations.load_watch_availability(
            reddmedia::RecommendationMediaType::Movie, "901", "US", movie, error)) return 2;
    if (!movie.listing_found || movie.providers.size() != 3U ||
        movie.link != "https://www.themoviedb.org/movie/901/watch?locale=US") return 3;
    std::set<std::string> movie_names;
    for (const auto& provider : movie.providers) movie_names.insert(provider.name);
    if (movie_names.count("Netflix") == 0U || movie_names.count("Prime Video") == 0U ||
        movie_names.count("Apple TV Store") == 0U) return 4;

    reddmedia::WatchAvailability television;
    if (!recommendations.load_watch_availability(
            reddmedia::RecommendationMediaType::Television, "902", "US", television, error) ||
        television.providers.size() != 1U || television.providers[0].name != "Disney Plus") return 5;

    std::vector<reddmedia::WatchProvider> catalog;
    if (!recommendations.load_watch_provider_catalog("US", catalog, error)) return 6;
    std::set<int> catalog_ids;
    for (const auto& provider : catalog) catalog_ids.insert(provider.id);
    if (catalog_ids.size() != catalog.size() || catalog_ids.count(8) == 0U ||
        catalog_ids.count(337) == 0U) return 7;

    std::string title;
    std::string overview;
    if (!recommendations.load_tv_episode_details("123", 1, 3, title, overview, error) ||
        title != "A Verified Episode" || overview != "Verified episode description.") return 8;
    std::string poster;
    if (!recommendations.load_tv_poster_path("123", 1, poster, error) ||
        poster != "/season-one.jpg") return 9;
    if (!recommendations.load_movie_poster_path("901", poster, error) ||
        poster != "/movie-901.jpg") return 10;

    reddmedia::WatchProviderPreferences selected(preferences);
    if (!selected.toggle(8, error) || !selected.toggle(337, error)) return 11;
    struct stat information {};
    if (stat(preferences.c_str(), &information) != 0 ||
        (information.st_mode & 0777) != 0600) return 12;
    reddmedia::WatchProviderPreferences reloaded(preferences);
    if (!reloaded.is_selected(8) || !reloaded.is_selected(337)) return 13;

    { std::ofstream state(jellyfin_state); state
        << "{\"Username\":\"ReddMedia\",\"AccessToken\":\"local-token\","
           "\"UserId\":\"user-one\"}"; }
    reddmedia::JellyfinApiClient client(jellyfin_state);
    reddmedia::LibraryNode season;
    season.id = "season-one";
    season.name = "Season 1";
    season.kind = reddmedia::LibraryNodeKind::Season;
    season.season_number = 1;
    season.series_tmdb_id = "123";
    season.poster_item_id = "series-one";
    season.poster_image_tag = "series-tag";
    std::vector<reddmedia::LibraryNode> episodes;
    if (!client.load_library_children(season, episodes, error) || episodes.size() != 1U) {
        std::fprintf(stderr, "episode load failed: %s\n", error.c_str());
        return 14;
    }
    const auto& episode = episodes[0];
    if (episode.season_number != 1 || episode.episode_number != 3 ||
        !episode.episode_title.empty() || episode.technical_details != "1080p  H.265  AAC" ||
        episode.poster_item_id != "series-one" || episode.series_tmdb_id != "123") return 15;

    reddmedia::DiagnosticInput diagnostic;
    diagnostic.server_state = reddmedia::MediaServerState::Ready;
    diagnostic.server_api_ready = true;
    diagnostic.runtime_path = runtime;
    diagnostic.ai_runtime_path = runtime;
    diagnostic.ai_model_path = model;
    diagnostic.tmdb_configured = true;
    reddmedia::LibraryNode incomplete = episode;
    incomplete.overview = "Known overview";
    incomplete.path.clear();
    diagnostic.library_nodes.push_back(incomplete);
    reddmedia::DiagnosticEngine diagnostics;
    const reddmedia::DiagnosticReport warning = diagnostics.evaluate(diagnostic);
    if (warning.overall != reddmedia::DiagnosticSeverity::Warning) return 16;
    bool found_episode_issue = false;
    for (const auto& issue : warning.issues) {
        if (issue.code == "EPISODE_IDENTITY_MISSING") found_episode_issue = true;
    }
    if (!found_episode_issue) return 17;
    diagnostic.library_nodes[0].episode_title = "A Verified Episode";
    const reddmedia::DiagnosticReport healthy = diagnostics.evaluate(diagnostic);
    if (healthy.overall != reddmedia::DiagnosticSeverity::Information) return 18;
    const std::string report = reddmedia::DiagnosticEngine::report_text(healthy, diagnostic);
    if (report.find("ReddMedia Diagnostic Report") == std::string::npos ||
        report.find("0123456789abcdef") != std::string::npos) return 19;

    std::printf("watch-providers=pass my-services=pass episode-identity=pass "
                "poster-fallback=pass diagnostics=pass\n");
    return 0;
}
'''


class ThreadedServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True


class Handler(http.server.BaseHTTPRequestHandler):
    def send_json(self, status_code: int, value: object) -> None:
        data = json.dumps(value).encode()
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def authorized(self, parsed: urllib.parse.ParseResult) -> bool:
        return urllib.parse.parse_qs(parsed.query).get("api_key", [""])[0] == API_KEY

    def do_GET(self) -> None:
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/Startup/User":
            self.send_response(403)
            self.end_headers()
            return
        if parsed.path == "/Users/Me":
            self.send_json(200, {"Id": "user-one"})
            return
        if parsed.path == "/Items":
            self.send_json(200, {"Items": [{
                "Id": "episode-three", "ParentId": "season-one", "SeriesId": "series-one",
                "SeasonId": "season-one", "Name": "[HDTV-1080p]h265 AAC", "Type": "Episode",
                "IndexNumber": 3, "ParentIndexNumber": 1,
                "MediaStreams": [
                    {"Type": "Video", "Codec": "hevc", "Height": 1080},
                    {"Type": "Audio", "Codec": "aac"},
                ],
            }]})
            return
        if not parsed.path.startswith("/3/"):
            self.send_response(404)
            self.end_headers()
            return
        if not self.authorized(parsed):
            self.send_response(401)
            self.end_headers()
            return
        if parsed.path == "/3/configuration":
            self.send_json(200, {"images": {}})
        elif parsed.path == "/3/movie/901/watch/providers":
            self.send_json(200, {"results": {"US": {
                "link": "https://www.themoviedb.org/movie/901/watch?locale=US",
                "flatrate": [
                    {"provider_id": 8, "provider_name": "Netflix", "display_priority": 1},
                    {"provider_id": 9, "provider_name": "Prime Video", "display_priority": 2},
                ],
                "buy": [{"provider_id": 10, "provider_name": "Apple TV Store", "display_priority": 3}],
            }}})
        elif parsed.path == "/3/tv/902/watch/providers":
            self.send_json(200, {"results": {"US": {
                "link": "https://www.themoviedb.org/tv/902/watch?locale=US",
                "flatrate": [{"provider_id": 337, "provider_name": "Disney Plus", "display_priority": 1}],
            }}})
        elif parsed.path == "/3/watch/providers/movie":
            self.send_json(200, {"results": [
                {"provider_id": 8, "provider_name": "Netflix", "display_priority": 2},
                {"provider_id": 9, "provider_name": "Prime Video", "display_priority": 3},
            ]})
        elif parsed.path == "/3/watch/providers/tv":
            self.send_json(200, {"results": [
                {"provider_id": 8, "provider_name": "Netflix", "display_priority": 1},
                {"provider_id": 337, "provider_name": "Disney Plus", "display_priority": 2},
            ]})
        elif parsed.path == "/3/tv/123/season/1/episode/3":
            self.send_json(200, {"name": "A Verified Episode", "overview": "Verified episode description."})
        elif parsed.path == "/3/tv/123/season/1":
            self.send_json(200, {"poster_path": "/season-one.jpg"})
        elif parsed.path == "/3/movie/901":
            self.send_json(200, {"poster_path": "/movie-901.jpg"})
        elif parsed.path in ("/3/discover/movie", "/3/discover/tv"):
            is_tv = parsed.path.endswith("/tv")
            title_key = "name" if is_tv else "title"
            date_key = "first_air_date" if is_tv else "release_date"
            owned_id, external_id = ((702, 902) if is_tv else (701, 901))
            self.send_json(200, {"page": 1, "total_pages": 1, "results": [
                {"id": owned_id, title_key: "Owned", date_key: "2024-01-01",
                 "overview": "owned", "genre_ids": [18]},
                {"id": external_id, title_key: "External", date_key: "2024-02-01",
                 "overview": "external", "genre_ids": [18]},
            ]})
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, *_args: object) -> None:
        pass


with tempfile.TemporaryDirectory(prefix="reddmedia-v19-tests-") as temporary:
    temp = pathlib.Path(temporary)
    compiler = shutil.which("g++")
    require(compiler is not None, "g++ is required for deterministic validation")
    source = temp / "harness.cpp"
    source.write_text(HARNESS, encoding="utf-8")
    binary = temp / "harness"
    subprocess.run([
        compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-DREDDMEDIA_AI_STUB=1",
        f"-I{ROOT / 'src'}", str(source),
        str(ROOT / "src/diagnostics/diagnostic_engine.cpp"),
        str(ROOT / "src/media_server/jellyfin_api_client.cpp"),
        str(ROOT / "src/media_server/library_poster.cpp"),
        str(ROOT / "src/recommendations/embedding_engine.cpp"),
        str(ROOT / "src/recommendations/recommendation_engine.cpp"),
        str(ROOT / "src/recommendations/tmdb_client.cpp"),
        str(ROOT / "src/recommendations/viewing_history.cpp"),
        str(ROOT / "src/recommendations/watch_provider_preferences.cpp"),
        "-ldl", "-pthread", "-o", str(binary),
    ], check=True)

    tmdb = ThreadedServer(("127.0.0.1", 0), Handler)
    tmdb_thread = threading.Thread(target=tmdb.serve_forever, daemon=True)
    tmdb_thread.start()
    try:
        jellyfin = ThreadedServer(("127.0.0.1", 8096), Handler)
    except OSError as exc:
        tmdb.shutdown()
        tmdb.server_close()
        tmdb_thread.join(timeout=2)
        raise SystemExit(f"v0.0.19 metadata test requires free localhost port 8096: {exc}") from exc
    jellyfin_thread = threading.Thread(target=jellyfin.serve_forever, daemon=True)
    jellyfin_thread.start()
    home = temp / "home"
    runtime = temp / "runtime"
    runtime.mkdir()
    model = temp / "model.gguf"
    model.write_text("test model", encoding="utf-8")
    env = os.environ.copy()
    env["HOME"] = str(home)
    env["REDDMEDIA_TMDB_BASE_URL"] = f"http://127.0.0.1:{tmdb.server_port}"
    try:
        subprocess.run([
            str(binary), str(temp / "history.sqlite3"), str(temp / "tmdb.token"),
            str(temp / "watch.conf"), str(temp / "jellyfin.json"), str(model), str(runtime),
        ], env=env, check=True, timeout=90)
    finally:
        jellyfin.shutdown()
        jellyfin.server_close()
        jellyfin_thread.join(timeout=2)
        tmdb.shutdown()
        tmdb.server_close()
        tmdb_thread.join(timeout=2)

    require(stat.S_IMODE((temp / "watch.conf").stat().st_mode) == 0o600,
            "My Services preferences are not owner-only")

print("ReddMedia v0.0.19 deterministic validation PASS")
