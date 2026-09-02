#include "lan_media_service.hpp"
#include "../media_server/jellyfin_api_client.hpp"

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <ifaddrs.h>
#include <map>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <set>
#include <sstream>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace reddmedia::lan {
namespace {

constexpr std::size_t kMaxRequestBytes = 64U * 1024U;
constexpr int kMaxConcurrentClients = 8;

std::string parent_directory(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? "." : path.substr(0, slash);
}

std::string executable_directory() {
    char path[4096]{};
    const ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1U);
    if (length <= 0) return ".";
    std::string directory = parent_directory(std::string(path, static_cast<std::size_t>(length)));
    const std::string helper_suffix = "/components/web_player";
    if (directory.size() >= helper_suffix.size() &&
        directory.compare(directory.size() - helper_suffix.size(), helper_suffix.size(), helper_suffix) == 0) {
        directory = parent_directory(parent_directory(directory));
    }
    return directory;
}

std::string lower_ascii(std::string value) {
    for (char& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

std::string extension_lower(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    const std::size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return {};
    return lower_ascii(path.substr(dot));
}

bool browser_direct_preferred(const std::string& path) {
    const std::string extension = extension_lower(path);
    return extension == ".mp4" || extension == ".m4v" || extension == ".webm" ||
           extension == ".ogv" || extension == ".ogg";
}

std::string media_content_type(const std::string& path) {
    const std::string extension = extension_lower(path);
    if (extension == ".mp4" || extension == ".m4v") return "video/mp4";
    if (extension == ".webm") return "video/webm";
    if (extension == ".ogv" || extension == ".ogg") return "video/ogg";
    if (extension == ".mov") return "video/quicktime";
    if (extension == ".mkv") return "video/x-matroska";
    if (extension == ".avi") return "video/x-msvideo";
    if (extension == ".ts" || extension == ".m2ts") return "video/mp2t";
    return "application/octet-stream";
}

std::string static_content_type(const std::string& path) {
    if (path.size() >= 5U && path.substr(path.size() - 5U) == ".html")
        return "text/html; charset=utf-8";
    if (path.size() >= 3U && path.substr(path.size() - 3U) == ".js")
        return "text/javascript; charset=utf-8";
    if (path.size() >= 4U && path.substr(path.size() - 4U) == ".css")
        return "text/css; charset=utf-8";
    return "application/octet-stream";
}

std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    std::ostringstream out;
    out << input.rdbuf();
    return out.str();
}

std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 16U);
    for (const unsigned char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20U) {
                static constexpr char hex[] = "0123456789abcdef";
                out += "\\u00";
                out.push_back(hex[(c >> 4U) & 0x0fU]);
                out.push_back(hex[c & 0x0fU]);
            } else {
                out.push_back(static_cast<char>(c));
            }
            break;
        }
    }
    return out;
}

std::string percent_decode(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2U < value.size()) {
            const auto hex_value = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };
            const int hi = hex_value(value[i + 1U]);
            const int lo = hex_value(value[i + 2U]);
            if (hi >= 0 && lo >= 0) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2U;
                continue;
            }
        }
        out.push_back(value[i] == '+' ? ' ' : value[i]);
    }
    return out;
}

std::map<std::string, std::string> query_values(const std::string& query) {
    std::map<std::string, std::string> values;
    std::size_t start = 0;
    while (start <= query.size()) {
        const std::size_t amp = query.find('&', start);
        const std::string pair = query.substr(
            start, amp == std::string::npos ? std::string::npos : amp - start);
        const std::size_t equal = pair.find('=');
        if (equal == std::string::npos) values[percent_decode(pair)] = {};
        else values[percent_decode(pair.substr(0, equal))] = percent_decode(pair.substr(equal + 1U));
        if (amp == std::string::npos) break;
        start = amp + 1U;
    }
    return values;
}

bool send_all(int client, const char* data, std::size_t bytes) {
    std::size_t sent = 0;
    while (sent < bytes) {
        const ssize_t amount = send(client, data + sent, bytes - sent, MSG_NOSIGNAL);
        if (amount <= 0) return false;
        sent += static_cast<std::size_t>(amount);
    }
    return true;
}

bool send_all(int client, const std::string& data) {
    return send_all(client, data.data(), data.size());
}

bool send_response(int client,
                   int status,
                   const char* reason,
                   const std::string& content_type,
                   const std::string& body,
                   bool head_only = false,
                   const std::vector<std::pair<std::string, std::string>>& extra_headers = {}) {
    std::ostringstream header;
    header << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
           << "Server: Nougat/0.0.55\r\n"
           << "Content-Type: " << content_type << "\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Cache-Control: no-store\r\n"
           << "X-Content-Type-Options: nosniff\r\n"
           << "Referrer-Policy: no-referrer\r\n"
           << "Connection: close\r\n";
    for (const auto& item : extra_headers) header << item.first << ": " << item.second << "\r\n";
    header << "\r\n";
    if (!send_all(client, header.str())) return false;
    return head_only || body.empty() || send_all(client, body);
}

bool send_json_error(int client, int status, const char* reason, const std::string& message) {
    return send_response(client, status, reason, "application/json; charset=utf-8",
                         "{\"ok\":false,\"error\":\"" + json_escape(message) + "\"}");
}

bool regular_file(const std::string& path, struct stat& info) {
    return !path.empty() && stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

struct ByteRange {
    bool requested = false;
    bool valid = false;
    off_t first = 0;
    off_t last = 0;
};

ByteRange parse_range(const std::string& value, off_t size) {
    ByteRange range;
    if (value.empty()) return range;
    range.requested = true;
    if (size <= 0 || value.rfind("bytes=", 0U) != 0U) return range;
    const std::string spec = value.substr(6U);
    if (spec.find(',') != std::string::npos) return range;
    const std::size_t dash = spec.find('-');
    if (dash == std::string::npos) return range;

    const std::string first_text = spec.substr(0, dash);
    const std::string last_text = spec.substr(dash + 1U);
    try {
        if (first_text.empty()) {
            if (last_text.empty()) return range;
            const long long suffix = std::stoll(last_text);
            if (suffix <= 0) return range;
            const off_t count = static_cast<off_t>(std::min<long long>(suffix, size));
            range.first = size - count;
            range.last = size - 1;
        } else {
            const long long parsed_first = std::stoll(first_text);
            if (parsed_first < 0 || parsed_first >= size) return range;
            range.first = static_cast<off_t>(parsed_first);
            if (last_text.empty()) {
                range.last = size - 1;
            } else {
                const long long parsed_last = std::stoll(last_text);
                if (parsed_last < parsed_first) return range;
                range.last = static_cast<off_t>(std::min<long long>(parsed_last, size - 1));
            }
        }
    } catch (...) {
        return range;
    }
    range.valid = range.first >= 0 && range.last >= range.first && range.last < size;
    return range;
}

struct SharedState {
    explicit SharedState(std::string root) : application_dir(std::move(root)) {}

    std::string application_dir;
    std::atomic<bool> stopping{false};
    std::atomic<int> active_clients{0};
    std::mutex catalog_mutex;
    reddmedia::JellyfinApiClient jellyfin;
    std::map<std::string, reddmedia::LibraryVideo> catalog;
};

bool refresh_catalog(const std::shared_ptr<SharedState>& state,
                     std::vector<reddmedia::LibraryVideo>& videos,
                     std::string& error) {
    std::lock_guard<std::mutex> lock(state->catalog_mutex);
    if (!state->jellyfin.load_videos(videos, error)) return false;
    state->catalog.clear();
    for (const auto& video : videos) state->catalog[video.id] = video;
    return true;
}

bool resolve_catalog_item(const std::shared_ptr<SharedState>& state,
                          const std::string& id,
                          reddmedia::LibraryVideo& video,
                          std::string& error) {
    if (id.empty()) {
        error = "Missing media id.";
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(state->catalog_mutex);
        const auto found = state->catalog.find(id);
        if (found != state->catalog.end()) {
            video = found->second;
            return true;
        }
    }
    std::vector<reddmedia::LibraryVideo> videos;
    if (!refresh_catalog(state, videos, error)) return false;
    std::lock_guard<std::mutex> lock(state->catalog_mutex);
    const auto found = state->catalog.find(id);
    if (found == state->catalog.end()) {
        error = "That media item is no longer in the Nougat library.";
        return false;
    }
    video = found->second;
    return true;
}

bool send_catalog(int client, const std::shared_ptr<SharedState>& state, bool head_only) {
    std::vector<reddmedia::LibraryVideo> videos;
    std::string error;
    if (!refresh_catalog(state, videos, error)) {
        return send_json_error(client, 503, "Service Unavailable",
                               error.empty() ? "The Nougat media catalog is not ready." : error);
    }

    std::ostringstream json;
    json << "{\"ok\":true,\"version\":\"0.0.55\",\"count\":" << videos.size() << ",\"items\":[";
    for (std::size_t i = 0; i < videos.size(); ++i) {
        const auto& video = videos[i];
        if (i != 0U) json << ',';
        json << "{\"id\":\"" << json_escape(video.id)
             << "\",\"name\":\"" << json_escape(video.name)
             << "\",\"type\":\"" << json_escape(video.type)
             << "\",\"year\":" << video.production_year
             << ",\"directPreferred\":" << (browser_direct_preferred(video.path) ? "true" : "false")
             << '}';
    }
    json << "]}";
    return send_response(client, 200, "OK", "application/json; charset=utf-8", json.str(), head_only);
}

bool send_direct_media(int client,
                       const std::shared_ptr<SharedState>& state,
                       const std::string& id,
                       const std::string& range_header,
                       bool head_only) {
    reddmedia::LibraryVideo video;
    std::string error;
    if (!resolve_catalog_item(state, id, video, error))
        return send_json_error(client, 404, "Not Found", error);

    struct stat info{};
    if (!regular_file(video.path, info))
        return send_json_error(client, 404, "Not Found", "The selected media file is unavailable.");

    const ByteRange range = parse_range(range_header, info.st_size);
    if (range.requested && !range.valid) {
        return send_response(client, 416, "Range Not Satisfiable", "text/plain; charset=utf-8", {}, head_only,
                             {{"Content-Range", "bytes */" + std::to_string(static_cast<long long>(info.st_size))}});
    }

    const off_t first = range.valid ? range.first : 0;
    const off_t last = range.valid ? range.last : info.st_size - 1;
    const unsigned long long content_length = info.st_size > 0
        ? static_cast<unsigned long long>(last - first + 1)
        : 0ULL;

    std::ostringstream header;
    header << "HTTP/1.1 " << (range.valid ? 206 : 200) << (range.valid ? " Partial Content\r\n" : " OK\r\n")
           << "Server: Nougat/0.0.55\r\n"
           << "Content-Type: " << media_content_type(video.path) << "\r\n"
           << "Content-Length: " << content_length << "\r\n"
           << "Accept-Ranges: bytes\r\n"
           << "Cache-Control: private, max-age=0\r\n"
           << "X-Content-Type-Options: nosniff\r\n";
    if (range.valid) {
        header << "Content-Range: bytes " << static_cast<long long>(first) << '-'
               << static_cast<long long>(last) << '/'
               << static_cast<long long>(info.st_size) << "\r\n";
    }
    header << "Connection: close\r\n\r\n";
    if (!send_all(client, header.str()) || head_only || content_length == 0ULL) return true;

    const int file = open(video.path.c_str(), O_RDONLY);
    if (file < 0) return false;
    if (lseek(file, first, SEEK_SET) < 0) {
        close(file);
        return false;
    }

    unsigned long long remaining = content_length;
    char buffer[128U * 1024U];
    bool ok = true;
    while (remaining > 0ULL && !state->stopping.load()) {
        const std::size_t wanted = static_cast<std::size_t>(
            std::min<unsigned long long>(remaining, sizeof(buffer)));
        const ssize_t amount = read(file, buffer, wanted);
        if (amount <= 0) {
            ok = amount == 0;
            break;
        }
        if (!send_all(client, buffer, static_cast<std::size_t>(amount))) {
            ok = false;
            break;
        }
        remaining -= static_cast<unsigned long long>(amount);
    }
    close(file);
    return ok && remaining == 0ULL;
}

bool send_transcoded_media(int client,
                           const std::shared_ptr<SharedState>& state,
                           const std::string& id,
                           bool head_only) {
    reddmedia::LibraryVideo video;
    std::string error;
    if (!resolve_catalog_item(state, id, video, error))
        return send_json_error(client, 404, "Not Found", error);

    struct stat info{};
    if (!regular_file(video.path, info))
        return send_json_error(client, 404, "Not Found", "The selected media file is unavailable.");
    if (access("/usr/bin/ffmpeg", X_OK) != 0)
        return send_json_error(client, 503, "Service Unavailable",
                               "FFmpeg is unavailable, so browser compatibility mode cannot start.");

    if (head_only) {
        return send_response(client, 200, "OK", "video/mp4", {}, true,
                             {{"X-Nougat-Stream-Mode", "ffmpeg-fragmented-mp4"}});
    }

    int output_pipe[2]{};
    if (pipe(output_pipe) != 0)
        return send_json_error(client, 500, "Internal Server Error", "Nougat could not create the browser stream pipe.");

    const pid_t child = fork();
    if (child < 0) {
        close(output_pipe[0]);
        close(output_pipe[1]);
        return send_json_error(client, 500, "Internal Server Error", "Nougat could not start FFmpeg.");
    }
    if (child == 0) {
        setpgid(0, 0);
        close(output_pipe[0]);
        dup2(output_pipe[1], STDOUT_FILENO);
        if (output_pipe[1] != STDOUT_FILENO) close(output_pipe[1]);
        const int null_fd = open("/dev/null", O_RDWR);
        if (null_fd >= 0) {
            dup2(null_fd, STDIN_FILENO);
            dup2(null_fd, STDERR_FILENO);
            if (null_fd > STDERR_FILENO) close(null_fd);
        }
        execl("/usr/bin/ffmpeg", "ffmpeg",
              "-nostdin", "-hide_banner", "-loglevel", "error",
              "-i", video.path.c_str(),
              "-map", "0:v:0?", "-map", "0:a:0?", "-sn",
              "-c:v", "libx264", "-preset", "veryfast", "-crf", "23",
              "-pix_fmt", "yuv420p",
              "-c:a", "aac", "-b:a", "160k",
              "-movflags", "+frag_keyframe+empty_moov+default_base_moof",
              "-f", "mp4", "pipe:1",
              static_cast<char*>(nullptr));
        _exit(127);
    }

    setpgid(child, child);
    close(output_pipe[1]);
    const std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Server: Nougat/0.0.55\r\n"
        "Content-Type: video/mp4\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Cache-Control: no-store\r\n"
        "X-Nougat-Stream-Mode: ffmpeg-fragmented-mp4\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n";
    bool ok = send_all(client, header);
    char buffer[64U * 1024U];
    while (ok && !state->stopping.load()) {
        const ssize_t amount = read(output_pipe[0], buffer, sizeof(buffer));
        if (amount == 0) break;
        if (amount < 0) {
            if (errno == EINTR) continue;
            ok = false;
            break;
        }
        std::ostringstream prefix;
        prefix << std::hex << static_cast<unsigned long long>(amount) << "\r\n";
        ok = send_all(client, prefix.str()) &&
             send_all(client, buffer, static_cast<std::size_t>(amount)) &&
             send_all(client, "\r\n", 2U);
    }
    close(output_pipe[0]);
    if (!ok || state->stopping.load()) {
        if (kill(-child, SIGTERM) != 0 && errno != ESRCH) kill(child, SIGTERM);
    }
    int status = 0;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    if (ok) ok = send_all(client, "0\r\n\r\n", 5U);
    return ok;
}

std::string request_header_value(const std::string& request, const std::string& wanted) {
    const std::string lower_wanted = lower_ascii(wanted);
    std::size_t line_start = request.find("\r\n");
    if (line_start == std::string::npos) return {};
    line_start += 2U;
    while (line_start < request.size()) {
        const std::size_t line_end = request.find("\r\n", line_start);
        if (line_end == std::string::npos || line_end == line_start) break;
        const std::string line = request.substr(line_start, line_end - line_start);
        const std::size_t colon = line.find(':');
        if (colon != std::string::npos && lower_ascii(line.substr(0, colon)) == lower_wanted) {
            std::size_t value_start = colon + 1U;
            while (value_start < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[value_start])) != 0) ++value_start;
            return line.substr(value_start);
        }
        line_start = line_end + 2U;
    }
    return {};
}

void handle_client(int client, const std::shared_ptr<SharedState>& state) {
    struct ActiveClientGuard {
        std::shared_ptr<SharedState> state;
        ~ActiveClientGuard() { state->active_clients.fetch_sub(1); }
    } guard{state};

    timeval receive_timeout{5, 0};
    timeval send_timeout{30, 0};
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));
    setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    std::string request;
    char buffer[4096];
    while (request.size() < kMaxRequestBytes && request.find("\r\n\r\n") == std::string::npos) {
        const ssize_t amount = recv(client, buffer, sizeof(buffer), 0);
        if (amount <= 0) break;
        request.append(buffer, static_cast<std::size_t>(amount));
    }
    if (request.find("\r\n\r\n") == std::string::npos) {
        send_json_error(client, 400, "Bad Request", "The HTTP request was incomplete.");
        close(client);
        return;
    }

    const std::size_t line_end = request.find("\r\n");
    std::istringstream first_line(request.substr(0, line_end));
    std::string method;
    std::string target;
    std::string protocol;
    first_line >> method >> target >> protocol;
    const bool head_only = method == "HEAD";
    if ((method != "GET" && !head_only) || target.empty() || protocol.rfind("HTTP/", 0U) != 0U) {
        send_json_error(client, 405, "Method Not Allowed", "Nougat Web Player accepts GET and HEAD requests only.");
        close(client);
        return;
    }

    std::string path = target;
    std::string query;
    const std::size_t question = target.find('?');
    if (question != std::string::npos) {
        path = target.substr(0, question);
        query = target.substr(question + 1U);
    }
    const auto query_map = query_values(query);

    if (path == "/nougat/v1/health") {
        send_response(client, 200, "OK", "application/json; charset=utf-8",
                      "{\"ok\":true,\"product\":\"Nougat Media Suite\",\"version\":\"0.0.55\",\"service\":\"LAN Web Player\",\"lanOnly\":true}",
                      head_only);
    } else if (path == "/nougat/v1/catalog") {
        send_catalog(client, state, head_only);
    } else if (path == "/nougat/v1/media") {
        const auto id = query_map.find("id");
        send_direct_media(client, state, id == query_map.end() ? std::string{} : id->second,
                          request_header_value(request, "range"), head_only);
    } else if (path == "/nougat/v1/transcode") {
        const auto id = query_map.find("id");
        send_transcoded_media(client, state, id == query_map.end() ? std::string{} : id->second, head_only);
    } else if (path == "/robots.txt") {
        send_response(client, 200, "OK", "text/plain; charset=utf-8", "User-agent: *\nDisallow: /\n", head_only);
    } else {
        std::string asset;
        if (path == "/" || path == "/index.html") asset = "index.html";
        else if (path == "/app.js") asset = "app.js";
        else if (path == "/styles.css") asset = "styles.css";
        if (asset.empty()) {
            send_json_error(client, 404, "Not Found", "That Nougat Web Player route does not exist.");
        } else {
            const std::string disk_path = state->application_dir + "/components/web_player/" + asset;
            const std::string body = read_file(disk_path);
            if (body.empty())
                send_json_error(client, 503, "Service Unavailable", "The Nougat Web Player assets are missing.");
            else
                send_response(client, 200, "OK", static_content_type(asset), body, head_only,
                              {{"Content-Security-Policy",
                                "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; media-src 'self' blob:; connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'"}});
        }
    }
    close(client);
}

std::vector<std::string> local_private_ipv4_addresses() {
    std::set<std::string> addresses;
    addresses.insert("127.0.0.1");
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0) return {addresses.begin(), addresses.end()};
    for (ifaddrs* current = interfaces; current; current = current->ifa_next) {
        if (!current->ifa_addr || current->ifa_addr->sa_family != AF_INET) continue;
        const auto* address = reinterpret_cast<const sockaddr_in*>(current->ifa_addr);
        char text[INET_ADDRSTRLEN]{};
        if (!inet_ntop(AF_INET, &address->sin_addr, text, sizeof(text))) continue;
        const std::string value(text);
        if (LanMediaService::is_private_lan_address(value)) addresses.insert(value);
    }
    freeifaddrs(interfaces);
    return {addresses.begin(), addresses.end()};
}

}  // namespace

struct LanMediaService::Impl {
    std::shared_ptr<SharedState> state;
    std::vector<int> listeners;
    std::thread worker;

    void shutdown() {
        if (state) state->stopping.store(true);
        for (const int listener : listeners) {
            ::shutdown(listener, SHUT_RDWR);
            close(listener);
        }
        listeners.clear();
        if (worker.joinable()) worker.join();
        state.reset();
    }

    ~Impl() { shutdown(); }
};

LanMediaService::LanMediaService() : impl_(std::make_unique<Impl>()) {
    status_.endpoints = {
        {"health", "/nougat/v1/health", "LAN service health and version contract"},
        {"catalog", "/nougat/v1/catalog", "Local playable Library catalog"},
        {"history", "/nougat/v1/history", "Reserved local viewing history and exact resume-state contract"},
        {"artwork", "/nougat/v1/artwork", "Reserved local artwork contract"},
        {"media", "/nougat/v1/media", "Direct byte-range delivery of locally owned media"},
        {"transcode", "/nougat/v1/transcode", "FFmpeg browser-compatibility stream for unsupported media"},
        {"hls", "/nougat/v1/hls", "Reserved versioned HLS surface"},
        {"livetv", "/nougat/v1/live-tv", "Reserved local Live TV channel, guide and stream contract"},
        {"devices", "/nougat/v1/devices", "Reserved LAN device/session inventory"},
        {"session", "/nougat/v1/session", "Reserved versioned playback/session state"},
        {"pair", "/nougat/v1/pair", "Reserved local pairing/PIN authentication surface"},
        {"web", "/", "Nougat first-party phone/tablet/laptop/TV browser player"},
    };
}

LanMediaService::~LanMediaService() = default;

void LanMediaService::prepare() {
    if (!impl_) impl_ = std::make_unique<Impl>();
    if (!impl_->listeners.empty()) return;

    status_.prepared = true;
    status_.serving = false;
    status_.access_urls.clear();
    status_.message.clear();
    impl_->state = std::make_shared<SharedState>(executable_directory());

    for (const std::string& address_text : local_private_ipv4_addresses()) {
        const int listener = socket(AF_INET, SOCK_STREAM, 0);
        if (listener < 0) continue;
        int enabled = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(status_.preferred_port);
        if (inet_pton(AF_INET, address_text.c_str(), &address.sin_addr) != 1 ||
            bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
            listen(listener, 16) != 0) {
            close(listener);
            continue;
        }
        impl_->listeners.push_back(listener);
        status_.access_urls.push_back(
            "http://" + address_text + ":" + std::to_string(status_.preferred_port));
    }

    if (impl_->listeners.empty()) {
        status_.message = "Nougat Web Player could not bind LAN port " +
            std::to_string(status_.preferred_port) + ".";
        impl_->state.reset();
        return;
    }

    status_.serving = true;
    status_.message = "Nougat Web Player is available on the private LAN.";
    const std::shared_ptr<SharedState> state = impl_->state;
    const std::vector<int> listeners = impl_->listeners;
    impl_->worker = std::thread([state, listeners]() {
        while (!state->stopping.load()) {
            fd_set read_set;
            FD_ZERO(&read_set);
            int highest = -1;
            for (const int listener : listeners) {
                if (listener < 0) continue;
                FD_SET(listener, &read_set);
                if (listener > highest) highest = listener;
            }
            if (highest < 0) break;
            timeval timeout{0, 250000};
            const int ready = select(highest + 1, &read_set, nullptr, nullptr, &timeout);
            if (ready < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (ready == 0) continue;
            for (const int listener : listeners) {
                if (listener < 0 || !FD_ISSET(listener, &read_set)) continue;
                sockaddr_in peer{};
                socklen_t peer_length = sizeof(peer);
                const int client = accept(listener, reinterpret_cast<sockaddr*>(&peer), &peer_length);
                if (client < 0) continue;
                char text[INET_ADDRSTRLEN]{};
                const std::string peer_address = inet_ntop(AF_INET, &peer.sin_addr, text, sizeof(text))
                    ? std::string(text) : std::string();
                if (!LanMediaService::is_private_lan_address(peer_address)) {
                    send_json_error(client, 403, "Forbidden", "Nougat Web Player accepts private-LAN clients only.");
                    close(client);
                    continue;
                }
                const int active = state->active_clients.fetch_add(1) + 1;
                if (active > kMaxConcurrentClients) {
                    state->active_clients.fetch_sub(1);
                    send_json_error(client, 503, "Service Unavailable", "Nougat Web Player is busy. Try again in a moment.");
                    close(client);
                    continue;
                }
                std::thread(handle_client, client, state).detach();
            }
        }
    });
}

void LanMediaService::stop() {
    if (impl_) impl_->shutdown();
    status_.serving = false;
    if (status_.prepared) status_.message = "Nougat Web Player stopped.";
}

bool LanMediaService::is_private_lan_address(const std::string& address) {
    if (address == "127.0.0.1" || address == "::1") return true;
    if (address.rfind("10.", 0U) == 0U) return true;
    if (address.rfind("192.168.", 0U) == 0U) return true;
    if (address.rfind("169.254.", 0U) == 0U) return true;
    if (address.rfind("fc", 0U) == 0U || address.rfind("fd", 0U) == 0U ||
        address.rfind("fe80:", 0U) == 0U) return true;
    if (address.rfind("172.", 0U) == 0U) {
        const std::size_t second_dot = address.find('.', 4U);
        if (second_dot != std::string::npos) {
            const int second = std::atoi(address.substr(4U, second_dot - 4U).c_str());
            return second >= 16 && second <= 31;
        }
    }
    return false;
}

}  // namespace reddmedia::lan
