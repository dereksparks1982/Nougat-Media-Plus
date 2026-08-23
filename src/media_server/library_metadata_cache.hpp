#pragma once

#include "jellyfin_api_client.hpp"

#include <string>
#include <vector>

namespace reddmedia {

class LibraryMetadataCache {
public:
    explicit LibraryMetadataCache(std::string directory = {});

    bool load(const std::string& key,
              std::vector<LibraryNode>& nodes,
              std::string& error) const;
    bool store(const std::string& key,
               const std::vector<LibraryNode>& nodes,
               std::string& error) const;
    bool remove(const std::string& key, std::string& error) const;

private:
    std::string path_for_key(const std::string& key) const;
    std::string directory_;
};

} // namespace reddmedia
