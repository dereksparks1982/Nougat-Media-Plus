#pragma once

#include "recommendation_types.hpp"

#include <string>
#include <vector>

namespace reddmedia {

enum class TmdbCredentialType {
    NotConfigured,
    ReadAccessToken,
    ApiKey,
    Invalid
};

class TmdbClient {
public:
    explicit TmdbClient(std::string credential_file = {});

    bool has_credential() const;
    TmdbCredentialType credential_type() const;
    std::string credential_label() const;
    bool test_saved_credential(std::string& error) const;
    bool save_credential(const std::string& credential, std::string& error);
    bool clear_credential(std::string& error);
    bool discover(RecommendationMediaType type,
                  int page,
                  std::vector<MediaDescriptor>& items,
                  int& total_pages,
                  std::string& error) const;
    bool load_poster_bmp(const std::string& poster_path,
                         int width,
                         int height,
                         std::string& bytes,
                         std::string& error) const;
    bool watch_availability(RecommendationMediaType type,
                            const std::string& tmdb_id,
                            const std::string& region,
                            WatchAvailability& availability,
                            std::string& error) const;
    bool watch_provider_catalog(const std::string& region,
                                std::vector<WatchProvider>& providers,
                                std::string& error) const;
    bool tv_episode_details(const std::string& series_tmdb_id,
                            int season_number,
                            int episode_number,
                            std::string& title,
                            std::string& overview,
                            std::string& error) const;
    bool tv_poster_path(const std::string& series_tmdb_id,
                        int season_number,
                        std::string& poster_path,
                        std::string& error) const;
    bool movie_poster_path(const std::string& tmdb_id,
                           std::string& poster_path,
                           std::string& error) const;

private:
    bool load_credential(std::string& credential, TmdbCredentialType& type) const;
    bool request_json(const std::string& target,
                      std::string& json,
                      std::string& error,
                      const std::string* credential_override = nullptr) const;

    std::string credential_file_;
    std::string base_url_;
    std::string image_base_url_;
};

} // namespace reddmedia
