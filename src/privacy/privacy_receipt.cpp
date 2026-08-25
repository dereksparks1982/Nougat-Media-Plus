#include "privacy_receipt.hpp"

#include <sstream>

namespace reddmedia {
namespace {

const char* yes_no(bool value) {
    return value ? "Yes" : "No";
}

} // namespace

std::string PrivacyReceipt::to_text() const {
    std::ostringstream out;
    out << "NOUGAT PRIVACY RECEIPT\n"
        << "Protocol: " << version << '\n'
        << "Search source: " << search_source << '\n'
        << "Network query: " << yes_no(network_query) << '\n'
        << "Direct fallback: " << yes_no(direct_fallback) << '\n'
        << "Persistent search ID: " << yes_no(persistent_search_id) << '\n'
        << "Query logged: " << yes_no(query_logged) << '\n'
        << "Transport: " << transport << '\n'
        << "Result integrity: " << result_integrity;
    return out.str();
}

} // namespace reddmedia
