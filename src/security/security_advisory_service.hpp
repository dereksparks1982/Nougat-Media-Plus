#pragma once

#include <string>
#include <vector>

namespace reddmedia::security {

struct RuntimeComponentAdvisory {
    std::string component;
    std::string installed_version;
    std::vector<std::string> cves;
    std::vector<std::string> cvss;
};

class SecurityAdvisoryService {
public:
    std::vector<RuntimeComponentAdvisory> inventory(std::string& status) const;
    const std::string& advisory_source() const { return advisory_source_; }

private:
    std::string advisory_source_ = "https://api.osv.dev/";
};

} // namespace reddmedia::security
