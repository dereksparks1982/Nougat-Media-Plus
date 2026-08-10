#include "p2p_stream_server.hpp"
#include "p2p_engine.hpp"

#include <arpa/inet.h>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
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

std::string lower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

bool parse_range(const std::string& headers, std::uint64_t file_size,
                 std::uint64_t& start, std::uint64_t& end, bool& partial) {
    start = 0;
    end = file_size == 0 ? 0 : file_size - 1;
    partial = false;
    const std::string lower = lower_copy(headers);
    const std::string key = "range: bytes=";
    const std::size_t pos = lower.find(key);
    if (pos == std::string::npos) return true;
    const std::size_t value_start = pos + key.size();
    const std::size_t line_end = lower.find("\r\n", value_start);
    const std::string value = lower.substr(value_start, line_end == std::string::npos ? std::string::npos : line_end - value_start);
    const std::size_t dash = value.find('-');
    if (dash == std::string::npos) return false;
    try {
        if (dash == 0) {
            if (dash + 1 >= value.size()) return false;
            const std::uint64_t suffix_length = static_cast<std::uint64_t>(std::stoull(value.substr(dash + 1)));
            if (suffix_length == 0) return false;
            start = suffix_length >= file_size ? 0 : file_size - suffix_length;
            end = file_size - 1;
        } else {
            start = static_cast<std::uint64_t>(std::stoull(value.substr(0, dash)));
            if (dash + 1 < value.size()) end = static_cast<std::uint64_t>(std::stoull(value.substr(dash + 1)));
            else end = file_size - 1;
        }
    } catch (...) {
        return false;
    }
    if (start >= file_size) return false;
    end = std::min(end, file_size - 1);
    if (end < start) return false;
    partial = true;
    return true;
}

std::string content_type_for(const std::string& path) {
    const std::string lower = lower_copy(path);
    if (lower.size() >= 4 && lower.substr(lower.size()-4) == ".mp4") return "video/mp4";
    if (lower.size() >= 5 && lower.substr(lower.size()-5) == ".webm") return "video/webm";
    if (lower.size() >= 4 && lower.substr(lower.size()-4) == ".mkv") return "video/x-matroska";
    if (lower.size() >= 4 && lower.substr(lower.size()-4) == ".avi") return "video/x-msvideo";
    if (lower.size() >= 4 && lower.substr(lower.size()-4) == ".mov") return "video/quicktime";
    return "application/octet-stream";
}
}

P2PStreamServer::P2PStreamServer(P2PEngine& engine) : engine_(engine) {}
P2PStreamServer::~P2PStreamServer() { stop(); }

bool P2PStreamServer::start(std::string& error) {
    stop();
    if (engine_.selected_file() < 0 || engine_.selected_file_size() == 0) {
        error = "Select a video file before streaming.";
        return false;
    }
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) { error = "Could not create local streaming socket."; return false; }
    int yes = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(0);
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        error = std::string("Could not bind localhost stream server: ") + std::strerror(errno);
        close(listen_fd_); listen_fd_ = -1; return false;
    }
    if (listen(listen_fd_, 8) != 0) {
        error = "Could not listen on localhost stream server.";
        close(listen_fd_); listen_fd_ = -1; return false;
    }
    socklen_t len = sizeof(address);
    if (getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&address), &len) != 0) {
        error = "Could not determine localhost stream port.";
        close(listen_fd_); listen_fd_ = -1; return false;
    }
    port_ = ntohs(address.sin_port);
    running_ = true;
    accept_thread_ = std::thread(&P2PStreamServer::accept_loop, this);
    return true;
}

void P2PStreamServer::stop() {
    running_ = false;
    ++request_generation_;
    if (listen_fd_ >= 0) {
        shutdown(listen_fd_, SHUT_RDWR);
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (accept_thread_.joinable()) accept_thread_.join();
    join_workers();
    port_ = 0;
    engine_.clear_stream_priority();
}

bool P2PStreamServer::running() const { return running_; }
std::uint16_t P2PStreamServer::port() const { return port_; }
std::string P2PStreamServer::url() const {
    if (!running_ || port_ == 0) return {};
    return "http://127.0.0.1:" + std::to_string(port_) + "/stream";
}

void P2PStreamServer::accept_loop() {
    while (running_) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        const int fd = accept(listen_fd_, reinterpret_cast<sockaddr*>(&client), &len);
        if (fd < 0) {
            if (!running_) break;
            continue;
        }
        if (ntohl(client.sin_addr.s_addr) != INADDR_LOOPBACK) {
            close(fd);
            continue;
        }
        std::lock_guard<std::mutex> lock(workers_mutex_);
        workers_.emplace_back(&P2PStreamServer::handle_client, this, fd);
    }
}

void P2PStreamServer::join_workers() {
    std::vector<std::thread> workers;
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        workers.swap(workers_);
    }
    for (std::thread& worker : workers) if (worker.joinable()) worker.join();
}

void P2PStreamServer::handle_client(int client_fd) {
    timeval socket_timeout{};
    socket_timeout.tv_sec = 2;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &socket_timeout, sizeof(socket_timeout));
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &socket_timeout, sizeof(socket_timeout));

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
        close(client_fd); return;
    }
    const std::uint64_t size = engine_.selected_file_size();
    if (size == 0) {
        send_all(client_fd, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
        close(client_fd); return;
    }
    std::uint64_t start = 0, end = size - 1;
    bool partial = false;
    if (!parse_range(request, size, start, end, partial)) {
        std::ostringstream response;
        response << "HTTP/1.1 416 Range Not Satisfiable\r\nContent-Range: bytes */" << size
                 << "\r\nConnection: close\r\n\r\n";
        send_all(client_fd, response.str());
        close(client_fd); return;
    }
    const std::uint64_t content_length = end - start + 1;
    const std::uint64_t generation = head_only ? request_generation_.load() : request_generation_.fetch_add(1) + 1;
    if (!head_only) engine_.clear_stream_priority();

    std::ostringstream response;
    response << (partial ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n");
    response << "Accept-Ranges: bytes\r\n";
    response << "Content-Type: " << content_type_for(engine_.selected_file_name()) << "\r\n";
    response << "Content-Length: " << content_length << "\r\n";
    if (partial) response << "Content-Range: bytes " << start << '-' << end << '/' << size << "\r\n";
    response << "Cache-Control: no-store\r\nConnection: close\r\n\r\n";
    if (!send_all(client_fd, response.str()) || head_only) { close(client_fd); return; }

    constexpr std::size_t chunk_size = 1024 * 1024;
    constexpr std::uint64_t priority_window = 16ULL * 1024ULL * 1024ULL;
    std::vector<char> buffer(chunk_size);
    std::uint64_t position = start;
    while (running_ && generation == request_generation_.load() && position <= end) {
        const std::size_t wanted = static_cast<std::size_t>(std::min<std::uint64_t>(chunk_size, end - position + 1));
        engine_.prioritize_range(position, std::min<std::uint64_t>(priority_window, end - position + 1));
        bool ready = false;
        while (running_ && generation == request_generation_.load() && !ready) {
            ready = engine_.wait_for_range(position, wanted, 2000);
            if (!ready) {
                const P2PStatus status = engine_.status();
                if (!status.error.empty() || !status.active) { close(client_fd); return; }
            }
        }
        if (!running_ || generation != request_generation_.load()) break;
        std::size_t bytes_read = 0;
        std::string error;
        if (!engine_.read_selected_range(position, buffer.data(), wanted, bytes_read, error) || bytes_read == 0) break;
        if (!send_all(client_fd, buffer.data(), bytes_read)) break;
        position += bytes_read;
    }
    close(client_fd);
}
