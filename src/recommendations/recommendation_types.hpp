#pragma once

#include <string>
#include <vector>

namespace reddmedia {

enum class RecommendationSource { Local, External };
enum class RecommendationMediaType { Movie, Television };
enum class RecommendationMode { Usual, Random };
enum class WatchProviderCategory { Subscription, Free, Ads, Rent, Buy };

struct WatchProvider {
    int id = 0;
    std::string name;
    std::string logo_path;
    WatchProviderCategory category = WatchProviderCategory::Subscription;
    int display_priority = 0;
};

struct WatchAvailability {
    std::string region = "US";
    std::string link;
    std::vector<WatchProvider> providers;
    long long refreshed_at = 0;
    bool listing_found = false;
};

struct MediaDescriptor {
    std::string id;
    std::string title;
    std::string overview;
    std::vector<std::string> genres;
    std::string local_path;
    std::string poster_path;
    std::string tmdb_id;
    RecommendationMediaType media_type = RecommendationMediaType::Movie;
    int year = 0;
};

struct RecommendationRequest {
    RecommendationSource source = RecommendationSource::Local;
    RecommendationMediaType media_type = RecommendationMediaType::Movie;
    RecommendationMode mode = RecommendationMode::Usual;
};

struct RecommendationResult {
    MediaDescriptor item;
    std::string reason;
};

inline const char* media_type_name(RecommendationMediaType type) {
    return type == RecommendationMediaType::Movie ? "Movie" : "TV";
}

inline const char* watch_provider_category_name(WatchProviderCategory category) {
    switch (category) {
    case WatchProviderCategory::Subscription: return "Included with subscription";
    case WatchProviderCategory::Free: return "Free";
    case WatchProviderCategory::Ads: return "Free with ads";
    case WatchProviderCategory::Rent: return "Rent";
    case WatchProviderCategory::Buy: return "Buy";
    }
    return "Availability";
}

} // namespace reddmedia
