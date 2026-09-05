#pragma once

#include <string>
#include <vector>

namespace nougat::satellite {

struct Snapshot {
    std::vector<std::string> track;
    std::vector<std::string> passes;
    std::vector<std::string> receive;
    std::vector<std::string> decode;
    std::vector<std::string> transmit;
    std::vector<std::string> imagery;
    std::vector<std::string> antenna;
    std::vector<std::string> hardware;
    std::vector<std::string> logs;
};

Snapshot collect_snapshot();

}  // namespace nougat::satellite
