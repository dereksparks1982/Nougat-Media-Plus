#pragma once

#include <functional>
#include <string>
#include <vector>
#include <sys/types.h>

namespace reddmedia {

struct NougatSearchResult {
    std::string url;
    std::string title;
    std::string snippet;
    std::string domain;
    std::string source_network;
    std::string source_node;
    long long crawled_at = 0;
    std::string content_hash;
    double score = 0.0;
};

struct NougatSearchResponse {
    long long total = 0;
    std::vector<NougatSearchResult> results;
    std::vector<std::pair<std::string, std::string>> peer_status;
    std::string error;
};

class NougatBridge {
public:
    explicit NougatBridge(std::string engine_path);
    ~NougatBridge();

    NougatSearchResponse search(const std::string& query, bool raw, bool include_peers,
                                int limit = 100, int offset = 0) const;
    bool crawl(const std::string& seed, int max_pages, bool same_domain,
               const std::function<void(const std::string&)>& on_log,
               std::string& summary, std::string& error) const;

    std::string node_id(std::string& error) const;
    std::vector<std::string> peers(std::string& error) const;
    bool add_peer(const std::string& peer, std::string& error) const;
    bool remove_peer(const std::string& peer, std::string& error) const;

    bool start_node(int port, std::string& error);
    void stop_node();
    bool node_running() const;
    int node_port() const { return node_port_; }

    bool open_url(const std::string& url, bool tor, std::string& error) const;
    std::string data_directory() const;

private:
    struct ProcessResult {
        int exit_code = -1;
        std::string output;
        std::string error;
    };

    ProcessResult run_capture(const std::vector<std::string>& arguments) const;
    static std::string hex_decode(const std::string& value);
    static std::vector<std::string> split_tabs(const std::string& line);
    static std::string trim(const std::string& value);
    static bool process_alive(pid_t pid);

    std::string engine_path_;
    pid_t node_pid_ = -1;
    int node_port_ = 0;
};

} // namespace reddmedia
