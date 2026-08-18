#include "tmdb_client.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace reddmedia {
namespace {

std::string parent_directory(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return ".";
    return slash == 0U ? "/" : path.substr(0, slash);
}

bool ensure_directory(const std::string& path) {
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
        }
        if (slash == std::string::npos) break;
        start = slash + 1U;
    }
    return true;
}

int hex_value(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

void append_utf8(std::string& output, unsigned value) {
    if (value <= 0x7fU) output.push_back(static_cast<char>(value));
    else if (value <= 0x7ffU) {
        output.push_back(static_cast<char>(0xc0U | (value >> 6U)));
        output.push_back(static_cast<char>(0x80U | (value & 0x3fU)));
    } else {
        output.push_back(static_cast<char>(0xe0U | (value >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (value & 0x3fU)));
    }
}

std::size_t value_position(const std::string& text, const std::string& key) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t position = text.find(marker);
    if (position == std::string::npos) return position;
    const std::size_t colon = text.find(':', position + marker.size());
    if (colon == std::string::npos) return colon;
    return text.find_first_not_of(" \t\r\n", colon + 1U);
}

std::string json_string_value(const std::string& text, const std::string& key) {
    std::size_t position = value_position(text, key);
    if (position == std::string::npos || text[position] != '"') return {};
    std::string result;
    for (++position; position < text.size(); ++position) {
        const char character = text[position];
        if (character == '"') break;
        if (character != '\\') {
            result.push_back(character);
            continue;
        }
        if (++position >= text.size()) break;
        const char escaped = text[position];
        if (escaped == 'n') result.push_back('\n');
        else if (escaped == 'r') result.push_back('\r');
        else if (escaped == 't') result.push_back('\t');
        else if (escaped == 'u' && position + 4U < text.size()) {
            unsigned value = 0;
            for (int count = 0; count < 4; ++count) {
                const int part = hex_value(text[++position]);
                if (part < 0) return result;
                value = value * 16U + static_cast<unsigned>(part);
            }
            append_utf8(result, value);
        } else result.push_back(escaped);
    }
    return result;
}

int json_int_value(const std::string& text, const std::string& key) {
    const std::size_t position = value_position(text, key);
    if (position == std::string::npos) return 0;
    return std::atoi(text.c_str() + static_cast<std::ptrdiff_t>(position));
}

std::vector<std::string> json_array_objects(const std::string& text, const std::string& key) {
    std::vector<std::string> objects;
    std::size_t position = value_position(text, key);
    if (position == std::string::npos || text[position] != '[') return objects;
    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    std::size_t start = std::string::npos;
    for (++position; position < text.size(); ++position) {
        const char character = text[position];
        if (in_string) {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') in_string = false;
            continue;
        }
        if (character == '"') in_string = true;
        else if (character == '{') {
            if (depth == 0) start = position;
            ++depth;
        } else if (character == '}' && depth > 0) {
            --depth;
            if (depth == 0 && start != std::string::npos) {
                objects.push_back(text.substr(start, position - start + 1U));
                start = std::string::npos;
            }
        } else if (character == ']' && depth == 0) break;
    }
    return objects;
}

std::vector<std::string> json_int_array(const std::string& text, const std::string& key) {
    std::vector<std::string> values;
    std::size_t position = value_position(text, key);
    if (position == std::string::npos || text[position] != '[') return values;
    const std::size_t end = text.find(']', position + 1U);
    if (end == std::string::npos) return values;
    while (++position < end) {
        position = text.find_first_of("0123456789", position);
        if (position == std::string::npos || position >= end) break;
        const std::size_t finish = text.find_first_not_of("0123456789", position);
        values.push_back(text.substr(position, finish - position));
        if (finish == std::string::npos) break;
        position = finish - 1U;
    }
    return values;
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

int release_year(const std::string& date) {
    if (date.size() < 4U || !std::all_of(date.begin(), date.begin() + 4,
                                        [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; })) {
        return 0;
    }
    return std::atoi(date.substr(0, 4).c_str());
}

} // namespace

TmdbClient::TmdbClient(std::string token_file)
    : token_file_(std::move(token_file)) {
    const char* base_override = std::getenv("REDDMEDIA_TMDB_BASE_URL");
    base_url_ = base_override && *base_override ? base_override : "https://api.themoviedb.org";
    if (!token_file_.empty()) return;
    const char* token_override = std::getenv("REDDMEDIA_TMDB_TOKEN_FILE");
    if (token_override && *token_override) token_file_ = token_override;
    else {
        const char* home = std::getenv("HOME");
        token_file_ = std::string(home ? home : ".") + "/.config/reddmedia/ai/tmdb.token";
    }
}

bool TmdbClient::load_token(std::string& token) const {
    std::ifstream input(token_file_);
    if (!input) return false;
    std::getline(input, token);
    while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back())) != 0) {
        token.pop_back();
    }
    return !token.empty();
}

bool TmdbClient::has_token() const {
    std::string token;
    return load_token(token);
}

bool TmdbClient::save_token(const std::string& token, std::string& error) {
    if (token.empty()) {
        error = "Enter a TMDb read-access token first.";
        return false;
    }
    if (!ensure_directory(parent_directory(token_file_))) {
        error = "ReddMedia could not create its private AI settings folder.";
        return false;
    }
    const int file = open(token_file_.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (file < 0) {
        error = "ReddMedia could not store the TMDb token.";
        return false;
    }
    const bool written = write_all(file, token + "\n");
    const bool closed = close(file) == 0;
    if (!written || !closed) {
        error = "ReddMedia could not store the TMDb token.";
        return false;
    }
    return true;
}

bool TmdbClient::request_json(const std::string& target,
                              std::string& json,
                              std::string& error) const {
    std::string token;
    if (!load_token(token)) {
        error = "TMDb token required for External recommendations.";
        return false;
    }
    int input_pipe[2] = {-1, -1};
    int output_pipe[2] = {-1, -1};
    if (pipe(input_pipe) != 0 || pipe(output_pipe) != 0) {
        if (input_pipe[0] >= 0) { close(input_pipe[0]); close(input_pipe[1]); }
        error = "ReddMedia could not start its TMDb request.";
        return false;
    }
    const pid_t child = fork();
    if (child < 0) {
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        error = "ReddMedia could not start its TMDb request.";
        return false;
    }
    if (child == 0) {
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        dup2(output_pipe[1], STDERR_FILENO);
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        const std::string url = base_url_ + target;
        execlp("curl", "curl", "-fsS", "--connect-timeout", "8", "--max-time", "25",
               "-H", "@-", url.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    close(input_pipe[0]);
    close(output_pipe[1]);
    const std::string headers = "Authorization: Bearer " + token +
        "\nAccept: application/json\n";
    const bool sent = write_all(input_pipe[1], headers);
    close(input_pipe[1]);
    std::string response;
    char buffer[8192];
    for (;;) {
        const ssize_t amount = read(output_pipe[0], buffer, sizeof(buffer));
        if (amount > 0) response.append(buffer, static_cast<std::size_t>(amount));
        else break;
    }
    close(output_pipe[0]);
    int status = 0;
    waitpid(child, &status, 0);
    if (!sent || !WIFEXITED(status) || WEXITSTATUS(status) != 0 || response.empty()) {
        error = response.empty() ? "TMDb is unavailable right now." : response;
        if (error.size() > 240U) error.resize(240U);
        return false;
    }
    json = std::move(response);
    return true;
}

bool TmdbClient::discover(RecommendationMediaType type,
                          int page,
                          std::vector<MediaDescriptor>& items,
                          int& total_pages,
                          std::string& error) const {
    page = std::max(1, std::min(500, page));
    const std::string kind = type == RecommendationMediaType::Movie ? "movie" : "tv";
    const std::string target = "/3/discover/" + kind +
        "?include_adult=false&include_video=false&language=en-US&sort_by=popularity.desc"
        "&vote_count.gte=25&page=" + std::to_string(page);
    std::string json;
    if (!request_json(target, json, error)) return false;
    total_pages = std::max(1, std::min(500, json_int_value(json, "total_pages")));
    std::vector<MediaDescriptor> loaded;
    for (const std::string& object : json_array_objects(json, "results")) {
        const int id = json_int_value(object, "id");
        const std::string title = json_string_value(
            object, type == RecommendationMediaType::Movie ? "title" : "name");
        if (id <= 0 || title.empty()) continue;
        MediaDescriptor item;
        item.tmdb_id = std::to_string(id);
        item.id = "tmdb:" + kind + ":" + item.tmdb_id;
        item.title = title;
        item.overview = json_string_value(object, "overview");
        item.poster_path = json_string_value(object, "poster_path");
        item.genres = json_int_array(object, "genre_ids");
        item.media_type = type;
        item.year = release_year(json_string_value(
            object, type == RecommendationMediaType::Movie ? "release_date" : "first_air_date"));
        loaded.push_back(std::move(item));
    }
    if (loaded.empty()) {
        error = "TMDb returned no matching titles.";
        return false;
    }
    items = std::move(loaded);
    return true;
}

} // namespace reddmedia
