#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BASELINE_HEAD = "b23b146f0df2b7c68c3e0b32152074d78f8ea1c3"
BASELINE_MAIN_BLOB = "b850dffb60ba33c9c9264e00053e6d818b31956d"
BASELINE_CMAKE_BLOB = "59ef38a0a9be3bf68212f70de297235b5db676d8"


def fail(message: str) -> None:
    print("STOP:", message)
    print("Nothing was committed or pushed. Terminal remains open.")
    raise SystemExit(1)


def git(*args: str) -> str:
    p = subprocess.run(["git", *args], cwd=ROOT, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode != 0:
        fail("git " + " ".join(args) + " failed:\n" + p.stdout)
    return p.stdout.strip()


def require(text: str, token: str, label: str) -> None:
    if token not in text:
        fail(f"{label}: required anchor not found: {token!r}")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        fail(f"{label}: expected exactly one anchor, found {count}: {old!r}")
    return text.replace(old, new, 1)


def find_block_end(text: str, brace: int) -> int:
    depth = 0
    i = brace
    state = "code"
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c == "/" and n == "/":
                state = "line"; i += 2; continue
            if c == "/" and n == "*":
                state = "block"; i += 2; continue
            if c == '"':
                state = "string"; i += 1; continue
            if c == "'":
                state = "char"; i += 1; continue
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    return i + 1
            i += 1
            continue
        if state == "line":
            if c == "\n": state = "code"
            i += 1
            continue
        if state == "block":
            if c == "*" and n == "/":
                state = "code"; i += 2
            else:
                i += 1
            continue
        if state in ("string", "char"):
            if c == "\\":
                i += 2
                continue
            if (state == "string" and c == '"') or (state == "char" and c == "'"):
                state = "code"
            i += 1
            continue
    fail("unterminated C++ block while applying v48")


def replace_function(text: str, signature: str, replacement: str) -> str:
    start = text.find(signature)
    if start < 0:
        fail("function anchor missing: " + signature)
    if text.find(signature, start + 1) >= 0:
        fail("function anchor is not unique: " + signature)
    brace = text.find("{", start + len(signature))
    if brace < 0:
        fail("function has no opening brace: " + signature)
    end = find_block_end(text, brace)
    return text[:start] + replacement.rstrip() + text[end:]


def insert_before_function(text: str, signature: str, insertion: str) -> str:
    start = text.find(signature)
    if start < 0:
        fail("insertion anchor missing: " + signature)
    if text.find(signature, start + 1) >= 0:
        fail("insertion anchor not unique: " + signature)
    return text[:start] + insertion.rstrip() + "\n\n    " + text[start:]


def inject_before_function_close(text: str, signature: str, insertion: str) -> str:
    start = text.find(signature)
    if start < 0:
        fail("function anchor missing: " + signature)
    brace = text.find("{", start + len(signature))
    end = find_block_end(text, brace)
    return text[:end - 1] + insertion + text[end - 1:]


def inject_after_function_open(text: str, signature: str, insertion: str) -> str:
    start = text.find(signature)
    if start < 0:
        fail("function anchor missing: " + signature)
    brace = text.find("{", start + len(signature))
    return text[:brace + 1] + insertion + text[brace + 1:]


GAME_SYSTEM = r'''static std::string game_system_for_path(const std::string& path) {
    const std::string lower = lower_copy(path);
    if (ends_with_lower(lower, ".nes")) return "NES";
    if (ends_with_lower(lower, ".sfc") || ends_with_lower(lower, ".smc")) return "SNES";
    if (ends_with_lower(lower, ".gb")) return "Game Boy";
    if (ends_with_lower(lower, ".gbc")) return "Game Boy Color";
    if (ends_with_lower(lower, ".gba")) return "Game Boy Advance";
    if (ends_with_lower(lower, ".n64") || ends_with_lower(lower, ".z64") || ends_with_lower(lower, ".v64")) return "Nintendo 64";
    if (ends_with_lower(lower, ".a26")) return "Atari 2600";
    if (ends_with_lower(lower, ".a52")) return "Atari 5200";
    if (ends_with_lower(lower, ".a78")) return "Atari 7800";
    if (ends_with_lower(lower, ".atr") || ends_with_lower(lower, ".xfd")) return "Atari 8-bit";
    if (ends_with_lower(lower, ".lnx")) return "Atari Lynx";
    if (ends_with_lower(lower, ".xex") || ends_with_lower(lower, ".iso")) return "Xbox 360";
    return {};
}'''

SAFE_ZIP = r'''static bool safe_zip_game_entry(const std::string& entry) {
    if (entry.empty() || entry.front() == '/' || entry.front() == '\\') return false;
    if (entry.find("../") != std::string::npos || entry.find("..\\") != std::string::npos) return false;
    const std::string system = game_system_for_path(entry);
    // Xbox 360 images can be many gigabytes. Keep them at the linked game location.
    return !system.empty() && system != "Xbox 360";
}'''

DOS_HELPERS = r'''
static std::string game_directory_artwork(const std::string& directory) {
    const std::filesystem::path folder(directory);
    const std::string name = folder.filename().string();
    const std::filesystem::path candidates[] = {
        folder / "cover.png", folder / "cover.jpg", folder / "cover.jpeg", folder / "cover.bmp",
        folder / (name + ".png"), folder / (name + ".jpg"), folder / (name + ".jpeg"), folder / (name + ".bmp")
    };
    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate, ec)) return candidate.string();
        ec.clear();
    }
    return {};
}

static int dos_launcher_score(const std::filesystem::path& file,
                              const std::string& directory_name_lower) {
    const std::string extension = lower_copy(file.extension().string());
    if (extension != ".exe" && extension != ".com" && extension != ".bat") return -1;

    const std::string stem = lower_copy(file.stem().string());
    static const std::set<std::string> reject = {
        "setup", "install", "installer", "uninstall", "unins000", "config",
        "configure", "setsound", "sound", "readme", "help"
    };
    if (reject.count(stem) != 0U) return -1;
    if (stem.rfind("unins", 0U) == 0U) return -1;

    int score = 10;
    std::string compact_dir;
    for (char c : directory_name_lower)
        if (std::isalnum(static_cast<unsigned char>(c))) compact_dir.push_back(c);
    std::string compact_stem;
    for (char c : stem)
        if (std::isalnum(static_cast<unsigned char>(c))) compact_stem.push_back(c);

    if (!compact_dir.empty() && !compact_stem.empty()) {
        if (compact_dir == compact_stem) score += 140;
        else if (compact_dir.find(compact_stem) != std::string::npos ||
                 compact_stem.find(compact_dir) != std::string::npos) score += 80;
    }
    if (stem == "start" || stem == "play" || stem == "run" || stem == "game") score += 110;
    if (extension == ".bat") score += 8;
    return score;
}

static std::string dos_entrypoint_for_directory(const std::string& directory) {
    const std::filesystem::path folder(directory);
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) return {};

    const std::string dir_name = lower_copy(folder.filename().string());
    int best_score = -1;
    std::string best_name;
    for (std::filesystem::directory_iterator it(folder, std::filesystem::directory_options::skip_permission_denied, ec), end;
         it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        if (!it->is_regular_file(ec)) { ec.clear(); continue; }
        const int score = dos_launcher_score(it->path(), dir_name);
        if (score > best_score || (score == best_score && it->path().filename().string() < best_name)) {
            best_score = score;
            best_name = it->path().filename().string();
        }
    }
    return best_score >= 10 ? best_name : std::string{};
}

static void scan_dos_game_directories(const std::string& root, bool bundled,
                                      std::vector<GameEntry>& games,
                                      std::set<std::string>& seen) {
    if (bundled) return;
    std::error_code ec;
    const std::filesystem::path base(root);
    if (!std::filesystem::is_directory(base, ec)) return;

    const auto add_directory = [&](const std::filesystem::path& folder) -> bool {
        const std::string entrypoint = dos_entrypoint_for_directory(folder.string());
        if (entrypoint.empty()) return false;
        const std::string key = "dos::" + folder.string();
        if (!seen.insert(key).second) return true;
        GameEntry game;
        game.title = game_title_from_path(folder.filename().string());
        game.path = folder.string();
        game.system = "DOS";
        game.bundled = false;
        game.artwork_path = game_directory_artwork(folder.string());
        game.entry_point = entrypoint;
        game.directory_game = true;
        games.push_back(std::move(game));
        return true;
    };

    if (add_directory(base)) return;

    const auto options = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::recursive_directory_iterator it(base, options, ec), end;
    for (; it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        if (it->is_symlink(ec) || !it->is_directory(ec)) { ec.clear(); continue; }
        if (add_directory(it->path())) it.disable_recursion_pending();
    }
}
'''

SCAN_GAME = r'''static void scan_game_directory(const std::string& root, bool bundled,
                                std::vector<GameEntry>& games,
                                std::set<std::string>& seen) {
    std::error_code ec;
    const std::filesystem::path base(root);
    if (!std::filesystem::is_directory(base, ec)) return;
    const auto options = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::recursive_directory_iterator it(base, options, ec), end;
    for (; it != end; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        const auto& entry = *it;
        if (entry.is_symlink(ec) || !entry.is_regular_file(ec)) continue;
        const std::string path = entry.path().string();
        const std::string system = game_system_for_path(path);
        if (!system.empty()) {
            if (system == "Xbox 360" && ends_with_lower(path, ".xex") &&
                lower_copy(entry.path().filename().string()) != "default.xex" &&
                std::filesystem::is_regular_file(entry.path().parent_path() / "default.xex", ec)) {
                ec.clear();
                continue;
            }
            if (!seen.insert(path).second) continue;
            std::string title = game_title_from_path(path);
            if (system == "Xbox 360" &&
                lower_copy(entry.path().filename().string()) == "default.xex") {
                title = game_title_from_path(entry.path().parent_path().filename().string());
            }
            games.push_back({title, path, system, bundled, false, {}, game_sidecar_artwork(path)});
            continue;
        }
        if (!ends_with_lower(path, ".zip")) continue;
        const std::string listing = run_command_capture("unzip -Z1 " + shell_quote(path) + " 2>/dev/null");
        if (listing.empty()) continue;
        std::istringstream lines(listing);
        std::string archivedName;
        while (std::getline(lines, archivedName)) {
            while (!archivedName.empty() && archivedName.back() == '\r') archivedName.pop_back();
            if (!safe_zip_game_entry(archivedName)) continue;
            const std::string key = path + "::" + archivedName;
            if (!seen.insert(key).second) continue;
            games.push_back({game_title_from_path(archivedName), path, game_system_for_path(archivedName), bundled,
                             true, archivedName, game_sidecar_artwork(path)});
        }
    }
}'''

GAME_METHODS = r'''
    std::string game_executable_on_path(const std::string& name) const {
        if (name.empty()) return {};
        if (name.find('/') != std::string::npos) {
            return exists_file(name) && access(name.c_str(), X_OK) == 0 ? name : std::string{};
        }
        const char* path_env = std::getenv("PATH");
        if (!path_env) return {};
        std::istringstream paths(path_env);
        std::string part;
        while (std::getline(paths, part, ':')) {
            if (part.empty()) part = ".";
            const std::filesystem::path candidate = std::filesystem::path(part) / name;
            if (exists_file(candidate.string()) && access(candidate.string().c_str(), X_OK) == 0)
                return candidate.string();
        }
        return {};
    }

    std::string first_game_executable(const std::vector<std::string>& candidates) const {
        for (const std::string& candidate : candidates) {
            const std::string found = game_executable_on_path(candidate);
            if (!found.empty()) return found;
        }
        return {};
    }

    std::string game_emulator_log_path() const {
        const std::filesystem::path dir = std::filesystem::path(home_dir()) /
            ".cache" / "reddmedia" / "games" / "logs";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) return {};
        chmod(dir.string().c_str(), 0700);
        return (dir / "embedded-emulator.log").string();
    }

    static std::string dosbox_quote(const std::string& value) {
        std::string out = "\"";
        for (char c : value) {
            if (c == '"') out += "\\\"";
            else out.push_back(c);
        }
        out += "\"";
        return out;
    }

    bool make_game_launch_request(const GameEntry& selected,
                                  const std::string& launchPath,
                                  nougat::games::LaunchRequest& request,
                                  std::string& error) const {
        request.title = selected.title;
        request.log_path = game_emulator_log_path();
        request.window_timeout_ms = selected.system == "Xbox 360" ? 90000 : 45000;
        request.environment = {
            {"SDL_VIDEODRIVER", "x11"},
            {"QT_QPA_PLATFORM", "xcb"},
            {"GDK_BACKEND", "x11"}
        };

        if (selected.system == "DOS") {
            if (!selected.directory_game || selected.entry_point.empty()) {
                error = "That DOS folder does not have a detected game launcher.";
                return false;
            }
            const char* override_value = std::getenv("NOUGAT_DOSBOX");
            std::vector<std::string> candidates;
            if (override_value && *override_value) candidates.emplace_back(override_value);
            candidates.push_back(exe_dir() + "/components/games/runtime/dosbox-staging/dosbox");
            candidates.push_back("dosbox-staging");
            candidates.push_back("dosbox");
            const std::string emulator = first_game_executable(candidates);
            if (emulator.empty()) {
                error = "DOSBox Staging/DOSBox is unavailable. Install it, place it in "
                        "components/games/runtime/dosbox-staging/, or set NOUGAT_DOSBOX.";
                return false;
            }
            request.backend = basename_only(emulator);
            request.argv = {
                emulator,
                "-c", "mount c " + dosbox_quote(launchPath),
                "-c", "c:",
                "-c", dosbox_quote(selected.entry_point),
                "-c", "exit"
            };
            return true;
        }

        if (selected.system == "Xbox 360") {
            request.environment.push_back({"APPIMAGE_EXTRACT_AND_RUN", "1"});
            const char* native_override = std::getenv("NOUGAT_XENIA");
            std::vector<std::string> native_candidates;
            if (native_override && *native_override) native_candidates.emplace_back(native_override);
            native_candidates.push_back(exe_dir() + "/components/games/runtime/xenia/xenia_canary");
            native_candidates.push_back(exe_dir() + "/components/games/runtime/xenia/xenia");
            native_candidates.push_back("xenia_canary");
            native_candidates.push_back("xenia-canary");
            native_candidates.push_back("xenia");

            const std::string native_xenia = first_game_executable(native_candidates);
            if (!native_xenia.empty() && !ends_with_lower(native_xenia, ".exe")) {
                request.backend = basename_only(native_xenia);
                request.argv = {native_xenia, launchPath};
                return true;
            }

            std::string windows_xenia;
            if (native_override && *native_override && ends_with_lower(native_override, ".exe") &&
                exists_file(native_override)) {
                windows_xenia = native_override;
            }
            if (windows_xenia.empty()) {
                const std::string canary = exe_dir() + "/components/games/runtime/xenia/xenia_canary.exe";
                const std::string master = exe_dir() + "/components/games/runtime/xenia/xenia.exe";
                if (exists_file(canary)) windows_xenia = canary;
                else if (exists_file(master)) windows_xenia = master;
            }

            if (!windows_xenia.empty()) {
                const char* runner_override = std::getenv("NOUGAT_XENIA_RUNNER");
                std::vector<std::string> runners;
                if (runner_override && *runner_override) runners.emplace_back(runner_override);
                runners.push_back("umu-run");
                runners.push_back("wine64");
                runners.push_back("wine");
                const std::string runner = first_game_executable(runners);
                if (runner.empty()) {
                    error = "Xenia Canary for Windows was found, but Nougat cannot find a Linux "
                            "runner. Set NOUGAT_XENIA_RUNNER to your Proton/Wine launcher.";
                    return false;
                }
                request.backend = basename_only(windows_xenia) + " via " + basename_only(runner);
                request.argv = {runner, windows_xenia, launchPath};
                return true;
            }

            error = "Xenia Canary is unavailable. Put a native build or xenia_canary.exe in "
                    "components/games/runtime/xenia/, or set NOUGAT_XENIA.";
            return false;
        }

        const std::string emulator = installed_game_emulator(selected.system);
        if (emulator.empty()) {
            error = "No supported " + selected.system + " emulator is available.";
            return false;
        }

        request.backend = basename_only(emulator);
        const bool atari800_backend = basename_only(emulator) == "AppRun" &&
                                      emulator.find("/atari800/") != std::string::npos;
        if (selected.system == "Atari 5200" &&
            (atari800_backend || basename_only(emulator) == "atari800")) {
            request.argv = {emulator, "-5200", launchPath};
        } else {
            request.argv = {emulator, launchPath};
        }
        return true;
    }

    void stop_game_session(bool returnToGames) {
        gameHost.stop();
        currentMediaIsGame = false;
        activeGameTitle.clear();
        activeGameSystem.clear();
        if (returnToGames && currentView == ViewMode::VideoPlayer) {
            switch_view(ViewMode::Games);
            gamesPanel = GamesPanel::Library;
        }
        if (d) redraw();
    }

    void poll_game_session() {
        if (!currentMediaIsGame && !gameHost.active()) return;
        const nougat::games::HostEvent event = gameHost.poll();
        if (!event.changed) return;

        {
            std::lock_guard<std::mutex> lock(gameState->mutex);
            if (!event.message.empty()) gameState->status = event.message;
            gameState->updated = true;
        }

        if (event.state == nougat::games::HostState::Failed ||
            event.state == nougat::games::HostState::Exited) {
            currentMediaIsGame = false;
            activeGameTitle.clear();
            activeGameSystem.clear();
            if (currentView == ViewMode::VideoPlayer) {
                switch_view(ViewMode::Games);
                gamesPanel = GamesPanel::Library;
            }
            redraw();
            return;
        }

        if (event.state == nougat::games::HostState::Embedded) {
            gameHost.resize(videoW, videoH);
            gameHost.focus();
        }
        redraw();
    }
'''

LAUNCH_GAME = r'''void launch_selected_game() {
        GameEntry selected;
        {
            std::lock_guard<std::mutex> lock(gameState->mutex);
            if (gamesSelected < 0 || gamesSelected >= static_cast<int>(gameState->games.size())) {
                gameState->status = "Select a game first.";
                gameState->updated = true;
                return;
            }
            selected = gameState->games[static_cast<std::size_t>(gamesSelected)];
        }

        std::string launchPath;
        if (selected.system == "DOS" && selected.directory_game) {
            std::error_code ec;
            if (std::filesystem::is_directory(selected.path, ec)) launchPath = selected.path;
        } else {
            launchPath = extracted_game_path(selected);
        }

        if (launchPath.empty()) {
            std::lock_guard<std::mutex> lock(gameState->mutex);
            gameState->status = selected.archived
                ? "Nougat could not safely extract that ROM from its ZIP archive."
                : "That game is unavailable. Press Refresh after reconnecting its game folder.";
            gameState->updated = true;
            return;
        }

        nougat::games::LaunchRequest request;
        std::string error;
        if (!make_game_launch_request(selected, launchPath, request, error)) {
            std::lock_guard<std::mutex> lock(gameState->mutex);
            gameState->status = error;
            gameState->updated = true;
            return;
        }

        cleanup_player();
        switch_view(ViewMode::VideoPlayer);
        currentMediaIsGame = true;
        activeGameTitle = selected.title;
        activeGameSystem = selected.system;

        if (!gameHost.start(d, win, video, videoW, videoH, request, error)) {
            currentMediaIsGame = false;
            activeGameTitle.clear();
            activeGameSystem.clear();
            switch_view(ViewMode::Games);
            std::lock_guard<std::mutex> lock(gameState->mutex);
            gameState->status = error;
            gameState->updated = true;
            redraw();
            return;
        }

        {
            std::lock_guard<std::mutex> lock(gameState->mutex);
            gameState->status = "Starting " + selected.title + " inside Nougat Video Player...";
            gameState->updated = true;
        }
        redraw();
    }'''


def patch_main(original: str) -> str:
    text = original
    text = replace_once(
        text,
        '#include "p2p_engine.hpp"',
        '#include "p2p_engine.hpp"\n#include "games/emulator_host.hpp"',
        "emulator host include",
    )
    text = replace_once(
        text,
        '''    std::string artwork_path;
};''',
        '''    std::string artwork_path;
    std::string entry_point;
    bool directory_game = false;
};''',
        "GameEntry extension",
    )
    text = replace_function(text, "static std::string game_system_for_path(", GAME_SYSTEM)
    text = replace_function(text, "static bool safe_zip_game_entry(", SAFE_ZIP)

    sidecar_start = text.find("static std::string game_sidecar_artwork(")
    if sidecar_start < 0:
        fail("game_sidecar_artwork anchor missing")
    sidecar_brace = text.find("{", sidecar_start)
    sidecar_end = find_block_end(text, sidecar_brace)
    text = text[:sidecar_end] + "\n" + DOS_HELPERS.rstrip() + text[sidecar_end:]

    text = replace_function(text, "static void scan_game_directory(", SCAN_GAME)
    text = replace_once(
        text,
        '''for (const std::string& folder : userFolders) scan_game_directory(folder, false, games, seen);''',
        '''for (const std::string& folder : userFolders) {
                scan_dos_game_directories(folder, false, games, seen);
                scan_game_directory(folder, false, games, seen);
            }''',
        "DOS scan integration",
    )
    text = replace_once(
        text,
        '''    std::shared_ptr<GameUiState> gameState = std::make_shared<GameUiState>();
    std::thread gameScanWorker;''',
        '''    std::shared_ptr<GameUiState> gameState = std::make_shared<GameUiState>();
    std::thread gameScanWorker;
    nougat::games::EmulatorHost gameHost;
    bool currentMediaIsGame = false;
    std::string activeGameTitle;
    std::string activeGameSystem;''',
        "App game host state",
    )

    text = insert_before_function(text, "void launch_selected_game()", GAME_METHODS)
    text = replace_function(text, "void launch_selected_game()", LAUNCH_GAME)

    text = inject_after_function_open(
        text, "void cleanup_player()",
        '''
        if (currentMediaIsGame || gameHost.active()) stop_game_session(false);''',
    )
    text = inject_after_function_open(
        text, "void stop_media()",
        '''
        if (currentMediaIsGame || gameHost.active()) {
            stop_game_session(true);
            return;
        }''',
    )
    text = inject_after_function_open(
        text, "void draw_video_message()",
        '''
        if (currentMediaIsGame) {
            if (gameHost.embedded()) return;
            hide_player_activity_overlay_window();
            XClearWindow(d, video);
            const std::string message = activeGameTitle.empty()
                ? "Starting game inside Nougat..."
                : "Starting " + activeGameTitle + " inside Nougat...";
            text(video, 24, 34, head_to_width(message, std::max(120, videoW - 48)), rgb8(248,235,214));
            XFlush(d);
            return;
        }''',
    )
    text = inject_before_function_close(
        text, "void layout()",
        '''
        if (gameHost.active()) gameHost.resize(videoW, videoH);
''',
    )
    text = inject_after_function_open(
        text, "bool final_player_cleanup_bounded(",
        '''
        if (gameHost.active()) gameHost.stop();
        currentMediaIsGame = false;
        activeGameTitle.clear();
        activeGameSystem.clear();''',
    )
    text = replace_once(
        text,
        '''        while (running) {
            while (XPending(d)) {''',
        '''        while (running) {
            poll_game_session();
            while (XPending(d)) {''',
        "game host event-loop polling",
    )

    text = text.replace('"v0.0.47"', '"v0.0.48"', 1)
    text = text.replace('"Nougat Media Suite v0.0.47"', '"Nougat Media Suite v0.0.48"')
    text = text.replace('Nougat Media Suite v0.0.47\\n', 'Nougat Media Suite v0.0.48\\n')
    text = text.replace('"No user ROM folders linked yet."', '"No user game folders linked yet."')
    text = text.replace('"Link ROM Folder"', '"Link Game Folder"')
    text = text.replace(
        '"ZIP archives stay in place; selected ROMs extract only to Nougat\\\'s private cache."',
        '"ROM ZIP archives stay in place; DOS folders and Xbox 360 images launch from linked game locations."'
    )
    return text


def patch_cmake(original: str) -> str:
    text = original.replace(
        "project(NougatMediaSuite VERSION 0.0.47 LANGUAGES CXX)",
        "project(NougatMediaSuite VERSION 0.0.48 LANGUAGES CXX)",
    )
    text = text.replace("Nougat_Media_Suite_v47", "Nougat_Media_Suite_v48")
    text = replace_once(
        text,
        "    src/main.cpp\n",
        "    src/main.cpp\n    src/games/emulator_host.cpp\n",
        "CMake emulator host source",
    )
    return text


def main() -> None:
    if "--patcher-self-test" in sys.argv:
        source = '''        gameScanWorker = std::thread([state,userFolders,bundledFolder]() {
            std::vector<GameEntry> games;
            std::set<std::string> seen;
            scan_game_directory(bundledFolder, true, games, seen);
            for (const std::string& folder : userFolders) scan_game_directory(folder, false, games, seen);
        });'''
        result = replace_once(
            source,
            '''for (const std::string& folder : userFolders) scan_game_directory(folder, false, games, seen);''',
            '''for (const std::string& folder : userFolders) {
                scan_dos_game_directories(folder, false, games, seen);
                scan_game_directory(folder, false, games, seen);
            }''',
            "DOS scan integration self-test",
        )
        require(result, "scan_dos_game_directories(folder, false, games, seen);",
                "DOS scan integration self-test")
        print("PASS: v0.0.48 DOS scan patch is indentation-independent.")
        return

    main_cpp = ROOT / "src/main.cpp"
    cmake = ROOT / "CMakeLists.txt"
    if not main_cpp.is_file() or not cmake.is_file():
        fail("Extract this changed-files package over the Nougat Media Suite v0.0.47 repository.")

    head = git("rev-parse", "HEAD")
    if head != BASELINE_HEAD:
        fail(f"v48 patch requires v0.0.47 baseline {BASELINE_HEAD}; current HEAD is {head}")

    if '#include "games/emulator_host.hpp"' in main_cpp.read_text(encoding="utf-8"):
        print("v0.0.48 embedded-emulation patch already appears to be applied.")
        return

    dirty = subprocess.run(
        ["git", "diff", "--quiet", "--", "src/main.cpp", "CMakeLists.txt",
         "NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"],
        cwd=ROOT,
    )
    if dirty.returncode != 0:
        fail("v47 source files that v48 must patch already have local edits. No overwrite was attempted.")

    main_blob = git("hash-object", "src/main.cpp")
    cmake_blob = git("hash-object", "CMakeLists.txt")
    if main_blob != BASELINE_MAIN_BLOB:
        fail(f"src/main.cpp is not the verified v47 blob ({main_blob})")
    if cmake_blob != BASELINE_CMAKE_BLOB:
        fail(f"CMakeLists.txt is not the verified v47 blob ({cmake_blob})")

    original_main = main_cpp.read_text(encoding="utf-8")
    original_cmake = cmake.read_text(encoding="utf-8")
    patched_main = patch_main(original_main)
    patched_cmake = patch_cmake(original_cmake)

    for token in [
        '#include "games/emulator_host.hpp"', '"DOS"', '"Xbox 360"',
        "scan_dos_game_directories", "NOUGAT_DOSBOX", "NOUGAT_XENIA",
        "gameHost.start", "Nougat Media Suite v0.0.48",
    ]:
        require(patched_main, token, "patched main.cpp")
    require(patched_cmake, "Nougat_Media_Suite_v48", "patched CMake")
    require(patched_cmake, "src/games/emulator_host.cpp", "patched CMake")

    launcher_updates = []
    for name in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
        path = ROOT / name
        if path.is_file():
            source = path.read_text(encoding="utf-8")
            if "Nougat_Media_Suite_v47" not in source:
                fail(f"{name} no longer targets v47; refusing to guess")
            launcher_updates.append((path, source.replace("Nougat_Media_Suite_v47", "Nougat_Media_Suite_v48")))

    # All transformations succeeded in memory. Only now touch the working tree.
    main_cpp.write_text(patched_main, encoding="utf-8")
    cmake.write_text(patched_cmake, encoding="utf-8")
    for path, content in launcher_updates:
        path.write_text(content, encoding="utf-8")

    print("=== NOUGAT MEDIA SUITE v0.0.48 PATCH APPLIED ===")
    print(git("diff", "--stat"))
    print()
    print("Next command:")
    print("  python3 tools/build_v48.py")
    print()
    print("No commit, tag, or GitHub push was performed.")


if __name__ == "__main__":
    main()
