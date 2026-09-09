#include "games/uo_client.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <unistd.h>

namespace nougat::games::uo {

namespace {

constexpr const char* kT2AClientVersion = "1.25.35";
constexpr const char* kT2ASourceArchiveSha256 =
    "dd52642a818354aed39a72d5a371a3231ef626f3e153a8428d5568eb579fcc35";
constexpr std::uintmax_t kT2AClientExeBytes = 896000U;

std::string getenv_string(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string(value);
}

std::string home_dir() {
    const std::string home = getenv_string("HOME");
    return home.empty() ? std::string{"."} : home;
}

bool executable_file(const std::string& path) {
    std::error_code error;
    return !path.empty() && std::filesystem::is_regular_file(path, error) && !error &&
           ::access(path.c_str(), X_OK) == 0;
}

std::string path_executable(const std::string& name) {
    const std::string path = getenv_string("PATH");
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t end = path.find(':', start);
        const std::string directory = path.substr(start, end == std::string::npos ? std::string::npos : end - start);
        const std::filesystem::path candidate = (directory.empty() ? std::filesystem::path{"."} : std::filesystem::path{directory}) / name;
        if (executable_file(candidate.string())) return candidate.string();
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return {};
}

std::string read_first_line(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::string line;
    if (input) std::getline(input, line);
    return line;
}

}  // namespace

const char* t2a_client_version() {
    return kT2AClientVersion;
}

const char* t2a_source_archive_sha256() {
    return kT2ASourceArchiveSha256;
}

std::string resolve_wine_executable() {
    const std::string override_path = getenv_string("NOUGAT_T2A_WINE");
    if (!override_path.empty()) return override_path;
    return path_executable("wine");
}

std::string resolve_uo_data_path() {
    const std::string override_path = getenv_string("NOUGAT_UO_DATA");
    if (!override_path.empty()) return override_path;
    return home_dir() + "/.local/share/nougat-play-portal/games/ultima-online/client-data";
}

std::string resolve_t2a_client_executable() {
    return (std::filesystem::path(resolve_uo_data_path()) / "client.exe").string();
}

std::string resolve_t2a_wine_prefix() {
    const std::string override_path = getenv_string("NOUGAT_T2A_WINEPREFIX");
    if (!override_path.empty()) return override_path;
    return home_dir() + "/.local/share/nougat-play-portal/runtimes/wine-t2a-1.25.35";
}

bool validate_t2a_data_path(const std::string& path, std::string& error) {
    if (path.empty()) {
        error = "Ultima Online T2A data path is empty.";
        return false;
    }

    const std::filesystem::path root(path);
    std::error_code fs_error;
    if (!std::filesystem::is_directory(root, fs_error) || fs_error) {
        error = "Ultima Online T2A data directory is missing: " + path;
        return false;
    }

    constexpr std::array<const char*, 17> required{
        "client.exe", "map0.mul", "statics0.mul", "staidx0.mul",
        "art.mul", "artidx.mul", "tiledata.mul", "anim.mul", "anim.idx",
        "gumpart.mul", "gumpidx.mul", "hues.mul", "multi.mul", "multi.idx",
        "sound.mul", "soundidx.mul", "radarcol.mul"
    };

    for (const char* name : required) {
        const auto file = root / name;
        fs_error.clear();
        if (!std::filesystem::is_regular_file(file, fs_error) || fs_error) {
            error = std::string("T2A data file is missing: ") + name;
            return false;
        }
    }

    fs_error.clear();
    const auto client_size = std::filesystem::file_size(root / "client.exe", fs_error);
    if (fs_error || client_size != kT2AClientExeBytes) {
        error = "T2A client.exe byte-size gate failed; expected 896000 bytes.";
        return false;
    }

    if (read_first_line(root / ".nougat-t2a-source-sha256") != kT2ASourceArchiveSha256) {
        error = "T2A source archive identity marker does not match the approved 1.25.35 package.";
        return false;
    }
    if (read_first_line(root / ".nougat-t2a-client-version") != kT2AClientVersion) {
        error = "T2A client version marker is not 1.25.35.";
        return false;
    }

    error.clear();
    return true;
}

bool prepare_t2a_login_config(const std::string& path,
                              const std::string& host,
                              std::uint16_t port,
                              std::string& error) {
    std::string validation;
    if (!validate_t2a_data_path(path, validation)) {
        error = validation;
        return false;
    }
    if (host.empty() || port == 0) {
        error = "T2A login endpoint is invalid.";
        return false;
    }

    const std::filesystem::path root(path);
    const std::filesystem::path login = root / "login.cfg";
    const std::filesystem::path temporary = root / ".login.cfg.nougat.tmp";
    std::ostringstream wanted;
    wanted << "; Nougat Play Portal managed local T2A shard\n"
           << "LoginServer=" << host << ',' << port << "\n";
    const std::string content = wanted.str();

    {
        std::ifstream current(login, std::ios::binary);
        if (current) {
            std::ostringstream stream;
            stream << current.rdbuf();
            if (stream.str() == content) {
                error.clear();
                return true;
            }
        }
    }

    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            error = "Could not prepare the managed T2A login.cfg.";
            return false;
        }
        output << content;
        if (!output) {
            error = "Could not write the managed T2A login.cfg.";
            return false;
        }
    }

    std::error_code fs_error;
    std::filesystem::rename(temporary, login, fs_error);
    if (fs_error) {
        std::filesystem::remove(temporary);
        error = "Could not atomically replace the managed T2A login.cfg.";
        return false;
    }

    error.clear();
    return true;
}

std::vector<std::string> build_t2a_wine_command(const ClientLaunchConfig& config) {
    if (config.wine_executable.empty()) throw std::invalid_argument("Wine executable is required");
    if (config.client_executable.empty()) throw std::invalid_argument("T2A client.exe is required");
    if (config.uo_data_path.empty()) throw std::invalid_argument("Ultima Online data path is required");
    if (config.wine_prefix.empty()) throw std::invalid_argument("T2A Wine prefix is required");

    // The real OSI T2A client reads its server endpoint from login.cfg.
    // No account name or password is ever placed on the process command line.
    return {config.wine_executable, config.client_executable};
}

}  // namespace nougat::games::uo
