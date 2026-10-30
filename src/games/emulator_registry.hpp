#pragma once

#include <string>
#include <vector>

namespace nougat::games {

struct EmulatorSupport {
    std::string system;
    std::string backend;
    std::string executable;
};

std::string find_emulator(const std::string& application_dir,
                          const std::string& system);
std::string emulator_display_name(const std::string& system,
                                  const std::string& executable);
bool pcsx2_bios_available(const std::string& application_dir);
std::vector<EmulatorSupport> ready_emulation_support(const std::string& application_dir);
// Detection is not the same as a successful launch or an embedded game.
// This probe never executes a binary or claims system compatibility.
enum class EmulatorProbeState {
    ExecutableMissing,
    FirmwareCandidateMissing,
    DetectedUnverified
};

struct EmulatorProbe {
    std::string system;
    std::string backend;
    std::string executable;
    EmulatorProbeState state = EmulatorProbeState::ExecutableMissing;
    std::string note;
    bool launch_tested = false;
    bool containment_tested = false;
};

std::vector<EmulatorProbe> inspect_emulation_support(const std::string& application_dir);


} // namespace nougat::games
