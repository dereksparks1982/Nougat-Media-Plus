#pragma once

#include <string>
#include <vector>

namespace reddmedia {

struct PrivacyPolicy {
    static constexpr int protocol_version = 1;
    bool allow_direct_fallback = false;
    bool allow_plaintext_dns = false;
    bool allow_query_in_argv = false;
    bool allow_query_in_url = false;
    bool allow_query_logging = false;
    bool allow_persistent_search_identifier = false;

    static PrivacyPolicy secure_search_baseline();
    std::vector<std::string> invariant_names() const;
    bool valid(std::string& error) const;
};

} // namespace reddmedia
