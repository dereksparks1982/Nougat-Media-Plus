#pragma once

#include <set>
#include <string>

namespace reddmedia {

bool migrate_private_state_file(const std::string& legacy_path,
                                const std::string& canonical_path);

class PersistentPathSet {
public:
    explicit PersistentPathSet(std::string path);

    bool contains(const std::string& path) const;
    std::set<std::string> snapshot() const;
    bool add(const std::string& path);
    bool remove(const std::string& path);

private:
    bool load();
    bool save() const;

    std::string path_;
    std::set<std::string> paths_;
};

class ContinueWatchingSuppressionStore {
public:
    ContinueWatchingSuppressionStore();
    bool contains(const std::string& path) const;
    std::set<std::string> snapshot() const;
    bool suppress(const std::string& path);
    bool restore(const std::string& path);

private:
    PersistentPathSet paths_;
};

class LibraryExclusionStore {
public:
    LibraryExclusionStore();
    bool contains(const std::string& path) const;
    std::set<std::string> snapshot() const;
    bool exclude(const std::string& path);

private:
    PersistentPathSet paths_;
};

} // namespace reddmedia
