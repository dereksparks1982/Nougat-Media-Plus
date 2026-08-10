#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class P2PEngine;

class P2PStreamServer {
public:
    explicit P2PStreamServer(P2PEngine& engine);
    ~P2PStreamServer();

    P2PStreamServer(const P2PStreamServer&) = delete;
    P2PStreamServer& operator=(const P2PStreamServer&) = delete;

    bool start(std::string& error);
    void stop();
    bool running() const;
    std::uint16_t port() const;
    std::string url() const;

private:
    void accept_loop();
    void handle_client(int client_fd);
    void join_workers();

    P2PEngine& engine_;
    std::atomic<bool> running_{false};
    int listen_fd_ = -1;
    std::uint16_t port_ = 0;
    std::thread accept_thread_;
    std::atomic<std::uint64_t> request_generation_{0};
    mutable std::mutex workers_mutex_;
    std::vector<std::thread> workers_;
};
