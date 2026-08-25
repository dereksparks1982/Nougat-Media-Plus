#pragma once

#include <string>

namespace reddmedia {

struct PrivacyReceipt {
    std::string version = "PrivacyReceipt/v1";
    std::string search_source = "Local Index";
    bool network_query = false;
    bool direct_fallback = false;
    bool persistent_search_id = false;
    bool query_logged = false;
    std::string transport = "Local";
    std::string result_integrity = "Stored SHA-256 metadata";

    std::string to_text() const;
};

} // namespace reddmedia
