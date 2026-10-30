#pragma once
#include "advanced_export.hpp"
#include "media_job_engine.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace nougat::studio {
struct NamedProfile {
    std::string name;
    Preset preset = Preset::VideoMp4H264;
    ExportEdits edits;
};

// Durable, local named profiles. No upload, shell execution or network access.
// Files live next to the Studio queue unless the caller explicitly selects
// another directory. Invalid files fail closed rather than being overwritten.
class ProfileLibrary {
public:
    explicit ProfileLibrary(std::filesystem::path file);
    bool upsert(const NamedProfile& profile, std::string& error);
    bool erase(const std::string& name, std::string& error);
    std::optional<NamedProfile> find(const std::string& name) const;
    std::vector<NamedProfile> list() const;
    bool available() const;
    std::string error() const;
private:
    void load();
    bool save_locked(std::string& error);
    std::filesystem::path file_;
    mutable std::mutex mutex_;
    std::vector<NamedProfile> items_;
    bool available_ = true;
    std::string last_error_;
};
} // namespace nougat::studio
