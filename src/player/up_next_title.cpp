#include "up_next_title.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace reddmedia {
namespace {

std::string lower_copy(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

std::string trim_separators(std::string value) {
    while (!value.empty()) {
        const char c = value.front();
        if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '_' || c == '.' || c == ':')
            value.erase(value.begin());
        else break;
    }
    while (!value.empty()) {
        const char c = value.back();
        if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '_' || c == '.') value.pop_back();
        else break;
    }
    return value;
}

bool starts_with_case_insensitive(const std::string& value, const std::string& prefix) {
    if (prefix.empty() || value.size() < prefix.size()) return false;
    return lower_copy(value.substr(0, prefix.size())) == lower_copy(prefix);
}

bool consume_year(std::string& value) {
    value = trim_separators(value);
    if (value.size() < 6U || value.front() != '(' || value[5] != ')') return false;
    for (std::size_t i = 1; i < 5; ++i) {
        if (std::isdigit(static_cast<unsigned char>(value[i])) == 0) return false;
    }
    value.erase(0, 6U);
    value = trim_separators(value);
    return true;
}

bool consume_season_or_episode_token(std::string& value) {
    value = trim_separators(value);
    if (value.empty()) return false;
    const std::string lower = lower_copy(value);

    if (lower.rfind("season", 0U) == 0U) {
        std::size_t pos = 6U;
        while (pos < value.size() && std::isspace(static_cast<unsigned char>(value[pos])) != 0) ++pos;
        const std::size_t digits = pos;
        while (pos < value.size() && std::isdigit(static_cast<unsigned char>(value[pos])) != 0) ++pos;
        if (pos > digits) {
            value.erase(0, pos);
            value = trim_separators(value);
            return true;
        }
    }

    if (lower.rfind("episode", 0U) == 0U) {
        std::size_t pos = 7U;
        while (pos < value.size() && std::isspace(static_cast<unsigned char>(value[pos])) != 0) ++pos;
        const std::size_t digits = pos;
        while (pos < value.size() && std::isdigit(static_cast<unsigned char>(value[pos])) != 0) ++pos;
        if (pos > digits) {
            value.erase(0, pos);
            value = trim_separators(value);
            return true;
        }
    }

    if (lower[0] == 's' && lower.size() > 1U && std::isdigit(static_cast<unsigned char>(lower[1])) != 0) {
        std::size_t pos = 1U;
        while (pos < lower.size() && std::isdigit(static_cast<unsigned char>(lower[pos])) != 0) ++pos;
        if (pos < lower.size() && lower[pos] == 'e') {
            ++pos;
            const std::size_t episode_digits = pos;
            while (pos < lower.size() && std::isdigit(static_cast<unsigned char>(lower[pos])) != 0) ++pos;
            if (pos == episode_digits) return false;
        }
        if (pos == lower.size() || std::isspace(static_cast<unsigned char>(lower[pos])) != 0 ||
            lower[pos] == '-' || lower[pos] == '_' || lower[pos] == '.') {
            value.erase(0, pos);
            value = trim_separators(value);
            return true;
        }
    }
    return false;
}

std::string cleaned_episode_name(const std::string& series_name,
                                 const std::string& episode_title,
                                 const std::string& fallback_name) {
    std::string value = episode_title.empty() ? fallback_name : episode_title;
    value = trim_separators(value);
    if (!series_name.empty() && starts_with_case_insensitive(value, series_name)) {
        value.erase(0, series_name.size());
        value = trim_separators(value);
    }
    while (consume_year(value)) {}
    for (int i = 0; i < 4 && consume_season_or_episode_token(value); ++i) {}
    value = trim_separators(value);
    return value.empty() ? std::string("Title unavailable") : value;
}

} // namespace

std::string up_next_episode_title(int season_number,
                                  int episode_number,
                                  const std::string& series_name,
                                  const std::string& episode_title,
                                  const std::string& fallback_name) {
    std::ostringstream out;
    if (season_number > 0 && episode_number > 0) {
        out << 'S' << std::setfill('0') << std::setw(2) << season_number
            << 'E' << std::setw(2) << episode_number;
    } else if (episode_number > 0) {
        out << "Episode " << episode_number;
    } else {
        out << "Episode number unavailable";
    }
    out << " - " << cleaned_episode_name(series_name, episode_title, fallback_name);
    return out.str();
}

} // namespace reddmedia
