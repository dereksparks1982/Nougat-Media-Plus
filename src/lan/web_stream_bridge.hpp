#pragma once

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace reddmedia::lan::web_stream_bridge {
namespace detail {

inline bool send_all(int fd, const char* data, std::size_t size) {
    std::size_t sent = 0;
    while (sent < size) {
        const ssize_t amount = send(fd, data + sent, size - sent, MSG_NOSIGNAL);
        if (amount <= 0) return false;
        sent += static_cast<std::size_t>(amount);
    }
    return true;
}

inline bool send_all(int fd, const std::string& data) {
    return send_all(fd, data.data(), data.size());
}

inline std::string json_escape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 16U);
    for (const unsigned char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20U) out += ' ';
            else out.push_back(static_cast<char>(c));
            break;
        }
    }
    return out;
}

inline bool send_json(int client, int status, const char* reason,
                      const std::string& body, bool head_only = false) {
    std::ostringstream header;
    header << "HTTP/1.1 " << status << ' ' << reason << "\r\n"
           << "Server: Nougat/0.0.68\r\n"
           << "Content-Type: application/json; charset=utf-8\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Cache-Control: no-store\r\n"
           << "X-Content-Type-Options: nosniff\r\n"
           << "Connection: close\r\n\r\n";
    if (!send_all(client, header.str())) return false;
    return head_only || body.empty() || send_all(client, body);
}

inline bool send_error(int client, int status, const char* reason,
                       const std::string& message, bool head_only = false) {
    return send_json(client, status, reason,
                     "{\"ok\":false,\"error\":\"" + json_escape(message) + "\"}",
                     head_only);
}

inline std::string percent_encode(const std::string& value) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size() * 3U);
    for (const unsigned char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[(c >> 4U) & 0x0fU]);
            out.push_back(hex[c & 0x0fU]);
        }
    }
    return out;
}

inline bool valid_source_url(const std::string& value) {
    if (value.empty() || value.size() > 4096U) return false;
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower.rfind("https://", 0U) == 0U || lower.rfind("http://", 0U) == 0U;
}

inline bool youtube_url(const std::string& source) {
    std::string lower = source;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower.find("youtube.com/") != std::string::npos ||
           lower.find("youtu.be/") != std::string::npos;
}

inline std::string find_executable_in_path(const std::string& name) {
    const char* raw = std::getenv("PATH");
    if (!raw || name.empty()) return {};
    std::istringstream paths(raw);
    std::string directory;
    while (std::getline(paths, directory, ':')) {
        if (directory.empty()) directory = ".";
        const std::string candidate = directory + "/" + name;
        if (access(candidate.c_str(), X_OK) == 0) return candidate;
    }
    return {};
}

inline bool supported_node_runtime_present() {
    FILE* pipe = popen("node --version 2>/dev/null", "r");
    if (!pipe) return false;
    char buffer[64]{};
    const bool read_ok = std::fgets(buffer, sizeof(buffer), pipe) != nullptr;
    pclose(pipe);
    if (!read_ok) return false;
    int major = 0;
    return std::sscanf(buffer, "v%d", &major) == 1 && major >= 22;
}

inline void append_youtube_compatibility_args(std::vector<std::string>& args,
                                               const std::string& source) {
    if (!youtube_url(source)) return;
    const std::string deno = find_executable_in_path("deno");
    const std::string node = find_executable_in_path("node");
    const std::string qjs = find_executable_in_path("qjs");
    if (!deno.empty()) {
        args.push_back("--js-runtimes");
        args.push_back("deno:" + deno);
    } else if (!node.empty() && supported_node_runtime_present()) {
        args.push_back("--js-runtimes");
        args.push_back("node:" + node);
    } else if (!qjs.empty()) {
        args.push_back("--js-runtimes");
        args.push_back("quickjs:" + qjs);
    }
    args.push_back("--extractor-args");
    args.push_back("youtube:player_client=default,web_safari");
}

inline void close_if_open(int fd) {
    if (fd >= 0) close(fd);
}

inline void terminate_group(pid_t pid) {
    if (pid <= 0) return;
    if (kill(-pid, SIGTERM) != 0 && errno != ESRCH) kill(pid, SIGTERM);
}

inline void wait_child(pid_t pid) {
    if (pid <= 0) return;
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
}

inline bool send_chunk(int client, const char* data, std::size_t size) {
    std::ostringstream prefix;
    prefix << std::hex << static_cast<unsigned long long>(size) << "\r\n";
    return send_all(client, prefix.str()) &&
           send_all(client, data, size) &&
           send_all(client, "\r\n", 2U);
}

inline bool play(int client, const std::string& application_dir,
                 const std::atomic<bool>& stopping, const std::string& source_url,
                 bool head_only) {
    if (!valid_source_url(source_url))
        return send_error(client, 400, "Bad Request", "Stream URL must use HTTP or HTTPS.", head_only);

    const std::string engine = application_dir + "/tools/yt-dlp/yt-dlp";
    if (access(engine.c_str(), X_OK) != 0)
        return send_error(client, 503, "Service Unavailable", "Stream engine (yt-dlp) is missing.", head_only);
    if (access("/usr/bin/ffmpeg", X_OK) != 0)
        return send_error(client, 503, "Service Unavailable", "FFmpeg is unavailable for browser stream compatibility.", head_only);

    if (head_only) {
        const std::string header =
            "HTTP/1.1 200 OK\r\n"
            "Server: Nougat/0.0.68\r\n"
            "Content-Type: video/mp4\r\n"
            "Cache-Control: no-store\r\n"
            "X-Nougat-Stream-Mode: yt-dlp-browser-bridge\r\n"
            "Connection: close\r\n\r\n";
        return send_all(client, header);
    }

    int source_pipe[2]{-1, -1};
    int output_pipe[2]{-1, -1};
    if (pipe(source_pipe) != 0 || pipe(output_pipe) != 0) {
        close_if_open(source_pipe[0]); close_if_open(source_pipe[1]);
        close_if_open(output_pipe[0]); close_if_open(output_pipe[1]);
        return send_error(client, 500, "Internal Server Error", "Nougat could not create the Stream bridge pipes.");
    }

    std::vector<std::string> args = {engine, "--ignore-config", "--no-playlist"};
    append_youtube_compatibility_args(args, source_url);
    args.push_back("-f");
    args.push_back(youtube_url(source_url)
        ? "b[protocol^=m3u8][height<=1080]/b[height<=1080]/bv*[height<=1080]+ba/b"
        : "bv*[height<=1080]+ba/b[height<=1080]");
    args.push_back("-o");
    args.push_back("-");
    args.push_back(source_url);

    const pid_t extractor = fork();
    if (extractor == 0) {
        setpgid(0, 0);
        dup2(source_pipe[1], STDOUT_FILENO);
        const int null_fd = open("/dev/null", O_RDWR);
        if (null_fd >= 0) {
            dup2(null_fd, STDIN_FILENO);
            dup2(null_fd, STDERR_FILENO);
            if (null_fd > STDERR_FILENO) close(null_fd);
        }
        close(source_pipe[0]); close(source_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        std::vector<char*> argv;
        argv.reserve(args.size() + 1U);
        for (std::string& arg : args) argv.push_back(arg.data());
        argv.push_back(nullptr);
        execv(engine.c_str(), argv.data());
        _exit(127);
    }
    if (extractor < 0) {
        close_if_open(source_pipe[0]); close_if_open(source_pipe[1]);
        close_if_open(output_pipe[0]); close_if_open(output_pipe[1]);
        return send_error(client, 500, "Internal Server Error", "Nougat could not start the Stream extractor.");
    }
    setpgid(extractor, extractor);

    const pid_t transcoder = fork();
    if (transcoder == 0) {
        setpgid(0, 0);
        dup2(source_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        const int null_fd = open("/dev/null", O_WRONLY);
        if (null_fd >= 0) {
            dup2(null_fd, STDERR_FILENO);
            if (null_fd > STDERR_FILENO) close(null_fd);
        }
        close(source_pipe[0]); close(source_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        execl("/usr/bin/ffmpeg", "ffmpeg",
              "-nostdin", "-hide_banner", "-loglevel", "error",
              "-i", "pipe:0",
              "-map", "0:v:0?", "-map", "0:a:0?", "-sn",
              "-c:v", "libx264", "-preset", "veryfast", "-crf", "23",
              "-pix_fmt", "yuv420p",
              "-c:a", "aac", "-b:a", "160k",
              "-movflags", "+frag_keyframe+empty_moov+default_base_moof",
              "-f", "mp4", "pipe:1",
              static_cast<char*>(nullptr));
        _exit(127);
    }
    if (transcoder < 0) {
        terminate_group(extractor);
        close_if_open(source_pipe[0]); close_if_open(source_pipe[1]);
        close_if_open(output_pipe[0]); close_if_open(output_pipe[1]);
        wait_child(extractor);
        return send_error(client, 500, "Internal Server Error", "Nougat could not start FFmpeg for the Stream bridge.");
    }
    setpgid(transcoder, transcoder);

    close(source_pipe[0]); close(source_pipe[1]);
    close(output_pipe[1]);

    char buffer[64U * 1024U];
    ssize_t first = -1;
    do {
        first = read(output_pipe[0], buffer, sizeof(buffer));
    } while (first < 0 && errno == EINTR);

    if (first <= 0) {
        terminate_group(extractor);
        terminate_group(transcoder);
        close(output_pipe[0]);
        wait_child(extractor);
        wait_child(transcoder);
        return send_error(client, 502, "Bad Gateway", "The Stream engine did not produce browser-playable media.");
    }

    const std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Server: Nougat/0.0.68\r\n"
        "Content-Type: video/mp4\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Cache-Control: no-store\r\n"
        "X-Nougat-Stream-Mode: yt-dlp-browser-bridge\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n";

    bool ok = send_all(client, header) && send_chunk(client, buffer, static_cast<std::size_t>(first));
    while (ok && !stopping.load()) {
        const ssize_t amount = read(output_pipe[0], buffer, sizeof(buffer));
        if (amount == 0) break;
        if (amount < 0) {
            if (errno == EINTR) continue;
            ok = false;
            break;
        }
        ok = send_chunk(client, buffer, static_cast<std::size_t>(amount));
    }
    close(output_pipe[0]);

    if (!ok || stopping.load()) {
        terminate_group(extractor);
        terminate_group(transcoder);
    }
    wait_child(extractor);
    wait_child(transcoder);
    if (ok) ok = send_all(client, "0\r\n\r\n", 5U);
    return ok;
}

}  // namespace detail

inline bool handle_request(int client, const std::string& application_dir,
                           const std::atomic<bool>& stopping,
                           const std::string& action, const std::string& source_url,
                           bool head_only) {
    if (!detail::valid_source_url(source_url))
        return detail::send_error(client, 400, "Bad Request", "Stream URL must use HTTP or HTTPS.", head_only);

    const std::string engine = application_dir + "/tools/yt-dlp/yt-dlp";
    if (access(engine.c_str(), X_OK) != 0)
        return detail::send_error(client, 503, "Service Unavailable", "Stream engine (yt-dlp) is missing.", head_only);

    if (action == "resolve") {
        const std::string route = "/nougat/v1/stream?action=play&url=" + detail::percent_encode(source_url);
        return detail::send_json(client, 200, "OK",
            "{\"ok\":true,\"url\":\"" + detail::json_escape(route) +
            "\",\"status\":\"Resolved by Nougat yt-dlp browser bridge.\"}", head_only);
    }
    if (action == "play")
        return detail::play(client, application_dir, stopping, source_url, head_only);

    return detail::send_error(client, 400, "Bad Request", "Unsupported Stream action.", head_only);
}

}  // namespace reddmedia::lan::web_stream_bridge
