#include "ytdlp_stream_server.hpp"

#include <arpa/inet.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

bool send_all(int fd, const char* data, std::size_t size) {
    std::size_t sent = 0;
    while (sent < size) {
        const ssize_t n = send(fd, data + sent, size - sent, MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

bool send_all(int fd, const std::string& data) {
    return send_all(fd, data.data(), data.size());
}

bool send_chunk(int fd, const char* data, std::size_t size) {
    std::ostringstream prefix;
    prefix << std::hex << size << "\r\n";
    if (!send_all(fd, prefix.str())) return false;
    if (size > 0 && !send_all(fd, data, size)) return false;
    return send_all(fd, "\r\n", 2);
}

bool finish_chunked(int fd) {
    return send_all(fd, "0\r\n\r\n", 5);
}

std::string section_from_ms(long long ms) {
    if (ms < 0) ms = 0;
    const long long whole = ms / 1000;
    const long long milli = ms % 1000;
    const long long hours = whole / 3600;
    const long long minutes = (whole % 3600) / 60;
    const long long seconds = whole % 60;
    char buffer[80];
    std::snprintf(buffer, sizeof(buffer), "*%02lld:%02lld:%02lld.%03lld-inf",
                  hours, minutes, seconds, milli);
    return buffer;
}

struct RangeRequest {
    bool present = false;
    bool suffix = false;
    std::uint64_t start = 0;
    std::uint64_t end = 0;
    bool has_end = false;
};

bool parse_range(const std::string& request, RangeRequest& out) {
    std::string lower = request;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    const std::string key = "range: bytes=";
    const std::size_t pos = lower.find(key);
    if (pos == std::string::npos) return true;
    const std::size_t begin = pos + key.size();
    const std::size_t line_end = lower.find("\r\n", begin);
    const std::string value = lower.substr(begin, line_end == std::string::npos ? std::string::npos : line_end - begin);
    const std::size_t dash = value.find('-');
    if (dash == std::string::npos) return false;
    out.present = true;
    try {
        if (dash == 0) {
            if (dash + 1 >= value.size()) return false;
            out.suffix = true;
            out.start = static_cast<std::uint64_t>(std::stoull(value.substr(dash + 1)));
            if (out.start == 0) return false;
        } else {
            out.start = static_cast<std::uint64_t>(std::stoull(value.substr(0, dash)));
            if (dash + 1 < value.size()) {
                out.end = static_cast<std::uint64_t>(std::stoull(value.substr(dash + 1)));
                out.has_end = true;
                if (out.end < out.start) return false;
            }
        }
    } catch (...) {
        return false;
    }
    return true;
}

}

YtDlpStreamServer::YtDlpStreamServer() = default;
YtDlpStreamServer::~YtDlpStreamServer() { stop(); }

bool YtDlpStreamServer::start(const std::string& engine, const std::string& source_url,
                              long long start_time_ms, std::string& error) {
    stop();
    engine_ = engine;
    source_url_ = source_url;
    base_time_ms_ = std::max<long long>(0, start_time_ms);
    feeder_failed_ = false;
    {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_buffer_.clear();
    }

    char path[256];
    std::snprintf(path, sizeof(path), "/tmp/reddmedia-ytdlp-%ld-%lld.cache",
                  static_cast<long>(getpid()), base_time_ms_);
    cache_path_ = path;
    unlink(cache_path_.c_str());

    if (!start_http_server(error)) {
        cleanup_cache();
        return false;
    }
    if (!start_feeder(error)) {
        running_ = false;
        if (listen_fd_ >= 0) {
            shutdown(listen_fd_, SHUT_RDWR);
            close(listen_fd_);
            listen_fd_ = -1;
        }
        if (accept_thread_.joinable()) accept_thread_.join();
        port_ = 0;
        cleanup_cache();
        return false;
    }
    return true;
}

bool YtDlpStreamServer::start_http_server(std::string& error) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        error = "Could not create YouTube localhost cache socket.";
        return false;
    }
    int yes = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0);
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        error = std::string("Could not bind YouTube cache server to localhost: ") + std::strerror(errno);
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    if (listen(listen_fd_, 8) != 0) {
        error = "Could not listen on YouTube localhost cache server.";
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    socklen_t length = sizeof(address);
    if (getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        error = "Could not determine YouTube localhost cache port.";
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    port_ = ntohs(address.sin_port);
    running_ = true;
    accept_thread_ = std::thread(&YtDlpStreamServer::accept_loop, this);
    return true;
}

bool YtDlpStreamServer::start_feeder(std::string& error) {
    int log_pipe[2];
    if (pipe(log_pipe) != 0) {
        error = "Could not create yt-dlp cache log pipe.";
        return false;
    }
    const int cache_fd = open(cache_path_.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    if (cache_fd < 0) {
        close(log_pipe[0]);
        close(log_pipe[1]);
        error = "Could not create temporary YouTube cache file.";
        return false;
    }

    std::vector<std::string> args = {
        engine_, "--ignore-config", "--no-playlist", "--downloader", "ffmpeg",
        "-f", "bv*[height<=1080]+ba/b[height<=1080]"
    };
    if (base_time_ms_ > 0) {
        args.push_back("--download-sections");
        args.push_back(section_from_ms(base_time_ms_));
        args.push_back("--force-keyframes-at-cuts");
    }
    args.push_back("-o");
    args.push_back("-");
    args.push_back(source_url_);

    const pid_t pid = fork();
    if (pid == 0) {
        setpgid(0, 0);
        dup2(cache_fd, STDOUT_FILENO);
        dup2(log_pipe[1], STDERR_FILENO);
        close(cache_fd);
        close(log_pipe[0]);
        close(log_pipe[1]);
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (std::string& arg : args) argv.push_back(arg.data());
        argv.push_back(nullptr);
        execv(engine_.c_str(), argv.data());
        _exit(127);
    }

    close(cache_fd);
    close(log_pipe[1]);
    if (pid < 0) {
        close(log_pipe[0]);
        error = "Could not start yt-dlp cache feeder.";
        return false;
    }
    setpgid(pid, pid);
    feeder_pid_ = static_cast<int>(pid);
    feeder_log_fd_ = log_pipe[0];
    fcntl(feeder_log_fd_, F_SETFL, fcntl(feeder_log_fd_, F_GETFL, 0) | O_NONBLOCK);
    feeder_running_ = true;
    return true;
}

void YtDlpStreamServer::append_log(const char* data, std::size_t size) {
    if (size == 0) return;
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_buffer_.append(data, size);
    if (log_buffer_.size() > 24000) log_buffer_.erase(0, log_buffer_.size() - 24000);
}

void YtDlpStreamServer::poll() {
    if (feeder_log_fd_ >= 0) {
        char buffer[4096];
        for (;;) {
            const ssize_t n = read(feeder_log_fd_, buffer, sizeof(buffer));
            if (n > 0) append_log(buffer, static_cast<std::size_t>(n));
            else break;
        }
    }
    if (feeder_pid_ > 0) {
        int status = 0;
        const pid_t result = waitpid(static_cast<pid_t>(feeder_pid_), &status, WNOHANG);
        if (result == feeder_pid_) {
            feeder_running_ = false;
            feeder_failed_ = !(WIFEXITED(status) && WEXITSTATUS(status) == 0);
            feeder_pid_ = -1;
            if (feeder_log_fd_ >= 0) {
                char buffer[4096];
                for (;;) {
                    const ssize_t n = read(feeder_log_fd_, buffer, sizeof(buffer));
                    if (n > 0) append_log(buffer, static_cast<std::size_t>(n));
                    else break;
                }
                close(feeder_log_fd_);
                feeder_log_fd_ = -1;
            }
        }
    }
}

void YtDlpStreamServer::stop_feeder() {
    if (feeder_pid_ > 0) {
        if (kill(-feeder_pid_, SIGTERM) != 0) kill(feeder_pid_, SIGTERM);
        int status = 0;
        bool done = false;
        for (int i = 0; i < 24; ++i) {
            const pid_t result = waitpid(static_cast<pid_t>(feeder_pid_), &status, WNOHANG);
            if (result == feeder_pid_ || result == -1) {
                done = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        if (!done) {
            if (kill(-feeder_pid_, SIGKILL) != 0) kill(feeder_pid_, SIGKILL);
            waitpid(static_cast<pid_t>(feeder_pid_), &status, 0);
        }
        feeder_pid_ = -1;
    }
    feeder_running_ = false;
    if (feeder_log_fd_ >= 0) {
        close(feeder_log_fd_);
        feeder_log_fd_ = -1;
    }
}

void YtDlpStreamServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        shutdown(listen_fd_, SHUT_RDWR);
        close(listen_fd_);
        listen_fd_ = -1;
    }
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        for (int fd : active_clients_) shutdown(fd, SHUT_RDWR);
    }
    stop_feeder();
    if (accept_thread_.joinable()) accept_thread_.join();
    join_workers();
    port_ = 0;
    cleanup_cache();
}

void YtDlpStreamServer::cleanup_cache() {
    if (!cache_path_.empty()) unlink(cache_path_.c_str());
    cache_path_.clear();
}

bool YtDlpStreamServer::wait_for_initial_cache(std::uint64_t minimum_bytes, int timeout_ms,
                                               std::string& error) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        poll();
        const std::uint64_t bytes = cache_bytes();
        if (bytes >= minimum_bytes) return true;
        if (!feeder_running_) {
            if (bytes >= 65536) return true;
            error = feeder_failed_ ? "yt-dlp cache feeder failed before playback could start."
                                   : "yt-dlp cache feeder ended before enough media was buffered.";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (cache_bytes() >= 65536) return true;
    error = "Timed out while buffering the YouTube stream.";
    return false;
}

bool YtDlpStreamServer::running() const { return running_; }
bool YtDlpStreamServer::feeder_running() const { return feeder_running_; }
bool YtDlpStreamServer::failed() const { return feeder_failed_; }
long long YtDlpStreamServer::base_time_ms() const { return base_time_ms_; }
std::uint16_t YtDlpStreamServer::port() const { return port_; }

std::uint64_t YtDlpStreamServer::cache_bytes() const {
    if (cache_path_.empty()) return 0;
    struct stat st{};
    if (stat(cache_path_.c_str(), &st) != 0 || st.st_size < 0) return 0;
    return static_cast<std::uint64_t>(st.st_size);
}

std::string YtDlpStreamServer::url() const {
    if (!running_ || port_ == 0) return {};
    return "http://127.0.0.1:" + std::to_string(port_) + "/stream";
}

std::string YtDlpStreamServer::take_log() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::string out;
    out.swap(log_buffer_);
    return out;
}

void YtDlpStreamServer::accept_loop() {
    while (running_) {
        sockaddr_in client{};
        socklen_t length = sizeof(client);
        const int fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client), &length);
        if (fd < 0) {
            if (!running_) break;
            continue;
        }
        if (ntohl(client.sin_addr.s_addr) != INADDR_LOOPBACK) {
            close(fd);
            continue;
        }
        std::lock_guard<std::mutex> lock(workers_mutex_);
        active_clients_.insert(fd);
        workers_.emplace_back(&YtDlpStreamServer::handle_client, this, fd);
    }
}

void YtDlpStreamServer::unregister_client(int client_fd) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    active_clients_.erase(client_fd);
}

void YtDlpStreamServer::join_workers() {
    std::vector<std::thread> workers;
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        workers.swap(workers_);
    }
    for (std::thread& worker : workers) if (worker.joinable()) worker.join();
}

void YtDlpStreamServer::handle_client(int client_fd) {
    struct ClientGuard {
        YtDlpStreamServer* owner;
        int fd;
        ~ClientGuard() { owner->unregister_client(fd); }
    } guard{this, client_fd};

    timeval recv_timeout{};
    recv_timeout.tv_sec = 5;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
    timeval send_timeout{};
    send_timeout.tv_sec = 15;
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    std::string request;
    char input[4096];
    while (request.find("\r\n\r\n") == std::string::npos && request.size() < 16384) {
        const ssize_t n = recv(client_fd, input, sizeof(input), 0);
        if (n <= 0) { close(client_fd); return; }
        request.append(input, static_cast<std::size_t>(n));
    }
    const bool head_only = request.rfind("HEAD ", 0) == 0;
    if (!head_only && request.rfind("GET ", 0) != 0) {
        send_all(client_fd, "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n");
        close(client_fd);
        return;
    }

    RangeRequest range;
    if (!parse_range(request, range)) {
        send_all(client_fd, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
        close(client_fd);
        return;
    }

    if (head_only) {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\nAccept-Ranges: bytes\r\n"
                 << "Content-Type: application/octet-stream\r\n"
                 << "X-ReddMedia-Cache-Bytes: " << cache_bytes() << "\r\n"
                 << "X-ReddMedia-Growing-Stream: 1\r\n"
                 << "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
        send_all(client_fd, response.str());
        close(client_fd);
        return;
    }

    auto wait_until_available = [&](std::uint64_t start, int loops) {
        std::uint64_t available = cache_bytes();
        for (int i = 0; i < loops && running_ && feeder_running_ && start >= available; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            available = cache_bytes();
        }
        return available;
    };

    auto stream_growing = [&](std::uint64_t start, bool partial) {
        std::uint64_t available = wait_until_available(start, 200);
        if (available == 0 || start >= available) {
            std::ostringstream response;
            response << "HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */"
                     << available << "\r\nConnection: close\r\n\r\n";
            send_all(client_fd, response.str());
            return;
        }

        std::ostringstream response;
        if (partial) {
            constexpr std::uint64_t kLiveRangeEnd = 999999999999ULL;
            const std::uint64_t live_end = start < kLiveRangeEnd ? kLiveRangeEnd : start + 999999999999ULL;
            response << "HTTP/1.1 206 Partial Content\r\n"
                     << "Content-Range: bytes " << start << '-' << live_end << "/*\r\n";
        } else {
            response << "HTTP/1.1 200 OK\r\n";
        }
        response << "Accept-Ranges: bytes\r\n"
                 << "Content-Type: application/octet-stream\r\n"
                 << "Transfer-Encoding: chunked\r\n"
                 << "X-ReddMedia-Growing-Stream: 1\r\n"
                 << "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
        if (!send_all(client_fd, response.str())) return;

        const int file_fd = open(cache_path_.c_str(), O_RDONLY);
        if (file_fd < 0) return;
        std::vector<char> buffer(1024 * 1024);
        std::uint64_t position = start;
        int idle_loops = 0;
        bool socket_ok = true;
        while (running_ && socket_ok) {
            available = cache_bytes();
            if (position < available) {
                const std::size_t wanted = static_cast<std::size_t>(
                    std::min<std::uint64_t>(buffer.size(), available - position));
                const ssize_t n = pread(file_fd, buffer.data(), wanted, static_cast<off_t>(position));
                if (n > 0) {
                    socket_ok = send_chunk(client_fd, buffer.data(), static_cast<std::size_t>(n));
                    position += static_cast<std::uint64_t>(n);
                    idle_loops = 0;
                    continue;
                }
            }
            if (!feeder_running_) break;
            if (++idle_loops > 2400) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        close(file_fd);
        if (socket_ok) finish_chunked(client_fd);
    };

    if (!range.present) {
        stream_growing(0, false);
        close(client_fd);
        return;
    }

    // An open-ended byte request is VLC's normal progressive-playback shape.
    // Treat it as an indeterminate-length aggregating resource instead of
    // freezing the current cache frontier into Content-Length/Content-Range.
    if (!range.suffix && !range.has_end) {
        stream_growing(range.start, true);
        close(client_fd);
        return;
    }

    std::uint64_t available = cache_bytes();
    if (!range.suffix && range.start >= available && feeder_running_) {
        available = wait_until_available(range.start, 80);
    }
    if (available == 0 || (!range.suffix && range.start >= available)) {
        std::ostringstream response;
        response << "HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */"
                 << available << "\r\nConnection: close\r\n\r\n";
        send_all(client_fd, response.str());
        close(client_fd);
        return;
    }

    if (!range.suffix && range.has_end && range.end >= available && feeder_running_) {
        for (int i = 0; i < 100 && running_ && feeder_running_ && range.end >= available; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            available = cache_bytes();
        }
    }

    std::uint64_t start = range.start;
    std::uint64_t end = available - 1;
    if (range.suffix) {
        start = range.start >= available ? 0 : available - range.start;
    } else if (range.has_end) {
        end = std::min(range.end, available - 1);
    }
    if (end < start) {
        close(client_fd);
        return;
    }

    const std::uint64_t length = end - start + 1;
    std::ostringstream response;
    response << "HTTP/1.1 206 Partial Content\r\n"
             << "Accept-Ranges: bytes\r\n"
             << "Content-Type: application/octet-stream\r\n"
             << "Content-Length: " << length << "\r\n"
             << "Content-Range: bytes " << start << '-' << end << '/';
    if (feeder_running_) response << '*';
    else response << available;
    response << "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
    if (!send_all(client_fd, response.str())) { close(client_fd); return; }

    const int file_fd = open(cache_path_.c_str(), O_RDONLY);
    if (file_fd < 0) { close(client_fd); return; }
    std::vector<char> buffer(1024 * 1024);
    std::uint64_t position = start;
    while (running_ && position <= end) {
        const std::size_t wanted = static_cast<std::size_t>(
            std::min<std::uint64_t>(buffer.size(), end - position + 1));
        const ssize_t n = pread(file_fd, buffer.data(), wanted, static_cast<off_t>(position));
        if (n <= 0) break;
        if (!send_all(client_fd, buffer.data(), static_cast<std::size_t>(n))) break;
        position += static_cast<std::uint64_t>(n);
    }
    close(file_fd);
    close(client_fd);
}

