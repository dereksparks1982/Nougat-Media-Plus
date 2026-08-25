#pragma once

#include "../nougat/nougat_bridge.hpp"

#include <string>

namespace reddmedia {

enum class SecureSearchStage {
    Idle,
    SearchingLocalIndex,
    SecuringRoute,
    EncryptingQuery,
    SearchingSources,
    VerifyingResults,
    Complete,
    Unavailable,
    Failed
};

class SecureSearchController {
public:
    static NougatSearchResponse search(NougatBridge& bridge,
                                       const std::string& query,
                                       bool raw,
                                       bool request_remote,
                                       int limit = 100,
                                       int offset = 0);
    static const char* stage_label(SecureSearchStage stage);
};

} // namespace reddmedia
