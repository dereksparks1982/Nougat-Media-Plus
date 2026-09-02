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

} // namespace nougat::games
