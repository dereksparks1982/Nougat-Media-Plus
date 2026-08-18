#!/usr/bin/env python3
"""Deterministic v0.0.16 Library and Discover validation."""

from __future__ import annotations

import http.server
import json
import os
import pathlib
import shutil
import socketserver
import sqlite3
import subprocess
import sys
import tempfile
import threading
import urllib.parse


ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


main_source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
api_source = (ROOT / "src/media_server/jellyfin_api_client.cpp").read_text(encoding="utf-8")
engine_source = (ROOT / "src/recommendations/recommendation_engine.cpp").read_text(encoding="utf-8")

for required in (
    '"Discover"',
    '"DISCOVER USUAL"',
    '"DISCOVER RANDOM"',
    '"Local Movie"',
    '"Local TV"',
    '"External Movie"',
    '"External TV"',
    "draw_tree_badge",
    "open_media(selected.path, 0)",
):
    require(required in main_source, f"missing approved UI/playback marker: {required}")

for forbidden in ('"Favorites"', '"Trailers"', '"Genres"', '"Collections"'):
    require(forbidden not in main_source, f"unapproved Library tab found: {forbidden}")

require("libraryState->videos" not in main_source, "flat video-file Library remains in the GUI")
require("MovieCollection" in main_source and "Series" in main_source and "Season" in main_source,
        "hierarchical Library navigation is incomplete")
require("AutomaticallyAddToCollection" in api_source,
        "metadata-driven movie collection grouping is not enabled")
require("/Library/VirtualFolders/Paths" in api_source,
        "multi-folder linking/unlinking API is missing")
require("RecommendationMode::Random" in engine_source and "history_.recent" in engine_source,
        "Random/Usual separation is missing")


RECOMMENDATION_HARNESS = r'''
#include "recommendations/recommendation_engine.hpp"
#include "recommendations/tmdb_client.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static bool expected(const reddmedia::RecommendationResult& result,
                     reddmedia::RecommendationMediaType type,
                     bool external) {
    if (result.item.media_type != type) return false;
    if (external) {
        const std::string wanted = type == reddmedia::RecommendationMediaType::Movie
            ? "tmdb:movie:901" : "tmdb:tv:902";
        return result.item.id == wanted && result.item.local_path.empty();
    }
    const std::string wanted = type == reddmedia::RecommendationMediaType::Movie
        ? "local-movie" : "local-tv";
    return result.item.id == wanted;
}

int main(int argc, char** argv) {
    if (argc != 4) return 90;
    const std::string history = argv[1];
    const std::string empty_history = argv[2];
    const std::string token = argv[3];
    reddmedia::RecommendationEngine engine("stub.gguf", history, token);
    std::string error;
    if (!engine.save_external_token("test-token", error)) return 1;
    struct stat token_info {};
    if (stat(token.c_str(), &token_info) != 0 || (token_info.st_mode & 0777) != 0600) return 2;

    reddmedia::MediaDescriptor watched_movie;
    watched_movie.id = "watched-movie";
    watched_movie.title = "Space Voyage";
    watched_movie.overview = "Exploration through deep space.";
    watched_movie.media_type = reddmedia::RecommendationMediaType::Movie;
    reddmedia::MediaDescriptor watched_tv = watched_movie;
    watched_tv.id = "watched-tv";
    watched_tv.title = "Orbital Crew";
    watched_tv.media_type = reddmedia::RecommendationMediaType::Television;
    if (!engine.record_started(watched_movie, error) || !engine.record_started(watched_tv, error)) return 3;

    std::vector<reddmedia::MediaDescriptor> local;
    reddmedia::MediaDescriptor movie = watched_movie;
    movie.id = "local-movie";
    movie.title = "Galaxy Travelers";
    movie.local_path = "/media/galaxy.mkv";
    movie.tmdb_id = "701";
    local.push_back(movie);
    reddmedia::MediaDescriptor television = watched_tv;
    television.id = "local-tv";
    television.title = "Station Stories";
    television.tmdb_id = "702";
    local.push_back(television);

    int paths = 0;
    for (const auto mode : {reddmedia::RecommendationMode::Usual,
                            reddmedia::RecommendationMode::Random}) {
        for (const auto source : {reddmedia::RecommendationSource::Local,
                                  reddmedia::RecommendationSource::External}) {
            for (const auto type : {reddmedia::RecommendationMediaType::Movie,
                                    reddmedia::RecommendationMediaType::Television}) {
                reddmedia::RecommendationRequest request;
                request.mode = mode;
                request.source = source;
                request.media_type = type;
                reddmedia::RecommendationResult result;
                if (!engine.recommend(request, local, result, error)) {
                    std::fprintf(stderr, "path failure: %s\n", error.c_str());
                    return 10 + paths;
                }
                if (!expected(result, type, source == reddmedia::RecommendationSource::External)) return 30 + paths;
                if (mode == reddmedia::RecommendationMode::Random &&
                    result.reason.find("history was not used") == std::string::npos) return 50 + paths;
                ++paths;
            }
        }
    }

    reddmedia::RecommendationEngine no_history("stub.gguf", empty_history, token);
    reddmedia::RecommendationRequest random_local;
    random_local.mode = reddmedia::RecommendationMode::Random;
    random_local.source = reddmedia::RecommendationSource::Local;
    random_local.media_type = reddmedia::RecommendationMediaType::Movie;
    reddmedia::RecommendationResult random_result;
    if (!no_history.recommend(random_local, local, random_result, error)) return 70;
    reddmedia::RecommendationRequest random_external = random_local;
    random_external.source = reddmedia::RecommendationSource::External;
    if (!no_history.recommend(random_external, local, random_result, error)) return 71;

    setenv("REDDMEDIA_TMDB_BASE_URL", "http://127.0.0.1:1", 1);
    reddmedia::TmdbClient failing(token);
    std::vector<reddmedia::MediaDescriptor> unavailable;
    int pages = 0;
    if (failing.discover(reddmedia::RecommendationMediaType::Movie, 1,
                         unavailable, pages, error)) return 72;
    std::printf("eight-path-discover=pass random-without-history=pass tmdb-failure=pass\n");
    return paths == 8 ? 0 : 73;
}
'''


LIBRARY_HARNESS = r'''
#include "media_server/jellyfin_api_client.hpp"
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) return 90;
    const std::string root = argv[1];
    const std::string state = argv[2];
    { std::ofstream out(state); out << "{\"Username\":\"ReddMedia\","
        "\"AccessToken\":\"token\",\"UserId\":\"user-one\"}"; }
    const std::string movie_a = root + "/movies-a";
    const std::string movie_b = root + "/movies-b";
    const std::string movie_c = root + "/movies-c";
    const std::string tv = root + "/tv";
    mkdir(movie_a.c_str(), 0700); mkdir(movie_b.c_str(), 0700);
    mkdir(movie_c.c_str(), 0700); mkdir(tv.c_str(), 0700);
    const std::string sentinel = movie_a + "/do-not-delete.txt";
    { std::ofstream out(sentinel); out << "preserve"; }

    reddmedia::JellyfinApiClient client(state);
    std::string error;
    std::vector<reddmedia::MediaFolder> folders;
    if (!client.load_media_folders(folders, error) || folders.size() != 3U) return 1;
    if (!client.add_media_folder(movie_c, reddmedia::LibraryMediaType::Movies, error)) return 2;
    if (!client.unlink_media_folder(movie_a, reddmedia::LibraryMediaType::Movies, error)) return 3;
    struct stat information {};
    if (stat(sentinel.c_str(), &information) != 0) return 4;

    std::vector<reddmedia::LibraryNode> roots;
    if (!client.load_library_roots(reddmedia::LibraryMediaType::Movies, roots, error) ||
        roots.size() != 1U || roots[0].kind != reddmedia::LibraryNodeKind::MovieCollection) return 5;
    std::vector<reddmedia::LibraryNode> children;
    if (!client.load_library_children(roots[0], children, error) ||
        children.size() != 1U || children[0].kind != reddmedia::LibraryNodeKind::Movie) return 6;
    if (!client.load_library_roots(reddmedia::LibraryMediaType::Television, roots, error) ||
        roots.size() != 1U || roots[0].kind != reddmedia::LibraryNodeKind::Series) return 7;
    if (!client.load_library_children(roots[0], children, error) ||
        children.size() != 1U || children[0].kind != reddmedia::LibraryNodeKind::Season) return 8;
    if (!client.load_library_children(children[0], roots, error) ||
        roots.size() != 1U || roots[0].kind != reddmedia::LibraryNodeKind::Episode) return 9;
    std::printf("multi-folder=pass unlink-preserves-files=pass hierarchy=pass\n");
    return 0;
}
'''


class ThreadedServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True


class TmdbHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        if self.headers.get("Authorization") != "Bearer test-token":
            self.send_response(401)
            self.end_headers()
            return
        is_tv = "/tv" in self.path
        owned_id, external_id = ((702, 902) if is_tv else (701, 901))
        title_key = "name" if is_tv else "title"
        date_key = "first_air_date" if is_tv else "release_date"
        prefix = "Series" if is_tv else "Movie"
        results = [
            {"id": owned_id, title_key: f"Owned {prefix}", "overview": "owned",
             "genre_ids": [18], date_key: "2024-01-01", "poster_path": "/owned.jpg"},
            {"id": external_id, title_key: f"External Test {prefix}",
             "overview": "Real mock metadata", "genre_ids": [18, 878],
             date_key: "2024-02-01", "poster_path": "/external.jpg"},
        ]
        data = json.dumps({"page": 1, "total_pages": 3, "results": results}).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *_args: object) -> None:
        pass


class JellyfinHandler(http.server.BaseHTTPRequestHandler):
    root: pathlib.Path

    @staticmethod
    def item(item_id: str, name: str, kind: str, path: str = "", parent: str = "", tmdb: str = "") -> dict:
        return {"Id": item_id, "Name": name, "Type": kind, "Path": path,
                "ParentId": parent, "ProductionYear": 2024, "Overview": "Catalog metadata",
                "Genres": ["Drama"], "ImageTags": {"Primary": "poster-tag"},
                "ProviderIds": {"Tmdb": tmdb}}

    def json_response(self, status: int, value: object) -> None:
        data = json.dumps(value).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        parsed = urllib.parse.urlparse(self.path)
        query = urllib.parse.parse_qs(parsed.query)
        if parsed.path == "/Startup/User":
            self.send_response(403); self.end_headers(); return
        if parsed.path == "/Users/Me":
            self.json_response(200, {"Id": "user-one"}); return
        if parsed.path == "/Library/VirtualFolders":
            self.json_response(200, [
                {"Name": "ReddMedia Movies", "CollectionType": "movies",
                 "Locations": [str(self.root / "movies-a"), str(self.root / "movies-b")]},
                {"Name": "ReddMedia TV", "CollectionType": "tvshows",
                 "Locations": [str(self.root / "tv")]},
            ]); return
        if parsed.path == "/Items":
            include = query.get("includeItemTypes", [""])[0]
            parent = query.get("parentId", [""])[0]
            if parent == "box-one":
                values = [self.item("movie-one", "Film One", "Movie",
                                    str(self.root / "movies-a/film.mkv"), parent, "101")]
            elif parent == "series-one":
                values = [self.item("season-one", "Season 1", "Season", "", parent)]
            elif parent == "season-one":
                values = [self.item("episode-one", "Pilot", "Episode",
                                    str(self.root / "tv/show/pilot.mkv"), parent)]
            elif "Series" in include and "Movie" not in include:
                values = [self.item("series-one", "Test Series", "Series", "", "", "202")]
            elif "BoxSet" in include:
                values = [self.item("box-one", "Film Collection", "BoxSet")]
            else:
                values = []
            self.json_response(200, {"Items": values, "TotalRecordCount": len(values)})
            return
        self.send_response(404); self.end_headers()

    def do_POST(self) -> None:
        length = int(self.headers.get("Content-Length", "0"))
        if length:
            self.rfile.read(length)
        if self.path.startswith("/Library/VirtualFolders") or self.path == "/Library/Refresh":
            self.send_response(204); self.end_headers(); return
        self.send_response(404); self.end_headers()

    def do_DELETE(self) -> None:
        if self.path.startswith("/Library/VirtualFolders"):
            self.send_response(204); self.end_headers(); return
        self.send_response(404); self.end_headers()

    def log_message(self, *_args: object) -> None:
        pass


with tempfile.TemporaryDirectory(prefix="reddmedia-v16-tests-") as temporary:
    temp = pathlib.Path(temporary)
    compiler = shutil.which("g++")
    require(compiler is not None, "g++ is required for deterministic validation")

    recommendation_cpp = temp / "recommendation_harness.cpp"
    recommendation_cpp.write_text(RECOMMENDATION_HARNESS, encoding="utf-8")
    recommendation_binary = temp / "recommendation_harness"
    subprocess.run([
        compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-DREDDMEDIA_AI_STUB=1",
        f"-I{ROOT / 'src'}", str(recommendation_cpp),
        str(ROOT / "src/recommendations/embedding_engine.cpp"),
        str(ROOT / "src/recommendations/recommendation_engine.cpp"),
        str(ROOT / "src/recommendations/tmdb_client.cpp"),
        str(ROOT / "src/recommendations/viewing_history.cpp"),
        "-ldl", "-pthread", "-o", str(recommendation_binary),
    ], check=True)

    tmdb_server = ThreadedServer(("127.0.0.1", 0), TmdbHandler)
    tmdb_thread = threading.Thread(target=tmdb_server.serve_forever, daemon=True)
    tmdb_thread.start()
    env = os.environ.copy()
    env["REDDMEDIA_TMDB_BASE_URL"] = f"http://127.0.0.1:{tmdb_server.server_port}"
    history = temp / "history.sqlite3"
    empty_history = temp / "empty.sqlite3"
    token = temp / "tmdb.token"
    try:
        subprocess.run([str(recommendation_binary), str(history), str(empty_history), str(token)],
                       env=env, check=True)
    finally:
        tmdb_server.shutdown()
        tmdb_server.server_close()
        tmdb_thread.join(timeout=2)
    require(history.read_bytes().startswith(b"SQLite format 3\x00"),
            "viewing history is not a real SQLite database")
    with sqlite3.connect(history) as database:
        rows = database.execute("SELECT COUNT(*) FROM viewing_history").fetchone()[0]
    require(rows == 2, "viewing history did not record both media types")
    require((token.stat().st_mode & 0o777) == 0o600, "TMDb token is not owner-only")

    library_cpp = temp / "library_harness.cpp"
    library_cpp.write_text(LIBRARY_HARNESS, encoding="utf-8")
    library_binary = temp / "library_harness"
    subprocess.run([
        compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", f"-I{ROOT / 'src'}",
        str(library_cpp), str(ROOT / "src/media_server/jellyfin_api_client.cpp"),
        "-pthread", "-o", str(library_binary),
    ], check=True)

    JellyfinHandler.root = temp
    try:
        jellyfin_server = ThreadedServer(("127.0.0.1", 8096), JellyfinHandler)
    except OSError as exc:
        raise SystemExit(f"deterministic Library test requires free localhost port 8096: {exc}") from exc
    jellyfin_thread = threading.Thread(target=jellyfin_server.serve_forever, daemon=True)
    jellyfin_thread.start()
    try:
        subprocess.run([str(library_binary), str(temp), str(temp / "client.json")], check=True)
    finally:
        jellyfin_server.shutdown()
        jellyfin_server.server_close()
        jellyfin_thread.join(timeout=2)

print("ReddMedia v0.0.16 deterministic validation PASS")
