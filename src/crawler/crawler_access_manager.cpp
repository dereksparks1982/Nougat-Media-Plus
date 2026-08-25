#include "crawler_access_manager.hpp"

namespace reddmedia {

const char* CrawlerAccessManager::state_name(CrawlerAccessState state) {
    switch (state) {
    case CrawlerAccessState::Allowed: return "ALLOWED";
    case CrawlerAccessState::RobotsRestricted: return "ROBOTS RESTRICTED";
    case CrawlerAccessState::BotPolicyBlocked: return "BOT POLICY BLOCKED";
    case CrawlerAccessState::RateLimited: return "RATE LIMITED";
    case CrawlerAccessState::AuthenticationRequired: return "AUTH REQUIRED";
    case CrawlerAccessState::ApiAvailable: return "API AVAILABLE";
    case CrawlerAccessState::FeedAvailable: return "FEED AVAILABLE";
    case CrawlerAccessState::PaymentRequired: return "PAYMENT REQUIRED";
    case CrawlerAccessState::TemporarilyUnavailable: return "TEMPORARILY UNAVAILABLE";
    }
    return "TEMPORARILY UNAVAILABLE";
}

const char* CrawlerAccessManager::source_name(CrawlerSourceKind source) {
    switch (source) {
    case CrawlerSourceKind::OfficialApi: return "OFFICIAL API";
    case CrawlerSourceKind::Feed: return "RSS/ATOM FEED";
    case CrawlerSourceKind::StructuredMetadata: return "STRUCTURED METADATA";
    case CrawlerSourceKind::PermittedPageCrawl: return "PERMITTED PAGE CRAWL";
    case CrawlerSourceKind::ExistingCache: return "EXISTING CACHE/INDEX";
    }
    return "EXISTING CACHE/INDEX";
}

std::vector<CrawlerSourceKind> CrawlerAccessManager::fallback_ladder() {
    return {
        CrawlerSourceKind::OfficialApi,
        CrawlerSourceKind::Feed,
        CrawlerSourceKind::StructuredMetadata,
        CrawlerSourceKind::PermittedPageCrawl,
        CrawlerSourceKind::ExistingCache
    };
}

CrawlerAccessDecision CrawlerAccessManager::classify_http_status(int status_code) {
    if (status_code >= 200 && status_code < 400) return {CrawlerAccessState::Allowed, "HTTP access permitted."};
    if (status_code == 401) return {CrawlerAccessState::AuthenticationRequired, "Authentication is required."};
    if (status_code == 402) return {CrawlerAccessState::PaymentRequired, "Payment is required; Nougat will not spend automatically."};
    if (status_code == 403) return {CrawlerAccessState::BotPolicyBlocked, "Crawler access was denied by site policy."};
    if (status_code == 429) return {CrawlerAccessState::RateLimited, "Crawler was rate limited."};
    if (status_code >= 500) return {CrawlerAccessState::TemporarilyUnavailable, "Origin/server is temporarily unavailable."};
    return {CrawlerAccessState::TemporarilyUnavailable, "Crawler access is unavailable for this response."};
}

} // namespace reddmedia
