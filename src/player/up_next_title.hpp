#pragma once

#include <string>

namespace reddmedia {

std::string up_next_episode_title(int season_number,
                                  int episode_number,
                                  const std::string& series_name,
                                  const std::string& episode_title,
                                  const std::string& fallback_name);

} // namespace reddmedia
