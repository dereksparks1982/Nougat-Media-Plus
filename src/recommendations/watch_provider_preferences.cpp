#include "watch_provider_preferences.hpp"

#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace reddmedia {
namespace {

std::string parent_directory(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return ".";
    return slash == 0U ? "/" : path.substr(0, slash);
}

bool ensure_private_directory(const std::string& path) {
    if (path.empty()) return false;
    std::string current = path.front() == '/' ? "/" : "";
    std::size_t start = path.front() == '/' ? 1U : 0U;
    while (start <= path.size()) {
        const std::size_t slash = path.find('/', start);
        const std::string part = path.substr(
            start, slash == std::string::npos ? std::string::npos : slash - start);
        if (!part.empty()) {
            if (current.size() > 1U && current.back() != '/') current.push_back('/');
            current += part;
            if (mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) return false;
            chmod(current.c_str(), 0700);
        }
        if (slash == std::string::npos) break;
        start = slash + 1U;
    }
    return true;
}

bool write_all(int descriptor, const std::string& text) {
    std::size_t written = 0;
    while (written < text.size()) {
        const ssize_t amount = write(descriptor, text.data() + written, text.size() - written);
        if (amount <= 0) return false;
        written += static_cast<std::size_t>(amount);
    }
    return true;
}

} // namespace

WatchProviderPreferences::WatchProviderPreferences(std::string settings_file)
    : settings_file_(std::move(settings_file)) {
    if (settings_file_.empty()) {
        const char* override_file = std::getenv("REDDMEDIA_WATCH_PREFERENCES_FILE");
        if (override_file && *override_file) settings_file_ = override_file;
        else {
            const char* home = std::getenv("HOME");
            settings_file_ = std::string(home ? home : ".") +
                "/.config/reddmedia/discover/watch_services.conf";
        }
    }
    load();
}

const std::string& WatchProviderPreferences::region() const {
    return region_;
}

bool WatchProviderPreferences::is_selected(int provider_id) const {
    return selected_provider_ids_.count(provider_id) > 0U;
}

const std::set<int>& WatchProviderPreferences::selected_provider_ids() const {
    return selected_provider_ids_;
}

bool WatchProviderPreferences::load() {
    std::ifstream input(settings_file_);
    if (!input) return true;
    chmod(settings_file_.c_str(), 0600);
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("region=", 0U) == 0U) {
            const std::string value = line.substr(7U);
            if (value.size() == 2U) region_ = value;
        } else if (line.rfind("provider=", 0U) == 0U) {
            const int provider_id = std::atoi(line.c_str() + 9);
            if (provider_id > 0) selected_provider_ids_.insert(provider_id);
        }
    }
    return true;
}

bool WatchProviderPreferences::save(std::string& error) const {
    if (!ensure_private_directory(parent_directory(settings_file_))) {
        error = "ReddMedia could not create its private watch-service settings folder.";
        return false;
    }
    std::ostringstream contents;
    contents << "region=" << region_ << '\n';
    for (const int provider_id : selected_provider_ids_) {
        contents << "provider=" << provider_id << '\n';
    }
    const std::string temporary = settings_file_ + ".new";
    const int descriptor = open(temporary.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (descriptor < 0) {
        error = "ReddMedia could not save My Services.";
        return false;
    }
    const bool written = write_all(descriptor, contents.str());
    const bool closed = close(descriptor) == 0;
    if (!written || !closed || rename(temporary.c_str(), settings_file_.c_str()) != 0) {
        unlink(temporary.c_str());
        error = "ReddMedia could not save My Services.";
        return false;
    }
    chmod(settings_file_.c_str(), 0600);
    return true;
}

bool WatchProviderPreferences::toggle(int provider_id, std::string& error) {
    if (provider_id <= 0) {
        error = "A valid watch service is required.";
        return false;
    }
    if (selected_provider_ids_.count(provider_id) > 0U) {
        selected_provider_ids_.erase(provider_id);
    } else {
        selected_provider_ids_.insert(provider_id);
    }
    return save(error);
}

} // namespace reddmedia
