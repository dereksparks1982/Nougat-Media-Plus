#pragma once

#include <string>
#include <vector>

namespace reddmedia {

enum class LibraryMediaType { Movies, Television };
enum class LibraryNodeKind { Movie, MovieCollection, Series, Season, Episode };

struct MediaFolder {
    std::string library_name;
    std::string path;
    LibraryMediaType media_type = LibraryMediaType::Movies;
    // v0.0.42: remembered mappings remain visible even when a removable or
    // temporarily unavailable path is not mounted. Jellyfin-linked entries
    // normally report available=true; registry-only entries can be false.
    bool available = true;
};

struct LibraryNode {
    std::string id;
    std::string parent_id;
    std::string series_id;
    std::string season_id;
    std::string name;
    std::string path;
    std::string overview;
    std::string series_name;
    std::string episode_title;
    std::string technical_details;
    std::vector<std::string> genres;
    std::string primary_image_tag;
    std::string backdrop_image_tag;
    std::string poster_item_id;
    std::string poster_image_tag;
    std::string tmdb_poster_path;
    std::string tmdb_id;
    std::string imdb_id;
    std::string series_tmdb_id;
    LibraryNodeKind kind = LibraryNodeKind::Movie;
    int production_year = 0;
    int child_count = 0;
    int season_number = 0;
    int episode_number = 0;
};

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
    bool add_media_folder(const std::string& path,
                          LibraryMediaType media_type,
                          std::string& error);
    bool unlink_media_folder(const std::string& path,
                             LibraryMediaType media_type,
                             std::string& error);
    bool load_media_folders(std::vector<MediaFolder>& folders, std::string& error);
    bool load_library_roots(LibraryMediaType media_type,
                            std::vector<LibraryNode>& nodes,
                            std::string& error);
    bool load_library_children(const LibraryNode& parent,
                               std::vector<LibraryNode>& nodes,
                               std::string& error);
    bool load_all_recommendation_items(std::vector<LibraryNode>& nodes,
                                       std::string& error);
    bool load_diagnostic_catalog_items(std::vector<LibraryNode>& nodes,
                                       std::string& error);
    bool load_primary_image_bmp(const std::string& item_id,
                                const std::string& image_tag,
                                int width,
                                int height,
                                std::string& bytes,
                                std::string& error);
    bool load_backdrop_image_bmp(const std::string& item_id,
                                 const std::string& image_tag,
                                 int width,
                                 int height,
                                 std::string& bytes,
                                 std::string& error);
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
    bool query_media_folders(std::vector<MediaFolder>& folders, std::string& error) const;
    bool load_mapping_registry(std::vector<MediaFolder>& folders) const;
    bool save_mapping_registry(const std::vector<MediaFolder>& folders, std::string& error) const;
    bool restore_mapping_registry(std::string& error);
    bool update_mapping_registry(const std::string& path, LibraryMediaType media_type,
                                 bool present, std::string& error) const;
    std::string authorization(bool include_token) const;

    std::string state_file_;
    std::string mapping_state_file_;
    std::string username_;
    std::string access_token_;
    std::string user_id_;
    bool initialized_ = false;
};

} // namespace reddmedia
