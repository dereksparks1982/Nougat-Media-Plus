#include "network_center.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace nougat::network {
namespace {

std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string run(const char* command) {
    std::array<char, 1024> buf{};
    std::string out;
    FILE* pipe = ::popen(command, "r");
    if (!pipe) return {};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe)) out += buf.data();
    ::pclose(pipe);
    return trim(out);
}

std::vector<std::string> lines(const std::string& text, std::size_t max_lines = 80) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (out.size() < max_lines && std::getline(in, line)) {
        line = trim(line);
        if (!line.empty()) out.push_back(line);
    }
    return out;
}

std::string read_first(const char* path) {
    std::ifstream f(path);
    std::string line;
    if (f && std::getline(f, line)) return trim(line);
    return {};
}

void append_command(std::vector<std::string>& out, const char* title, const char* command, std::size_t max_lines = 24) {
    const std::string value = run(command);
    if (value.empty()) {
        out.emplace_back(std::string(title) + ": unavailable");
        return;
    }
    out.emplace_back(std::string(title) + ":");
    for (const auto& line : lines(value, max_lines)) out.emplace_back("  " + line);
}

}  // namespace

Snapshot collect_snapshot() {
    Snapshot s;

    const std::string default_iface = run("ip -o route show default 2>/dev/null | awk 'NR==1 {print $5}'");
    const std::string gateway = run("ip -o route show default 2>/dev/null | awk 'NR==1 {print $3}'");
    const std::string local4 = default_iface.empty()
        ? run("hostname -I 2>/dev/null | awk '{print $1}'")
        : run(("ip -o -4 addr show dev " + default_iface + " scope global 2>/dev/null | awk 'NR==1 {print $4}'").c_str());
    const std::string local6 = default_iface.empty()
        ? std::string{}
        : run(("ip -o -6 addr show dev " + default_iface + " scope global 2>/dev/null | awk 'NR==1 {print $4}'").c_str());
    const std::string mac = default_iface.empty()
        ? std::string{}
        : read_first(("/sys/class/net/" + default_iface + "/address").c_str());
    const std::string state = default_iface.empty()
        ? std::string{}
        : read_first(("/sys/class/net/" + default_iface + "/operstate").c_str());
    const std::string mtu = default_iface.empty()
        ? std::string{}
        : read_first(("/sys/class/net/" + default_iface + "/mtu").c_str());
    const std::string ssid = run("iwgetid -r 2>/dev/null");
    const std::string dns = run("awk '/^nameserver / {printf \"%s \", $2}' /etc/resolv.conf 2>/dev/null");

    s.overview.emplace_back("Health: collecting live host/network state");
    s.overview.emplace_back("Active interface: " + (default_iface.empty() ? std::string("none") : default_iface));
    s.overview.emplace_back("Interface state: " + (state.empty() ? std::string("unknown") : state));
    s.overview.emplace_back("Local IPv4: " + (local4.empty() ? std::string("unavailable") : local4));
    s.overview.emplace_back("Local IPv6: " + (local6.empty() ? std::string("none detected") : local6));
    s.overview.emplace_back("Public IPv4: external lookup not queried automatically");
    s.overview.emplace_back("Gateway: " + (gateway.empty() ? std::string("unavailable") : gateway));
    s.overview.emplace_back("DNS: " + (dns.empty() ? std::string("unavailable") : dns));
    s.overview.emplace_back("MAC: " + (mac.empty() ? std::string("unavailable") : mac));
    s.overview.emplace_back("MTU: " + (mtu.empty() ? std::string("unknown") : mtu));
    s.overview.emplace_back("Wi-Fi SSID: " + (ssid.empty() ? std::string("not connected / not wireless") : ssid));
    const std::string wifi_link = default_iface.empty() ? std::string{} : run(("iw dev " + default_iface + " link 2>/dev/null").c_str());
    for (const auto& line : lines(wifi_link, 12)) s.overview.emplace_back("Wi-Fi: " + line);

    append_command(s.connections, "Active TCP/UDP connections", "ss -tunapH 2>/dev/null | head -n 80", 80);
    append_command(s.devices, "LAN neighbors", "ip neigh show 2>/dev/null | head -n 80", 80);

    const std::string listeners = run("ss -lntupH 2>/dev/null | head -n 80");
    s.security.emplace_back("Defensive assessment only. Findings are indicators to review, not proof of compromise.");
    if (listeners.empty()) {
        s.security.emplace_back("Listening sockets: unavailable");
    } else {
        const auto listener_lines = lines(listeners, 80);
        s.security.emplace_back("Listening sockets detected: " + std::to_string(listener_lines.size()));
        for (const auto& line : listener_lines) s.security.emplace_back("  " + line);
    }
    const std::string firewall = run("(ufw status 2>/dev/null || true) | head -n 12");
    if (!firewall.empty()) {
        s.security.emplace_back("Firewall:");
        for (const auto& line : lines(firewall, 12)) s.security.emplace_back("  " + line);
    } else {
        s.security.emplace_back("Firewall status: unavailable without changing system configuration");
    }
    s.security.emplace_back("Gateway baseline: " + (gateway.empty() ? std::string("not available") : gateway));
    s.security.emplace_back("DNS baseline: " + (dns.empty() ? std::string("not available") : dns));

    s.diagnostics.emplace_back("Internet reachability: active probe not run automatically");
    append_command(s.diagnostics, "Routes", "ip route show 2>/dev/null | head -n 40", 40);

    s.logs.emplace_back("Network Center snapshot collected from Linux interfaces, routes, sockets, neighbors, DNS, Wi-Fi state and reachability.");
    s.logs.emplace_back("No system settings were changed by this scan.");
    return s;
}

}  // namespace nougat::network
