#include "nougat_bridge.hpp"

#include <X11/Xlib.h>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <utility>
#include <unistd.h>

namespace reddmedia {
namespace {

std::string home_dir() {
    const char* value = std::getenv("HOME");
    return value ? value : ".";
}

void set_nougat_home() {
    const std::string path = home_dir() + "/.local/share/reddmedia/nougat";
    setenv("NOUGAT_HOME", path.c_str(), 1);
}

std::vector<char*> argv_for(const std::string& engine_path, const std::vector<std::string>& arguments,
                            std::vector<std::string>& storage) {
    storage.clear();
    storage.push_back("python3");
    storage.push_back(engine_path);
    storage.insert(storage.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    argv.reserve(storage.size() + 1);
    for (std::string& item : storage) argv.push_back(item.data());
    argv.push_back(nullptr);
    return argv;
}

} // namespace

NougatBridge::NougatBridge(std::string engine_path) : engine_path_(std::move(engine_path)) {}

NougatBridge::~NougatBridge() {
    stop_node();
}

std::string NougatBridge::trim(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) --last;
    return value.substr(first, last - first);
}

std::vector<std::string> NougatBridge::split_tabs(const std::string& line) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    for (;;) {
        const std::size_t found = line.find('\t', start);
        if (found == std::string::npos) {
            parts.push_back(line.substr(start));
            break;
        }
        parts.push_back(line.substr(start, found - start));
        start = found + 1;
    }
    return parts;
}

std::string NougatBridge::hex_decode(const std::string& value) {
    if ((value.size() % 2U) != 0U) return {};
    std::string out;
    out.reserve(value.size() / 2U);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + c - 'a';
        if (c >= 'A' && c <= 'F') return 10 + c - 'A';
        return -1;
    };
    for (std::size_t i = 0; i < value.size(); i += 2U) {
        const int hi = nibble(value[i]);
        const int lo = nibble(value[i + 1U]);
        if (hi < 0 || lo < 0) return {};
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}

NougatBridge::ProcessResult NougatBridge::run_capture(const std::vector<std::string>& arguments) const {
    ProcessResult result;
    int pipefd[2] = {-1, -1};
    if (pipe(pipefd) != 0) {
        result.error = std::string("pipe failed: ") + std::strerror(errno);
        return result;
    }
    const pid_t pid = fork();
    if (pid < 0) {
        result.error = std::string("fork failed: ") + std::strerror(errno);
        close(pipefd[0]); close(pipefd[1]);
        return result;
    }
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        set_nougat_home();
        std::vector<std::string> storage;
        std::vector<char*> argv = argv_for(engine_path_, arguments, storage);
        execvp("python3", argv.data());
        _exit(127);
    }
    close(pipefd[1]);
    char buffer[4096];
    for (;;) {
        const ssize_t n = read(pipefd[0], buffer, sizeof(buffer));
        if (n > 0) result.output.append(buffer, static_cast<std::size_t>(n));
        else if (n == 0) break;
        else if (errno != EINTR) break;
    }
    close(pipefd[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    if (WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) result.exit_code = 128 + WTERMSIG(status);
    if (result.exit_code != 0 && result.error.empty()) result.error = trim(result.output);
    return result;
}

NougatSearchResponse NougatBridge::search(const std::string& query, bool raw, bool include_peers,
                                          int limit, int offset) const {
    NougatSearchResponse response;
    std::vector<std::string> args = {"search", query, "--limit", std::to_string(limit),
                                     "--offset", std::to_string(offset)};
    if (raw) args.push_back("--raw");
    if (include_peers) args.push_back("--peers");
    const ProcessResult process = run_capture(args);
    if (process.exit_code != 0) {
        response.error = process.error.empty() ? "Nougat search engine failed." : process.error;
        return response;
    }
    std::istringstream lines(process.output);
    std::string line;
    while (std::getline(lines, line)) {
        const std::vector<std::string> parts = split_tabs(line);
        if (parts.empty()) continue;
        if (parts[0] == "META" && parts.size() >= 2U) {
            try { response.total = std::stoll(parts[1]); } catch (...) { response.total = 0; }
        } else if (parts[0] == "RESULT" && parts.size() >= 10U) {
            NougatSearchResult item;
            item.url = hex_decode(parts[1]);
            item.title = hex_decode(parts[2]);
            item.snippet = hex_decode(parts[3]);
            item.domain = hex_decode(parts[4]);
            item.source_network = hex_decode(parts[5]);
            item.source_node = hex_decode(parts[6]);
            try { item.crawled_at = std::stoll(parts[7]); } catch (...) { item.crawled_at = 0; }
            item.content_hash = parts[8];
            try { item.score = std::stod(parts[9]); } catch (...) { item.score = 0.0; }
            response.results.push_back(std::move(item));
        } else if (parts[0] == "PEER" && parts.size() >= 3U) {
            response.peer_status.emplace_back(hex_decode(parts[1]), hex_decode(parts[2]));
        }
    }
    return response;
}

bool NougatBridge::crawl(const std::string& seed, int max_pages, bool same_domain,
                         const std::function<void(const std::string&)>& on_log,
                         std::string& summary, std::string& error) const {
    int pipefd[2] = {-1, -1};
    if (pipe(pipefd) != 0) { error = "Could not start crawler pipe."; return false; }
    const pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]); close(pipefd[1]);
        error = "Could not start crawler process.";
        return false;
    }
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        set_nougat_home();
        std::vector<std::string> args = {"crawl", seed, "--max-pages", std::to_string(max_pages)};
        if (!same_domain) args.push_back("--follow-external");
        std::vector<std::string> storage;
        std::vector<char*> argv = argv_for(engine_path_, args, storage);
        execvp("python3", argv.data());
        _exit(127);
    }
    close(pipefd[1]);
    FILE* stream = fdopen(pipefd[0], "r");
    if (!stream) {
        close(pipefd[0]);
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        error = "Could not read crawler output.";
        return false;
    }
    char* line_ptr = nullptr;
    std::size_t capacity = 0;
    bool saw_done = false;
    bool saw_fail = false;
    while (getline(&line_ptr, &capacity, stream) >= 0) {
        std::string line = trim(line_ptr ? line_ptr : "");
        const std::vector<std::string> parts = split_tabs(line);
        if (parts.size() >= 2U && parts[0] == "LOG") {
            on_log(hex_decode(parts[1]));
        } else if (parts.size() >= 4U && parts[0] == "DONE") {
            summary = "DONE: indexed=" + parts[1] + " seen=" + parts[2] + " failures=" + parts[3];
            on_log(summary);
            saw_done = true;
        } else if (parts.size() >= 2U && parts[0] == "FAIL") {
            error = hex_decode(parts[1]);
            on_log("FAILED: " + error);
            saw_fail = true;
        } else if (!line.empty()) {
            on_log(line);
        }
    }
    if (line_ptr) free(line_ptr);
    fclose(stream);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    const bool process_ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if (!process_ok && error.empty()) error = "Nougat crawler process failed.";
    return process_ok && saw_done && !saw_fail;
}

std::string NougatBridge::node_id(std::string& error) const {
    const ProcessResult process = run_capture({"node-id"});
    if (process.exit_code != 0) { error = process.error; return {}; }
    return trim(process.output);
}

std::vector<std::string> NougatBridge::peers(std::string& error) const {
    const ProcessResult process = run_capture({"list-peers"});
    if (process.exit_code != 0) { error = process.error; return {}; }
    std::vector<std::string> out;
    std::istringstream lines(process.output);
    std::string line;
    while (std::getline(lines, line)) {
        const std::vector<std::string> parts = split_tabs(line);
        if (parts.size() >= 2U && parts[0] == "PEER") out.push_back(hex_decode(parts[1]));
    }
    return out;
}

bool NougatBridge::add_peer(const std::string& peer, std::string& error) const {
    const ProcessResult process = run_capture({"add-peer", peer});
    if (process.exit_code != 0) { error = process.error; return false; }
    return true;
}

bool NougatBridge::remove_peer(const std::string& peer, std::string& error) const {
    const ProcessResult process = run_capture({"remove-peer", peer});
    if (process.exit_code != 0) { error = process.error; return false; }
    return true;
}

bool NougatBridge::process_alive(pid_t pid) {
    if (pid <= 0) return false;
    if (kill(pid, 0) == 0) return true;
    return errno == EPERM;
}

bool NougatBridge::start_node(int port, std::string& error) {
    if (node_running()) return true;
    const pid_t pid = fork();
    if (pid < 0) { error = "Could not fork Nougat node process."; return false; }
    if (pid == 0) {
        int nullfd = open("/dev/null", O_RDWR);
        if (nullfd >= 0) {
            dup2(nullfd, STDIN_FILENO); dup2(nullfd, STDOUT_FILENO); dup2(nullfd, STDERR_FILENO);
            if (nullfd > STDERR_FILENO) close(nullfd);
        }
        set_nougat_home();
        std::vector<std::string> args = {"serve", "--port", std::to_string(port)};
        std::vector<std::string> storage;
        std::vector<char*> argv = argv_for(engine_path_, args, storage);
        execvp("python3", argv.data());
        _exit(127);
    }
    node_pid_ = pid;
    node_port_ = port;
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    if (!process_alive(node_pid_)) {
        int status = 0; waitpid(node_pid_, &status, WNOHANG);
        node_pid_ = -1; node_port_ = 0;
        error = "Nougat node could not start. The selected port may already be in use.";
        return false;
    }
    return true;
}

void NougatBridge::stop_node() {
    if (node_pid_ <= 0) return;
    if (process_alive(node_pid_)) kill(node_pid_, SIGTERM);
    for (int i = 0; i < 20; ++i) {
        int status = 0;
        const pid_t result = waitpid(node_pid_, &status, WNOHANG);
        if (result == node_pid_ || result == -1) { node_pid_ = -1; node_port_ = 0; return; }
        usleep(25000);
    }
    kill(node_pid_, SIGKILL);
    waitpid(node_pid_, nullptr, 0);
    node_pid_ = -1; node_port_ = 0;
}

bool NougatBridge::node_running() const {
    return process_alive(node_pid_);
}

bool NougatBridge::open_url(const std::string& url, bool tor, std::string& error) const {
    if (url.empty()) { error = "No URL is selected."; return false; }
    const bool onion = url.find(".onion") != std::string::npos;
    const pid_t pid = fork();
    if (pid < 0) { error = "Could not open result."; return false; }
    if (pid == 0) {
        int nullfd = open("/dev/null", O_RDWR);
        if (nullfd >= 0) {
            dup2(nullfd, STDIN_FILENO); dup2(nullfd, STDOUT_FILENO); dup2(nullfd, STDERR_FILENO);
            if (nullfd > STDERR_FILENO) close(nullfd);
        }
        if (tor || onion) {
            const char* candidates[][4] = {
                {"tor-browser", url.c_str(), nullptr, nullptr},
                {"torbrowser-launcher", url.c_str(), nullptr, nullptr},
                {"flatpak", "run", "org.torproject.torbrowser-launcher", url.c_str()}
            };
            for (const auto& candidate : candidates) {
                if (std::string(candidate[0]) == "flatpak") {
                    execlp(candidate[0], candidate[0], candidate[1], candidate[2], candidate[3], static_cast<char*>(nullptr));
                } else {
                    execlp(candidate[0], candidate[0], candidate[1], static_cast<char*>(nullptr));
                }
            }
        } else {
            execlp("xdg-open", "xdg-open", url.c_str(), static_cast<char*>(nullptr));
        }
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, WNOHANG);
    return true;
}

std::string NougatBridge::data_directory() const {
    return home_dir() + "/.local/share/reddmedia/nougat";
}

} // namespace reddmedia
