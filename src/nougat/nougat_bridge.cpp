#include "nougat_bridge.hpp"

#include <X11/Xlib.h>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

std::vector<char*> argv_for(const std::string& program_path,
                            const std::vector<std::string>& arguments,
                            std::vector<std::string>& storage) {
    storage.clear();
    storage.push_back("python3");
    storage.push_back(program_path);
    storage.insert(storage.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    argv.reserve(storage.size() + 1U);
    for (std::string& item : storage) argv.push_back(item.data());
    argv.push_back(nullptr);
    return argv;
}

bool write_all(int fd, const std::string& value) {
    std::size_t offset = 0;
    while (offset < value.size()) {
        const ssize_t n = write(fd, value.data() + offset, value.size() - offset);
        if (n > 0) offset += static_cast<std::size_t>(n);
        else if (n < 0 && errno == EINTR) continue;
        else return false;
    }
    return true;
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
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1U]))) --last;
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
        start = found + 1U;
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

std::string NougatBridge::sibling_worker(const std::string& filename) const {
    const std::filesystem::path engine(engine_path_);
    return (engine.parent_path() / filename).string();
}

NougatBridge::ProcessResult NougatBridge::run_capture_program(
    const std::string& program_path,
    const std::vector<std::string>& arguments,
    const std::string& stdin_payload) const {
    ProcessResult result;
    int stdout_pipe[2] = {-1, -1};
    int stdin_pipe[2] = {-1, -1};
    if (pipe(stdout_pipe) != 0 || pipe(stdin_pipe) != 0) {
        result.error = std::string("pipe failed: ") + std::strerror(errno);
        if (stdout_pipe[0] >= 0) { close(stdout_pipe[0]); close(stdout_pipe[1]); }
        if (stdin_pipe[0] >= 0) { close(stdin_pipe[0]); close(stdin_pipe[1]); }
        return result;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        result.error = std::string("fork failed: ") + std::strerror(errno);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        return result;
    }

    if (pid == 0) {
        close(stdout_pipe[0]);
        close(stdin_pipe[1]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stdout_pipe[1], STDERR_FILENO);
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdout_pipe[1]);
        close(stdin_pipe[0]);
        set_nougat_home();
        std::vector<std::string> storage;
        std::vector<char*> argv = argv_for(program_path, arguments, storage);
        execvp("python3", argv.data());
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stdin_pipe[0]);
    if (!stdin_payload.empty() && !write_all(stdin_pipe[1], stdin_payload)) {
        result.error = "Could not send private local worker input.";
    }
    close(stdin_pipe[1]);

    char buffer[4096];
    for (;;) {
        const ssize_t n = read(stdout_pipe[0], buffer, sizeof(buffer));
        if (n > 0) result.output.append(buffer, static_cast<std::size_t>(n));
        else if (n == 0) break;
        else if (errno != EINTR) break;
    }
    close(stdout_pipe[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    if (WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status)) result.exit_code = 128 + WTERMSIG(status);
    if (result.exit_code != 0 && result.error.empty()) result.error = trim(result.output);
    return result;
}

NougatBridge::ProcessResult NougatBridge::run_capture(const std::vector<std::string>& arguments) const {
    return run_capture_program(engine_path_, arguments, {});
}

NougatSearchResponse NougatBridge::search(const std::string& query, bool raw, bool include_peers,
                                          int limit, int offset) const {
    NougatSearchResponse response;
    std::vector<std::string> args = {"search", "--limit", std::to_string(limit),
                                     "--offset", std::to_string(offset)};
    if (raw) args.push_back("--raw");
    // include_peers is intentionally not sent to the worker. Remote search is
    // owned by SecureSearchController and stays fail-closed until a private
    // transport is actually available.
    (void)include_peers;
    const ProcessResult process = run_capture_program(
        sibling_worker("nougat_search_worker.py"), args, query + "\n");
    if (process.exit_code != 0) {
        response.error = process.error.empty() ? "Nougat local search worker failed." : process.error;
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
    std::vector<std::string> args = {"crawl", "--max-pages", std::to_string(max_pages)};
    if (!same_domain) args.push_back("--follow-external");
    const ProcessResult process = run_capture_program(
        sibling_worker("nougat_crawler_worker.py"), args, seed + "\n");
    if (process.exit_code != 0 && process.output.empty()) {
        error = process.error.empty() ? "Nougat crawler worker failed." : process.error;
        return false;
    }

    bool saw_done = false;
    bool saw_fail = false;
    std::istringstream lines(process.output);
    std::string line;
    while (std::getline(lines, line)) {
        const std::vector<std::string> parts = split_tabs(trim(line));
        if (parts.size() >= 2U && parts[0] == "LOG") {
            on_log(hex_decode(parts[1]));
        } else if (parts.size() >= 3U && parts[0] == "ACCESS") {
            on_log("Access: " + hex_decode(parts[1]) + " | " + hex_decode(parts[2]));
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
    if (process.exit_code != 0 && error.empty()) {
        error = process.error.empty() ? "Nougat crawler process failed." : process.error;
    }
    return process.exit_code == 0 && saw_done && !saw_fail;
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
