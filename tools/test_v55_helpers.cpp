#include "library/user_library_state.hpp"
#include "player/up_next_title.hpp"
#include "games/emulator_registry.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

static bool write_file(const fs::path& path, const std::string& data, mode_t mode = 0600) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << data;
    out.close();
    chmod(path.string().c_str(), mode);
    return static_cast<bool>(out);
}

int main() {
    const fs::path root = fs::temp_directory_path() / ("nougat-v55-helper-test-" + std::to_string(getpid()));
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root / "home", ec);
    fs::create_directories(root / "app", ec);
    if (ec) return 2;
    setenv("HOME", (root / "home").c_str(), 1);

    if (reddmedia::up_next_episode_title(1, 9, "The Boondocks",
            "The Boondocks S01 S01E09 Return of the King", "") !=
        "S01E09 - Return of the King") {
        std::cerr << "up-next duplicate season test failed\n";
        return 10;
    }
    if (reddmedia::up_next_episode_title(2, 3, "How I Met Your Mother",
            "How I Met Your Mother (2005) - S02E03 - Brunch", "") !=
        "S02E03 - Brunch") {
        std::cerr << "up-next series/year cleanup test failed\n";
        return 11;
    }

    reddmedia::ContinueWatchingSuppressionStore suppressions;
    if (!suppressions.suppress("/media/show/e01.mkv") || !suppressions.contains("/media/show/e01.mkv")) return 20;
    reddmedia::ContinueWatchingSuppressionStore suppressions_reload;
    if (!suppressions_reload.contains("/media/show/e01.mkv")) return 21;
    if (!suppressions_reload.restore("/media/show/e01.mkv")) return 22;
    reddmedia::ContinueWatchingSuppressionStore suppressions_restored;
    if (suppressions_restored.contains("/media/show/e01.mkv")) return 23;

    reddmedia::LibraryExclusionStore exclusions;
    if (!exclusions.exclude("/media/movie.mkv") || !exclusions.contains("/media/movie.mkv")) return 30;
    reddmedia::LibraryExclusionStore exclusions_reload;
    if (!exclusions_reload.contains("/media/movie.mkv")) return 31;

    const fs::path legacy = root / "home/.config/reddmedia/server/library_mappings.tsv";
    const fs::path canonical = root / "home/.config/reddmedia/library/library_mappings.tsv";
    if (!write_file(legacy, "\"movies\"\t\"/media/Movies\"\n")) return 40;
    if (!reddmedia::migrate_private_state_file(legacy.string(), canonical.string())) return 41;
    std::ifstream migrated(canonical);
    std::string migrated_text((std::istreambuf_iterator<char>(migrated)), std::istreambuf_iterator<char>());
    if (migrated_text != "\"movies\"\t\"/media/Movies\"\n") return 42;
    if (!fs::exists(legacy)) return 43; // migration is copy-only for rollback safety

    const fs::path mesen = root / "app/components/games/runtime/mesen2/Mesen";
    if (!write_file(mesen, "#!/bin/sh\n", 0700)) return 50;
    if (nougat::games::find_emulator((root / "app").string(), "NES") != mesen.string()) return 51;

    const fs::path stella = root / "app/components/games/runtime/stella/stella";
    if (!write_file(stella, "#!/bin/sh\n", 0700)) return 52;
    if (nougat::games::find_emulator((root / "app").string(), "Atari 2600") != stella.string()) return 53;

    const fs::path atari800 = root / "app/components/games/runtime/atari800/AppRun";
    if (!write_file(atari800, "#!/bin/sh\n", 0700)) return 54;
    if (nougat::games::find_emulator((root / "app").string(), "Atari 8-bit") != atari800.string()) return 55;

    const fs::path pcsx2 = root / "app/components/games/runtime/pcsx2/AppRun";
    if (!write_file(pcsx2, "#!/bin/sh\n", 0700)) return 60;
    if (nougat::games::pcsx2_bios_available((root / "app").string())) return 61;
    const fs::path bios = root / "home/.config/PCSX2/bios/SCPH-test.bin";
    if (!write_file(bios, std::string(512U * 1024U, '\0'))) return 62;
    if (!nougat::games::pcsx2_bios_available((root / "app").string())) return 63;
    const auto ready = nougat::games::ready_emulation_support((root / "app").string());
    bool nes = false, a2600 = false, a8 = false, ps2 = false;
    for (const auto& item : ready) {
        if (item.system == "NES") nes = true;
        if (item.system == "Atari 2600") a2600 = true;
        if (item.system == "Atari 8-bit") a8 = true;
        if (item.system == "PlayStation 2") ps2 = true;
    }
    if (!nes || !a2600 || !a8 || !ps2) return 64;

    fs::remove_all(root, ec);
    std::cout << "Nougat v0.0.55 helper behavior tests PASS\n";
    return 0;
}
