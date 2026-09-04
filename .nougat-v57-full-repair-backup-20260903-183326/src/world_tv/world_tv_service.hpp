#pragma once

#include <string>
#include <vector>

namespace reddmedia {

struct WorldTvStation {
    std::string channel_id;
    std::string feed_id;
    std::string name;
    std::string country;
    std::string language;
    std::string preferred_url;
    std::string homepage;
    std::string resolver;
    int max_height = 1080;
};

struct WorldTvResolveResult {
    bool ok = false;
    std::string url;
    std::string referrer;
    std::string user_agent;
    std::string error_class;
    std::string error;
};

struct WorldTvGuideInfo {
    bool available = false;
    std::string current_title;
    std::string next_title;
    long long current_start = 0;
    long long current_end = 0;
    long long next_start = 0;
    long long next_end = 0;
    std::string source;
};

class WorldTvService {
public:
    explicit WorldTvService(std::string worker_path);

    WorldTvResolveResult resolve(const std::string& channel_id,
                                 const std::string& feed_id,
                                 const std::string& preferred_url,
                                 const std::string& resolver,
                                 int max_height,
                                 const std::string& exclude_url = {}) const;

    bool refresh_artwork(const std::string& channel_id,
                         const std::string& feed_id,
                         const std::string& output_ppm,
                         std::string& error) const;

    WorldTvGuideInfo guide(const std::string& channel_id,
                           const std::string& feed_id) const;

private:
    std::string run_worker(const std::vector<std::string>& args, int& exit_code) const;
    static std::string value_for(const std::string& output, const std::string& key);
    static long long number_for(const std::string& output, const std::string& key);

    std::string worker_path_;
};

} // namespace reddmedia
