#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace nougat::workshop {

struct CommandResult {
    int exit_code = -1;
    std::string standard_output;
    std::string standard_error;

    bool ok() const { return exit_code == 0; }
};

class SplitArchiveService {
public:
    explicit SplitArchiveService(std::filesystem::path worker_script);

    const std::filesystem::path& worker_script() const { return worker_script_; }
    bool available() const;

    CommandResult inspect(const std::filesystem::path& source) const;
    CommandResult split_by_max_size(
        const std::filesystem::path& source,
        const std::filesystem::path& output_directory,
        std::uint64_t max_part_bytes,
        const std::string& archive_name = {}) const;
    CommandResult split_by_part_count(
        const std::filesystem::path& source,
        const std::filesystem::path& output_directory,
        unsigned part_count,
        const std::string& archive_name = {}) const;
    CommandResult verify(const std::filesystem::path& manifest_or_part) const;
    CommandResult reassemble(
        const std::filesystem::path& manifest_or_part,
        const std::filesystem::path& output_directory) const;

private:
    CommandResult run(const std::vector<std::string>& arguments) const;
    std::filesystem::path worker_script_;
};

} // namespace nougat::workshop
