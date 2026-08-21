#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace reddmedia {

struct LibraryPoster {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgb;
};

bool decode_library_poster_bmp(const std::string& bytes,
                               LibraryPoster& poster,
                               std::string& error);

bool normalize_library_poster_bmp(const std::string& source_bytes,
                                  std::string& bmp_bytes,
                                  std::string& error);

} // namespace reddmedia
