#pragma once

#include "recommendation_types.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace reddmedia {

struct ViewingRecord {
    MediaDescriptor item;
    long long last_watched = 0;
    int play_count = 0;
    bool completed = false;
};

class ViewingHistory {
public:
    explicit ViewingHistory(std::string database_path = {});
    ~ViewingHistory();

    ViewingHistory(const ViewingHistory&) = delete;
    ViewingHistory& operator=(const ViewingHistory&) = delete;

    bool record_started(const MediaDescriptor& item, std::string& error);
    bool record_completed(const MediaDescriptor& item, std::string& error);
    bool recent(RecommendationMediaType type,
                std::vector<ViewingRecord>& records,
                std::string& error,
                int limit = 100);

private:
    bool open_database(std::string& error);
    void close_database();

    std::string database_path_;
    void* sqlite_library_ = nullptr;
    void* database_ = nullptr;
    std::mutex mutex_;
};

} // namespace reddmedia
