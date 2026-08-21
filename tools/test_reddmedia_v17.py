#!/usr/bin/env python3
"""Deterministic v0.0.17 Library, Discover, TMDb, poster, and UI validation."""

from __future__ import annotations

import http.server
import json
import os
import pathlib
import shutil
import socketserver
import struct
import subprocess
import sys
import tempfile
import threading
import urllib.parse


ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
API_KEY = "0123456789abcdef0123456789abcdef"
WRONG_KEY = "ffffffffffffffffffffffffffffffff"
READ_TOKEN = "eyJ" + ("a" * 52) + "." + ("b" * 52)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def bmp(width: int = 2, height: int = 2) -> bytes:
    row_size = ((width * 3 + 3) // 4) * 4
    pixels = bytearray()
    for y in range(height):
        row = bytearray()
        for x in range(width):
            row.extend((40 + x * 40, 60 + y * 40, 180))
        row.extend(b"\0" * (row_size - width * 3))
        pixels.extend(row)
    size = 54 + len(pixels)
    return (
        b"BM"
        + struct.pack("<IHHI", size, 0, 0, 54)
        + struct.pack("<IIIHHIIIIII", 40, width, height, 1, 24, 0, len(pixels), 0, 0, 0, 0)
        + bytes(pixels)
    )


main_source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
engine_source = (ROOT / "src/recommendations/recommendation_engine.cpp").read_text(encoding="utf-8")
tmdb_source = (ROOT / "src/recommendations/tmdb_client.cpp").read_text(encoding="utf-8")
jellyfin_source = (ROOT / "src/media_server/jellyfin_api_client.cpp").read_text(encoding="utf-8")
bible_source = (ROOT / "COMPANY_BIBLE.md").read_text(encoding="utf-8")
gitignore_source = (ROOT / ".gitignore").read_text(encoding="utf-8")

for marker in (
    '"Start Server"',
    '"Stop Server"',
    '"Refresh Server"',
    '"Test TMDb"',
    '"Save / Replace"',
    '"Clear TMDb"',
    "draw_loading_bar",
    "posterState->progress",
    '"v0.0.17"',
):
    require(marker in main_source, f"missing v0.0.17 UI marker: {marker}")

require("format=Jpg" in jellyfin_source and "normalize_library_poster_bmp" in jellyfin_source,
        "Jellyfin poster normalization path is missing")
require("ReddMedia blocked a mismatched recommendation type" in engine_source,
        "final Movie/TV separation gate is missing")
require("TMDb rejected this credential (401)" in tmdb_source,
        "clear TMDb 401 handling is missing")
require("components/ai/runtime/" in gitignore_source and
        "components/jellyfin/runtime/" in gitignore_source,
        "generated runtimes are not excluded from Git")
require("ReddMedia Company Bible" in bible_source and "ReddMedia only" in bible_source,
        "canonical project Bible is missing its ReddMedia-only scope")


HARNESS = r'''
#include "media_server/jellyfin_api_client.hpp"
#include "media_server/library_poster.hpp"
#include "recommendations/recommendation_engine.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

static bool expected_type(const reddmedia::RecommendationResult& result,
                          reddmedia::RecommendationMediaType type) {
    if (result.item.media_type != type) return false;
    return type == reddmedia::RecommendationMediaType::Movie
        ? result.item.id == "tmdb:movie:901"
        : result.item.id == "tmdb:tv:902";
}

int main(int argc, char** argv) {
    if (argc != 7) return 90;
    const std::string history = argv[1];
    const std::string credential_file = argv[2];
    const std::string state_file = argv[3];
    const std::string api_key = argv[4];
    const std::string wrong_key = argv[5];
    const std::string read_token = argv[6];
    reddmedia::RecommendationEngine engine("stub.gguf", history, credential_file);
    std::string error;

    if (!engine.save_external_credential(api_key, error)) {
        std::fprintf(stderr, "api key save failed: %s\n", error.c_str());
        return 1;
    }
    struct stat information {};
    if (stat(credential_file.c_str(), &information) != 0 ||
        (information.st_mode & 0777) != 0600) return 2;
    std::ifstream original_input(credential_file);
    std::string original;
    std::getline(original_input, original);
    if (original != api_key || engine.external_credential_label().find("API key") == std::string::npos) {
        return 3;
    }
    if (engine.save_external_credential(wrong_key, error) ||
        error.find("401") == std::string::npos) return 4;
    std::ifstream preserved_input(credential_file);
    std::string preserved;
    std::getline(preserved_input, preserved);
    if (preserved != api_key) return 5;
    if (!engine.test_external_credential(error)) return 6;

    reddmedia::MediaDescriptor watched_movie;
    watched_movie.id = "watched-movie";
    watched_movie.title = "Space Voyage";
    watched_movie.overview = "Deep space exploration.";
    watched_movie.media_type = reddmedia::RecommendationMediaType::Movie;
    reddmedia::MediaDescriptor watched_tv = watched_movie;
    watched_tv.id = "watched-tv";
    watched_tv.title = "Orbital Crew";
    watched_tv.media_type = reddmedia::RecommendationMediaType::Television;
    if (!engine.record_started(watched_movie, error) ||
        !engine.record_started(watched_tv, error)) return 7;

    std::vector<reddmedia::MediaDescriptor> local;
    reddmedia::MediaDescriptor movie = watched_movie;
    movie.id = "local-movie";
    movie.local_path = "/media/movie.mkv";
    movie.tmdb_id = "701";
    local.push_back(movie);
    reddmedia::MediaDescriptor television = watched_tv;
    television.id = "local-tv";
    television.tmdb_id = "702";
    local.push_back(television);

    for (const auto mode : {reddmedia::RecommendationMode::Usual,
                            reddmedia::RecommendationMode::Random}) {
        for (const auto source : {reddmedia::RecommendationSource::Local,
                                  reddmedia::RecommendationSource::External}) {
            for (const auto type : {reddmedia::RecommendationMediaType::Movie,
                                    reddmedia::RecommendationMediaType::Television}) {
                const int repeats = mode == reddmedia::RecommendationMode::Random ? 40 : 1;
                for (int repeat = 0; repeat < repeats; ++repeat) {
                    reddmedia::RecommendationRequest request;
                    request.mode = mode;
                    request.source = source;
                    request.media_type = type;
                    reddmedia::RecommendationResult result;
                    if (!engine.recommend(request, local, result, error)) {
                        std::fprintf(stderr, "recommendation failed: %s\n", error.c_str());
                        return 10;
                    }
                    if (result.item.media_type != type) return 11;
                    if (source == reddmedia::RecommendationSource::External &&
                        !expected_type(result, type)) return 12;
                    if (source == reddmedia::RecommendationSource::Local) {
                        const std::string wanted =
                            type == reddmedia::RecommendationMediaType::Movie
                                ? "local-movie" : "local-tv";
                        if (result.item.id != wanted) return 13;
                    }
                }
            }
        }
    }

    if (!engine.save_external_credential(read_token, error) ||
        engine.external_credential_label().find("read access token") == std::string::npos) return 14;
    if (!engine.test_external_credential(error)) return 15;
    std::string external_poster;
    if (!engine.load_external_poster_bmp("/poster.bmp", 180, 260, external_poster, error)) return 16;
    reddmedia::LibraryPoster decoded;
    if (!reddmedia::decode_library_poster_bmp(external_poster, decoded, error) ||
        decoded.width != 2 || decoded.height != 2) return 17;

    { std::ofstream state(state_file); state
        << "{\"Username\":\"ReddMedia\",\"AccessToken\":\"local-token\","
           "\"UserId\":\"user-one\"}"; }
    reddmedia::JellyfinApiClient client(state_file);
    std::string local_poster;
    if (!client.load_primary_image_bmp("movie-one", "tag-one", 132, 158,
                                       local_poster, error)) {
        std::fprintf(stderr, "local poster failed: %s\n", error.c_str());
        return 18;
    }
    if (!reddmedia::decode_library_poster_bmp(local_poster, decoded, error)) return 19;
    if (!engine.clear_external_credential(error) || engine.external_credential_available()) return 20;

    std::printf("type-separation=pass tmdb-api-key=pass tmdb-read-token=pass "
                "replacement-preserves-old=pass posters=pass clear=pass\n");
    return 0;
}
'''


class ThreadedServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True


class Handler(http.server.BaseHTTPRequestHandler):
    poster_requests = 0

    def authorized(self, parsed: urllib.parse.ParseResult) -> bool:
        query = urllib.parse.parse_qs(parsed.query)
        return (
            self.headers.get("Authorization") == f"Bearer {READ_TOKEN}"
            or query.get("api_key", [""])[0] == API_KEY
        )

    def send_json(self, status: int, value: object) -> None:
        data = json.dumps(value).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/Startup/User":
            self.send_response(403)
            self.end_headers()
            return
        if parsed.path == "/Users/Me":
            self.send_json(200, {"Id": "user-one"})
            return
        if parsed.path == "/Items/movie-one/Images/Primary":
            require(urllib.parse.parse_qs(parsed.query).get("format") == ["Jpg"],
                    "Jellyfin poster request did not use the supported source format")
            Handler.poster_requests += 1
            data = bmp()
            self.send_response(200)
            self.send_header("Content-Type", "image/bmp")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        if parsed.path == "/poster.bmp":
            data = bmp()
            self.send_response(200)
            self.send_header("Content-Type", "image/bmp")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        if parsed.path.startswith("/3/"):
            if not self.authorized(parsed):
                self.send_response(401)
                self.end_headers()
                return
            if parsed.path == "/3/configuration":
                self.send_json(200, {"images": {"secure_base_url": "http://127.0.0.1/"}})
                return
            is_tv = parsed.path == "/3/discover/tv"
            title_key = "name" if is_tv else "title"
            date_key = "first_air_date" if is_tv else "release_date"
            owned_id, external_id = ((702, 902) if is_tv else (701, 901))
            values = [
                {"id": owned_id, title_key: "Owned", date_key: "2024-01-01",
                 "overview": "owned", "genre_ids": [18], "poster_path": "/owned.bmp"},
                {"id": external_id, title_key: "External", date_key: "2024-02-01",
                 "overview": "external", "genre_ids": [18], "poster_path": "/poster.bmp"},
            ]
            self.send_json(200, {"page": 1, "total_pages": 1, "results": values})
            return
        self.send_response(404)
        self.end_headers()

    def log_message(self, *_args: object) -> None:
        pass


with tempfile.TemporaryDirectory(prefix="reddmedia-v17-tests-") as temporary:
    temp = pathlib.Path(temporary)
    compiler = shutil.which("g++")
    require(compiler is not None, "g++ is required for deterministic validation")
    harness_source = temp / "harness.cpp"
    harness_source.write_text(HARNESS, encoding="utf-8")
    harness_binary = temp / "harness"
    subprocess.run([
        compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-DREDDMEDIA_AI_STUB=1",
        f"-I{ROOT / 'src'}", str(harness_source),
        str(ROOT / "src/media_server/jellyfin_api_client.cpp"),
        str(ROOT / "src/media_server/library_poster.cpp"),
        str(ROOT / "src/recommendations/embedding_engine.cpp"),
        str(ROOT / "src/recommendations/recommendation_engine.cpp"),
        str(ROOT / "src/recommendations/tmdb_client.cpp"),
        str(ROOT / "src/recommendations/viewing_history.cpp"),
        "-ldl", "-pthread", "-o", str(harness_binary),
    ], check=True)

    server = ThreadedServer(("127.0.0.1", 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        jellyfin_server = ThreadedServer(("127.0.0.1", 8096), Handler)
    except OSError as exc:
        server.shutdown()
        server.server_close()
        thread.join(timeout=2)
        raise SystemExit(f"v0.0.17 poster test requires free localhost port 8096: {exc}") from exc
    jellyfin_thread = threading.Thread(target=jellyfin_server.serve_forever, daemon=True)
    jellyfin_thread.start()
    env = os.environ.copy()
    env["HOME"] = str(temp / "home")
    env["REDDMEDIA_TMDB_BASE_URL"] = f"http://127.0.0.1:{server.server_port}"
    env["REDDMEDIA_TMDB_IMAGE_BASE_URL"] = f"http://127.0.0.1:{server.server_port}"
    try:
        subprocess.run([
            str(harness_binary),
            str(temp / "history.sqlite3"),
            str(temp / "tmdb.credential"),
            str(temp / "jellyfin-state.json"),
            API_KEY,
            WRONG_KEY,
            READ_TOKEN,
        ], env=env, check=True, timeout=90)
    finally:
        jellyfin_server.shutdown()
        jellyfin_server.server_close()
        jellyfin_thread.join(timeout=2)
        server.shutdown()
        server.server_close()
        thread.join(timeout=2)

    require(Handler.poster_requests == 1, "Jellyfin poster path did not make exactly one real request")
    cache_files = list((temp / "home/.cache/reddmedia/posters").rglob("*.bmp"))
    require(len(cache_files) >= 2, "local and external poster caches were not created")

print("ReddMedia v0.0.17 deterministic validation PASS")
