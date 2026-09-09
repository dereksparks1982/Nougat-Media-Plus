#include "youtube_compat.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace nougat::youtube {

namespace {

bool executable_on_path(const std::string& name) {
    const char* path_value = std::getenv("PATH");
    if (path_value == nullptr) {
        return false;
    }
    std::stringstream stream(path_value);
    std::string directory;
    while (std::getline(stream, directory, ':')) {
        if (directory.empty()) {
            directory = ".";
        }
        std::error_code error;
        const auto candidate = std::filesystem::path(directory) / name;
        const auto permissions = std::filesystem::status(candidate, error).permissions();
        if (error || !std::filesystem::is_regular_file(candidate, error)) {
            continue;
        }
        using perms = std::filesystem::perms;
        if ((permissions & (perms::owner_exec | perms::group_exec | perms::others_exec)) != perms::none) {
            return true;
        }
    }
    return false;
}

std::optional<int> node_major_version() {
    if (!executable_on_path("node")) {
        return std::nullopt;
    }
    std::array<char, 64> buffer{};
    FILE* pipe = popen("node --version 2>/dev/null", "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }
    const char* line = std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe);
    (void)pclose(pipe);
    if (line == nullptr) {
        return std::nullopt;
    }
    std::string version(buffer.data());
    const auto digit = version.find_first_of("0123456789");
    if (digit == std::string::npos) {
        return std::nullopt;
    }
    try {
        return std::stoi(version.substr(digit));
    } catch (...) {
        return std::nullopt;
    }
}

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

}  // namespace

bool is_youtube_url(const std::string& url) {
    const std::string lower = lower_copy(url);
    return lower.find("youtube.com/") != std::string::npos || lower.find("youtu.be/") != std::string::npos;
}

FeederPolicy feeder_policy(const std::string& url) {
    FeederPolicy policy;
    policy.youtube = is_youtube_url(url);
    policy.force_external_downloader = !policy.youtube;
    if (!policy.youtube) {
        policy.format_selector = "bv*[height<=1080]+ba/b[height<=1080]";
        return policy;
    }

    // stdout feeder must stay directly consumable: progressive or HLS first,
    // avoiding a separate FFmpeg reopen of signed YouTube media URLs.
    policy.format_selector = "b[protocol^=m3u8][height<=1080]/b[height<=1080]/bv*[height<=1080]+ba/b";

    std::vector<std::string> runtimes;
    if (executable_on_path("deno")) {
        runtimes.emplace_back("deno");
    }
    if (const auto major = node_major_version(); major.has_value() && *major >= 22) {
        runtimes.emplace_back("node");
    }
    if (executable_on_path("qjs")) {
        runtimes.emplace_back("quickjs");
    }

    if (!runtimes.empty()) {
        std::string joined;
        for (std::size_t index = 0; index < runtimes.size(); ++index) {
            if (index != 0U) {
                joined += ',';
            }
            joined += runtimes[index];
        }
        policy.compatibility_args.emplace_back("--js-runtimes");
        policy.compatibility_args.push_back(joined);
    }
    policy.compatibility_args.emplace_back("--extractor-args");
    policy.compatibility_args.emplace_back("youtube:player_client=default,web_safari");
    return policy;
}

}  // namespace nougat::youtube
