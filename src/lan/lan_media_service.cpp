#include "lan_media_service.hpp"
#include "web_stream_bridge.hpp"
#include "../media_server/jellyfin_api_client.hpp"

#include <arpa/inet.h>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <poll.h>
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
#include <array>
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
    if (path.size() >= 3U && path.substr(path.size() - 3U) == ".md")
        return "text/markdown; charset=utf-8";
    if (path.size() >= 4U && path.substr(path.size() - 4U) == ".png")
        return "image/png";
    if (path.size() >= 12U && path.substr(path.size() - 12U) == ".webmanifest")
        return "application/manifest+json";
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
        if (amount < 0 && errno == EINTR) continue;
        if (amount <= 0) return false;
        sent += static_cast<std::size_t>(amount);
    }
    return true;
}

bool pipe_readable_before(int fd, const std::atomic<bool>& stopping,
                          std::chrono::milliseconds duration) {
    const auto deadline = std::chrono::steady_clock::now() + duration;
    while (!stopping.load() && std::chrono::steady_clock::now() < deadline) {
        pollfd watcher{};
        watcher.fd = fd;
        watcher.events = POLLIN | POLLHUP | POLLERR;
        const int ready = poll(&watcher, 1, 500);
        if (ready > 0) return true;
        if (ready < 0 && errno != EINTR) return false;
    }
    return false;
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
    if (dash == std::string::npos || spec.find('-', dash + 1U) != std::string::npos) return range;
    // Do not accept values like "bytes=5junk-"; stoll permits trailing junk.
    const auto positive_integer = [](const std::string& text, unsigned long long& result) {
        if (text.empty() || !std::all_of(text.begin(), text.end(),
                [](unsigned char c) { return c >= '0' && c <= '9'; })) return false;
        try {
            std::size_t position = 0;
            result = std::stoull(text, &position);
            return position == text.size();
        } catch (...) {
            return false;
        }
    };
    const std::string start_text = spec.substr(0, dash);
    const std::string stop_text = spec.substr(dash + 1U);
    const auto size_unsigned = static_cast<unsigned long long>(size);
    unsigned long long start_value = 0;
    unsigned long long stop_value = 0;
    if (start_text.empty()) {
        if (!positive_integer(stop_text, stop_value) || stop_value == 0ULL) return range;
        range.first = static_cast<off_t>(size_unsigned - std::min(stop_value, size_unsigned));
        range.last = size - 1;
    } else {
        if (!positive_integer(start_text, start_value) || start_value >= size_unsigned) return range;
        range.first = static_cast<off_t>(start_value);
        if (stop_text.empty()) {
            range.last = size - 1;
        } else {
            if (!positive_integer(stop_text, stop_value) || stop_value < start_value) return range;
            range.last = static_cast<off_t>(std::min(stop_value, size_unsigned - 1ULL));
        }
    }
    range.valid = range.first >= 0 && range.last >= range.first && range.last < size;
    return range;
}

struct SharedState {
    explicit SharedState(std::string root) : application_dir(std::move(root)) {}

    std::string application_dir;
    std::atomic<bool> stopping{false};
    std::atomic<int> active_clients{0};
    std::mutex clients_mutex;
    std::condition_variable clients_drained;
    std::set<int> open_clients;
    std::mutex terms_mutex;
    std::set<std::string> accepted_sessions;
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

    std::ostringstream items;
    std::size_t ready_count = 0;
    for (std::size_t i = 0; i < videos.size(); ++i) {
        const auto& video = videos[i];
        struct stat info{};
        const bool ready = regular_file(video.path, info);
        if (ready) ++ready_count;
        if (i != 0U) items << ',';
        items << "{\"id\":\"" << json_escape(video.id)
              << "\",\"name\":\"" << json_escape(video.name)
              << "\",\"type\":\"" << json_escape(video.type)
              << "\",\"year\":" << video.production_year
              << ",\"ready\":" << (ready ? "true" : "false")
              << ",\"directPreferred\":" << (browser_direct_preferred(video.path) ? "true" : "false")
              << '}';
    }
    std::ostringstream json;
    json << "{\"ok\":true,\"version\":\"0.0.55\",\"count\":" << videos.size()
         << ",\"available\":" << ready_count << ",\"items\":[" << items.str() << "]}";
    return send_response(client, 200, "OK", "application/json; charset=utf-8", json.str(), head_only);
}

bool send_artwork(int client,
                  const std::shared_ptr<SharedState>& state,
                  const std::string& id,
                  bool head_only) {
    if (id.empty())
        return send_json_error(client, 400, "Bad Request", "Missing media id.");
    // Artwork must belong to an indexed library item, just like direct media.
    reddmedia::LibraryVideo indexed;
    std::string error;
    if (!resolve_catalog_item(state, id, indexed, error))
        return send_json_error(client, 404, "Not Found", "That media item is not in the Nougat library.");
    std::string bytes;
    {
        std::lock_guard<std::mutex> lock(state->catalog_mutex);
        if (!state->jellyfin.load_primary_image_bmp(id, std::string{}, 360, 540, bytes, error)) {
            return send_json_error(client, 404, "Not Found",
                                   error.empty() ? "No poster is available for this media item." : error);
        }
    }
    return send_response(client, 200, "OK", "image/bmp", bytes, head_only,
                         {{"Cache-Control", "private, max-age=86400"},
                          {"X-Nougat-Artwork", "jellyfin-primary"}});
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

    // Open once and read from the SAME descriptor that was sized. This avoids
    // sending a successful response before discovering that the file vanished.
    const int file = open(video.path.c_str(), O_RDONLY | O_CLOEXEC);
    if (file < 0)
        return send_json_error(client, 404, "Not Found", "The selected media file is unavailable.");
    struct stat info{};
    if (fstat(file, &info) != 0 || !S_ISREG(info.st_mode)) {
        close(file);
        return send_json_error(client, 404, "Not Found", "The selected media is not a regular file.");
    }
    const ByteRange range = parse_range(range_header, info.st_size);
    if (range.requested && !range.valid) {
        close(file);
        return send_response(client, 416, "Range Not Satisfiable", "text/plain; charset=utf-8",
                             {}, head_only,
                             {{"Content-Range", "bytes */" + std::to_string(static_cast<long long>(info.st_size))}});
    }
    const off_t first = range.valid ? range.first : 0;
    const off_t last = range.valid ? range.last : info.st_size - 1;
    const unsigned long long length = info.st_size > 0
        ? static_cast<unsigned long long>(last - first + 1)
        : 0ULL;
    if (!head_only && length > 0ULL && lseek(file, first, SEEK_SET) < 0) {
        close(file);
        return send_json_error(client, 503, "Service Unavailable", "Cannot seek the selected media.");
    }

    std::ostringstream header;
    header << "HTTP/1.1 " << (range.valid ? 206 : 200)
           << (range.valid ? " Partial Content\r\n" : " OK\r\n")
           << "Server: Nougat/0.0.55\r\n"
           << "Content-Type: " << media_content_type(video.path) << "\r\n"
           << "Content-Length: " << length << "\r\n"
           << "Accept-Ranges: bytes\r\n"
           << "Cache-Control: private, max-age=0\r\n"
           << "X-Content-Type-Options: nosniff\r\n";
    if (range.valid) {
        header << "Content-Range: bytes " << static_cast<long long>(first) << '-'
               << static_cast<long long>(last) << '/'
               << static_cast<long long>(info.st_size) << "\r\n";
    }
    header << "Connection: close\r\n\r\n";
    const bool header_sent = send_all(client, header.str());
    if (!header_sent || head_only || length == 0ULL) {
        close(file);
        return header_sent;
    }

    unsigned long long remaining = length;
    char buffer[128U * 1024U];
    bool ok = true;
    while (remaining > 0ULL && !state->stopping.load()) {
        const std::size_t wanted = static_cast<std::size_t>(
            std::min<unsigned long long>(remaining, sizeof(buffer)));
        const ssize_t bytes = read(file, buffer, wanted);
        if (bytes < 0 && errno == EINTR) continue;
        if (bytes <= 0) { ok = false; break; }
        if (!send_all(client, buffer, static_cast<std::size_t>(bytes))) {
            ok = false;
            break;
        }
        remaining -= static_cast<unsigned long long>(bytes);
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
    // Do not send HTTP 200 before FFmpeg produces a usable first fragment.
    char buffer[64U * 1024U];
    ssize_t first_amount = -1;
    if (pipe_readable_before(output_pipe[0], state->stopping, std::chrono::seconds(15))) {
        do { first_amount = read(output_pipe[0], buffer, sizeof(buffer)); }
        while (first_amount < 0 && errno == EINTR);
    }
    if (first_amount <= 0) {
        close(output_pipe[0]);
        if (kill(-child, SIGTERM) != 0 && errno != ESRCH) kill(child, SIGTERM);
        int status = 0;
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
        return send_json_error(client, 502, "Bad Gateway",
                               "FFmpeg could not produce browser-playable media.");
    }
    const std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Server: Nougat/0.0.55\r\n"
        "Content-Type: video/mp4\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Cache-Control: no-store\r\n"
        "X-Nougat-Stream-Mode: ffmpeg-fragmented-mp4\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n";
    std::ostringstream first_chunk;
    first_chunk << std::hex << static_cast<unsigned long long>(first_amount) << "\r\n";
    bool ok = send_all(client, header) && send_all(client, first_chunk.str()) &&
              send_all(client, buffer, static_cast<std::size_t>(first_amount)) &&
              send_all(client, "\r\n", 2U);
    int idle_seconds = 0;
    while (ok && !state->stopping.load()) {
        if (!pipe_readable_before(output_pipe[0], state->stopping,
                                  std::chrono::seconds(1))) {
            // An abandoned connection or stalled transcoder must not hold one
            // of the limited LAN worker slots indefinitely.
            if (++idle_seconds >= 60) { ok = false; break; }
            continue;
        }
        idle_seconds = 0;
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
    if (ok && !state->stopping.load()) ok = send_all(client, "0\r\n\r\n", 5U);
    return ok && !state->stopping.load();
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

std::string issue_terms_token() {
    // Non-predictable, process-local cookie. No password, user account, or cloud.
    std::array<unsigned char, 32> bytes{};
    const int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return {};
    std::size_t filled = 0;
    while (filled < bytes.size()) {
        const ssize_t count = read(fd, bytes.data() + filled, bytes.size() - filled);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) break;
        filled += static_cast<std::size_t>(count);
    }
    close(fd);
    if (filled != bytes.size()) return {};
    static constexpr char hex[] = "0123456789abcdef";
    std::string token;
    token.reserve(bytes.size() * 2U);
    for (unsigned char byte : bytes) {
        token.push_back(hex[byte >> 4U]);
        token.push_back(hex[byte & 0x0fU]);
    }
    return token;
}

bool terms_cookie_valid(const std::shared_ptr<SharedState>& state,
                        const std::string& cookie_header) {
    std::size_t start = 0;
    while (start < cookie_header.size()) {
        const std::size_t semicolon = cookie_header.find(';', start);
        std::string cookie = cookie_header.substr(
            start, semicolon == std::string::npos ? std::string::npos : semicolon - start);
        const std::size_t first = cookie.find_first_not_of(" \t");
        if (first != std::string::npos) cookie.erase(0, first);
        constexpr const char* name = "nougat_terms=";
        if (cookie.rfind(name, 0U) == 0U) {
            const std::string token = cookie.substr(std::strlen(name));
            if (token.size() != 64U) return false;
            std::lock_guard<std::mutex> lock(state->terms_mutex);
            return state->accepted_sessions.find(token) != state->accepted_sessions.end();
        }
        if (semicolon == std::string::npos) break;
        start = semicolon + 1U;
    }
    return false;
}

bool issue_terms_session(int client, const std::shared_ptr<SharedState>& state,
                         const std::string& request) {
    // A cross-site browser cannot silently accept terms on the user's behalf.
    // This is an acknowledgement session, NOT strong authentication: an HTTP
    // client can script the endpoint and local LAN traffic is not encrypted.
    const std::string host = request_header_value(request, "host");
    const std::string origin = request_header_value(request, "origin");
    if (host.empty() || origin != "http://" + host)
        return send_json_error(client, 403, "Forbidden",
                               "The terms acknowledgement must originate from this LAN viewer.");
    const std::string token = issue_terms_token();
    if (token.empty())
        return send_json_error(client, 503, "Service Unavailable",
                               "Nougat could not initialize the terms session.");
    {
        std::lock_guard<std::mutex> lock(state->terms_mutex);
        // Limit long-running server memory usage from repeated acknowledgements.
        if (state->accepted_sessions.size() >= 4096U) state->accepted_sessions.clear();
        state->accepted_sessions.insert(token);
    }
    return send_response(client, 200, "OK", "application/json; charset=utf-8",
                         "{\"ok\":true,\"accepted\":true,\"notice\":\"2026-10-30-v2\"}",
                         false, {{"Set-Cookie", "nougat_terms=" + token +
                                          "; Path=/nougat/v1/; HttpOnly; SameSite=Strict; Max-Age=86400"}});
}

void handle_client(int client, const std::shared_ptr<SharedState>& state) {
    struct ActiveClientGuard {
        std::shared_ptr<SharedState> state;
        int client;
        ~ActiveClientGuard() {
            {
                std::lock_guard<std::mutex> lock(state->clients_mutex);
                state->open_clients.erase(client);
                close(client); // Avoid descriptor reuse during shutdown.
                state->active_clients.fetch_sub(1);
            }
            state->clients_drained.notify_all();
        }
    } guard{state, client};

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
        return;
    }

    const std::size_t line_end = request.find("\r\n");
    std::istringstream first_line(request.substr(0, line_end));
    std::string method;
    std::string target;
    std::string protocol;
    first_line >> method >> target >> protocol;
    const bool head_only = method == "HEAD";
    const bool terms_post = method == "POST" && target == "/nougat/v1/terms/accept";
    if ((method != "GET" && !head_only && !terms_post) ||
        target.empty() || protocol.rfind("HTTP/", 0U) != 0U) {
        send_json_error(client, 405, "Method Not Allowed",
                        "Nougat Web Player accepts GET and HEAD, plus POST for terms acknowledgement.");
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

    if (terms_post) {
        issue_terms_session(client, state, request);
    } else if (path == "/nougat/v1/terms/status") {
        const bool valid = terms_cookie_valid(state, request_header_value(request, "cookie"));
        send_response(client, valid ? 200 : 403, valid ? "OK" : "Forbidden",
                      "application/json; charset=utf-8",
                      valid ? "{\"ok\":true,\"accepted\":true,\"notice\":\"2026-10-30-v2\"}"
                            : "{\"ok\":false,\"accepted\":false}",
                      head_only);
    } else if (path.rfind("/nougat/v1/", 0U) == 0U &&
               path != "/nougat/v1/health" &&
               !terms_cookie_valid(state, request_header_value(request, "cookie"))) {
        send_json_error(client, 403, "Forbidden",
                        "Accept the Nougat Terms of Service in the LAN viewer before using media APIs.");
    } else if (path == "/nougat/v1/health") {
        send_response(client, 200, "OK", "application/json; charset=utf-8",
                      "{\"ok\":true,\"product\":\"Nougat Media Suite\",\"version\":\"0.0.55\",\"service\":\"LAN Web Player\",\"lanOnly\":true}",
                      head_only);
    } else if (path == "/nougat/v1/catalog") {
        send_catalog(client, state, head_only);
    } else if (path == "/nougat/v1/artwork") {
        const auto id = query_map.find("id");
        send_artwork(client, state, id == query_map.end() ? std::string{} : id->second, head_only);
    } else if (path == "/nougat/v1/media") {
        const auto id = query_map.find("id");
        send_direct_media(client, state, id == query_map.end() ? std::string{} : id->second,
                          request_header_value(request, "range"), head_only);
    } else if (path == "/nougat/v1/transcode") {
        const auto id = query_map.find("id");
        send_transcoded_media(client, state, id == query_map.end() ? std::string{} : id->second, head_only);
    } else if (path == "/nougat/v1/stream") {
        const auto action = query_map.find("action");
        const auto url = query_map.find("url");
        web_stream_bridge::handle_request(
            client, state->application_dir, state->stopping,
            action == query_map.end() ? std::string{} : action->second,
            url == query_map.end() ? std::string{} : url->second,
            head_only);
    } else if (path == "/robots.txt") {
        send_response(client, 200, "OK", "text/plain; charset=utf-8", "User-agent: *\nDisallow: /\n", head_only);
    } else {
        std::string asset;
        // Explicit allowlist: do not read arbitrary request-selected paths.
        if (path == "/" || path == "/index.html") asset = "index.html";
        else if (path == "/app.js") asset = "app.js";
        else if (path == "/styles.css") asset = "styles.css";
        else if (path == "/config.js") asset = "config.js";
        else if (path == "/legal-notice.js") asset = "legal-notice.js";
        else if (path == "/legal-notice.css") asset = "legal-notice.css";
        else if (path == "/LEGAL_NOTICE.md") asset = "LEGAL_NOTICE.md";
        else if (path == "/sw.js") asset = "sw.js";
        else if (path == "/manifest.webmanifest") asset = "manifest.webmanifest";
        else if (path == "/nougat-icon.png") asset = "nougat-icon.png";
        if (asset.empty()) {
            send_json_error(client, 404, "Not Found", "That Nougat Web Player route does not exist.");
        } else {
            // Use the existing approved Nougat N artwork for the local web icon.
            const std::string disk_path = asset == "nougat-icon.png"
                ? state->application_dir + "/assets/branding/nougat-media-plus-master-N.png"
                : state->application_dir + "/components/web_player/" + asset;
            // Local LAN pages must use their serving host, not the separate
            // GitHub Pages HTTPS bridge configured in the repository file.
            // This preserves offline/WAN-independent playback and matches CSP.
            const std::string body = asset == "config.js"
                ? "window.NOUGAT_WEB_PLAYER = Object.freeze({enabled:true,baseUrl:''});\n"
                : read_file(disk_path);
            if (body.empty())
                send_json_error(client, 503, "Service Unavailable", "The Nougat Web Player assets are missing.");
            else
                send_response(client, 200, "OK", static_content_type(asset), body, head_only,
                              {{"Content-Security-Policy",
                                "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; media-src 'self' blob:; connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'"}});
        }
    }
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
        if (state) {
            state->stopping.store(true);
            std::lock_guard<std::mutex> lock(state->clients_mutex);
            for (int client : state->open_clients) ::shutdown(client, SHUT_RDWR);
        }
        for (const int listener : listeners) {
            ::shutdown(listener, SHUT_RDWR);
            close(listener);
        }
        listeners.clear();
        if (worker.joinable()) worker.join();
        if (state) {
            // Detached handlers keep their shared state alive until completion.
            std::unique_lock<std::mutex> lock(state->clients_mutex);
            state->clients_drained.wait_for(lock, std::chrono::seconds(10),
                [&] { return state->active_clients.load() == 0; });
        }
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
        {"stream", "/nougat/v1/stream", "Standalone yt-dlp engine bridged to browser-compatible playback"},
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
                {
                    std::lock_guard<std::mutex> lock(state->clients_mutex);
                    state->open_clients.insert(client);
                }
                try {
                    std::thread(handle_client, client, state).detach();
                } catch (...) {
                    {
                        std::lock_guard<std::mutex> lock(state->clients_mutex);
                        state->open_clients.erase(client);
                        state->active_clients.fetch_sub(1);
                        close(client);
                    }
                    state->clients_drained.notify_all();
                }
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
    in_addr ipv4{};
    if (inet_pton(AF_INET, address.c_str(), &ipv4) == 1) {
        const std::uint32_t value = ntohl(ipv4.s_addr);
        return (value >> 24U) == 127U ||
               (value >> 24U) == 10U ||
               (value >> 20U) == 0xAC1U ||
               (value >> 16U) == 0xC0A8U ||
               (value >> 16U) == 0xA9FEU;
    }
    in6_addr ipv6{};
    if (inet_pton(AF_INET6, address.c_str(), &ipv6) == 1) {
        const unsigned char* octets = ipv6.s6_addr;
        return IN6_IS_ADDR_LOOPBACK(&ipv6) ||
               (octets[0] & 0xfeU) == 0xfcU ||
               (octets[0] == 0xfeU && (octets[1] & 0xc0U) == 0x80U);
    }
    return false; // Reject DNS names and malformed private-network lookalikes.
}

}  // namespace reddmedia::lan