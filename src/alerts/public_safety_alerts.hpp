#pragma once

#include <string>
#include <vector>

namespace reddmedia::alerts {

struct PublicSafetyAlert {
    std::string id;
    std::string event;
    std::string severity;
    std::string sent;
    std::string expires;
    std::string area;
    std::string headline;
    std::string source_url;
};

class PublicSafetyAlertService {
public:
    bool refresh_area(const std::string& area_code,
                      std::vector<PublicSafetyAlert>& alerts,
                      std::string& status) const;
    const std::string& source_name() const { return source_name_; }
    std::string history_path() const;

private:
    std::string source_name_ = "NOAA/NWS api.weather.gov";
};

} // namespace reddmedia::alerts
