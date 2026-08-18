#pragma once

#include "recommendation_types.hpp"

#include <string>
#include <vector>

namespace reddmedia {

class TmdbClient {
public:
    explicit TmdbClient(std::string token_file = {});

    bool has_token() const;
    bool save_token(const std::string& token, std::string& error);
    bool discover(RecommendationMediaType type,
                  int page,
                  std::vector<MediaDescriptor>& items,
                  int& total_pages,
                  std::string& error) const;

private:
    bool load_token(std::string& token) const;
    bool request_json(const std::string& target, std::string& json, std::string& error) const;

    std::string token_file_;
    std::string base_url_;
};

} // namespace reddmedia
