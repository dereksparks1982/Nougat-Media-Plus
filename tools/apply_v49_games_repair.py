#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_BASELINE = "f65c320c68cf5451f1151c59fbb2bccc4f5c434e"
EXPECTED_BLOBS = {
    "src/main.cpp": "d5ece8c4b73e931a540ea03185b063a832f1a647",
    "src/games/emulator_host.cpp": "a9d491f276e0873d9f1efd14d2428e4dd0ea4ef4",
    "src/games/emulator_host.hpp": "bdec1c3f560c412e40de3a7c36bedb1fce107ad2",
    "CMakeLists.txt": "358d2e50de16f1e6fdaa2c35d2d96292acbecf35",
    "NougatMediaSuite.desktop": "a8eaacb6af2116f9e5596214d09375a91a9f5945",
    "com.elderredsoftworks.NougatMediaSuite.desktop": "a8eaacb6af2116f9e5596214d09375a91a9f5945",
}


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def git_blob(path: Path) -> str:
    result = run(["git", "hash-object", str(path.relative_to(ROOT))])
    need(result.returncode == 0, "git hash-object failed for " + str(path))
    return result.stdout.strip()


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    need(count == 1, f"{label}: expected exactly one baseline anchor, found {count}")
    return text.replace(old, new, 1)


def replace_region(text: str, start: str, end: str, replacement: str, label: str) -> str:
    start_at = text.find(start)
    need(start_at >= 0, f"{label}: start anchor missing")
    end_at = text.find(end, start_at)
    need(end_at >= 0, f"{label}: end anchor missing")
    return text[:start_at] + replacement.rstrip() + "\n\n    " + text[end_at:]


def patch_main(path: Path) -> None:
    text = path.read_text(encoding="utf-8")

    # Duplicate/region/revision selection helpers live directly beside the existing
    # game title normalizer so scanning and artwork use one identity model.
    anchor = '''static std::string game_title_from_path(const std::string& path) {
    std::string title = stem_only(path);
    for (char& c : title) if (c == '_' || c == '-') c = ' ';
    while (title.find("  ") != std::string::npos) title.replace(title.find("  "), 2, " ");
    return title.empty() ? basename_only(path) : title;
}
'''
    helpers = anchor + r'''
static std::string game_trim(std::string value) {
    auto removable = [](unsigned char c) {
        return std::isspace(c) != 0 || c == '-' || c == '_' || c == '.';
    };
    while (!value.empty() && removable(static_cast<unsigned char>(value.back()))) value.pop_back();
    std::size_t first = 0;
    while (first < value.size() && removable(static_cast<unsigned char>(value[first]))) ++first;
    return value.substr(first);
}

static std::string game_source_stem(const GameEntry& game) {
    return stem_only(game.archived ? game.archive_entry : game.path);
}

static std::string game_clean_display_title(const GameEntry& game) {
    std::string title = game_title_from_path(game_source_stem(game));
    const std::size_t round = title.find('(');
    const std::size_t square = title.find('[');
    std::size_t cut = std::string::npos;
    if (round != std::string::npos) cut = round;
    if (square != std::string::npos) cut = cut == std::string::npos ? square : std::min(cut, square);
    if (cut != std::string::npos) title = title.substr(0, cut);
    title = game_trim(title);
    return title.empty() ? game.title : title;
}

static std::string game_variant_identity(const std::string& source_lower) {
    // Hacks/translations/homebrew are intentionally distinct editions. Betas,
    // prototypes and demos are not preserved here because the final retail build
    // should replace them when both are present.
    static const char* words[] = {"hack", "translation", "homebrew", "fan translation", "unlicensed"};
    std::string variant;
    std::size_t pos = 0;
    while (pos < source_lower.size()) {
        const std::size_t open_round = source_lower.find('(', pos);
        const std::size_t open_square = source_lower.find('[', pos);
        std::size_t open = std::string::npos;
        char close_char = ')';
        if (open_round != std::string::npos && (open_square == std::string::npos || open_round < open_square)) {
            open = open_round;
            close_char = ')';
        } else if (open_square != std::string::npos) {
            open = open_square;
            close_char = ']';
        }
        if (open == std::string::npos) break;
        const std::size_t close = source_lower.find(close_char, open + 1U);
        if (close == std::string::npos) break;
        const std::string tag = source_lower.substr(open + 1U, close - open - 1U);
        bool special = false;
        for (const char* word : words) if (tag.find(word) != std::string::npos) special = true;
        if (special) {
            if (!variant.empty()) variant += "|";
            variant += tag;
        }
        pos = close + 1U;
    }
    return variant;
}

static std::string game_normalized_identity(const GameEntry& game) {
    std::string base = lower_copy(game_clean_display_title(game));
    std::string normalized;
    bool last_space = false;
    for (unsigned char c : base) {
        if (std::isalnum(c) != 0) {
            normalized.push_back(static_cast<char>(c));
            last_space = false;
        } else if (!last_space && !normalized.empty()) {
            normalized.push_back(' ');
            last_space = true;
        }
    }
    normalized = game_trim(normalized);
    const std::string variant = game_variant_identity(lower_copy(game_source_stem(game)));
    if (!variant.empty()) normalized += "::variant::" + variant;
    return normalized.empty() ? lower_copy(game.title) : normalized;
}

static int game_region_rank(const GameEntry& game) {
    const std::string source = lower_copy(game_source_stem(game));
    const auto has = [&source](const std::string& token) { return source.find(token) != std::string::npos; };

    // Owner rule: USA wins before revision. Known English-region fallbacks win
    // only when USA is absent; known non-English releases are last-resort cards.
    if (has("(usa)") || has("[usa]") || has("(us)") || has("[us]") ||
        has("(u)") || has("[u]") || has("ntsc-u") || has("_usa") || has("-usa")) return 4;

    if (has("(english)") || has("[english]") || has("(en)") || has("[en]") ||
        has("(en,") || has("[en,") || has("(europe)") || has("[europe]") ||
        has("(uk)") || has("[uk]") || has("(united kingdom)") ||
        has("(australia)") || has("[australia]") || has("(canada)") ||
        has("[canada]") || has("(world)") || has("[world]")) return 3;

    if (has("(japan)") || has("[japan]") || has("(j)") || has("[j]") ||
        has("(korea)") || has("[korea]") || has("(china)") || has("[china]") ||
        has("(germany)") || has("[germany]") || has("(france)") || has("[france]") ||
        has("(spain)") || has("[spain]") || has("(italy)") || has("[italy]") ||
        has("(brazil)") || has("[brazil]")) return 1;

    // Untagged sets are kept above a known non-English release because many
    // personal dumps omit region tags entirely.
    return 2;
}

static bool game_is_prerelease(const GameEntry& game) {
    const std::string source = lower_copy(game_source_stem(game));
    static const char* tokens[] = {
        "beta", "prototype", "proto", "demo", "sample", "preview",
        "pre-release", "prerelease", "alpha", "kiosk"
    };
    for (const char* token : tokens) if (source.find(token) != std::string::npos) return true;
    return false;
}

static int game_parse_number_after(const std::string& source, std::size_t pos) {
    while (pos < source.size() && (std::isspace(static_cast<unsigned char>(source[pos])) != 0 || source[pos] == ':' || source[pos] == '=')) ++pos;
    if (pos >= source.size()) return 0;
    if (std::isalpha(static_cast<unsigned char>(source[pos])) != 0) {
        return 100 + (std::tolower(static_cast<unsigned char>(source[pos])) - 'a' + 1);
    }
    int major = 0;
    bool any = false;
    while (pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos])) != 0) {
        any = true;
        major = std::min(9999, major * 10 + (source[pos] - '0'));
        ++pos;
    }
    int minor = 0;
    if (pos < source.size() && source[pos] == '.') {
        ++pos;
        int places = 0;
        while (pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos])) != 0 && places < 3) {
            minor = minor * 10 + (source[pos] - '0');
            ++pos;
            ++places;
        }
    }
    return any ? major * 1000 + minor : 0;
}

static int game_revision_rank(const GameEntry& game) {
    const std::string source = lower_copy(game_source_stem(game));
    int best = 0;
    static const char* revision_markers[] = {"rev ", "revision "};
    for (const char* marker : revision_markers) {
        std::size_t pos = 0;
        while ((pos = source.find(marker, pos)) != std::string::npos) {
            best = std::max(best, game_parse_number_after(source, pos + std::strlen(marker)));
            pos += std::strlen(marker);
        }
    }
    // Common v1.0 / Version 1.1 style release tags. Require a delimiter before
    // bare 'v' so titles such as V-Rally are not mistaken for version metadata.
    std::size_t pos = 0;
    while ((pos = source.find('v', pos)) != std::string::npos) {
        const bool delimiter = pos == 0U || std::isspace(static_cast<unsigned char>(source[pos - 1U])) != 0 ||
                               source[pos - 1U] == '(' || source[pos - 1U] == '[';
        if (delimiter && pos + 1U < source.size() && std::isdigit(static_cast<unsigned char>(source[pos + 1U])) != 0)
            best = std::max(best, game_parse_number_after(source, pos + 1U));
        ++pos;
    }
    pos = source.find("version ");
    if (pos != std::string::npos) best = std::max(best, game_parse_number_after(source, pos + 8U));
    return best;
}

static bool game_candidate_preferred(const GameEntry& candidate, const GameEntry& current) {
    const int candidate_region = game_region_rank(candidate);
    const int current_region = game_region_rank(current);
    if (candidate_region != current_region) return candidate_region > current_region;

    const bool candidate_final = !game_is_prerelease(candidate);
    const bool current_final = !game_is_prerelease(current);
    if (candidate_final != current_final) return candidate_final;

    const int candidate_revision = game_revision_rank(candidate);
    const int current_revision = game_revision_rank(current);
    if (candidate_revision != current_revision) return candidate_revision > current_revision;

    if (candidate.archived != current.archived) return !candidate.archived;
    if (candidate.bundled != current.bundled) return !candidate.bundled;
    const std::string candidate_source = lower_copy(game_source_stem(candidate));
    const std::string current_source = lower_copy(game_source_stem(current));
    return candidate_source < current_source;
}

static std::vector<GameEntry> filter_game_library_preferences(std::vector<GameEntry> games,
                                                               std::size_t& filtered_count) {
    std::map<std::string, GameEntry> winners;
    filtered_count = 0;
    for (GameEntry& game : games) {
        const std::string identity = game.system + "::" + game_normalized_identity(game);
        const auto found = winners.find(identity);
        if (found == winners.end()) {
            game.title = game_clean_display_title(game);
            winners.emplace(identity, std::move(game));
            continue;
        }
        if (game_candidate_preferred(game, found->second)) {
            game.title = game_clean_display_title(game);
            found->second = std::move(game);
        }
        ++filtered_count;
    }
    std::vector<GameEntry> result;
    result.reserve(winners.size());
    for (auto& entry : winners) result.push_back(std::move(entry.second));
    return result;
}
'''
    text = replace_once(text, anchor, helpers, "game preference helpers")

    old_scan_sort = '''            std::stable_sort(games.begin(), games.end(), [](const GameEntry& a, const GameEntry& b) {
                if (a.system != b.system) return a.system < b.system;
                return lower_copy(a.title) < lower_copy(b.title);
            });
            std::lock_guard<std::mutex> lock(state->mutex);
            state->games = std::move(games);
            state->busy = false;
            state->updated = true;
            state->loaded = true;
            state->status = state->games.empty()
                ? "No games found. Add a ROM folder or install the bundled starter library."
                : std::to_string(state->games.size()) + (state->games.size() == 1U ? " game indexed." : " games indexed.");
'''
    new_scan_sort = '''            std::size_t filtered = 0;
            games = filter_game_library_preferences(std::move(games), filtered);
            std::stable_sort(games.begin(), games.end(), [](const GameEntry& a, const GameEntry& b) {
                if (a.system != b.system) return a.system < b.system;
                return lower_copy(a.title) < lower_copy(b.title);
            });
            std::lock_guard<std::mutex> lock(state->mutex);
            state->games = std::move(games);
            state->busy = false;
            state->updated = true;
            state->loaded = true;
            if (state->games.empty()) {
                state->status = "No games found. Add a ROM folder or install the bundled starter library.";
            } else {
                state->status = std::to_string(state->games.size()) +
                    (state->games.size() == 1U ? " game indexed." : " games indexed.");
                if (filtered > 0U) state->status += " " + std::to_string(filtered) +
                    " older/foreign duplicate variant" + (filtered == 1U ? " filtered." : "s filtered.");
            }
'''
    text = replace_once(text, old_scan_sort, new_scan_sort, "game scan preference filter")

    old_poll_scan = '''        if (count == 0U) gamesSelected = -1;
        else if (gamesSelected < 0 || gamesSelected >= static_cast<int>(count)) gamesSelected = 0;
        gamesScroll = std::max(0, std::min(gamesScroll, std::max(0, static_cast<int>(count) - 1)));
        if (!fullscreen && currentView == ViewMode::Games) redraw();
'''
    new_poll_scan = '''        if (count == 0U) gamesSelected = -1;
        else if (gamesSelected < 0 || gamesSelected >= static_cast<int>(count)) gamesSelected = 0;
        gamesScroll = std::max(0, std::min(gamesScroll, std::max(0, static_cast<int>(count) - 1)));
        if (!busy) start_game_artwork_prefetch();
        if (!fullscreen && currentView == ViewMode::Games) redraw();
'''
    text = replace_once(text, old_poll_scan, new_poll_scan, "game scan artwork prefetch hook")

    old_emulator_head = '''        const std::string bundledMesen = exe_dir() + "/components/games/runtime/mesen2/Mesen";
        const std::string bundledRmg = exe_dir() + "/components/games/runtime/rmg/AppRun";
        const std::string bundledAtari800 = exe_dir() + "/components/games/runtime/atari800/AppRun";
'''
    new_emulator_head = '''        const std::string bundledMesen = exe_dir() + "/components/games/runtime/mesen2/Mesen";
        const std::string bundledRmg = exe_dir() + "/components/games/runtime/rmg/AppRun";
        const std::string bundledAtari800 = exe_dir() + "/components/games/runtime/atari800/AppRun";
        const std::string bundledStella = exe_dir() + "/components/games/runtime/stella/stella";
        if (system == "Atari 2600" && exists_file(bundledStella) && access(bundledStella.c_str(), X_OK) == 0)
            return bundledStella;
'''
    text = replace_once(text, old_emulator_head, new_emulator_head, "bundled Stella emulator")

    artwork_start = '''static std::string url_encode_component(const std::string& value) {'''
    artwork_end = '''std::string game_executable_on_path(const std::string& name) const {'''
    artwork_block = r'''static std::string game_manifest_escape(const std::string& value) {
        std::string out;
        out.reserve(value.size());
        for (char c : value) {
            if (c == '\\') out += "\\\\";
            else if (c == '\t') out += "\\t";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else out.push_back(c);
        }
        return out;
    }

    std::string game_source_identity(const GameEntry& game) const {
        return game.archived ? game.path + "::" + game.archive_entry : game.path;
    }

    std::string game_cached_remote_artwork_path(const GameEntry& game) const {
        const std::filesystem::path dir = std::filesystem::path(home_dir()) / ".cache" / "reddmedia" / "games" / "artwork";
        return (dir / (std::to_string(stable_game_cache_hash(game_source_identity(game))) + ".png")).string();
    }

    std::string game_prepared_artwork_path(const GameEntry& game) const {
        const std::filesystem::path dir = std::filesystem::path(home_dir()) / ".cache" / "reddmedia" / "games" / "artwork-prepared-v49";
        return (dir / (std::to_string(stable_game_cache_hash(game_source_identity(game))) + ".bmp")).string();
    }

    std::string bundled_game_artwork_path(const GameEntry& game) const {
        if (!game.bundled) return {};
        const std::string name = lower_copy(basename_only(game.path));
        const std::filesystem::path dir = std::filesystem::path(exe_dir()) / "components" / "games" / "bundled" / "artwork";
        if (name == "2048.nes") return (dir / "2048.png").string();
        if (name == "waveforms.nes") return (dir / "Waveforms.png").string();
        return {};
    }

    std::string game_artwork_manifest_path() const {
        return (std::filesystem::path(home_dir()) / ".cache" / "reddmedia" / "games" / "artwork-v49-manifest.tsv").string();
    }

    void stop_game_artwork_prefetch() {
        const pid_t pid = gameArtworkPrefetchPid;
        gameArtworkPrefetchPid = -1;
        if (pid <= 1) return;
        if (kill(-pid, SIGTERM) != 0 && errno != ESRCH) kill(pid, SIGTERM);
        for (int i = 0; i < 12; ++i) {
            int status = 0;
            const pid_t waited = waitpid(pid, &status, WNOHANG);
            if (waited == pid || (waited < 0 && errno == ECHILD)) return;
            usleep(25000);
        }
        if (kill(-pid, SIGKILL) != 0 && errno != ESRCH) kill(pid, SIGKILL);
        int status = 0;
        (void)waitpid(pid, &status, WNOHANG);
    }

    void start_game_artwork_prefetch() {
        std::vector<GameEntry> games;
        {
            std::lock_guard<std::mutex> lock(gameState->mutex);
            if (gameState->busy) return;
            games = gameState->games;
        }
        stop_game_artwork_prefetch();
        if (games.empty()) return;

        const std::filesystem::path cache_dir = std::filesystem::path(home_dir()) / ".cache" / "reddmedia" / "games";
        const std::filesystem::path prepared_dir = cache_dir / "artwork-prepared-v49";
        std::error_code ec;
        std::filesystem::create_directories(prepared_dir, ec);
        if (ec) return;
        chmod(cache_dir.string().c_str(), 0700);
        chmod(prepared_dir.string().c_str(), 0700);

        const std::string manifest = game_artwork_manifest_path();
        const std::string temporary = manifest + ".tmp";
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) return;
        for (const GameEntry& game : games) {
            output << game_manifest_escape(game.system) << '\t'
                   << game_manifest_escape(game_source_identity(game)) << '\t'
                   << game_manifest_escape(game_source_stem(game)) << '\t'
                   << game_manifest_escape(game.title) << '\t'
                   << game_manifest_escape(game.artwork_path) << '\t'
                   << game_manifest_escape(bundled_game_artwork_path(game)) << '\t'
                   << game_manifest_escape(game_cached_remote_artwork_path(game)) << '\t'
                   << game_manifest_escape(game_prepared_artwork_path(game)) << '\n';
        }
        output.close();
        if (!output) { unlink(temporary.c_str()); return; }
        chmod(temporary.c_str(), 0600);
        if (rename(temporary.c_str(), manifest.c_str()) != 0) {
            unlink(temporary.c_str());
            return;
        }

        const std::string worker = exe_dir() + "/components/games/artwork_cache_worker.py";
        if (!exists_file(worker)) return;
        const std::string log_path = (cache_dir / "artwork-v49-worker.log").string();
        const pid_t child = fork();
        if (child < 0) return;
        if (child == 0) {
            setpgid(0, 0);
            const int fd = open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                if (fd > STDERR_FILENO) close(fd);
            }
            execlp("python3", "python3", worker.c_str(), "--manifest", manifest.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }
        setpgid(child, child);
        gameArtworkPrefetchPid = child;
        lastGameArtworkPrefetchPollMs = 0;
        lastGameArtworkPrefetchRedrawMs = 0;
    }

    void poll_game_artwork_prefetch() {
        if (gameArtworkPrefetchPid <= 1) return;
        const long long now = now_ms();
        if (now - lastGameArtworkPrefetchPollMs < 100) return;
        lastGameArtworkPrefetchPollMs = now;
        int status = 0;
        const pid_t waited = waitpid(gameArtworkPrefetchPid, &status, WNOHANG);
        if (waited == gameArtworkPrefetchPid || (waited < 0 && errno == ECHILD)) {
            gameArtworkPrefetchPid = -1;
            gameArtworkFailed.clear();
            gamesInteractiveRedrawPending = true;
            return;
        }
        if (!fullscreen && currentView == ViewMode::Games && now - lastGameArtworkPrefetchRedrawMs >= 750) {
            lastGameArtworkPrefetchRedrawMs = now;
            gamesInteractiveRedrawPending = true;
        }
    }

    bool cached_game_artwork(const GameEntry& game, reddmedia::LibraryPoster& poster) {
        const std::string artwork_path = game_prepared_artwork_path(game);
        if (!exists_file(artwork_path)) return false;
        const auto cached = gameArtworkCache.find(artwork_path);
        if (cached != gameArtworkCache.end()) { poster = cached->second; return true; }
        if (gameArtworkFailed.count(artwork_path) != 0U) return false;

        std::ifstream in(artwork_path, std::ios::binary);
        if (!in) return false;
        std::ostringstream buffer;
        buffer << in.rdbuf();
        std::string error;
        if (!reddmedia::decode_library_poster_bmp(buffer.str(), poster, error)) {
            gameArtworkFailed.insert(artwork_path);
            return false;
        }
        gameArtworkCache[artwork_path] = poster;
        return true;
    }
'''
    text = replace_region(text, artwork_start, artwork_end, artwork_block, "persistent game artwork pipeline")

    old_fields = '''    std::map<std::string, reddmedia::LibraryPoster> gameArtworkCache;
    std::set<std::string> gameArtworkFailed;
    std::map<std::string,long long> gameArtworkRequested;
    long long lastGameArtworkPollMs=0;
'''
    new_fields = '''    std::map<std::string, reddmedia::LibraryPoster> gameArtworkCache;
    std::set<std::string> gameArtworkFailed;
    pid_t gameArtworkPrefetchPid = -1;
    long long lastGameArtworkPrefetchPollMs = 0;
    long long lastGameArtworkPrefetchRedrawMs = 0;
    bool gamesInteractiveRedrawPending = false;
    long long lastGamesInteractiveRedrawMs = 0;
    bool stellaTopOptionsArmed = true;
    long long lastStellaTopOptionsMs = 0;
'''
    text = replace_once(text, old_fields, new_fields, "game artwork/scroll fields")

    old_mesen_launch = '''        // Mesen documents ROM path first, then --fullscreen.
        if (backend_lower == "mesen" ||
            backend_lower == "mesen2") {
            request.argv =
                {emulator, launchPath, "--fullscreen"};
            return true;
        }

        const bool atari800_backend =
'''
    new_mesen_launch = '''        // Mesen documents ROM path first, then --fullscreen.
        if (backend_lower == "mesen" ||
            backend_lower == "mesen2") {
            request.argv =
                {emulator, launchPath, "--fullscreen"};
            return true;
        }

        // Stella uses a clean game surface with its integrated Options UI on Tab.
        // Keep it windowed for Nougat's native reparenting host; Nougat owns the
        // player geometry and exposes Stella's Options from the top-edge gesture.
        if (backend_lower == "stella") {
            request.argv = {emulator, "-fullscreen", "0", "-center", "0", launchPath};
            return true;
        }

        const bool atari800_backend =
'''
    text = replace_once(text, old_mesen_launch, new_mesen_launch, "Stella launch presentation")

    old_games_scroll = '''    bool handle_games_scrollbar_press(int x,int y) {
        if (!gamesVerticalScrollTrack.contains(x,y)) return false;
        if (gamesVerticalScrollThumb.contains(x,y)) {
            gamesVerticalScrollDragging=true; gamesVerticalScrollDragOffset=y-gamesVerticalScrollThumb.y;
        } else { update_games_vertical_scroll_from_pointer(y,true); redraw(); }
        return true;
    }

    bool handle_games_scrollbar_motion(int y) {
        if (!gamesVerticalScrollDragging) return false;
        const int before=gamesScroll; update_games_vertical_scroll_from_pointer(y,false);
        if (before!=gamesScroll) redraw();
        return true;
    }
'''
    new_games_scroll = '''    void request_games_interactive_redraw(bool immediate=false) {
        const long long now = now_ms();
        if (immediate || now - lastGamesInteractiveRedrawMs >= 33) {
            gamesInteractiveRedrawPending = false;
            lastGamesInteractiveRedrawMs = now;
            if (!fullscreen && currentView == ViewMode::Games) redraw();
        } else {
            gamesInteractiveRedrawPending = true;
        }
    }

    void flush_games_interactive_redraw() {
        if (!gamesInteractiveRedrawPending || fullscreen || currentView != ViewMode::Games) return;
        const long long now = now_ms();
        if (now - lastGamesInteractiveRedrawMs < 33) return;
        gamesInteractiveRedrawPending = false;
        lastGamesInteractiveRedrawMs = now;
        redraw();
    }

    bool handle_games_wheel_steps(Window target, int x, int y, int steps) {
        if (steps == 0 || currentView != ViewMode::Games || target != win ||
            gamesPanel != GamesPanel::Library || !gamesListBox.contains(x, y)) return false;
        std::size_t count = 0;
        { std::lock_guard<std::mutex> lock(gameState->mutex); count = gameState->games.size(); }
        const LibraryGridMetrics grid = games_grid_metrics();
        const int max_scroll = std::max(0, static_cast<int>(count) - grid.visibleItems);
        const int amount = gamesDisplayMode == GamesDisplayMode::Grid ? grid.columns : 1;
        gamesScroll = std::max(0, std::min(max_scroll, gamesScroll + steps * amount));
        request_games_interactive_redraw();
        return true;
    }

    bool handle_games_scrollbar_press(int x,int y) {
        if (!gamesVerticalScrollTrack.contains(x,y)) return false;
        if (gamesVerticalScrollThumb.contains(x,y)) {
            gamesVerticalScrollDragging=true; gamesVerticalScrollDragOffset=y-gamesVerticalScrollThumb.y;
        } else { update_games_vertical_scroll_from_pointer(y,true); request_games_interactive_redraw(true); }
        return true;
    }

    bool handle_games_scrollbar_motion(int y) {
        if (!gamesVerticalScrollDragging) return false;
        const int before=gamesScroll; update_games_vertical_scroll_from_pointer(y,false);
        if (before!=gamesScroll) request_games_interactive_redraw();
        return true;
    }
'''
    text = replace_once(text, old_games_scroll, new_games_scroll, "Games interactive scroll throttling")

    old_wheel_branch = '''        if (currentView == ViewMode::Games && target == win && gamesListBox.contains(x,y) && gamesPanel == GamesPanel::Library) {
            std::size_t count=0; { std::lock_guard<std::mutex> lock(gameState->mutex); count=gameState->games.size(); }
            const LibraryGridMetrics grid=games_grid_metrics();
            const int maxScroll=std::max(0,static_cast<int>(count)-grid.visibleItems);
            const int amount=gamesDisplayMode==GamesDisplayMode::Grid ? grid.columns : 1;
            gamesScroll=std::max(0,std::min(maxScroll,gamesScroll+(button==Button4?-amount:amount)));
            redraw(); return true;
        }
'''
    new_wheel_branch = '''        if (currentView == ViewMode::Games && target == win && gamesListBox.contains(x,y) && gamesPanel == GamesPanel::Library) {
            handle_games_wheel_steps(target, x, y, button == Button4 ? -1 : 1);
            return true;
        }
'''
    text = replace_once(text, old_wheel_branch, new_wheel_branch, "Games wheel redraw")

    old_card_actions = '''            else if (action==MenuAction::CardInfo) { { std::lock_guard<std::mutex> lock(gameState->mutex); gameState->status=game.title+" | "+game.system+" | "+(game.bundled?"Bundled":"Linked")+(game.archived?" | ZIP: "+game.archive_entry:""); gameState->updated=true; } redraw(); }
            else if (action==MenuAction::CardRefreshArtwork) { request_game_remote_artwork(game,true); redraw(); }
            else if (action==MenuAction::CardOpenArtwork) {
                std::string art=game.artwork_path; if (art.empty()) { const std::string bundled=bundled_game_artwork_path(game); art=exists_file(bundled)?bundled:game_cached_remote_artwork_path(game); }
                if (exists_file(art)) open_source_path_in_files(art);
            }
'''
    new_card_actions = '''            else if (action==MenuAction::CardInfo) { { std::lock_guard<std::mutex> lock(gameState->mutex); gameState->status=game.title+" | "+game.system+" | "+(game.bundled?"Bundled":"Linked")+(game.archived?" | ZIP: "+game.archive_entry:""); gameState->updated=true; } redraw(); }
            else if (action==MenuAction::CardRefreshArtwork) {
                const std::string remote = game_cached_remote_artwork_path(game);
                const std::string prepared = game_prepared_artwork_path(game);
                unlink(remote.c_str());
                unlink(prepared.c_str());
                gameArtworkCache.erase(prepared);
                gameArtworkFailed.erase(prepared);
                start_game_artwork_prefetch();
                redraw();
            }
            else if (action==MenuAction::CardOpenArtwork) {
                std::string art = game_prepared_artwork_path(game);
                if (!exists_file(art)) art = game.artwork_path;
                if (art.empty() || !exists_file(art)) {
                    const std::string bundled = bundled_game_artwork_path(game);
                    art = exists_file(bundled) ? bundled : game_cached_remote_artwork_path(game);
                }
                if (exists_file(art)) open_source_path_in_files(art);
            }
'''
    text = replace_once(text, old_card_actions, new_card_actions, "game artwork context actions")

    old_poll_call = '''            poll_game_scan();
            poll_game_artwork_downloads();
            poll_world_tv_resolver();
'''
    new_poll_call = '''            poll_game_scan();
            poll_game_artwork_prefetch();
            poll_world_tv_resolver();
'''
    text = replace_once(text, old_poll_call, new_poll_call, "artwork worker poll")

    # Coalesce Games wheel events before they can pile up behind expensive paints.
    old_button_press = '''                else if (e.type == ButtonPress) {
                    if (e.xbutton.button == 8U) navigate_back();
                    else if (e.xbutton.button == 9U) navigate_forward();
                    else if (e.xbutton.button == Button4 || e.xbutton.button == Button5) handle_wheel(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button);
                    else handle_button(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button, e.xbutton.time);
                }
'''
    new_button_press = '''                else if (e.type == ButtonPress) {
                    if (e.xbutton.button == 8U) navigate_back();
                    else if (e.xbutton.button == 9U) navigate_forward();
                    else if (e.xbutton.button == Button4 || e.xbutton.button == Button5) {
                        if (currentView == ViewMode::Games && e.xbutton.window == win &&
                            gamesPanel == GamesPanel::Library && gamesListBox.contains(e.xbutton.x, e.xbutton.y)) {
                            int steps = e.xbutton.button == Button4 ? -1 : 1;
                            int wheel_x = e.xbutton.x;
                            int wheel_y = e.xbutton.y;
                            XEvent queued;
                            while (XCheckTypedWindowEvent(d, win, ButtonPress, &queued)) {
                                if ((queued.xbutton.button == Button4 || queued.xbutton.button == Button5) &&
                                    gamesListBox.contains(queued.xbutton.x, queued.xbutton.y)) {
                                    steps += queued.xbutton.button == Button4 ? -1 : 1;
                                    wheel_x = queued.xbutton.x;
                                    wheel_y = queued.xbutton.y;
                                    continue;
                                }
                                XPutBackEvent(d, &queued);
                                break;
                            }
                            handle_games_wheel_steps(win, wheel_x, wheel_y, steps);
                        } else {
                            handle_wheel(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button);
                        }
                    }
                    else handle_button(e.xbutton.window, e.xbutton.x, e.xbutton.y, e.xbutton.button, e.xbutton.time);
                }
'''
    text = replace_once(text, old_button_press, new_button_press, "Games wheel event coalescing")

    old_release = '''                else if (e.type == ButtonRelease) {
                    if (currentView == ViewMode::Nougat) nougatOutputSelecting = false;
                    volumeDragging = false;
                    homeVerticalScrollDragging = false;
                    homeContinueScrollDragging = false;
                    libraryVerticalScrollDragging = false;
                    gamesVerticalScrollDragging = false;
                    discoverServicesScrollDragging = false;
                }
'''
    new_release = '''                else if (e.type == ButtonRelease) {
                    if (currentView == ViewMode::Nougat) nougatOutputSelecting = false;
                    const bool finishedGamesDrag = gamesVerticalScrollDragging;
                    volumeDragging = false;
                    homeVerticalScrollDragging = false;
                    homeContinueScrollDragging = false;
                    libraryVerticalScrollDragging = false;
                    gamesVerticalScrollDragging = false;
                    discoverServicesScrollDragging = false;
                    if (finishedGamesDrag) request_games_interactive_redraw(true);
                }
'''
    text = replace_once(text, old_release, new_release, "Games drag final frame")

    # Flush at most one pending Games paint after the queued X events have been drained.
    old_after_events = '''            run_pending_video_click();
            tick_resume_seek();
'''
    new_after_events = '''            flush_games_interactive_redraw();
            run_pending_video_click();
            tick_resume_seek();
'''
    text = replace_once(text, old_after_events, new_after_events, "Games pending redraw flush")

    # Stella top-edge Options gesture, matching the clean Mesen presentation contract.
    old_poll_game = '''    void launch_selected_game() {
'''
    stella_poll = r'''    void poll_stella_top_options() {
        if (!currentMediaIsGame || activeGameSystem != "Atari 2600" || !gameHost.embedded()) {
            stellaTopOptionsArmed = true;
            return;
        }
        int pointer_x = 0;
        int pointer_y = 0;
        if (!gameHost.pointer_position(pointer_x, pointer_y)) return;
        const long long now = now_ms();
        if (pointer_y > 48) stellaTopOptionsArmed = true;
        if (pointer_x >= 0 && pointer_y >= 0 && pointer_y <= 6 &&
            stellaTopOptionsArmed && now - lastStellaTopOptionsMs >= 1000) {
            if (gameHost.send_key(XK_Tab)) {
                stellaTopOptionsArmed = false;
                lastStellaTopOptionsMs = now;
            }
        }
    }

    void launch_selected_game() {
'''
    text = replace_once(text, old_poll_game, stella_poll, "Stella top-edge options gesture")

    old_top_loop = '''        while (running) {
            poll_game_session();
            while (XPending(d)) {
'''
    new_top_loop = '''        while (running) {
            poll_game_session();
            poll_stella_top_options();
            while (XPending(d)) {
'''
    text = replace_once(text, old_top_loop, new_top_loop, "Stella options poll")

    old_shutdown_game = '''        if (gameScanWorker.joinable()) {
            bool busy=false;
            { std::lock_guard<std::mutex> lock(gameState->mutex); busy=gameState->busy; }
            if (busy) gameScanWorker.detach(); else gameScanWorker.join();
        }
'''
    new_shutdown_game = '''        stop_game_artwork_prefetch();
        if (gameScanWorker.joinable()) {
            bool busy=false;
            { std::lock_guard<std::mutex> lock(gameState->mutex); busy=gameState->busy; }
            if (busy) gameScanWorker.detach(); else gameScanWorker.join();
        }
'''
    text = replace_once(text, old_shutdown_game, new_shutdown_game, "artwork worker shutdown")

    text = replace_once(text,
                        'printf("Nougat Media Suite v0.0.48\\n");',
                        'printf("Nougat Media Suite v0.0.49\\n");',
                        "v49 version identity")

    version_hook = '''    if (argc > 1 && std::string(argv[1]) == "--v47-fullscreen-controls-self-test") {
'''
    self_test = r'''    if (argc > 1 && std::string(argv[1]) == "--v49-games-self-test") {
        std::vector<GameEntry> games;
        const auto add = [&games](const char* path) {
            GameEntry game;
            game.path = path;
            game.system = "Atari 2600";
            game.title = game_title_from_path(path);
            games.push_back(std::move(game));
        };
        add("/tmp/Mario (Japan) (Rev 3).bin");
        add("/tmp/Mario (USA) (Rev 1).bin");
        add("/tmp/Mario (USA) (Rev 2).bin");
        add("/tmp/Adventure (Japan) (Rev 2).bin");
        add("/tmp/Adventure (Europe) (Rev 1).bin");
        add("/tmp/River Raid (USA) (Beta).bin");
        add("/tmp/River Raid (USA) (Rev 1).bin");
        add("/tmp/Only Japan (Japan).bin");
        std::size_t filtered = 0;
        const std::vector<GameEntry> preferred = filter_game_library_preferences(std::move(games), filtered);
        bool mario = false;
        bool adventure = false;
        bool river = false;
        bool japan_only = false;
        for (const GameEntry& game : preferred) {
            const std::string source = game_source_stem(game);
            if (game.title == "Mario") mario = source.find("(USA) (Rev 2)") != std::string::npos;
            if (game.title == "Adventure") adventure = source.find("(Europe) (Rev 1)") != std::string::npos;
            if (game.title == "River Raid") river = source.find("(USA) (Rev 1)") != std::string::npos;
            if (game.title == "Only Japan") japan_only = source.find("(Japan)") != std::string::npos;
        }
        if (!mario || !adventure || !river || !japan_only || preferred.size() != 4U || filtered != 4U) {
            std::fprintf(stderr,
                "Nougat v0.0.49 Games preference self-test FAIL. mario=%d english=%d final=%d foreign-only=%d visible=%zu filtered=%zu\\n",
                mario, adventure, river, japan_only, preferred.size(), filtered);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.49 Games preference PASS: USA first, English fallback, newest final revision, foreign-only fallback.\\n");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--v47-fullscreen-controls-self-test") {
'''
    text = replace_once(text, version_hook, self_test, "v49 Games native self-test")

    path.write_text(text, encoding="utf-8")


def patch_host_header(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    old = '''    HostEvent poll();
    void resize(int width, int height);
    void focus();
    void stop();
'''
    new = '''    HostEvent poll();
    void resize(int width, int height);
    void focus();
    bool pointer_position(int& x, int& y) const;
    bool send_key(KeySym keysym);
    void stop();
'''
    text = replace_once(text, old, new, "emulator host input API")
    path.write_text(text, encoding="utf-8")


def patch_host_cpp(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    anchor = '''void EmulatorHost::stop() {
'''
    addition = r'''bool EmulatorHost::pointer_position(int& x, int& y) const {
    x = 0;
    y = 0;
    if (!impl_->display || !impl_->embedded_window) return false;
    Window root_return = 0;
    Window child_return = 0;
    int root_x = 0;
    int root_y = 0;
    unsigned int mask = 0;
    XErrorTrap trap(impl_->display);
    const Bool ok = XQueryPointer(impl_->display, impl_->embedded_window,
                                  &root_return, &child_return,
                                  &root_x, &root_y, &x, &y, &mask);
    return ok != False && trap.sync_ok();
}

bool EmulatorHost::send_key(KeySym keysym) {
    if (!impl_->display || !impl_->embedded_window || keysym == NoSymbol) return false;
    const KeyCode code = XKeysymToKeycode(impl_->display, keysym);
    if (code == 0) return false;

    XWindowAttributes attrs{};
    if (!safe_window_attributes(impl_->display, impl_->embedded_window, attrs)) return false;

    XKeyEvent key{};
    key.display = impl_->display;
    key.window = impl_->embedded_window;
    key.root = impl_->root;
    key.subwindow = None;
    key.time = CurrentTime;
    key.x = 1;
    key.y = 1;
    key.x_root = attrs.x + 1;
    key.y_root = attrs.y + 1;
    key.same_screen = True;
    key.keycode = code;

    XErrorTrap trap(impl_->display);
    XEvent event{};
    key.type = KeyPress;
    event.xkey = key;
    if (XSendEvent(impl_->display, impl_->embedded_window, True, KeyPressMask, &event) == 0) return false;
    key.type = KeyRelease;
    event.xkey = key;
    if (XSendEvent(impl_->display, impl_->embedded_window, True, KeyReleaseMask, &event) == 0) return false;
    XFlush(impl_->display);
    return trap.sync_ok();
}

void EmulatorHost::stop() {
'''
    text = replace_once(text, anchor, addition, "emulator host pointer/key implementation")
    path.write_text(text, encoding="utf-8")


def patch_cmake(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    text = text.replace("VERSION 0.0.48", "VERSION 0.0.49")
    text = text.replace("Nougat_Media_Suite_v48", "Nougat_Media_Suite_v49")
    need("0.0.48" not in text and "Nougat_Media_Suite_v48" not in text, "CMake v48 identity remained")
    path.write_text(text, encoding="utf-8")


def patch_desktop(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    text = replace_once(text, "Nougat_Media_Suite_v48", "Nougat_Media_Suite_v49", path.name + " executable")
    path.write_text(text, encoding="utf-8")


def already_applied() -> bool:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8") if (ROOT / "CMakeLists.txt").is_file() else ""
    main = (ROOT / "src/main.cpp").read_text(encoding="utf-8") if (ROOT / "src/main.cpp").is_file() else ""
    return "Nougat_Media_Suite_v49" in cmake and "--v49-games-self-test" in main and "artwork-prepared-v49" in main


def main() -> int:
    try:
        need((ROOT / ".git").exists(), "run this from the Nougat Media Suite project tree")
        if already_applied():
            print("PASS: v0.0.49 Games repair is already applied.")
            return 0

        head = run(["git", "rev-parse", "HEAD"])
        need(head.returncode == 0, "could not read Git HEAD")
        need(head.stdout.strip() == EXPECTED_BASELINE,
             "v0.0.49 requires accepted v0.0.48 baseline " + EXPECTED_BASELINE +
             "; current HEAD is " + head.stdout.strip())

        for relative, expected in EXPECTED_BLOBS.items():
            path = ROOT / relative
            need(path.is_file(), relative + " is missing")
            actual = git_blob(path)
            need(actual == expected,
                 f"{relative} differs from accepted v0.0.48 baseline; expected blob {expected}, got {actual}")

        patch_main(ROOT / "src/main.cpp")
        patch_host_header(ROOT / "src/games/emulator_host.hpp")
        patch_host_cpp(ROOT / "src/games/emulator_host.cpp")
        patch_cmake(ROOT / "CMakeLists.txt")
        patch_desktop(ROOT / "NougatMediaSuite.desktop")
        patch_desktop(ROOT / "com.elderredsoftworks.NougatMediaSuite.desktop")

        for relative in (
            "tools/install_game_runtimes_v49.py",
            "tools/check_game_runtimes_v49.py",
            "tools/test_nougat_media_suite_v49.py",
            "tools/build_v49.py",
            "components/games/artwork_cache_worker.py",
        ):
            target = ROOT / relative
            need(target.is_file(), "candidate payload is missing " + relative)
            target.chmod(target.stat().st_mode | 0o111)

        print("=== v0.0.49 GAMES REPAIR APPLIED ===")
        print("PASS: accepted v0.0.48 source baseline verified before patching")
        print("PASS: USA/English/newest-final game filtering installed")
        print("PASS: Stella Atari 2600 runtime wiring installed")
        print("PASS: persistent background artwork preparation installed")
        print("PASS: Games wheel/scrollbar coalescing and frame-limited redraw installed")
        print("PASS: Stella top-edge Options gesture wired through embedded host")
        print("NO GIT COMMIT, TAG, OR PUSH WAS PERFORMED.")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("Terminal remains open. No Git action was performed.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
