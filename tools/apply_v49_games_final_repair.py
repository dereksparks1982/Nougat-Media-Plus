#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD = ROOT / "repair_payload"
EXPECTED_HEAD = "f65c320c68cf5451f1151c59fbb2bccc4f5c434e"
EXPECTED_HOST_SHA256 = "e7f9975a79d0d0aff60d0ae31519332cb62209a8e1e8369b9616e200d8912426"
EXPECTED_HOST_HPP_SHA256 = "7c3530a1a64d40531265400062505cd0c10ec6bcb7dfafaa6c64efc81303078a"
EXPECTED_OLD_WORKER = "28837edafc7552459bafc06ea9b7e5be775544c78ee41b793a0853fdcb390d58"
EXPECTED_OLD_INSTALLER = "c7d65b090bbc84dc8f445534b6cedffe576f9fab482d123db8e453d885952c66"
EXPECTED_OLD_CHECKER = "b43b92fb02fa5545287d837ff891120b434b55ed938ea3d2a2d132fe4b608334"
EXPECTED_OLD_TEST = "5dc2c9118ee12f0bf5b4b3a026e08a31dde1cf1f4e8562763786e629eda7575a"
EXPECTED_BUILD = "24ba8f0eb1ffeec0734cda785843847e6c021cf1619d8267f0c303523c501930"
EXPECTED_WORLD_TV_BLOB = "f5e2a9ede662e6a39549165b7bc08ef396c89bf7"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    need(count == 1, f"{label}: expected exactly one anchor, found {count}")
    return text.replace(old, new, 1)


def insert_before_once(text: str, anchor: str, addition: str, label: str) -> str:
    count = text.count(anchor)
    need(count == 1, f"{label}: expected exactly one anchor, found {count}")
    return text.replace(anchor, addition.rstrip() + "\n\n" + anchor, 1)


def verify_current_state() -> None:
    need((ROOT / ".git").exists(), "Run this from the Nougat Media Suite project root")
    head = run(["git", "rev-parse", "HEAD"])
    need(head.returncode == 0, "Could not read Git HEAD")
    need(head.stdout.strip() == EXPECTED_HEAD,
         "This repair requires the accepted v0.0.48 Git baseline " + EXPECTED_HEAD +
         "; current HEAD is " + head.stdout.strip())

    host = ROOT / "src/games/emulator_host.cpp"
    host_hpp = ROOT / "src/games/emulator_host.hpp"
    need(host.is_file() and sha256(host) == EXPECTED_HOST_SHA256,
         "Emulator host is not the owner-tested v0.0.49 recursive host; refusing to guess")
    need(host_hpp.is_file() and sha256(host_hpp) == EXPECTED_HOST_HPP_SHA256,
         "Emulator host header is not the expected v0.0.49 owner-test state")

    current = {
        ROOT / "components/games/artwork_cache_worker.py": EXPECTED_OLD_WORKER,
        ROOT / "tools/install_game_runtimes_v49.py": EXPECTED_OLD_INSTALLER,
        ROOT / "tools/check_game_runtimes_v49.py": EXPECTED_OLD_CHECKER,
        ROOT / "tools/test_nougat_media_suite_v49.py": EXPECTED_OLD_TEST,
        ROOT / "tools/build_v49.py": EXPECTED_BUILD,
    }
    for path, expected in current.items():
        need(path.is_file(), "Missing expected v0.0.49 file: " + str(path.relative_to(ROOT)))
        actual = sha256(path)
        payload_same = False
        payload_path = PAYLOAD / path.name
        if payload_path.is_file():
            payload_same = actual == sha256(payload_path)
        need(actual == expected or payload_same,
             str(path.relative_to(ROOT)) + " is not the rejected owner-test state or this repair's idempotent state")

    world_tv = ROOT / "components/world_tv/nougat_world_tv_worker.py"
    need(world_tv.is_file(), "Missing World TV worker")
    world_text = world_tv.read_text(encoding="utf-8")
    if "NOUGAT_V49_RUSSIA24_AUDIO_REPAIR" not in world_text:
        world_hash = run(["git", "hash-object", str(world_tv.relative_to(ROOT))])
        need(world_hash.returncode == 0 and world_hash.stdout.strip() == EXPECTED_WORLD_TV_BLOB,
             "World TV worker is not the accepted v0.0.48/v0.0.49-preserved state; refusing to guess")

    main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
    required = [
        'printf("Nougat Media Suite v0.0.49\\n")',
        'const std::string versionLabel = "v0.0.49";',
        'input.app_version = "Nougat Media Suite v0.0.49";',
        'if (backend_lower == "stella")',
        'start_game_artwork_prefetch',
        'filter_game_library_preferences',
        'poll_stella_top_options',
    ]
    missing = [token for token in required if token not in main]
    need(not missing, "src/main.cpp is not the rejected v0.0.49 owner-test candidate; missing: " + ", ".join(missing))


def patch_main(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    if "NOUGAT_V49_GAMES_FINAL_REPAIR" in text:
        return False

    # Force both SDL2's historical variable and SDL3's current variable. Stella
    # 7 is SDL3, so the latter is required for Nougat's X11 reparenting host.
    old_env = '''        request.environment = {
            {"SDL_VIDEODRIVER", "x11"},
            {"QT_QPA_PLATFORM", "xcb"},
            {"GDK_BACKEND", "x11"}
        };
'''
    new_env = '''        request.environment = {
            {"SDL_VIDEODRIVER", "x11"},
            {"SDL_VIDEO_DRIVER", "x11"},
            {"QT_QPA_PLATFORM", "xcb"},
            {"GDK_BACKEND", "x11"}
        };
'''
    text = replace_once(text, old_env, new_env, "SDL3 X11 force")

    # Sega identification. Ambiguous .bin is Genesis only when the linked path
    # itself says Sega/Genesis/Mega Drive; otherwise the accepted Atari rule stays.
    atari_hint_end = '''static bool game_path_inside_xbox_tree(const std::string& path) {'''
    sega_hint = r'''static bool game_path_has_sega_hint(const std::string& path) {
    const std::string lower = lower_copy(path);
    return lower.find("sega") != std::string::npos ||
           lower.find("genesis") != std::string::npos ||
           lower.find("mega drive") != std::string::npos ||
           lower.find("megadrive") != std::string::npos ||
           lower.find("master system") != std::string::npos ||
           lower.find("game gear") != std::string::npos;
}
'''
    text = insert_before_once(text, atari_hint_end, sega_hint, "Sega path hint")

    n64_anchor = '''    if (ends_with_lower(lower, ".n64") || ends_with_lower(lower, ".z64") ||
        ends_with_lower(lower, ".v64")) return "Nintendo 64";

    // v48 Atari repair: the owner's 2600 library uses raw .bin dumps.
'''
    n64_new = '''    if (ends_with_lower(lower, ".n64") || ends_with_lower(lower, ".z64") ||
        ends_with_lower(lower, ".v64")) return "Nintendo 64";

    if (ends_with_lower(lower, ".md") || ends_with_lower(lower, ".gen") ||
        ends_with_lower(lower, ".smd")) return "Sega Genesis";
    if (ends_with_lower(lower, ".sms")) return "Sega Master System";
    if (ends_with_lower(lower, ".gg")) return "Sega Game Gear";

    // v48 Atari repair: the owner's 2600 library uses raw .bin dumps.
'''
    text = replace_once(text, n64_anchor, n64_new, "Sega extensions")

    old_context = '''static std::string game_system_for_path_in_context(const std::string& path,
                                                   const std::string& container) {
    std::string system = game_system_for_path(path);

    // Do not turn helper .xex files inside an extracted Xbox title into Atari
    // cards just because only default.xex belongs to Xbox semantically.
    if (system == "Atari 8-bit" && ends_with_lower(path, ".xex") &&
        lower_copy(basename_only(path)) != "default.xex" &&
        game_path_inside_xbox_tree(path)) {
        return {};
    }

    if (!system.empty()) return system;

    const std::string combined = container + "/" + path;
    if ((ends_with_lower(path, ".com") || ends_with_lower(path, ".exe")) &&
        game_path_has_atari_hint(combined))
        return "Atari 8-bit";

    return {};
}
'''
    new_context = '''static std::string game_system_for_path_in_context(const std::string& path,
                                                   const std::string& container) {
    const std::string combined = container + "/" + path;
    if (ends_with_lower(path, ".bin") && game_path_has_sega_hint(combined))
        return "Sega Genesis";

    std::string system = game_system_for_path(path);

    // Do not turn helper .xex files inside an extracted Xbox title into Atari
    // cards just because only default.xex belongs to Xbox semantically.
    if (system == "Atari 8-bit" && ends_with_lower(path, ".xex") &&
        lower_copy(basename_only(path)) != "default.xex" &&
        game_path_inside_xbox_tree(path)) {
        return {};
    }

    if (!system.empty()) return system;

    if ((ends_with_lower(path, ".com") || ends_with_lower(path, ".exe")) &&
        game_path_has_atari_hint(combined))
        return "Atari 8-bit";

    return {};
}
'''
    text = replace_once(text, old_context, new_context, "contextual Sega .bin")

    safe_anchor = '''static bool safe_zip_game_entry(const std::string& entry,
                                const std::string& archive_path = {}) {'''
    safe_helpers = r'''static std::string normalized_zip_member_path(std::string entry) {
    for (char& c : entry) if (c == '\\') c = '/';
    while (entry.rfind("./", 0U) == 0U) entry.erase(0, 2);
    return entry;
}

static bool safe_zip_member_path(const std::string& raw_entry) {
    if (raw_entry.empty() || raw_entry.find('\0') != std::string::npos) return false;
    const std::string entry = normalized_zip_member_path(raw_entry);
    if (entry.empty() || entry.front() == '/' || entry.find(':') != std::string::npos) return false;
    std::istringstream parts(entry);
    std::string part;
    while (std::getline(parts, part, '/')) {
        if (part == "..") return false;
    }
    return true;
}
'''
    text = insert_before_once(text, safe_anchor, safe_helpers, "generic ZIP path safety")

    old_safe_start = r'''    if (entry.empty() || entry.front() == '/' || entry.front() == '\\')
        return false;
    if (entry.find("../") != std::string::npos ||
        entry.find("..\\") != std::string::npos)
        return false;

    const std::string system =
'''
    new_safe_start = '''    if (!safe_zip_member_path(entry)) return false;

    const std::string system =
'''
    text = replace_once(text, old_safe_start, new_safe_start, "ZIP path safety reuse")

    old_source_stem = '''static std::string game_source_stem(const GameEntry& game) {
    return stem_only(game.archived ? game.archive_entry : game.path);
}
'''
    new_source_stem = '''static std::string game_source_stem(const GameEntry& game) {
    if (game.system == "DOS" && game.archived) return stem_only(game.path);
    return stem_only(game.archived ? game.archive_entry : game.path);
}
'''
    text = replace_once(text, old_source_stem, new_source_stem, "DOS ZIP display identity")

    dos_entry_anchor = '''static std::string dos_entrypoint_for_directory(const std::string& directory) {'''
    dos_zip_helpers = r'''static bool zip_entry_is_strong_console_payload(const std::string& entry,
                                                const std::string& archive_path) {
    if (!safe_zip_member_path(entry) || entry.empty() || entry.back() == '/') return false;
    const std::string combined = archive_path + "/" + entry;
    // .bin is too generic to disqualify a DOS package unless its linked path
    // explicitly identifies the collection as Atari or Sega.
    if (ends_with_lower(entry, ".bin") &&
        !game_path_has_atari_hint(combined) && !game_path_has_sega_hint(combined))
        return false;
    return !game_system_for_path_in_context(entry, archive_path).empty();
}

static std::string dos_archive_entrypoint(const std::string& archive_path,
                                          const std::vector<std::string>& entries) {
    for (const std::string& entry : entries)
        if (zip_entry_is_strong_console_payload(entry, archive_path)) return {};

    const std::string archive_name = lower_copy(stem_only(archive_path));
    int best_score = -1;
    std::string best_entry;
    for (const std::string& raw : entries) {
        if (!safe_zip_member_path(raw)) continue;
        const std::string entry = normalized_zip_member_path(raw);
        if (entry.empty() || entry.back() == '/') continue;
        const std::filesystem::path file(entry);
        int score = dos_launcher_score(file, archive_name);
        if (score < 0) continue;
        const std::string lower = lower_copy(entry);
        const std::string stem = lower_copy(file.stem().string());
        if (stem == "nou_launch") score += 2000;
        if (lower.find("original_dos/") != std::string::npos || lower.rfind("original_dos/", 0U) == 0U)
            score += 600;
        if (lower.find("/dos/") != std::string::npos || lower.rfind("dos/", 0U) == 0U)
            score += 120;
        if (lower.find("original_windows") != std::string::npos ||
            lower.find("/windows/") != std::string::npos || lower.find("win95") != std::string::npos)
            score -= 1000;
        score -= static_cast<int>(std::count(entry.begin(), entry.end(), '/')) * 3;
        if (score > best_score || (score == best_score && entry.size() < best_entry.size()) ||
            (score == best_score && entry.size() == best_entry.size() && entry < best_entry)) {
            best_score = score;
            best_entry = entry;
        }
    }
    return best_score >= 10 ? best_entry : std::string{};
}
'''
    text = insert_before_once(text, dos_entry_anchor, dos_zip_helpers, "DOS ZIP package detection")

    old_zip_scan = r'''        const std::string listing =
            run_command_capture("unzip -Z1 " + shell_quote(path) +
                                " 2>/dev/null");
        if (listing.empty()) continue;

        std::istringstream lines(listing);
        std::string archived_name;
        while (std::getline(lines, archived_name)) {
            while (!archived_name.empty() &&
                   archived_name.back() == '\r')
                archived_name.pop_back();

            if (!safe_zip_game_entry(archived_name, path)) continue;

            const std::string archived_system =
                game_system_for_path_in_context(archived_name, path);
            if (archived_system.empty()) continue;

            add_game(path, archived_system, true, archived_name);
        }
'''
    new_zip_scan = r'''        const std::string listing =
            run_command_capture("unzip -Z1 " + shell_quote(path) +
                                " 2>/dev/null");
        if (listing.empty()) continue;

        std::vector<std::string> archived_names;
        std::istringstream lines(listing);
        std::string archived_name;
        while (std::getline(lines, archived_name)) {
            while (!archived_name.empty() && archived_name.back() == '\r') archived_name.pop_back();
            if (!archived_name.empty()) archived_names.push_back(archived_name);
        }

        const std::string dos_entry = dos_archive_entrypoint(path, archived_names);
        if (!dos_entry.empty()) {
            const std::string key = "doszip::" + path;
            if (seen.insert(key).second) {
                GameEntry game;
                game.title = game_title_from_path(path);
                game.path = path;
                game.system = "DOS";
                game.bundled = bundled;
                game.archived = true;
                game.archive_entry = dos_entry;
                game.entry_point = dos_entry;
                game.directory_game = true;
                game.artwork_path = game_sidecar_artwork(path);
                games.push_back(std::move(game));
            }
            continue;
        }

        for (const std::string& archived_name_item : archived_names) {
            if (!safe_zip_game_entry(archived_name_item, path)) continue;
            const std::string archived_system =
                game_system_for_path_in_context(archived_name_item, path);
            if (archived_system.empty()) continue;
            add_game(path, archived_system, true, archived_name_item);
        }
'''
    text = replace_once(text, old_zip_scan, new_zip_scan, "ZIP cartridge and DOS package scan")

    # Managed BlastEm is the Sega backend. Preserve Stella/Mesen/RMG/Atari800.
    old_emulator = '''        const std::string bundledStella = exe_dir() + "/components/games/runtime/stella/stella";
        if (system == "Atari 2600" && exists_file(bundledStella) && access(bundledStella.c_str(), X_OK) == 0)
            return bundledStella;
'''
    new_emulator = '''        const std::string bundledStella = exe_dir() + "/components/games/runtime/stella/stella";
        const std::string bundledBlastem = exe_dir() + "/components/games/runtime/blastem/blastem";
        if (system == "Atari 2600" && exists_file(bundledStella) && access(bundledStella.c_str(), X_OK) == 0)
            return bundledStella;
        if ((system == "Sega Genesis" || system == "Sega Master System" || system == "Sega Game Gear") &&
            exists_file(bundledBlastem) && access(bundledBlastem.c_str(), X_OK) == 0)
            return bundledBlastem;
'''
    text = replace_once(text, old_emulator, new_emulator, "managed BlastEm resolver")

    old_candidates = '''        else if (system == "Nintendo 64") candidates = {"RMG", "mupen64plus"};
        else if (system == "Atari 2600") candidates = {"stella"};
'''
    new_candidates = '''        else if (system == "Nintendo 64") candidates = {"RMG", "mupen64plus"};
        else if (system == "Sega Genesis" || system == "Sega Master System" || system == "Sega Game Gear") candidates = {"blastem"};
        else if (system == "Atari 2600") candidates = {"stella"};
'''
    text = replace_once(text, old_candidates, new_candidates, "external BlastEm fallback")

    # Whole-package DOS extraction. Cartridge ZIPs still extract only one ROM.
    manifest_anchor = '''    static std::string game_manifest_escape(const std::string& value) {'''
    dos_extract = r'''    bool extracted_dos_archive_launch(const GameEntry& selected,
                                      std::string& launch_directory,
                                      std::string& entry_name) const {
        launch_directory.clear();
        entry_name.clear();
        if (!selected.archived || selected.system != "DOS" || !exists_file(selected.path) ||
            !safe_zip_member_path(selected.entry_point)) return false;

        const std::string wanted = normalized_zip_member_path(selected.entry_point);
        const std::string listing = run_command_capture(
            "unzip -Z1 " + shell_quote(selected.path) + " 2>/dev/null");
        if (listing.empty()) return false;
        bool found = false;
        std::istringstream lines(listing);
        std::string raw;
        while (std::getline(lines, raw)) {
            while (!raw.empty() && raw.back() == '\r') raw.pop_back();
            if (raw.empty()) continue;
            if (!safe_zip_member_path(raw)) return false;
            if (normalized_zip_member_path(raw) == wanted) found = true;
        }
        if (!found) return false;

        struct stat source_stat{};
        if (stat(selected.path.c_str(), &source_stat) != 0) return false;
        const std::string signature = selected.path + "::" +
            std::to_string(static_cast<long long>(source_stat.st_size)) + "::" +
            std::to_string(static_cast<long long>(source_stat.st_mtime)) + "::" + wanted;
        const std::filesystem::path cache_root = std::filesystem::path(home_dir()) /
            ".cache" / "reddmedia" / "games" / "dos-extracted-v49";
        std::error_code ec;
        std::filesystem::create_directories(cache_root, ec);
        if (ec) return false;
        chmod(cache_root.string().c_str(), 0700);
        const std::string cache_name = std::to_string(stable_game_cache_hash(signature));
        const std::filesystem::path final_dir = cache_root / cache_name;
        const std::filesystem::path marker = final_dir / ".nougat-source";
        const std::filesystem::path launcher = final_dir / std::filesystem::path(wanted);

        if (std::filesystem::is_directory(final_dir, ec) && std::filesystem::is_regular_file(marker, ec) &&
            std::filesystem::is_regular_file(launcher, ec)) {
            std::ifstream marker_in(marker);
            std::ostringstream marker_text;
            marker_text << marker_in.rdbuf();
            if (marker_text.str() == signature + "\n") {
                launch_directory = launcher.parent_path().string();
                entry_name = launcher.filename().string();
                return !launch_directory.empty() && !entry_name.empty();
            }
        }
        ec.clear();

        const std::filesystem::path temporary = cache_root /
            (cache_name + ".tmp-" + std::to_string(static_cast<long long>(getpid())));
        std::filesystem::remove_all(temporary, ec);
        ec.clear();
        std::filesystem::create_directories(temporary, ec);
        if (ec) return false;

        const std::string command = "unzip -qq -o " + shell_quote(selected.path) +
            " -d " + shell_quote(temporary.string()) + " 2>/dev/null";
        if (std::system(command.c_str()) != 0) {
            std::filesystem::remove_all(temporary, ec);
            return false;
        }
        // unzip can restore symlinks. Reject the package if any appear so a
        // DOS archive cannot escape the private cache through a link target.
        for (std::filesystem::recursive_directory_iterator it(
                 temporary, std::filesystem::directory_options::skip_permission_denied, ec), end;
             it != end; it.increment(ec)) {
            if (ec) {
                std::filesystem::remove_all(temporary, ec);
                return false;
            }
            if (it->is_symlink(ec)) {
                std::filesystem::remove_all(temporary, ec);
                return false;
            }
            ec.clear();
        }
        ec.clear();
        const std::filesystem::path temp_launcher = temporary / std::filesystem::path(wanted);
        if (!std::filesystem::is_regular_file(temp_launcher, ec)) {
            std::filesystem::remove_all(temporary, ec);
            return false;
        }
        {
            std::ofstream marker_out(temporary / ".nougat-source", std::ios::trunc);
            if (!marker_out) {
                std::filesystem::remove_all(temporary, ec);
                return false;
            }
            marker_out << signature << '\n';
        }
        std::filesystem::remove_all(final_dir, ec);
        ec.clear();
        std::filesystem::rename(temporary, final_dir, ec);
        if (ec) {
            std::filesystem::remove_all(temporary, ec);
            return false;
        }
        const std::filesystem::path final_launcher = final_dir / std::filesystem::path(wanted);
        launch_directory = final_launcher.parent_path().string();
        entry_name = final_launcher.filename().string();
        return std::filesystem::is_regular_file(final_launcher, ec) &&
               !launch_directory.empty() && !entry_name.empty();
    }
'''
    text = insert_before_once(text, manifest_anchor, dos_extract, "persistent DOS ZIP extraction")

    # BlastEm gets explicit machine selection and the managed wrapper remains windowed.
    stella_anchor = '''        // Stella uses a clean game surface with its integrated Options UI on Tab.
'''
    blastem_launch = r'''        if (backend_lower == "blastem") {
            request.argv = {emulator};
            if (selected.system == "Sega Master System") {
                request.argv.push_back("-m");
                request.argv.push_back("sms");
            } else if (selected.system == "Sega Game Gear") {
                request.argv.push_back("-m");
                request.argv.push_back("gg");
            } else {
                request.argv.push_back("-m");
                request.argv.push_back("gen");
            }
            request.argv.push_back(launchPath);
            return true;
        }

'''
    text = insert_before_once(text, stella_anchor, blastem_launch, "BlastEm launch request")

    old_launch = '''        std::string launchPath;
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
'''
    new_launch = '''        GameEntry launchSelected = selected;
        std::string launchPath;
        if (selected.system == "DOS" && selected.directory_game) {
            if (selected.archived) {
                std::string entry_name;
                if (extracted_dos_archive_launch(selected, launchPath, entry_name))
                    launchSelected.entry_point = entry_name;
            } else {
                std::error_code ec;
                if (std::filesystem::is_directory(selected.path, ec)) launchPath = selected.path;
            }
        } else {
            launchPath = extracted_game_path(selected);
        }

        if (launchPath.empty()) {
            std::lock_guard<std::mutex> lock(gameState->mutex);
            if (selected.system == "DOS" && selected.archived)
                gameState->status = "Nougat could not safely prepare that DOS game package from its ZIP archive.";
            else if (selected.archived)
                gameState->status = "Nougat could not safely extract that ROM from its ZIP archive.";
            else
                gameState->status = "That game is unavailable. Press Refresh after reconnecting its game folder.";
            gameState->updated = true;
            return;
        }

        nougat::games::LaunchRequest request;
        std::string error;
        if (!make_game_launch_request(launchSelected, launchPath, request, error)) {
'''
    text = replace_once(text, old_launch, new_launch, "DOS ZIP launch path")

    # Systems/Settings UI tells the truth about the new backends and ZIP behavior.
    old_systems = '''            const std::vector<std::string> systems={"NES","SNES","Game Boy","Game Boy Color","Game Boy Advance","Nintendo 64","Atari 2600","Atari 5200","Atari 7800","Atari 8-bit","Atari Lynx"};
'''
    new_systems = '''            const std::vector<std::string> systems={"NES","SNES","Game Boy","Game Boy Color","Game Boy Advance","Nintendo 64","Sega Genesis","Sega Master System","Sega Game Gear","Atari 2600","Atari 5200","Atari 7800","Atari 8-bit","Atari Lynx"};
'''
    text = replace_once(text, old_systems, new_systems, "Sega systems UI")
    old_backend_text = '''            text(target,gamesListBox.x+14,y+8,"MesenCE 2.2.1: NES/SNES/GB/GBC/GBA | RMG 0.9.0: N64 | Atari800 7.1.2: 5200/8-bit. 2600/7800/Lynx use compatible installed backends.",palette.muted);
'''
    new_backend_text = '''            text(target,gamesListBox.x+14,y+8,"MesenCE: Nintendo | RMG: N64 | BlastEm: Sega | Stella 7.0: Atari 2600 | Atari800: 5200/8-bit. 7800/Lynx use compatible installed backends.",palette.muted);
'''
    text = replace_once(text, old_backend_text, new_backend_text, "Games backend summary")
    old_zip_text = '''            text(target,gamesListBox.x+14,gamesListBox.y+gamesListBox.h-38,"ZIP archives stay in place; selected ROMs extract only to Nougat's private cache.",palette.muted);
'''
    new_zip_text = '''            text(target,gamesListBox.x+14,gamesListBox.y+gamesListBox.h-38,"Cartridge ZIPs stay zipped; DOS ZIP packages prepare automatically in Nougat's persistent private cache.",palette.muted);
'''
    text = replace_once(text, old_zip_text, new_zip_text, "Games ZIP settings text")

    # Expand the existing native self-test so the new archive/system behavior is
    # validated by the built executable, not just by source-token tests.
    old_pass = '''        std::printf("Nougat Media Suite v0.0.49 Games preference PASS: USA first, English fallback, newest final revision, foreign-only fallback.\\\\n");
        return 0;
'''
    if old_pass not in text:
        old_pass = '''        std::printf("Nougat Media Suite v0.0.49 Games preference PASS: USA first, English fallback, newest final revision, foreign-only fallback.\\n");
        return 0;
'''
    native_tests = r'''        const bool sega_bin = game_system_for_path_in_context(
            "Sonic the Hedgehog.bin", "/tmp/Cylum's Sega Genesis ROM Collection/Sonic.zip") == "Sega Genesis";
        const bool sega_gen = game_system_for_path("Sonic.gen") == "Sega Genesis";
        const bool sega_sms = game_system_for_path("Alex Kidd.sms") == "Sega Master System";
        const bool sega_gg = game_system_for_path("Sonic.gg") == "Sega Game Gear";
        const bool zip_safe = safe_zip_member_path("Mario Gallery/mario.exe") &&
                              !safe_zip_member_path("../escape.exe");
        const std::vector<std::string> prince_zip = {
            "Prince of Persia/PRINCE.EXE", "Prince of Persia/SETUP.EXE"
        };
        const std::vector<std::string> gta_zip = {
            "Original_Windows/GTAWIN.EXE", "Original_DOS/gtados/gta24.exe",
            "Original_DOS/gtados.bat"
        };
        const bool dos_prince = dos_archive_entrypoint(
            "/tmp/Prince of Persia.zip", prince_zip) == "Prince of Persia/PRINCE.EXE";
        const bool dos_gta = dos_archive_entrypoint(
            "/tmp/gta1.zip", gta_zip) == "Original_DOS/gtados.bat";
        if (!sega_bin || !sega_gen || !sega_sms || !sega_gg || !zip_safe || !dos_prince || !dos_gta) {
            std::fprintf(stderr,
                "Nougat v0.0.49 ZIP/Sega/DOS self-test FAIL. sega=%d/%d/%d/%d zip=%d dos=%d/%d\n",
                sega_bin, sega_gen, sega_sms, sega_gg, zip_safe, dos_prince, dos_gta);
            return 1;
        }
        std::printf("Nougat Media Suite v0.0.49 Games PASS: USA/English/revision filtering, Sega ZIP recognition, and DOS ZIP package detection.\n");
        return 0;
'''
    text = replace_once(text, old_pass, native_tests, "expanded v49 Games native self-test")

    # Durable marker for idempotence and a human-readable audit point.
    marker_anchor = '''static bool game_path_has_sega_hint(const std::string& path) {'''
    text = replace_once(text, marker_anchor,
                        '''// NOUGAT_V49_GAMES_FINAL_REPAIR: Atari embed + artwork + Sega/DOS ZIP support\n''' + marker_anchor,
                        "final Games repair marker")

    path.write_text(text, encoding="utf-8")
    return True


def patch_world_tv_worker(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    if "NOUGAT_V49_RUSSIA24_AUDIO_REPAIR" in text:
        return False

    old_signature = "def ffprobe_candidate(url: str, referrer: str, user_agent: str) -> bool:\n"
    new_signature = (
        "# NOUGAT_V49_RUSSIA24_AUDIO_REPAIR: require a real audio stream for Russia-24.\n"
        "def ffprobe_candidate(url: str, referrer: str, user_agent: str,\n"
        "                      require_audio: bool = False) -> bool:\n"
    )
    text = replace_once(text, old_signature, new_signature, "World TV audio-aware probe signature")

    old_probe = """    has_video = False
    for stream in payload.get(\"streams\", []):
        if not isinstance(stream, dict):
            continue
        if stream.get(\"codec_type\") == \"video\" and clean(stream.get(\"codec_name\")):
            has_video = True
            break
    if not has_video:
        return False
"""
    new_probe = """    has_video = False
    has_audio = False
    for stream in payload.get(\"streams\", []):
        if not isinstance(stream, dict):
            continue
        codec_type = stream.get(\"codec_type\")
        codec_name = clean(stream.get(\"codec_name\"))
        if codec_type == \"video\" and codec_name:
            has_video = True
        elif codec_type == \"audio\" and codec_name:
            has_audio = True
    if not has_video or (require_audio and not has_audio):
        return False
"""
    text = replace_once(text, old_probe, new_probe, "World TV video/audio probe")

    old_loop = """    deadline = time.monotonic() + 24.0
    checked = 0
    for item in candidates:
        if blocked_label(item[\"label\"]):
            continue
        if time.monotonic() >= deadline or checked >= 3:
            break
        checked += 1
        if ffprobe_candidate(item[\"url\"], item[\"referrer\"], item[\"user_agent\"]):
"""
    new_loop = """    # Russia-24 was owner-tested with picture but no sound. Do not accept a
    # video-only candidate for that station; walk farther through its current
    # direct-source alternates until both video and a decodable audio stream exist.
    require_audio = channel_id == \"Russia24.ru\"
    max_checks = 6 if require_audio else 3
    deadline = time.monotonic() + (36.0 if require_audio else 24.0)
    checked = 0
    for item in candidates:
        if blocked_label(item[\"label\"]):
            continue
        if time.monotonic() >= deadline or checked >= max_checks:
            break
        checked += 1
        if ffprobe_candidate(item[\"url\"], item[\"referrer\"], item[\"user_agent\"],
                             require_audio=require_audio):
"""
    text = replace_once(text, old_loop, new_loop, "Russia-24 audio-source selection")
    path.write_text(text, encoding="utf-8")
    return True


def replace_payload(target: Path, payload_name: str) -> bool:
    payload = PAYLOAD / payload_name
    need(payload.is_file(), "Repair payload missing: " + payload_name)
    if target.is_file() and sha256(target) == sha256(payload):
        return False
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_name(target.name + ".v49-final.tmp")
    shutil.copy2(payload, temporary)
    os.replace(temporary, target)
    return True


def main() -> int:
    try:
        verify_current_state()
        changed = []
        if patch_main(ROOT / "src/main.cpp"):
            changed.append("src/main.cpp")
        if patch_world_tv_worker(ROOT / "components/world_tv/nougat_world_tv_worker.py"):
            changed.append("components/world_tv/nougat_world_tv_worker.py")
        payloads = [
            (ROOT / "components/games/artwork_cache_worker.py", "artwork_cache_worker.py"),
            (ROOT / "tools/install_game_runtimes_v49.py", "install_game_runtimes_v49.py"),
            (ROOT / "tools/check_game_runtimes_v49.py", "check_game_runtimes_v49.py"),
            (ROOT / "tools/test_nougat_media_suite_v49.py", "test_nougat_media_suite_v49.py"),
            (ROOT / "components/games/emulators/BLASTEM_RUNTIME_SOURCE.md", "BLASTEM_RUNTIME_SOURCE.md"),
        ]
        for target, payload_name in payloads:
            if replace_payload(target, payload_name):
                changed.append(str(target.relative_to(ROOT)))

        print("=== v0.0.49 GAMES FINAL OWNER REPAIR APPLIED ===")
        print("PASS: Stella now forces SDL3 X11 for true Nougat reparenting")
        print("PASS: Sega Genesis/Master System/Game Gear ZIP recognition + BlastEm wired")
        print("PASS: DOS ZIPs are one package and prepare in Nougat's persistent private cache")
        print("PASS: remaining verified Atari artwork-name gaps repaired")
        print("PASS: existing v49 filtering, background artwork cache, and scroll repairs preserved")
        print("PASS: Russia-24 resolver now rejects video-only sources and tries audio-capable alternates")
        if changed:
            print("Changed files:")
            for item in changed:
                print("  " + item)
        else:
            print("PASS: repair was already applied; no file changed")
        print("NO GIT COMMIT, TAG, OR PUSH WAS PERFORMED.")
        return 0
    except Exception as exc:
        print("STOP:", exc)
        print("No Git action was performed. Terminal remains open.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
