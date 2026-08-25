#include "tmdb_client.hpp"

#include "../media_server/library_poster.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <ctime>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace reddmedia {
namespace {

struct CurlResult {
    int http_status = 0;
    int process_status = -1;
    std::string body;
};

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
            chmod(current.c_str(), 0700);
        }
        if (slash == std::string::npos) break;
        start = slash + 1U;
    }
    return true;
}

std::string trim_copy(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::string url_encode_query(const std::string& value) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string encoded;
    for (const unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded.push_back(static_cast<char>(c));
        } else {
            encoded.push_back('%');
            encoded.push_back(hex[(c >> 4U) & 0x0fU]);
            encoded.push_back(hex[c & 0x0fU]);
        }
    }
    return encoded;
}

TmdbCredentialType detect_credential_type(const std::string& credential) {
    if (credential.empty()) return TmdbCredentialType::NotConfigured;
    const bool safe = std::all_of(credential.begin(), credential.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '.' || character == '_' || character == '-';
    });
    if (!safe) return TmdbCredentialType::Invalid;
    if (credential.size() == 32U &&
        std::all_of(credential.begin(), credential.end(), [](unsigned char character) {
            return std::isxdigit(character) != 0;
        })) {
        return TmdbCredentialType::ApiKey;
    }
    if (credential.size() >= 80U && credential.rfind("eyJ", 0U) == 0U &&
        credential.find('.') != std::string::npos) {
        return TmdbCredentialType::ReadAccessToken;
    }
    return TmdbCredentialType::Invalid;
}

std::string config_quote(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char character : value) {
        if (character == '\\' || character == '"') escaped.push_back('\\');
        escaped.push_back(character);
    }
    return escaped;
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

CurlResult run_curl(const std::string& url, const std::string& authorization = {}) {
    CurlResult result;
    int input_pipe[2] = {-1, -1};
    int output_pipe[2] = {-1, -1};
    if (pipe(input_pipe) != 0 || pipe(output_pipe) != 0) {
        if (input_pipe[0] >= 0) { close(input_pipe[0]); close(input_pipe[1]); }
        if (output_pipe[0] >= 0) { close(output_pipe[0]); close(output_pipe[1]); }
        return result;
    }
    const pid_t child = fork();
    if (child < 0) {
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        return result;
    }
    if (child == 0) {
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        dup2(output_pipe[1], STDERR_FILENO);
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        execlp("curl", "curl", "--config", "-", static_cast<char*>(nullptr));
        _exit(127);
    }
    close(input_pipe[0]);
    close(output_pipe[1]);
    std::string config =
        "silent\nshow-error\nconnect-timeout = 8\nmax-time = 25\n"
        "url = \"" + config_quote(url) + "\"\n"
        "header = \"Accept: application/json\"\n"
        "write-out = \"\\nREDDMEDIA_HTTP_STATUS:%{http_code}\"\n";
    if (!authorization.empty()) {
        config += "header = \"Authorization: " + config_quote(authorization) + "\"\n";
    }
    const bool sent = write_all(input_pipe[1], config);
    close(input_pipe[1]);
    char buffer[8192];
    for (;;) {
        const ssize_t amount = read(output_pipe[0], buffer, sizeof(buffer));
        if (amount > 0) result.body.append(buffer, static_cast<std::size_t>(amount));
        else break;
    }
    close(output_pipe[0]);
    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
    result.process_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    const std::string marker = "\nREDDMEDIA_HTTP_STATUS:";
    const std::size_t marker_position = result.body.rfind(marker);
    if (marker_position != std::string::npos) {
        result.http_status = std::atoi(result.body.c_str() +
            static_cast<std::ptrdiff_t>(marker_position + marker.size()));
        result.body.resize(marker_position);
    }
    if (!sent) result.process_status = -1;
    return result;
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
        if (character != '\\') { result.push_back(character); continue; }
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

std::string json_compound_value(const std::string& text,
                                const std::string& key,
                                char opening,
                                char closing) {
    std::size_t position = value_position(text, key);
    if (position == std::string::npos || text[position] != opening) return {};
    const std::size_t start = position;
    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    for (; position < text.size(); ++position) {
        const char character = text[position];
        if (in_string) {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') in_string = false;
            continue;
        }
        if (character == '"') in_string = true;
        else if (character == opening) ++depth;
        else if (character == closing && --depth == 0) {
            return text.substr(start, position - start + 1U);
        }
    }
    return {};
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
        else if (character == '{') { if (depth++ == 0) start = position; }
        else if (character == '}' && depth > 0) {
            if (--depth == 0 && start != std::string::npos) {
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

int release_year(const std::string& date) {
    if (date.size() < 4U || !std::all_of(date.begin(), date.begin() + 4,
                                        [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; })) {
        return 0;
    }
    return std::atoi(date.substr(0, 4).c_str());
}

std::string safe_cache_component(const std::string& value) {
    std::string result;
    for (const unsigned char character : value) {
        result.push_back(std::isalnum(character) != 0 ? static_cast<char>(character) : '_');
        if (result.size() >= 120U) break;
    }
    return result.empty() ? "poster" : result;
}

bool numeric_id(const std::string& value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isdigit(character) != 0;
    });
}

bool valid_region(const std::string& region) {
    return region.size() == 2U && std::all_of(region.begin(), region.end(), [](unsigned char character) {
        return std::isupper(character) != 0;
    });
}

void append_provider_group(const std::string& region_object,
                           const std::string& key,
                           WatchProviderCategory category,
                           std::vector<WatchProvider>& providers) {
    for (const std::string& object : json_array_objects(region_object, key)) {
        WatchProvider provider;
        provider.id = json_int_value(object, "provider_id");
        provider.name = json_string_value(object, "provider_name");
        provider.logo_path = json_string_value(object, "logo_path");
        provider.display_priority = json_int_value(object, "display_priority");
        provider.category = category;
        if (provider.id > 0 && !provider.name.empty()) providers.push_back(std::move(provider));
    }
}

void append_catalog(const std::string& json, std::vector<WatchProvider>& providers) {
    for (const std::string& object : json_array_objects(json, "results")) {
        WatchProvider provider;
        provider.id = json_int_value(object, "provider_id");
        provider.name = json_string_value(object, "provider_name");
        provider.logo_path = json_string_value(object, "logo_path");
        provider.display_priority = json_int_value(object, "display_priority");
        if (provider.id > 0 && !provider.name.empty()) providers.push_back(std::move(provider));
    }
}

bool read_binary_file(const std::string& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::ostringstream contents;
    contents << input.rdbuf();
    bytes = contents.str();
    return !bytes.empty();
}

bool write_private_file(const std::string& path, const std::string& bytes) {
    const int file = open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (file < 0) return false;
    const bool written = write_all(file, bytes);
    const bool closed = close(file) == 0;
    chmod(path.c_str(), 0600);
    return written && closed;
}

} // namespace

TmdbClient::TmdbClient(std::string credential_file)
    : credential_file_(std::move(credential_file)) {
    const char* base_override = std::getenv("REDDMEDIA_TMDB_BASE_URL");
    base_url_ = base_override && *base_override ? base_override : "https://api.themoviedb.org";
    const char* image_override = std::getenv("REDDMEDIA_TMDB_IMAGE_BASE_URL");
    image_base_url_ = image_override && *image_override
        ? image_override : "https://image.tmdb.org/t/p/w500";
    if (!credential_file_.empty()) return;
    const char* credential_override = std::getenv("REDDMEDIA_TMDB_TOKEN_FILE");
    if (credential_override && *credential_override) credential_file_ = credential_override;
    else {
        const char* home = std::getenv("HOME");
        credential_file_ = std::string(home ? home : ".") + "/.config/reddmedia/ai/tmdb.token";
    }
}

bool TmdbClient::load_credential(std::string& credential, TmdbCredentialType& type) const {
    std::ifstream input(credential_file_);
    if (!input) { type = TmdbCredentialType::NotConfigured; return false; }
    std::getline(input, credential);
    credential = trim_copy(credential);
    chmod(credential_file_.c_str(), 0600);
    type = detect_credential_type(credential);
    return type == TmdbCredentialType::ReadAccessToken || type == TmdbCredentialType::ApiKey;
}

bool TmdbClient::has_credential() const {
    std::string credential;
    TmdbCredentialType type = TmdbCredentialType::NotConfigured;
    return load_credential(credential, type);
}

TmdbCredentialType TmdbClient::credential_type() const {
    std::string credential;
    TmdbCredentialType type = TmdbCredentialType::NotConfigured;
    load_credential(credential, type);
    return type;
}

std::string TmdbClient::credential_label() const {
    switch (credential_type()) {
    case TmdbCredentialType::ReadAccessToken: return "TMDb read access token saved";
    case TmdbCredentialType::ApiKey: return "TMDb API key saved";
    case TmdbCredentialType::Invalid: return "TMDb credential needs replacement";
    case TmdbCredentialType::NotConfigured: return "No TMDb credential saved";
    }
    return "No TMDb credential saved";
}

bool TmdbClient::request_json(const std::string& target,
                              std::string& json,
                              std::string& error,
                              const std::string* credential_override) const {
    std::string credential;
    TmdbCredentialType type = TmdbCredentialType::NotConfigured;
    if (credential_override) {
        credential = trim_copy(*credential_override);
        type = detect_credential_type(credential);
    } else if (!load_credential(credential, type)) {
        error = type == TmdbCredentialType::Invalid
            ? "The saved TMDb credential is not recognized. Use Save / Replace."
            : "TMDb credential required. Use Save / Replace to enter an API key or read access token.";
        return false;
    }
    if (type != TmdbCredentialType::ReadAccessToken && type != TmdbCredentialType::ApiKey) {
        error = "TMDb credentials must be a 32-character API key or a read access token.";
        return false;
    }
    std::string url = base_url_ + target;
    std::string authorization;
    if (type == TmdbCredentialType::ApiKey) {
        url += target.find('?') == std::string::npos ? "?api_key=" : "&api_key=";
        url += credential;
    } else {
        authorization = "Bearer " + credential;
    }
    const CurlResult response = run_curl(url, authorization);
    if (response.http_status == 401) {
        error = "TMDb rejected this credential (401). Use Save / Replace to enter a valid API key or read access token.";
        return false;
    }
    if (response.process_status != 0 || response.http_status == 0) {
        error = "TMDb is unavailable right now. Check the network and try again.";
        return false;
    }
    if (response.http_status != 200) {
        error = "TMDb request failed with HTTP " + std::to_string(response.http_status) + ".";
        return false;
    }
    if (response.body.empty()) {
        error = "TMDb returned an empty response.";
        return false;
    }
    json = response.body;
    return true;
}

bool TmdbClient::test_saved_credential(std::string& error) const {
    std::string json;
    return request_json("/3/configuration", json, error);
}

bool TmdbClient::save_credential(const std::string& credential_input, std::string& error) {
    const std::string credential = trim_copy(credential_input);
    const TmdbCredentialType type = detect_credential_type(credential);
    if (type != TmdbCredentialType::ApiKey && type != TmdbCredentialType::ReadAccessToken) {
        error = "TMDb credentials must be a 32-character API key or a read access token.";
        return false;
    }
    std::string json;
    if (!request_json("/3/configuration", json, error, &credential)) return false;
    if (!ensure_directory(parent_directory(credential_file_))) {
        error = "Nougat Media Suite could not create its private TMDb settings folder.";
        return false;
    }
    const std::string temporary = credential_file_ + ".new";
    if (!write_private_file(temporary, credential + "\n") ||
        rename(temporary.c_str(), credential_file_.c_str()) != 0) {
        unlink(temporary.c_str());
        error = "Nougat Media Suite could not store the validated TMDb credential.";
        return false;
    }
    chmod(credential_file_.c_str(), 0600);
    return true;
}

bool TmdbClient::clear_credential(std::string& error) {
    if (unlink(credential_file_.c_str()) != 0 && errno != ENOENT) {
        error = "Nougat Media Suite could not clear the saved TMDb credential.";
        return false;
    }
    unlink((credential_file_ + ".new").c_str());
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
        error = "TMDb returned no matching " + std::string(kind == "movie" ? "movies" : "TV shows") + ".";
        return false;
    }
    items = std::move(loaded);
    return true;
}

bool TmdbClient::search_title(RecommendationMediaType type,
                              const std::string& title,
                              int year,
                              std::vector<MediaDescriptor>& items,
                              std::string& error) const {
    items.clear();
    if (trim_copy(title).empty()) {
        error = "A title is required for metadata matching.";
        return false;
    }
    const std::string kind = type == RecommendationMediaType::Movie ? "movie" : "tv";
    std::string target = "/3/search/" + kind + "?include_adult=false&language=en-US&query=" +
        url_encode_query(title);
    if (year > 0) {
        target += type == RecommendationMediaType::Movie
            ? "&year=" + std::to_string(year)
            : "&first_air_date_year=" + std::to_string(year);
    }
    std::string json;
    if (!request_json(target, json, error)) return false;
    std::vector<MediaDescriptor> loaded;
    for (const std::string& object : json_array_objects(json, "results")) {
        const int id = json_int_value(object, "id");
        const std::string candidate_title = json_string_value(
            object, type == RecommendationMediaType::Movie ? "title" : "name");
        if (id <= 0 || candidate_title.empty()) continue;
        MediaDescriptor item;
        item.tmdb_id = std::to_string(id);
        item.id = "tmdb:" + kind + ":" + item.tmdb_id;
        item.title = candidate_title;
        item.overview = json_string_value(object, "overview");
        item.poster_path = json_string_value(object, "poster_path");
        item.genres = json_int_array(object, "genre_ids");
        item.media_type = type;
        item.year = release_year(json_string_value(
            object, type == RecommendationMediaType::Movie ? "release_date" : "first_air_date"));
        loaded.push_back(std::move(item));
    }
    if (loaded.empty()) {
        error = "TMDb returned no title candidates for " + title + ".";
        return false;
    }
    items = std::move(loaded);
    return true;
}

bool TmdbClient::external_imdb_id(RecommendationMediaType type,
                                  const std::string& tmdb_id,
                                  std::string& imdb_id,
                                  std::string& error) const {
    imdb_id.clear();
    if (tmdb_id.empty() || !std::all_of(tmdb_id.begin(), tmdb_id.end(), [](unsigned char c) {
            return std::isdigit(c) != 0;
        })) {
        error = "A valid TMDb title ID is required for exact IMDb lookup.";
        return false;
    }
    const std::string kind = type == RecommendationMediaType::Movie ? "movie" : "tv";
    std::string json;
    if (!request_json("/3/" + kind + "/" + tmdb_id + "/external_ids", json, error)) return false;
    imdb_id = json_string_value(json, "imdb_id");
    if (imdb_id.size() < 3U || imdb_id[0] != 't' || imdb_id[1] != 't' ||
        !std::all_of(imdb_id.begin() + 2, imdb_id.end(), [](unsigned char c) {
            return std::isdigit(c) != 0;
        })) {
        imdb_id.clear();
        error = "TMDb did not provide an exact IMDb title ID for this match.";
        return false;
    }
    return true;
}

bool TmdbClient::tv_season_count(const std::string& tmdb_id,
                                 int& season_count,
                                 std::string& error) const {
    season_count = 0;
    if (tmdb_id.empty() || !std::all_of(tmdb_id.begin(), tmdb_id.end(), [](unsigned char c) {
            return std::isdigit(c) != 0;
        })) {
        error = "A valid TMDb TV ID is required for season-count matching.";
        return false;
    }
    std::string json;
    if (!request_json("/3/tv/" + tmdb_id + "?language=en-US", json, error)) return false;
    season_count = json_int_value(json, "number_of_seasons");
    if (season_count <= 0) {
        error = "TMDb did not provide a usable season count for this TV candidate.";
        return false;
    }
    return true;
}

bool TmdbClient::load_poster_bmp(const std::string& poster_path,
                                 int width,
                                 int height,
                                 std::string& bytes,
                                 std::string& error) const {
    if (poster_path.empty()) {
        error = "TMDb did not provide a poster for this title.";
        return false;
    }
    const char* home = std::getenv("HOME");
    const std::string cache_directory = std::string(home ? home : ".") +
        "/.cache/reddmedia/posters/tmdb";
    const std::string cache_path = cache_directory + "/" + safe_cache_component(poster_path) +
        "_" + std::to_string(std::max(32, width)) + "x" +
        std::to_string(std::max(32, height)) + ".bmp";
    if (read_binary_file(cache_path, bytes) && bytes.size() >= 2U &&
        bytes[0] == 'B' && bytes[1] == 'M') {
        return true;
    }
    std::string url = image_base_url_;
    if (!url.empty() && url.back() == '/' && poster_path.front() == '/') url.pop_back();
    else if (!url.empty() && url.back() != '/' && poster_path.front() != '/') url.push_back('/');
    url += poster_path;
    const CurlResult response = run_curl(url);
    if (response.process_status != 0 || response.http_status != 200 || response.body.empty()) {
        error = "The TMDb poster is unavailable right now.";
        return false;
    }
    if (!normalize_library_poster_bmp(response.body, bytes, error)) return false;
    if (ensure_directory(cache_directory)) write_private_file(cache_path, bytes);
    return true;
}

bool TmdbClient::watch_availability(RecommendationMediaType type,
                                    const std::string& tmdb_id,
                                    const std::string& region,
                                    WatchAvailability& availability,
                                    std::string& error) const {
    if (!numeric_id(tmdb_id) || !valid_region(region)) {
        error = "A valid TMDb title and two-letter region are required for watch availability.";
        return false;
    }
    const std::string kind = type == RecommendationMediaType::Movie ? "movie" : "tv";
    std::string json;
    if (!request_json("/3/" + kind + "/" + tmdb_id + "/watch/providers", json, error)) {
        return false;
    }
    WatchAvailability loaded;
    loaded.region = region;
    loaded.refreshed_at = static_cast<long long>(std::time(nullptr));
    const std::string results = json_compound_value(json, "results", '{', '}');
    const std::string region_object = json_compound_value(results, region, '{', '}');
    if (!region_object.empty()) {
        loaded.listing_found = true;
        loaded.link = json_string_value(region_object, "link");
        append_provider_group(region_object, "flatrate", WatchProviderCategory::Subscription,
                              loaded.providers);
        append_provider_group(region_object, "free", WatchProviderCategory::Free,
                              loaded.providers);
        append_provider_group(region_object, "ads", WatchProviderCategory::Ads,
                              loaded.providers);
        append_provider_group(region_object, "rent", WatchProviderCategory::Rent,
                              loaded.providers);
        append_provider_group(region_object, "buy", WatchProviderCategory::Buy,
                              loaded.providers);
    }
    availability = std::move(loaded);
    return true;
}

bool TmdbClient::watch_provider_catalog(const std::string& region,
                                        std::vector<WatchProvider>& providers,
                                        std::string& error) const {
    if (!valid_region(region)) {
        error = "A two-letter watch region is required.";
        return false;
    }
    std::vector<WatchProvider> loaded;
    for (const std::string& kind : {std::string("movie"), std::string("tv")}) {
        std::string json;
        if (!request_json("/3/watch/providers/" + kind +
                          "?language=en-US&watch_region=" + region, json, error)) {
            return false;
        }
        append_catalog(json, loaded);
    }
    std::sort(loaded.begin(), loaded.end(), [](const WatchProvider& left,
                                                const WatchProvider& right) {
        if (left.display_priority != right.display_priority) {
            return left.display_priority < right.display_priority;
        }
        return left.name < right.name;
    });
    std::set<int> seen_provider_ids;
    loaded.erase(std::remove_if(loaded.begin(), loaded.end(), [&seen_provider_ids](
        const WatchProvider& provider) {
        return !seen_provider_ids.insert(provider.id).second;
    }), loaded.end());
    providers = std::move(loaded);
    return true;
}

bool TmdbClient::tv_episode_details(const std::string& series_tmdb_id,
                                    int season_number,
                                    int episode_number,
                                    std::string& title,
                                    std::string& overview,
                                    std::string& error) const {
    if (!numeric_id(series_tmdb_id) || season_number < 0 || episode_number <= 0) {
        error = "Exact series, season, and episode numbers are required for TMDb episode metadata.";
        return false;
    }
    std::string json;
    const std::string target = "/3/tv/" + series_tmdb_id + "/season/" +
        std::to_string(season_number) + "/episode/" + std::to_string(episode_number) +
        "?language=en-US";
    if (!request_json(target, json, error)) return false;
    title = json_string_value(json, "name");
    overview = json_string_value(json, "overview");
    if (title.empty()) {
        error = "TMDb did not provide a verified title for this episode.";
        return false;
    }
    return true;
}

bool TmdbClient::tv_poster_path(const std::string& series_tmdb_id,
                                int season_number,
                                std::string& poster_path,
                                std::string& error) const {
    if (!numeric_id(series_tmdb_id) || season_number < 0) {
        error = "A valid TMDb series and season are required for artwork fallback.";
        return false;
    }
    std::string target = "/3/tv/" + series_tmdb_id;
    if (season_number > 0) target += "/season/" + std::to_string(season_number);
    target += "?language=en-US";
    std::string json;
    if (!request_json(target, json, error)) return false;
    poster_path = json_string_value(json, "poster_path");
    if (poster_path.empty() && season_number > 0) {
        if (!request_json("/3/tv/" + series_tmdb_id + "?language=en-US", json, error)) {
            return false;
        }
        poster_path = json_string_value(json, "poster_path");
    }
    if (poster_path.empty()) {
        error = "TMDb did not provide usable series artwork.";
        return false;
    }
    return true;
}

bool TmdbClient::movie_poster_path(const std::string& tmdb_id,
                                   std::string& poster_path,
                                   std::string& error) const {
    if (!numeric_id(tmdb_id)) {
        error = "A valid TMDb movie ID is required for artwork fallback.";
        return false;
    }
    std::string json;
    if (!request_json("/3/movie/" + tmdb_id + "?language=en-US", json, error)) return false;
    poster_path = json_string_value(json, "poster_path");
    if (poster_path.empty()) {
        error = "TMDb did not provide usable movie artwork.";
        return false;
    }
    return true;
}

} // namespace reddmedia
