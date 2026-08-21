#pragma once

#include <set>
#include <string>

namespace reddmedia {

class WatchProviderPreferences {
public:
    explicit WatchProviderPreferences(std::string settings_file = {});

    const std::string& region() const;
    bool is_selected(int provider_id) const;
    const std::set<int>& selected_provider_ids() const;
    bool toggle(int provider_id, std::string& error);

private:
    bool load();
    bool save(std::string& error) const;

    std::string settings_file_;
    std::string region_ = "US";
    std::set<int> selected_provider_ids_;
};

} // namespace reddmedia
