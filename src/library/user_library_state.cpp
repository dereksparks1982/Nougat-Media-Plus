#include "user_library_state.hpp"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace reddmedia {
namespace {

std::string home_dir() {
    const char* home = std::getenv("HOME");
    return home && *home ? std::string(home) : std::string(".");
}

bool ensure_parent(const std::string& path) {
    std::error_code ec;
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (parent.empty()) return true;
    std::filesystem::create_directories(parent, ec);
    return !ec;
}

bool regular_file(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(std::filesystem::path(path), ec) && !ec;
}

bool write_private_atomic(const std::string& path, const std::string& bytes) {
    if (!ensure_parent(path)) return false;
    const std::string temporary = path + ".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.close();
        if (!out) {
            unlink(temporary.c_str());
            return false;
        }
    }
    chmod(temporary.c_str(), 0600);
    if (rename(temporary.c_str(), path.c_str()) != 0) {
        unlink(temporary.c_str());
        return false;
    }
    chmod(path.c_str(), 0600);
    return true;
}

} // namespace

bool migrate_private_state_file(const std::string& legacy_path,
                                const std::string& canonical_path) {
    if (canonical_path.empty() || legacy_path.empty()) return false;
    if (regular_file(canonical_path)) return true;
    if (!regular_file(legacy_path)) return false;
    std::ifstream in(legacy_path, std::ios::binary);
    if (!in) return false;
    std::ostringstream data;
    data << in.rdbuf();
    if (!in.good() && !in.eof()) return false;
    return write_private_atomic(canonical_path, data.str());
}

PersistentPathSet::PersistentPathSet(std::string path) : path_(std::move(path)) {
    load();
}

bool PersistentPathSet::contains(const std::string& path) const {
    return !path.empty() && paths_.find(path) != paths_.end();
}

std::set<std::string> PersistentPathSet::snapshot() const {
    return paths_;
}

bool PersistentPathSet::add(const std::string& path) {
    if (path.empty()) return false;
    const auto inserted = paths_.insert(path);
    return !inserted.second || save();
}

bool PersistentPathSet::remove(const std::string& path) {
    if (path.empty()) return false;
    if (paths_.erase(path) == 0U) return true;
    return save();
}

bool PersistentPathSet::load() {
    paths_.clear();
    std::ifstream input(path_);
    if (!input) return false;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        std::string path;
        if (row >> std::quoted(path) && !path.empty()) paths_.insert(std::move(path));
    }
    return true;
}

bool PersistentPathSet::save() const {
    std::ostringstream out;
    for (const std::string& path : paths_) out << std::quoted(path) << '\n';
    return write_private_atomic(path_, out.str());
}

ContinueWatchingSuppressionStore::ContinueWatchingSuppressionStore()
    : paths_(home_dir() + "/.config/reddmedia/library/continue_watching_hidden.tsv") {}

bool ContinueWatchingSuppressionStore::contains(const std::string& path) const {
    return paths_.contains(path);
}

std::set<std::string> ContinueWatchingSuppressionStore::snapshot() const {
    return paths_.snapshot();
}

bool ContinueWatchingSuppressionStore::suppress(const std::string& path) {
    return paths_.add(path);
}

bool ContinueWatchingSuppressionStore::restore(const std::string& path) {
    return paths_.remove(path);
}

LibraryExclusionStore::LibraryExclusionStore()
    : paths_(home_dir() + "/.config/reddmedia/library/excluded_items.tsv") {}

bool LibraryExclusionStore::contains(const std::string& path) const {
    return paths_.contains(path);
}

std::set<std::string> LibraryExclusionStore::snapshot() const {
    return paths_.snapshot();
}

bool LibraryExclusionStore::exclude(const std::string& path) {
    return paths_.add(path);
}

} // namespace reddmedia
