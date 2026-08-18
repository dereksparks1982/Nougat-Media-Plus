#pragma once

#include <string>
#include <vector>

namespace reddmedia {

struct LibraryVideo {
    std::string id;
    std::string name;
    std::string path;
    std::string type;
    int production_year = 0;
};

class JellyfinApiClient {
public:
    explicit JellyfinApiClient(std::string state_file = {});

    bool initialize(std::string& error);
    bool add_media_folder(const std::string& path, std::string& error);
    bool refresh_library(std::string& error);
    bool load_videos(std::vector<LibraryVideo>& videos, std::string& error);
    bool wait_for_video_in_folder(const std::string& folder,
                                  std::vector<LibraryVideo>& videos,
                                  std::string& error,
                                  int timeout_seconds);

private:
    struct HttpResponse {
        int status = 0;
        std::string body;
    };

    HttpResponse request(const std::string& method,
                         const std::string& target,
                         const std::string& body,
                         bool authenticated,
                         int timeout_seconds = 15) const;
    bool authenticate(std::string& error);
    bool load_state();
    bool save_state(std::string& error) const;
    bool validate_saved_token();
    std::string authorization(bool include_token) const;

    std::string state_file_;
    std::string username_;
    std::string access_token_;
    std::string user_id_;
    bool initialized_ = false;
};

} // namespace reddmedia
