#pragma once

#include "embedding_engine.hpp"
#include "recommendation_types.hpp"
#include "tmdb_client.hpp"
#include "viewing_history.hpp"

#include <string>
#include <vector>

namespace reddmedia {

class RecommendationEngine {
public:
    RecommendationEngine(std::string model_path,
                         std::string history_path = {},
                         std::string tmdb_token_path = {});

    bool record_started(const MediaDescriptor& item, std::string& error);
    bool external_token_available() const;
    bool save_external_token(const std::string& token, std::string& error);

    bool recommend(const RecommendationRequest& request,
                   const std::vector<MediaDescriptor>& local_items,
                   RecommendationResult& result,
                   std::string& error);

private:
    bool usual_recommendation(const RecommendationRequest& request,
                              const std::vector<MediaDescriptor>& candidates,
                              RecommendationResult& result,
                              std::string& error);
    bool external_candidates(const RecommendationRequest& request,
                             const std::vector<MediaDescriptor>& local_items,
                             std::vector<MediaDescriptor>& candidates,
                             std::string& error);

    ViewingHistory history_;
    EmbeddingEngine embeddings_;
    TmdbClient tmdb_;
};

} // namespace reddmedia
