#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

class YtDlpStreamServer {
public:
    YtDlpStreamServer();
    ~YtDlpStreamServer();

    YtDlpStreamServer(const YtDlpStreamServer&) = delete;
    YtDlpStreamServer& operator=(const YtDlpStreamServer&) = delete;

    bool start(const std::string& engine, const std::string& source_url,
               long long start_time_ms, std::string& error);
    void stop();
    void poll();

    bool wait_for_initial_cache(std::uint64_t minimum_bytes, int timeout_ms,
                                std::string& error);
    bool running() const;
    bool feeder_running() const;
    bool failed() const;
    std::uint64_t cache_bytes() const;
    long long base_time_ms() const;
    std::uint16_t port() const;
    std::string url() const;
    std::string take_log();

private:
    bool start_http_server(std::string& error);
    bool start_feeder(std::string& error);
    void stop_feeder();
    void accept_loop();
    void handle_client(int client_fd);
    void join_workers();
    void unregister_client(int client_fd);
    void append_log(const char* data, std::size_t size);
    void cleanup_cache();

    std::string engine_;
    std::string source_url_;
    std::string cache_path_;
    long long base_time_ms_ = 0;

    std::atomic<bool> running_{false};
    std::atomic<bool> feeder_running_{false};
    std::atomic<bool> feeder_failed_{false};
    int feeder_pid_ = -1;
    int feeder_log_fd_ = -1;

    int listen_fd_ = -1;
    std::uint16_t port_ = 0;
    std::thread accept_thread_;
    mutable std::mutex workers_mutex_;
    std::vector<std::thread> workers_;
    std::unordered_set<int> active_clients_;

    mutable std::mutex log_mutex_;
    std::string log_buffer_;
};
