#pragma once

#include <string>
#include <vector>

namespace reddmedia {

enum class CrawlerAccessState {
    Allowed,
    RobotsRestricted,
    BotPolicyBlocked,
    RateLimited,
    AuthenticationRequired,
    ApiAvailable,
    FeedAvailable,
    PaymentRequired,
    TemporarilyUnavailable
};

enum class CrawlerSourceKind {
    OfficialApi,
    Feed,
    StructuredMetadata,
    PermittedPageCrawl,
    ExistingCache
};

struct CrawlerAccessDecision {
    CrawlerAccessState state = CrawlerAccessState::TemporarilyUnavailable;
    std::string detail;
};

class CrawlerAccessManager {
public:
    static const char* state_name(CrawlerAccessState state);
    static const char* source_name(CrawlerSourceKind source);
    static std::vector<CrawlerSourceKind> fallback_ladder();
    static CrawlerAccessDecision classify_http_status(int status_code);
};

} // namespace reddmedia
