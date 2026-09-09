#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nougat::games::uo {

struct ClientLaunchConfig {
    std::string wine_executable;
    std::string client_executable;
    std::string uo_data_path;
    std::string wine_prefix;
    std::string host{"127.0.0.1"};
    std::uint16_t port{2593};
};

[[nodiscard]] const char* t2a_client_version();
[[nodiscard]] const char* t2a_source_archive_sha256();
[[nodiscard]] std::string resolve_wine_executable();
[[nodiscard]] std::string resolve_t2a_client_executable();
[[nodiscard]] std::string resolve_uo_data_path();
[[nodiscard]] std::string resolve_t2a_wine_prefix();
[[nodiscard]] bool validate_t2a_data_path(const std::string& path, std::string& error);
[[nodiscard]] bool prepare_t2a_login_config(const std::string& path,
                                            const std::string& host,
                                            std::uint16_t port,
                                            std::string& error);
[[nodiscard]] std::vector<std::string> build_t2a_wine_command(const ClientLaunchConfig& config);

}  // namespace nougat::games::uo
