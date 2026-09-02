#pragma once

#include <string>

namespace reddmedia::safety {

class ChildSafeControls {
public:
    ChildSafeControls();

    bool load(std::string& status);
    bool enabled() const { return enabled_; }
    bool password_configured() const { return !salt_.empty() && !password_hash_.empty(); }

    bool enable_with_password(const std::string& password, std::string& status);
    bool verify_password(const std::string& password) const;
    bool disable_with_password(const std::string& password, std::string& status);

    const std::string& config_path() const { return config_path_; }

private:
    bool save(std::string& status) const;
    static std::string derive_hash(const std::string& password, const std::string& salt);
    static std::string random_salt();

    std::string config_path_;
    bool enabled_ = false;
    std::string salt_;
    std::string password_hash_;
};

} // namespace reddmedia::safety
