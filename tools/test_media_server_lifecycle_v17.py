#!/usr/bin/env python3
"""Compile and exercise ReddMedia v0.0.17 server ownership behavior."""

from __future__ import annotations

import os
import pathlib
import select
import socket
import subprocess
import sys
import tempfile
import time


ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def port_open() -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as connection:
        connection.settimeout(0.2)
        return connection.connect_ex(("127.0.0.1", 8096)) == 0


def wait_for_port(expected: bool, seconds: float = 8.0) -> None:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if port_open() == expected:
            return
        time.sleep(0.1)
    require(False, f"port 8096 did not become {'open' if expected else 'free'}")


require(not port_open(), "port 8096 must be free for the server lifecycle test")

main_source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
manager_source = (ROOT / "src/media_server/media_server_manager.cpp").read_text(encoding="utf-8")
for marker in ("Start Server", "Stop Server", "Refresh Server"):
    require(marker in main_source, f"missing visible server control: {marker}")
for marker in ("PR_SET_PDEATHSIG", "kill(-pid, SIGTERM)", "waitpid(pid, &status"):
    require(marker in manager_source, f"missing owned-server shutdown marker: {marker}")

HARNESS = r'''
#include "media_server/media_server_manager.hpp"
#include <cstdio>
#include <string>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 2) return 90;
    reddmedia::MediaServerManager server;
    server.start();
    bool ready = false;
    for (int attempt = 0; attempt < 200; ++attempt) {
        server.refresh();
        if (server.state() == reddmedia::MediaServerState::Ready) {
            ready = true;
            break;
        }
        usleep(100000);
    }
    if (!ready) return 1;
    const std::string mode(argv[1]);
    if (mode == "hold") {
        if (!server.owns_server()) return 2;
        std::puts("READY");
        std::fflush(stdout);
        while (true) pause();
    }
    if (mode == "independent") {
        if (server.owns_server()) return 3;
        server.stop();
        return 0;
    }
    if (!server.owns_server()) return 4;
    server.stop();
    return server.state() == reddmedia::MediaServerState::Stopped ? 0 : 5;
}
'''

FAKE_SERVER = r'''
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;
static void stop_server(int) { running = 0; }

int main() {
    std::signal(SIGTERM, stop_server);
    const int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) return 1;
    int enabled = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(8096);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) return 2;
    if (listen(listener, 8) != 0) return 3;
    while (running) {
        fd_set sockets;
        FD_ZERO(&sockets);
        FD_SET(listener, &sockets);
        timeval timeout {0, 100000};
        const int ready = select(listener + 1, &sockets, nullptr, nullptr, &timeout);
        if (ready <= 0) continue;
        const int client = accept(listener, nullptr, nullptr);
        if (client < 0) continue;
        char request[512] {};
        recv(client, request, sizeof(request), 0);
        const char response[] =
            "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
        send(client, response, std::strlen(response), MSG_NOSIGNAL);
        close(client);
    }
    close(listener);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="reddmedia-v17-server-life-") as temporary:
    work = pathlib.Path(temporary)
    runtime = work / "components/jellyfin/runtime/jellyfin"
    runtime.mkdir(parents=True)
    harness_source = work / "harness.cpp"
    fake_source = work / "fake_server.cpp"
    harness_binary = work / "reddmedia-manager-harness"
    fake_binary = runtime / "jellyfin"
    harness_source.write_text(HARNESS, encoding="utf-8")
    fake_source.write_text(FAKE_SERVER, encoding="utf-8")

    common_flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror"]
    subprocess.run(["g++", *common_flags, str(fake_source), "-o", str(fake_binary)], check=True)
    subprocess.run(
        [
            "g++", *common_flags, "-I", str(ROOT / "src"), str(harness_source),
            str(ROOT / "src/media_server/media_server_manager.cpp"), "-o", str(harness_binary),
        ],
        check=True,
    )

    subprocess.run([str(harness_binary), "normal"], check=True, timeout=30)
    wait_for_port(False)

    parent = subprocess.Popen(
        [str(harness_binary), "hold"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    require(parent.stdout is not None, "parent-death harness stdout is unavailable")
    readable, _, _ = select.select([parent.stdout], [], [], 30)
    require(bool(readable), "parent-death harness did not become ready")
    require(parent.stdout.readline().strip() == "READY", "parent-death harness startup failed")
    wait_for_port(True)
    os.kill(parent.pid, 9)
    parent.wait(timeout=5)
    wait_for_port(False)

    independent = subprocess.Popen([str(fake_binary)])
    try:
        wait_for_port(True)
        subprocess.run([str(harness_binary), "independent"], check=True, timeout=30)
        require(independent.poll() is None, "ReddMedia stopped an independent server process")
        require(port_open(), "ReddMedia closed the independent server port")
    finally:
        independent.terminate()
        try:
            independent.wait(timeout=5)
        except subprocess.TimeoutExpired:
            independent.kill()
            independent.wait(timeout=5)
    wait_for_port(False)

print(
    "server-graceful-stop=pass server-parent-death-stop=pass "
    "server-independent-preserved=pass controls=pass"
)
