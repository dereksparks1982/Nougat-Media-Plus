#pragma once

#include <string>
#include <vector>

namespace nougat::youtube {

struct FeederPolicy {
    bool youtube{false};
    bool force_external_downloader{false};
    std::string format_selector;
    std::vector<std::string> compatibility_args;
};

[[nodiscard]] bool is_youtube_url(const std::string& url);
[[nodiscard]] FeederPolicy feeder_policy(const std::string& url);

}  // namespace nougat::youtube
