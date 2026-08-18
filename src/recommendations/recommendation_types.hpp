#pragma once

#include <string>
#include <vector>

namespace reddmedia {

enum class RecommendationSource { Local, External };
enum class RecommendationMediaType { Movie, Television };
enum class RecommendationMode { Usual, Random };

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

} // namespace reddmedia
