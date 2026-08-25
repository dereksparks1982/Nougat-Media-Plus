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
    bool record_completed(const MediaDescriptor& item, std::string& error);
    bool recent_history(RecommendationMediaType type,
                        std::vector<ViewingRecord>& records,
                        std::string& error,
                        int limit = 100);
    bool external_credential_available() const;
    std::string external_credential_label() const;
    bool test_external_credential(std::string& error) const;
    bool save_external_credential(const std::string& credential, std::string& error);
    bool clear_external_credential(std::string& error);
    bool load_external_poster_bmp(const std::string& poster_path,
                                  int width,
                                  int height,
                                  std::string& bytes,
                                  std::string& error) const;
    bool load_watch_availability(RecommendationMediaType type,
                                 const std::string& tmdb_id,
                                 const std::string& region,
                                 WatchAvailability& availability,
                                 std::string& error) const;
    bool load_watch_provider_catalog(const std::string& region,
                                     std::vector<WatchProvider>& providers,
                                     std::string& error) const;
    bool load_tv_episode_details(const std::string& series_tmdb_id,
                                 int season_number,
                                 int episode_number,
                                 std::string& title,
                                 std::string& overview,
                                 std::string& error) const;
    bool load_tv_poster_path(const std::string& series_tmdb_id,
                             int season_number,
                             std::string& poster_path,
                             std::string& error) const;
    bool load_movie_poster_path(const std::string& tmdb_id,
                                std::string& poster_path,
                                std::string& error) const;
    bool resolve_metadata_identity(RecommendationMediaType type,
                                   const std::string& title,
                                   int year,
                                   int observed_seasons,
                                   MediaDescriptor& resolved,
                                   std::string& imdb_id,
                                   std::string& error) const;

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
