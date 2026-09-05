#pragma once

#include <string>
#include <vector>

namespace nougat::network {

struct Snapshot {
    std::vector<std::string> overview;
    std::vector<std::string> connections;
    std::vector<std::string> devices;
    std::vector<std::string> security;
    std::vector<std::string> diagnostics;
    std::vector<std::string> logs;
};

Snapshot collect_snapshot();

}  // namespace nougat::network
