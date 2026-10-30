// Linux-only source test. Probe discovery without launching any emulator.
// Build manually from the internal checkout once owner testing is authorized:
// c++ -std=c++17 -Wall -Wextra -Werror -Isrc
//   tools/tests/test_emulator_registry_probe.cpp src/games/emulator_registry.cpp
//   -o /tmp/nougat-registry-probe-test
#include "games/emulator_registry.hpp"

#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>

int main() {
    // The registry must distinguish a found executable from actual gameplay.
    // /bin/true is a harmless executable used ONLY to test discovery.
    const int set_ok = setenv("NOUGAT_MESEN", "/bin/true", 1);
    assert(set_ok == 0);
    const auto results = nougat::games::inspect_emulation_support("/nonexistent/nougat-test-root");
    assert(results.size() == 25U);
    bool found_nes = false;
    bool found_snes = false;
    for (const auto& item : results) {
        assert(!item.launch_tested);
        assert(!item.containment_tested);
        assert(!item.note.empty());
        if (item.system == "NES" || item.system == "SNES") {
            assert(item.executable == "/bin/true");
            assert(item.state == nougat::games::EmulatorProbeState::DetectedUnverified);
            found_nes |= item.system == "NES";
            found_snes |= item.system == "SNES";
        }
        if (item.state == nougat::games::EmulatorProbeState::ExecutableMissing) {
            assert(item.executable.empty());
        }
        if (item.state == nougat::games::EmulatorProbeState::FirmwareCandidateMissing) {
            assert(item.system == "PlayStation 2");
            assert(!item.executable.empty());
        }
    }
    assert(found_nes && found_snes);
    const auto ready = nougat::games::ready_emulation_support("/nonexistent/nougat-test-root");
    bool nes_in_legacy_list = false;
    for (const auto& entry : ready) {
        if (entry.system == "NES") {
            assert(entry.executable == "/bin/true");
            nes_in_legacy_list = true;
        }
    }
    assert(nes_in_legacy_list);
    unsetenv("NOUGAT_MESEN");
    return 0;
}
