#include "emulator_registry.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace nougat::games {
namespace {

std::string home_dir() {
    const char* home = std::getenv("HOME");
    return home && *home ? std::string(home) : std::string(".");
}

bool executable_file(const std::string& path) {
    struct stat info {};
    return !path.empty() && stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) && access(path.c_str(), X_OK) == 0;
}

std::string executable_on_path(const std::string& candidate) {
    if (candidate.empty()) return {};
    if (candidate.find('/') != std::string::npos) return executable_file(candidate) ? candidate : std::string{};
    const char* path = std::getenv("PATH");
    if (!path) return {};
    std::istringstream parts(path);
    std::string part;
    while (std::getline(parts, part, ':')) {
        if (part.empty()) part = ".";
        const std::string full = (std::filesystem::path(part) / candidate).string();
        if (executable_file(full)) return full;
    }
    return {};
}

std::string first_executable(const std::vector<std::string>& candidates) {
    for (const std::string& candidate : candidates) {
        const std::string found = executable_on_path(candidate);
        if (!found.empty()) return found;
    }
    return {};
}

void append_override(std::vector<std::string>& candidates, const char* name) {
    const char* value = std::getenv(name);
    if (value && *value) candidates.emplace_back(value);
}

bool directory_has_bios(const std::filesystem::path& directory) {
    std::error_code ec;
    if (!std::filesystem::is_directory(directory, ec) || ec) return false;
    for (const auto& entry : std::filesystem::directory_iterator(directory, std::filesystem::directory_options::skip_permission_denied, ec)) {
        if (ec) return false;
        std::error_code file_ec;
        if (!entry.is_regular_file(file_ec) || file_ec) continue;
        const auto size = entry.file_size(file_ec);
        if (!file_ec && size >= 256U * 1024U && size <= 16U * 1024U * 1024U) return true;
    }
    return false;
}

std::string basename_only(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string find_dosbox(const std::string& application_dir) {
    std::vector<std::string> candidates;
    append_override(candidates, "NOUGAT_DOSBOX");
    candidates.push_back(application_dir + "/components/games/runtime/dosbox-staging/dosbox");
    candidates.push_back("dosbox-staging");
    candidates.push_back("dosbox");
    return first_executable(candidates);
}

std::string find_xenia(const std::string& application_dir) {
    std::vector<std::string> native;
    append_override(native, "NOUGAT_XENIA");
    native.push_back(application_dir + "/components/games/runtime/xenia/xenia_canary");
    native.push_back(application_dir + "/components/games/runtime/xenia/xenia");
    native.push_back("xenia_canary");
    native.push_back("xenia-canary");
    native.push_back("xenia");
    const std::string found = first_executable(native);
    if (!found.empty() && lower_copy(std::filesystem::path(found).extension().string()) != ".exe") return found;

    const std::vector<std::string> windows = {
        application_dir + "/components/games/runtime/xenia/xenia_canary.exe",
        application_dir + "/components/games/runtime/xenia/xenia.exe"
    };
    std::string windows_xenia;
    const char* override_value = std::getenv("NOUGAT_XENIA");
    if (override_value && *override_value && lower_copy(std::filesystem::path(override_value).extension().string()) == ".exe" &&
        std::filesystem::is_regular_file(std::filesystem::path(override_value))) windows_xenia = override_value;
    if (windows_xenia.empty()) {
        for (const std::string& candidate : windows) {
            if (std::filesystem::is_regular_file(std::filesystem::path(candidate))) { windows_xenia = candidate; break; }
        }
    }
    if (windows_xenia.empty()) return {};
    std::vector<std::string> runners;
    append_override(runners, "NOUGAT_XENIA_RUNNER");
    runners.push_back("umu-run");
    runners.push_back("wine64");
    runners.push_back("wine");
    return first_executable(runners).empty() ? std::string{} : windows_xenia;
}

} // namespace

std::string find_emulator(const std::string& application_dir,
                          const std::string& system) {
    std::vector<std::string> candidates;
    if (system == "NES" || system == "SNES" || system == "Game Boy" ||
        system == "Game Boy Color" || system == "Game Boy Advance") {
        append_override(candidates, "NOUGAT_MESEN");
        candidates.push_back(application_dir + "/components/games/runtime/mesen2/Mesen");
        candidates.push_back(application_dir + "/components/games/runtime/mesen/Mesen");
        if (system == "NES") { candidates.push_back("Mesen"); candidates.push_back("mesen"); candidates.push_back("fceux"); candidates.push_back("nestopia"); }
        else if (system == "SNES") { candidates.push_back("Mesen"); candidates.push_back("mesen"); candidates.push_back("snes9x"); }
        else if (system == "Game Boy" || system == "Game Boy Color") { candidates.push_back("sameboy"); candidates.push_back("mgba"); candidates.push_back("Mesen"); candidates.push_back("mesen"); }
        else { candidates.push_back("mgba"); candidates.push_back("Mesen"); candidates.push_back("mesen"); }
        return first_executable(candidates);
    }
    if (system == "Nintendo 64") {
        candidates = {application_dir + "/components/games/runtime/rmg/AppRun", "RMG", "mupen64plus"};
    } else if (system == "Sega Genesis" || system == "Sega Master System" || system == "Sega Game Gear") {
        candidates = {application_dir + "/components/games/runtime/blastem/blastem", "blastem"};
    } else if (system == "Atari 2600") {
        append_override(candidates, "NOUGAT_STELLA");
        candidates.push_back(application_dir + "/components/games/runtime/stella/stella");
        candidates.push_back("stella");
    } else if (system == "Atari 5200" || system == "Atari 8-bit") {
        append_override(candidates, "NOUGAT_ATARI800");
        candidates.push_back(application_dir + "/components/games/runtime/atari800/AppRun");
        candidates.push_back(application_dir + "/components/games/runtime/atari800/atari800");
        candidates.push_back("atari800");
    } else if (system == "Atari 7800") candidates = {"a7800"};
    else if (system == "Atari Lynx") candidates = {"mednafen"};
    else if (system == "GameCube" || system == "Wii") candidates = {"dolphin-emu", "dolphin"};
    else if (system == "PlayStation") candidates = {"duckstation-qt", "duckstation"};
    else if (system == "PlayStation 2") {
        append_override(candidates, "NOUGAT_PCSX2");
        candidates.push_back(application_dir + "/components/games/runtime/pcsx2/AppRun");
        candidates.push_back(application_dir + "/components/games/runtime/pcsx2/pcsx2-qt");
        candidates.push_back(application_dir + "/components/games/runtime/pcsx2/pcsx2");
        candidates.push_back("pcsx2-qt");
        candidates.push_back("pcsx2");
    } else if (system == "PlayStation Portable") candidates = {"PPSSPPSDL", "PPSSPPQt", "ppsspp"};
    else if (system == "PlayStation 3") candidates = {"rpcs3"};
    else if (system == "Wii U") candidates = {"Cemu", "cemu"};
    else if (system == "Arcade") candidates = {"mame"};
    else if (system == "Nintendo Switch") candidates = {"Ryujinx", "ryujinx", "suyu", "yuzu"};
    else if (system == "DOS") return find_dosbox(application_dir);
    else if (system == "Xbox 360") return find_xenia(application_dir);
    return first_executable(candidates);
}

std::string emulator_display_name(const std::string& system,
                                  const std::string& executable) {
    if (system == "PlayStation 2") return "PCSX2";
    if (system == "Atari 5200" || system == "Atari 8-bit") return "Atari800";
    if (system == "Nintendo 64") return "RMG";
    if (system == "Xbox 360") return "Xenia Canary";
    if (system == "DOS") return "DOSBox";
    const std::string base = basename_only(executable);
    return base == "AppRun" ? system + " backend" : base;
}

bool pcsx2_bios_available(const std::string& application_dir) {
    const char* override_value = std::getenv("NOUGAT_PCSX2_BIOS_DIR");
    if (override_value && *override_value) {
        const std::filesystem::path override_path(override_value);
        std::error_code ec;
        if (std::filesystem::is_regular_file(override_path, ec) && !ec) {
            const auto size = std::filesystem::file_size(override_path, ec);
            if (!ec && size >= 256U * 1024U && size <= 16U * 1024U * 1024U) return true;
        }
        if (directory_has_bios(override_path)) return true;
    }
    const std::string home = home_dir();
    const std::vector<std::filesystem::path> directories = {
        std::filesystem::path(home) / ".config" / "PCSX2" / "bios",
        std::filesystem::path(home) / ".config" / "pcsx2" / "bios",
        std::filesystem::path(home) / ".local" / "share" / "PCSX2" / "bios",
        std::filesystem::path(home) / ".var" / "app" / "net.pcsx2.PCSX2" / "config" / "PCSX2" / "bios",
        std::filesystem::path(application_dir) / "components" / "games" / "runtime" / "pcsx2" / "bios"
    };
    return std::any_of(directories.begin(), directories.end(), directory_has_bios);
}

std::vector<EmulatorSupport> ready_emulation_support(const std::string& application_dir) {
    static const char* systems[] = {
        "NES", "SNES", "Game Boy", "Game Boy Color", "Game Boy Advance", "Nintendo 64",
        "Sega Genesis", "Sega Master System", "Sega Game Gear", "Atari 2600", "Atari 5200",
        "Atari 7800", "Atari 8-bit", "Atari Lynx", "PlayStation", "PlayStation 2",
        "PlayStation Portable", "PlayStation 3", "GameCube", "Wii", "Wii U", "Arcade",
        "Nintendo Switch", "DOS", "Xbox 360"
    };
    std::vector<EmulatorSupport> ready;
    for (const char* system : systems) {
        const std::string executable = find_emulator(application_dir, system);
        if (executable.empty()) continue;
        if (std::string(system) == "PlayStation 2" && !pcsx2_bios_available(application_dir)) continue;
        ready.push_back({system, emulator_display_name(system, executable), executable});
    }
    return ready;
}

} // namespace nougat::games
