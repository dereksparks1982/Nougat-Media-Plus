#include "jellyfin_api_client.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>

namespace reddmedia {
namespace {

constexpr const char* kClientHeader =
    "MediaBrowser Client=\"ReddMedia\", DeviceId=\"reddmedia-local\", "
    "Device=\"ReddMedia\", Version=\"0.0.16\"";

constexpr const char* kMovieLibraryName = "ReddMedia Movies";
constexpr const char* kTelevisionLibraryName = "ReddMedia TV";

std::string lower_copy(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

bool path_is_inside_folder(const std::string& path, const std::string& folder) {
    if (path.empty() || folder.empty()) return false;
    if (path == folder) return true;
    if (path.size() <= folder.size() || path.compare(0, folder.size(), folder) != 0) {
        return false;
    }
    if (folder.back() == '/') return true;
    return path[folder.size()] == '/';
}

std::string json_escape(const std::string& value) {
    std::ostringstream result;
    for (const unsigned char character : value) {
        switch (character) {
        case '\\': result << "\\\\"; break;
        case '"': result << "\\\""; break;
        case '\n': result << "\\n"; break;
        case '\r': result << "\\r"; break;
        case '\t': result << "\\t"; break;
        default:
            if (character < 0x20U) {
                result << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<unsigned>(character) << std::dec;
            } else {
                result << static_cast<char>(character);
            }
        }
    }
    return result.str();
}

void append_utf8(std::string& output, unsigned value) {
    if (value <= 0x7fU) {
        output.push_back(static_cast<char>(value));
    } else if (value <= 0x7ffU) {
        output.push_back(static_cast<char>(0xc0U | (value >> 6U)));
        output.push_back(static_cast<char>(0x80U | (value & 0x3fU)));
    } else {
        output.push_back(static_cast<char>(0xe0U | (value >> 12U)));
        output.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (value & 0x3fU)));
    }
}

int hex_value(char character) {
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

std::string json_string_value(const std::string& text, const std::string& key) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t key_position = text.find(marker);
    if (key_position == std::string::npos) return {};
    const std::size_t colon = text.find(':', key_position + marker.size());
    if (colon == std::string::npos) return {};
    const std::size_t quote = text.find('"', colon + 1U);
    if (quote == std::string::npos) return {};

    std::string result;
    for (std::size_t index = quote + 1U; index < text.size(); ++index) {
        const char character = text[index];
        if (character == '"') break;
        if (character != '\\') {
            result.push_back(character);
            continue;
        }
        if (++index >= text.size()) break;
        const char escaped = text[index];
        switch (escaped) {
        case '"': result.push_back('"'); break;
        case '\\': result.push_back('\\'); break;
        case '/': result.push_back('/'); break;
        case 'b': result.push_back('\b'); break;
        case 'f': result.push_back('\f'); break;
        case 'n': result.push_back('\n'); break;
        case 'r': result.push_back('\r'); break;
        case 't': result.push_back('\t'); break;
        case 'u': {
            if (index + 4U >= text.size()) return result;
            unsigned value = 0;
            for (int count = 0; count < 4; ++count) {
                const int part = hex_value(text[++index]);
                if (part < 0) return result;
                value = value * 16U + static_cast<unsigned>(part);
            }
            append_utf8(result, value);
            break;
        }
        default: result.push_back(escaped); break;
        }
    }
    return result;
}

int json_int_value(const std::string& text, const std::string& key) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t key_position = text.find(marker);
    if (key_position == std::string::npos) return 0;
    const std::size_t colon = text.find(':', key_position + marker.size());
    if (colon == std::string::npos) return 0;
    const std::size_t start = text.find_first_of("-0123456789", colon + 1U);
    if (start == std::string::npos) return 0;
    return std::atoi(text.c_str() + static_cast<std::ptrdiff_t>(start));
}

std::vector<std::string> json_array_objects(const std::string& text, const std::string& key) {
    std::vector<std::string> objects;
    const std::string marker = "\"" + key + "\"";
    std::size_t position = text.find(marker);
    if (position == std::string::npos) return objects;
    position = text.find('[', position + marker.size());
    if (position == std::string::npos) return objects;

    bool in_string = false;
    bool escaped = false;
    int object_depth = 0;
    std::size_t object_start = std::string::npos;
    for (++position; position < text.size(); ++position) {
        const char character = text[position];
        if (in_string) {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') in_string = false;
            continue;
        }
        if (character == '"') {
            in_string = true;
            continue;
        }
        if (character == ']' && object_depth == 0) break;
        if (character == '{') {
            if (object_depth == 0) object_start = position;
            ++object_depth;
        } else if (character == '}' && object_depth > 0) {
            --object_depth;
            if (object_depth == 0 && object_start != std::string::npos) {
                objects.push_back(text.substr(object_start, position - object_start + 1U));
                object_start = std::string::npos;
            }
        }
    }
    return objects;
}

std::vector<std::string> json_root_array_objects(const std::string& text) {
    std::vector<std::string> objects;
    std::size_t position = text.find('[');
    if (position == std::string::npos) return objects;
    bool in_string = false;
    bool escaped = false;
    int object_depth = 0;
    std::size_t object_start = std::string::npos;
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
            if (object_depth == 0) object_start = position;
            ++object_depth;
        } else if (character == '}' && object_depth > 0) {
            --object_depth;
            if (object_depth == 0 && object_start != std::string::npos) {
                objects.push_back(text.substr(object_start, position - object_start + 1U));
                object_start = std::string::npos;
            }
        } else if (character == ']' && object_depth == 0) break;
    }
    return objects;
}

std::vector<std::string> json_string_array(const std::string& text, const std::string& key) {
    std::vector<std::string> values;
    const std::string marker = "\"" + key + "\"";
    std::size_t position = text.find(marker);
    if (position == std::string::npos) return values;
    position = text.find('[', position + marker.size());
    if (position == std::string::npos) return values;
    const std::size_t end = text.find(']', position + 1U);
    if (end == std::string::npos) return values;
    while (position < end) {
        const std::size_t quote = text.find('"', position + 1U);
        if (quote == std::string::npos || quote >= end) break;
        const std::string tail = text.substr(quote);
        const std::string value = json_string_value("{\"value\":" + tail + "}", "value");
        if (!value.empty()) values.push_back(value);
        std::size_t next = quote + 1U;
        bool escaped = false;
        while (next < end) {
            if (!escaped && text[next] == '"') { ++next; break; }
            if (!escaped && text[next] == '\\') escaped = true;
            else escaped = false;
            ++next;
        }
        position = next;
    }
    return values;
}

std::string url_encode(const std::string& value) {
    static const char* digits = "0123456789ABCDEF";
    std::string result;
    for (const unsigned char character : value) {
        if (std::isalnum(character) != 0 || character == '-' || character == '_' ||
            character == '.' || character == '~') {
            result.push_back(static_cast<char>(character));
        } else {
            result.push_back('%');
            result.push_back(digits[character >> 4U]);
            result.push_back(digits[character & 0x0fU]);
        }
    }
    return result;
}

std::string parent_directory(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return ".";
    if (slash == 0) return "/";
    return path.substr(0, slash);
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
            if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) return false;
        }
        if (slash == std::string::npos) break;
        start = slash + 1U;
    }
    return true;
}

std::string decode_chunked(const std::string& body) {
    std::string result;
    std::size_t position = 0;
    while (position < body.size()) {
        const std::size_t line_end = body.find("\r\n", position);
        if (line_end == std::string::npos) return {};
        std::string length_text = body.substr(position, line_end - position);
        const std::size_t extension = length_text.find(';');
        if (extension != std::string::npos) length_text.resize(extension);
        char* end = nullptr;
        const unsigned long length = std::strtoul(length_text.c_str(), &end, 16);
        if (end == length_text.c_str()) return {};
        position = line_end + 2U;
        if (length == 0UL) break;
        if (length > body.size() - position) return {};
        result.append(body, position, static_cast<std::size_t>(length));
        position += static_cast<std::size_t>(length);
        if (body.compare(position, 2U, "\r\n") != 0) return {};
        position += 2U;
    }
    return result;
}

const char* library_name(LibraryMediaType media_type) {
    return media_type == LibraryMediaType::Movies ? kMovieLibraryName : kTelevisionLibraryName;
}

const char* collection_type(LibraryMediaType media_type) {
    return media_type == LibraryMediaType::Movies ? "movies" : "tvshows";
}

LibraryNodeKind node_kind(const std::string& type) {
    if (type == "BoxSet") return LibraryNodeKind::MovieCollection;
    if (type == "Series") return LibraryNodeKind::Series;
    if (type == "Season") return LibraryNodeKind::Season;
    if (type == "Episode") return LibraryNodeKind::Episode;
    return LibraryNodeKind::Movie;
}

LibraryNode parse_library_node(const std::string& object) {
    LibraryNode node;
    node.id = json_string_value(object, "Id");
    node.parent_id = json_string_value(object, "ParentId");
    node.series_id = json_string_value(object, "SeriesId");
    node.season_id = json_string_value(object, "SeasonId");
    node.name = json_string_value(object, "Name");
    node.path = json_string_value(object, "Path");
    node.overview = json_string_value(object, "Overview");
    node.genres = json_string_array(object, "Genres");
    node.production_year = json_int_value(object, "ProductionYear");
    node.child_count = json_int_value(object, "ChildCount");
    node.kind = node_kind(json_string_value(object, "Type"));
    const std::size_t image_tags = object.find("\"ImageTags\"");
    if (image_tags != std::string::npos) {
        node.primary_image_tag = json_string_value(object.substr(image_tags), "Primary");
    }
    const std::size_t provider_ids = object.find("\"ProviderIds\"");
    if (provider_ids != std::string::npos) {
        node.tmdb_id = json_string_value(object.substr(provider_ids), "Tmdb");
    }
    return node;
}

std::string common_item_fields() {
    return "Path%2CProductionYear%2COverview%2CGenres%2CProviderIds%2CParentId%2C"
           "SeriesId%2CSeasonId%2CChildCount";
}

} // namespace

JellyfinApiClient::JellyfinApiClient(std::string state_file)
    : state_file_(std::move(state_file)) {
    if (!state_file_.empty()) return;
    const char* override_file = std::getenv("REDDMEDIA_SERVER_CLIENT_CONFIG");
    if (override_file && *override_file) {
        state_file_ = override_file;
        return;
    }
    const char* home = std::getenv("HOME");
    state_file_ = std::string(home ? home : ".") + "/.config/reddmedia/server/client.json";
}

std::string JellyfinApiClient::authorization(bool include_token) const {
    std::string header = kClientHeader;
    if (include_token && !access_token_.empty()) header += ", Token=\"" + access_token_ + "\"";
    return header;
}

JellyfinApiClient::HttpResponse JellyfinApiClient::request(
    const std::string& method,
    const std::string& target,
    const std::string& body,
    bool authenticated,
    int timeout_seconds) const {
    HttpResponse response;
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) return response;

    timeval timeout {timeout_seconds, 0};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(8096);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    if (connect(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        close(socket_fd);
        return response;
    }

    std::ostringstream message;
    message << method << ' ' << target << " HTTP/1.1\r\n"
            << "Host: 127.0.0.1:8096\r\n"
            << "Accept: application/json\r\n"
            << "Content-Type: application/json\r\n"
            << "Authorization: " << authorization(authenticated) << "\r\n"
            << "Connection: close\r\n"
            << "Content-Length: " << body.size() << "\r\n\r\n"
            << body;
    const std::string request_text = message.str();
    std::size_t sent = 0;
    while (sent < request_text.size()) {
        const ssize_t amount = send(socket_fd, request_text.data() + sent,
                                    request_text.size() - sent, MSG_NOSIGNAL);
        if (amount <= 0) {
            close(socket_fd);
            return response;
        }
        sent += static_cast<std::size_t>(amount);
    }

    std::string raw;
    char buffer[8192];
    for (;;) {
        const ssize_t amount = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (amount > 0) raw.append(buffer, static_cast<std::size_t>(amount));
        else break;
    }
    close(socket_fd);

    const std::size_t status_end = raw.find("\r\n");
    const std::size_t header_end = raw.find("\r\n\r\n");
    if (status_end == std::string::npos || header_end == std::string::npos) return response;
    std::istringstream status_line(raw.substr(0, status_end));
    std::string protocol;
    status_line >> protocol >> response.status;
    const std::string headers = lower_copy(raw.substr(status_end + 2U, header_end - status_end - 2U));
    response.body = raw.substr(header_end + 4U);
    if (headers.find("transfer-encoding: chunked") != std::string::npos) {
        response.body = decode_chunked(response.body);
    }
    return response;
}

bool JellyfinApiClient::load_state() {
    std::ifstream input(state_file_);
    if (!input) return false;
    std::ostringstream contents;
    contents << input.rdbuf();
    const std::string json = contents.str();
    username_ = json_string_value(json, "Username");
    access_token_ = json_string_value(json, "AccessToken");
    user_id_ = json_string_value(json, "UserId");
    return !username_.empty() && !access_token_.empty();
}

bool JellyfinApiClient::save_state(std::string& error) const {
    if (!ensure_directory(parent_directory(state_file_))) {
        error = "Could not create ReddMedia's private server settings folder.";
        return false;
    }
    const int file = open(state_file_.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (file < 0) {
        error = "Could not save ReddMedia's private server session.";
        return false;
    }
    const std::string json = "{\n  \"Username\": \"" + json_escape(username_) +
                             "\",\n  \"AccessToken\": \"" + json_escape(access_token_) +
                             "\",\n  \"UserId\": \"" + json_escape(user_id_) + "\"\n}\n";
    std::size_t written = 0;
    while (written < json.size()) {
        const ssize_t amount = write(file, json.data() + written, json.size() - written);
        if (amount <= 0) {
            close(file);
            error = "Could not save ReddMedia's private server session.";
            return false;
        }
        written += static_cast<std::size_t>(amount);
    }
    if (close(file) != 0) {
        error = "Could not finish saving ReddMedia's private server session.";
        return false;
    }
    return true;
}

bool JellyfinApiClient::validate_saved_token() {
    if (access_token_.empty()) return false;
    const HttpResponse response = request("GET", "/Users/Me", "", true);
    if (response.status != 200) return false;
    const std::string id = json_string_value(response.body, "Id");
    if (!id.empty()) user_id_ = id;
    return !user_id_.empty();
}

bool JellyfinApiClient::authenticate(std::string& error) {
    if (username_.empty()) {
        error = "ReddMedia could not identify its local media-library account.";
        return false;
    }
    const std::string body = "{\"Username\":\"" + json_escape(username_) + "\",\"Pw\":\"\"}";
    const HttpResponse response = request("POST", "/Users/AuthenticateByName", body, false);
    if (response.status != 200) {
        error = "ReddMedia could not open its private local media-library session.";
        return false;
    }
    access_token_ = json_string_value(response.body, "AccessToken");
    user_id_ = json_string_value(response.body, "Id");
    if (user_id_.empty()) {
        const std::string user_object = json_string_value(response.body, "User");
        (void)user_object;
        const std::size_t user_position = response.body.find("\"User\"");
        if (user_position != std::string::npos) {
            user_id_ = json_string_value(response.body.substr(user_position), "Id");
        }
    }
    if (access_token_.empty()) {
        error = "The local media catalog did not return a session token.";
        return false;
    }
    const HttpResponse me = request("GET", "/Users/Me", "", true);
    if (me.status == 200) user_id_ = json_string_value(me.body, "Id");
    if (user_id_.empty()) {
        error = "The local media catalog did not return a user identity.";
        return false;
    }
    return save_state(error);
}

bool JellyfinApiClient::initialize(std::string& error) {
    if (initialized_) return true;
    load_state();

    // Jellyfin 10.11 starts a temporary setup server before the real API is
    // ready. Its /System/Info/Public response is therefore a liveness signal,
    // not a readiness signal. /Startup/User is served by the real API: 200
    // means first-time setup is open, while 401/403 means setup is complete.
    HttpResponse startup_user;
    for (int attempt = 0; attempt < 60; ++attempt) {
        startup_user = request("GET", "/Startup/User", "", false);
        if (startup_user.status == 200 || startup_user.status == 401 ||
            startup_user.status == 403) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (startup_user.status != 200 && startup_user.status != 401 &&
        startup_user.status != 403) {
        error = "ReddMedia's local media catalog is not ready yet.";
        return false;
    }

    if (startup_user.status == 200) {
        const std::string configuration =
            "{\"ServerName\":\"ReddMedia\",\"UICulture\":\"en-US\","
            "\"MetadataCountryCode\":\"US\",\"PreferredMetadataLanguage\":\"en\"}";
        if (request("POST", "/Startup/Configuration", configuration, false).status != 204) {
            error = "ReddMedia could not configure its local media catalog.";
            return false;
        }
        const std::string remote =
            "{\"EnableRemoteAccess\":false,\"EnableAutomaticPortMapping\":false}";
        if (request("POST", "/Startup/RemoteAccess", remote, false).status != 204) {
            error = "ReddMedia could not lock the media catalog to local use.";
            return false;
        }
        username_ = json_string_value(startup_user.body, "Name");
        if (username_.empty()) {
            error = "ReddMedia's local media-library account has no name.";
            return false;
        }
        if (request("POST", "/Startup/Complete", "{}", false).status != 204) {
            error = "ReddMedia could not finish local media-library setup.";
            return false;
        }
    } else if (validate_saved_token()) {
        initialized_ = true;
        return true;
    } else if (username_.empty()) {
        error = "The hidden catalog was configured outside ReddMedia. Remove ~/.config/reddmedia/server and ~/.local/share/reddmedia/server, then reopen ReddMedia.";
        return false;
    }

    if (!authenticate(error)) return false;
    initialized_ = true;
    return true;
}

bool JellyfinApiClient::add_media_folder(const std::string& path, std::string& error) {
    return add_media_folder(path, LibraryMediaType::Movies, error);
}

bool JellyfinApiClient::load_media_folders(std::vector<MediaFolder>& folders,
                                           std::string& error) {
    if (!initialize(error)) return false;
    const HttpResponse response = request("GET", "/Library/VirtualFolders", "", true);
    if (response.status != 200) {
        error = "ReddMedia could not read its linked media folders.";
        return false;
    }
    std::vector<MediaFolder> loaded;
    for (const std::string& object : json_root_array_objects(response.body)) {
        const std::string name = json_string_value(object, "Name");
        const std::string type = json_string_value(object, "CollectionType");
        if (name != kMovieLibraryName && name != kTelevisionLibraryName) continue;
        const LibraryMediaType media_type = type == "tvshows"
            ? LibraryMediaType::Television : LibraryMediaType::Movies;
        for (const std::string& path : json_string_array(object, "Locations")) {
            loaded.push_back({name, path, media_type});
        }
    }
    folders = std::move(loaded);
    return true;
}

bool JellyfinApiClient::add_media_folder(const std::string& path,
                                         LibraryMediaType media_type,
                                         std::string& error) {
    if (path.empty()) {
        error = "Choose a media folder first.";
        return false;
    }
    struct stat information {};
    if (stat(path.c_str(), &information) != 0 || !S_ISDIR(information.st_mode)) {
        error = "That media folder is no longer available.";
        return false;
    }
    if (!initialize(error)) return false;

    // v0.0.15 created one untyped, hash-named catalog library per selected
    // path. When the owner chooses that same path under Movies or TV, migrate
    // only the catalog link into the typed v0.0.16 library. Media files are
    // never touched.
    const HttpResponse legacy_response = request("GET", "/Library/VirtualFolders", "", true);
    if (legacy_response.status != 200) {
        error = "ReddMedia could not inspect its existing media-folder links.";
        return false;
    }
    for (const std::string& object : json_root_array_objects(legacy_response.body)) {
        const std::string name = json_string_value(object, "Name");
        const std::vector<std::string> locations = json_string_array(object, "Locations");
        if (name == kMovieLibraryName || name == kTelevisionLibraryName ||
            std::find(locations.begin(), locations.end(), path) == locations.end()) {
            continue;
        }
        const std::string target = locations.size() == 1U
            ? "/Library/VirtualFolders?name=" + url_encode(name) + "&refreshLibrary=false"
            : "/Library/VirtualFolders/Paths?name=" + url_encode(name) +
              "&path=" + url_encode(path) + "&refreshLibrary=false";
        if (request("DELETE", target, "", true, 30).status != 204) {
            error = "ReddMedia could not migrate the existing v0.0.15 folder link.";
            return false;
        }
    }

    std::vector<MediaFolder> folders;
    if (!load_media_folders(folders, error)) return false;
    for (const MediaFolder& folder : folders) {
        if (folder.path == path && folder.media_type == media_type) return refresh_library(error);
        if (folder.path == path && folder.media_type != media_type) {
            error = "That folder is already linked to the other ReddMedia library.";
            return false;
        }
    }
    bool library_exists = false;
    for (const MediaFolder& folder : folders) {
        if (folder.media_type == media_type) library_exists = true;
    }
    HttpResponse added;
    if (library_exists) {
        const std::string body = "{\"Name\":\"" +
            json_escape(library_name(media_type)) + "\",\"Path\":\"" +
            json_escape(path) + "\"}";
        added = request("POST", "/Library/VirtualFolders/Paths?refreshLibrary=false",
                        body, true, 30);
    } else {
        const std::string target = "/Library/VirtualFolders?name=" +
            url_encode(library_name(media_type)) + "&collectionType=" +
            collection_type(media_type) + "&paths=" + url_encode(path) +
            "&refreshLibrary=false";
        const std::string auto_collections = media_type == LibraryMediaType::Movies
            ? ",\"AutomaticallyAddToCollection\":true" : "";
        const std::string body = "{\"LibraryOptions\":{\"Enabled\":true,"
            "\"EnableRealtimeMonitor\":true" + auto_collections + "}}";
        added = request("POST", target, body, true, 30);
    }
    if (added.status != 204) {
        error = "ReddMedia could not link that media folder.";
        return false;
    }
    return refresh_library(error);
}

bool JellyfinApiClient::unlink_media_folder(const std::string& path,
                                            LibraryMediaType media_type,
                                            std::string& error) {
    if (!initialize(error)) return false;
    std::vector<MediaFolder> folders;
    if (!load_media_folders(folders, error)) return false;
    int matching_type = 0;
    bool found = false;
    for (const MediaFolder& folder : folders) {
        if (folder.media_type == media_type) ++matching_type;
        if (folder.media_type == media_type && folder.path == path) found = true;
    }
    if (!found) {
        error = "That folder is not linked to this ReddMedia library.";
        return false;
    }
    const std::string target = matching_type <= 1
        ? "/Library/VirtualFolders?name=" + url_encode(library_name(media_type)) +
          "&refreshLibrary=false"
        : "/Library/VirtualFolders/Paths?name=" + url_encode(library_name(media_type)) +
          "&path=" + url_encode(path) + "&refreshLibrary=false";
    const HttpResponse response = request("DELETE", target, "", true, 30);
    if (response.status != 204) {
        error = "ReddMedia could not unlink that media folder.";
        return false;
    }
    return refresh_library(error);
}

bool JellyfinApiClient::load_library_roots(LibraryMediaType media_type,
                                           std::vector<LibraryNode>& nodes,
                                           std::string& error) {
    if (!initialize(error)) return false;
    const std::string item_types = media_type == LibraryMediaType::Movies
        ? "BoxSet%2CMovie" : "Series";
    const std::string collapse = media_type == LibraryMediaType::Movies
        ? "&collapseBoxSetItems=true" : "";
    const std::string target = "/Items?userId=" + url_encode(user_id_) +
        "&recursive=true&includeItemTypes=" + item_types +
        "&fields=" + common_item_fields() +
        "&sortBy=SortName&sortOrder=Ascending&enableImages=true" + collapse;
    const HttpResponse response = request("GET", target, "", true, 60);
    if (response.status != 200) {
        error = "ReddMedia could not read this media library.";
        return false;
    }
    std::vector<LibraryNode> loaded;
    for (const std::string& object : json_array_objects(response.body, "Items")) {
        LibraryNode node = parse_library_node(object);
        if (!node.id.empty() && !node.name.empty()) loaded.push_back(std::move(node));
    }
    nodes = std::move(loaded);
    return true;
}

bool JellyfinApiClient::load_library_children(const LibraryNode& parent,
                                              std::vector<LibraryNode>& nodes,
                                              std::string& error) {
    if (!initialize(error)) return false;
    std::string item_types;
    if (parent.kind == LibraryNodeKind::MovieCollection) item_types = "Movie";
    else if (parent.kind == LibraryNodeKind::Series) item_types = "Season";
    else if (parent.kind == LibraryNodeKind::Season) item_types = "Episode";
    else {
        error = "That library item does not contain another level.";
        return false;
    }
    const std::string target = "/Items?userId=" + url_encode(user_id_) +
        "&parentId=" + url_encode(parent.id) + "&recursive=false&includeItemTypes=" +
        item_types + "&fields=" + common_item_fields() +
        "&sortBy=SortName&sortOrder=Ascending&enableImages=true";
    const HttpResponse response = request("GET", target, "", true, 60);
    if (response.status != 200) {
        error = "ReddMedia could not open that library level.";
        return false;
    }
    std::vector<LibraryNode> loaded;
    for (const std::string& object : json_array_objects(response.body, "Items")) {
        LibraryNode node = parse_library_node(object);
        if (!node.id.empty() && !node.name.empty()) loaded.push_back(std::move(node));
    }
    nodes = std::move(loaded);
    return true;
}

bool JellyfinApiClient::load_all_recommendation_items(std::vector<LibraryNode>& nodes,
                                                      std::string& error) {
    if (!initialize(error)) return false;
    const std::string target = "/Items?userId=" + url_encode(user_id_) +
        "&recursive=true&includeItemTypes=Movie%2CSeries&fields=" +
        common_item_fields() +
        "&sortBy=SortName&sortOrder=Ascending&enableImages=true";
    const HttpResponse response = request("GET", target, "", true, 60);
    if (response.status != 200) {
        error = "ReddMedia could not read local recommendation metadata.";
        return false;
    }
    std::vector<LibraryNode> loaded;
    for (const std::string& object : json_array_objects(response.body, "Items")) {
        LibraryNode node = parse_library_node(object);
        if (!node.id.empty() && !node.name.empty()) loaded.push_back(std::move(node));
    }
    nodes = std::move(loaded);
    return true;
}

bool JellyfinApiClient::load_primary_image_bmp(const std::string& item_id,
                                               int width,
                                               int height,
                                               std::string& bytes,
                                               std::string& error) {
    if (!initialize(error)) return false;
    if (item_id.empty()) {
        error = "A library item is required before loading its poster.";
        return false;
    }
    width = std::max(32, std::min(1024, width));
    height = std::max(32, std::min(1536, height));
    const std::string target = "/Items/" + url_encode(item_id) +
        "/Images/Primary?format=Bmp&maxWidth=" + std::to_string(width) +
        "&maxHeight=" + std::to_string(height) + "&quality=90";
    const HttpResponse response = request("GET", target, "", true, 30);
    if (response.status != 200 || response.body.empty()) {
        error = "No poster is available for this library item.";
        return false;
    }
    bytes = response.body;
    return true;
}

bool JellyfinApiClient::refresh_library(std::string& error) {
    if (!initialize(error)) return false;
    const HttpResponse response = request("POST", "/Library/Refresh", "{}", true, 300);
    if (response.status != 204) {
        error = "ReddMedia could not finish scanning the media library.";
        return false;
    }
    return true;
}

bool JellyfinApiClient::load_videos(std::vector<LibraryVideo>& videos, std::string& error) {
    if (!initialize(error)) return false;
    const std::string target =
        "/Items?userId=" + url_encode(user_id_) +
        "&recursive=true&includeItemTypes=Movie%2CEpisode%2CVideo&mediaTypes=Video"
        "&fields=Path%2CProductionYear&sortBy=SortName&sortOrder=Ascending"
        "&enableTotalRecordCount=true";
    const HttpResponse response = request("GET", target, "", true, 60);
    if (response.status != 200) {
        error = "ReddMedia could not read the media library.";
        return false;
    }

    std::vector<LibraryVideo> loaded;
    for (const std::string& object : json_array_objects(response.body, "Items")) {
        LibraryVideo video;
        video.id = json_string_value(object, "Id");
        video.name = json_string_value(object, "Name");
        video.path = json_string_value(object, "Path");
        video.type = json_string_value(object, "Type");
        video.production_year = json_int_value(object, "ProductionYear");
        if (!video.path.empty() && !video.name.empty()) loaded.push_back(std::move(video));
    }
    videos = std::move(loaded);
    return true;
}

bool JellyfinApiClient::wait_for_video_in_folder(
    const std::string& folder,
    std::vector<LibraryVideo>& videos,
    std::string& error,
    int timeout_seconds) {
    if (folder.empty()) {
        error = "The media scan has no folder to verify.";
        return false;
    }
    if (timeout_seconds < 0) timeout_seconds = 0;

    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(timeout_seconds);
    for (;;) {
        if (!load_videos(videos, error)) return false;
        for (const LibraryVideo& video : videos) {
            if (path_is_inside_folder(video.path, folder)) return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            error = "The media scan did not index a playable video from that folder "
                    "before the verification timeout.";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace reddmedia
