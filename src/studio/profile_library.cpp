#include "profile_library.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace nougat::studio {
namespace {
std::string hex(const std::string& s) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string result;
    for (unsigned char c : s) {
        result += digits[c >> 4U];
        result += digits[c & 15U];
    }
    return result;
}
bool unhex(const std::string& s, std::string& out) {
    if (s.size() % 2U != 0U || s.size() > 16384U) return false;
    auto hexnum = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    out.clear();
    for (std::size_t i = 0; i < s.size(); i += 2U) {
        const int a = hexnum(s[i]), b = hexnum(s[i+1U]);
        if (a < 0 || b < 0) return false;
        const char c = static_cast<char>((a << 4) | b);
        if (c == '\0') return false;
        out.push_back(c);
    }
    return true;
}
bool valid_name(const std::string& n) {
    if (n.empty() || n.size() > 64U) return false;
    for (unsigned char c : n)
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '-' && c != '_' && c != ' ')
            return false;
    return true;
}
bool write_all(int fd, const std::string& data) {
    std::size_t at = 0;
    while (at < data.size()) {
        const ssize_t n = write(fd, data.data() + at, data.size() - at);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        at += static_cast<std::size_t>(n);
    }
    return true;
}
bool path_symlink(const std::filesystem::path& p) {
    struct stat st{};
    return lstat(p.c_str(), &st) == 0 && S_ISLNK(st.st_mode);
}
} // namespace

ProfileLibrary::ProfileLibrary(std::filesystem::path file)
    : file_(std::move(file)) { load(); }

void ProfileLibrary::load() {
    if (path_symlink(file_)) {
        available_ = false;
        last_error_ = "Studio profile library cannot be a symbolic link.";
        return;
    }
    std::ifstream input(file_);
    if (!input) {
        std::error_code ec;
        if (std::filesystem::exists(file_, ec) || ec) {
            available_ = false;
            last_error_ = "Studio profile file cannot be opened.";
        }
        return;
    }
    std::string line;
    if (!std::getline(input, line) || line != "NOUGAT_STUDIO_PROFILES_V1") {
        available_ = false;
        last_error_ = "Unknown Studio profile storage format.";
        return;
    }
    std::vector<NamedProfile> read;
    while (std::getline(input, line)) {
        if (line.size() > 16000U || read.size() >= 100U) {
            available_ = false; last_error_ = "Studio profile count/size limit reached."; return;
        }
        const auto first = line.find('\t'), second =
            first == std::string::npos ? first : line.find('\t', first+1U);
        if (first == std::string::npos || second == std::string::npos ||
            line.find('\t', second+1U) != std::string::npos) {
            available_ = false; last_error_ = "Malformed Studio profile record."; return;
        }
        std::string name, payload;
        if (!unhex(line.substr(0, first), name) ||
            !unhex(line.substr(second+1U), payload) ||
            !valid_name(name)) {
            available_ = false; last_error_ = "Invalid Studio profile record."; return;
        }
        int preset_value = -1;
        try {
            const auto field = line.substr(first+1U, second-first-1U);
            std::size_t used = 0;
            preset_value = std::stoi(field, &used);
            if (used != field.size()) preset_value = -1;
        } catch (...) { preset_value = -1; }
        if (preset_value < 0 || preset_value > 8 ||
            std::any_of(read.begin(), read.end(), [&](const NamedProfile& p) {
                return p.name == name;
            })) {
            available_ = false; last_error_ = "Invalid or duplicate profile."; return;
        }
        NamedProfile profile;
        profile.name = name;
        profile.preset = static_cast<Preset>(preset_value);
        if (!deserialize_export_edits(payload, profile.edits)) {
            available_ = false; last_error_ = "Invalid profile options."; return;
        }
        read.push_back(std::move(profile));
    }
    if (input.bad()) {
        available_ = false; last_error_ = "Studio profile file could not be read."; return;
    }
    items_ = std::move(read);
}
bool ProfileLibrary::save_locked(std::string& error) {
    if (path_symlink(file_)) {
        error = "Studio profile destination must not be a symlink."; return false;
    }
    std::error_code ec;
    auto folder = file_.parent_path();
    if (folder.empty()) { error = "Studio profile folder must be absolute."; return false; }
    std::filesystem::create_directories(folder, ec);
    if (ec) { error = "Cannot create private Studio profile directory."; return false; }
    std::string pattern = (folder / ".nougat-profile-XXXXXX").string();
    std::vector<char> buf(pattern.begin(),pattern.end());
    buf.push_back('\0');
    const int fd = mkstemp(buf.data());
    if (fd < 0) { error = "Cannot create profile save temporary file."; return false; }
    (void)fchmod(fd, 0600);
    std::string data = "NOUGAT_STUDIO_PROFILES_V1\n";
    for (const auto& profile : items_) {
        data += hex(profile.name) + "\t" +
                std::to_string(static_cast<int>(profile.preset)) + "\t" +
                hex(serialize_export_edits(profile.edits)) + "\n";
    }
    const bool flushed = write_all(fd, data) && fsync(fd) == 0;
    close(fd);
    if (!flushed || rename(buf.data(), file_.c_str()) != 0) {
        unlink(buf.data());
        error = "Could not atomically save Studio profiles.";
        return false;
    }
    const int dir = open(folder.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dir >= 0) { (void)fsync(dir); close(dir); }
    return true;
}
bool ProfileLibrary::upsert(const NamedProfile& profile, std::string& error) {
    if (!valid_name(profile.name) ||
        !validate_export_edits(profile.edits, profile.preset, error)) {
        if (error.empty()) error = "Profile name must be 1-64 simple characters.";
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!available_) { error = last_error_; return false; }
    auto it = std::find_if(items_.begin(),items_.end(),[&](const NamedProfile& p) {
        return p.name == profile.name;
    });
    if (it == items_.end() && items_.size() >= 100U) {
        error = "Studio profile library is full."; return false;
    }
    const auto old = items_;
    if (it == items_.end()) items_.push_back(profile);
    else *it = profile;
    if (!save_locked(error)) { items_ = old; return false; }
    return true;
}
bool ProfileLibrary::erase(const std::string& name, std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!available_) { error = last_error_; return false; }
    auto it = std::find_if(items_.begin(),items_.end(),[&](const NamedProfile& p) {
        return p.name == name;
    });
    if (it == items_.end()) { error = "Profile not found."; return false; }
    const auto old = items_;
    items_.erase(it);
    if (!save_locked(error)) { items_ = old; return false; }
    return true;
}
std::optional<NamedProfile> ProfileLibrary::find(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(items_.begin(),items_.end(),[&](const NamedProfile& p) {
        return p.name == name;
    });
    return it == items_.end() ? std::nullopt : std::optional<NamedProfile>(*it);
}
std::vector<NamedProfile> ProfileLibrary::list() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return items_;
}
bool ProfileLibrary::available() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_;
}
std::string ProfileLibrary::error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}
} // namespace nougat::studio
