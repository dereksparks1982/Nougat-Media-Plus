#include "privacy_policy.hpp"

namespace reddmedia {

PrivacyPolicy PrivacyPolicy::secure_search_baseline() {
    return PrivacyPolicy{};
}

std::vector<std::string> PrivacyPolicy::invariant_names() const {
    return {
        "no-direct-fallback",
        "no-plaintext-dns",
        "no-query-in-argv",
        "no-query-in-url",
        "no-query-logging",
        "no-persistent-search-identifier"
    };
}

bool PrivacyPolicy::valid(std::string& error) const {
    if (allow_direct_fallback || allow_plaintext_dns || allow_query_in_argv ||
        allow_query_in_url || allow_query_logging || allow_persistent_search_identifier) {
        error = "Secure Search baseline contains a forbidden privacy relaxation.";
        return false;
    }
    error.clear();
    return true;
}

} // namespace reddmedia
