#pragma once

#include <string>

namespace nougat::games::uo {

struct PlayableLaunchResult {
    bool ok{false};
    bool server_reused{false};
    long server_pid{-1};
    long client_pid{-1};
    std::string error;
};

[[nodiscard]] PlayableLaunchResult launch_t2a_playable();

}  // namespace nougat::games::uo
