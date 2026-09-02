#include "lan_media_service.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <thread>

namespace {
std::atomic<bool> running{true};

void handle_signal(int) {
    running.store(false);
}
}

int main() {
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGINT, handle_signal);
    std::signal(SIGHUP, handle_signal);

    reddmedia::lan::LanMediaService service;
    service.prepare();
    if (!service.status().serving) {
        std::fprintf(stderr, "%s\n", service.status().message.c_str());
        return 2;
    }

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    service.stop();
    return 0;
}
