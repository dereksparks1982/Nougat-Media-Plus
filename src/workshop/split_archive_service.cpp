#include "split_archive_service.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace nougat::workshop {
namespace {

std::string read_all(int fd) {
    std::string output;
    std::array<char, 8192> buffer{};
    for (;;) {
        const ssize_t count = ::read(fd, buffer.data(), buffer.size());
        if (count > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(count));
            continue;
        }
        if (count < 0 && errno == EINTR) {
            continue;
        }
        break;
    }
    return output;
}

} // namespace

SplitArchiveService::SplitArchiveService(fs::path worker_script)
    : worker_script_(std::move(worker_script)) {}

bool SplitArchiveService::available() const {
    std::error_code ec;
    return fs::is_regular_file(worker_script_, ec) && !ec;
}

CommandResult SplitArchiveService::inspect(const fs::path& source) const {
    return run({"inspect", source.string(), "--json"});
}

CommandResult SplitArchiveService::split_by_max_size(
    const fs::path& source,
    const fs::path& output_directory,
    std::uint64_t max_part_bytes,
    const std::string& archive_name) const {
    std::vector<std::string> args = {
        "split", source.string(), "--output", output_directory.string(),
        "--max-part-bytes", std::to_string(max_part_bytes), "--json"
    };
    if (!archive_name.empty()) {
        args.emplace_back("--name");
        args.push_back(archive_name);
    }
    return run(args);
}

CommandResult SplitArchiveService::split_by_part_count(
    const fs::path& source,
    const fs::path& output_directory,
    unsigned part_count,
    const std::string& archive_name) const {
    std::vector<std::string> args = {
        "split", source.string(), "--output", output_directory.string(),
        "--parts", std::to_string(part_count), "--json"
    };
    if (!archive_name.empty()) {
        args.emplace_back("--name");
        args.push_back(archive_name);
    }
    return run(args);
}

CommandResult SplitArchiveService::verify(const fs::path& manifest_or_part) const {
    return run({"verify", manifest_or_part.string(), "--json"});
}

CommandResult SplitArchiveService::reassemble(
    const fs::path& manifest_or_part,
    const fs::path& output_directory) const {
    return run({
        "reassemble", manifest_or_part.string(),
        "--output", output_directory.string(), "--json"
    });
}

CommandResult SplitArchiveService::run(const std::vector<std::string>& arguments) const {
    CommandResult result;
    if (!available()) {
        result.exit_code = 127;
        result.standard_error = "Nougat Workshop split archive worker is missing: " + worker_script_.string();
        return result;
    }

    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};
    if (::pipe(stdout_pipe) != 0 || ::pipe(stderr_pipe) != 0) {
        if (stdout_pipe[0] >= 0) ::close(stdout_pipe[0]);
        if (stdout_pipe[1] >= 0) ::close(stdout_pipe[1]);
        if (stderr_pipe[0] >= 0) ::close(stderr_pipe[0]);
        if (stderr_pipe[1] >= 0) ::close(stderr_pipe[1]);
        result.exit_code = 126;
        result.standard_error = std::string("Unable to create Workshop worker pipes: ") + std::strerror(errno);
        return result;
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(stdout_pipe[0]); ::close(stdout_pipe[1]);
        ::close(stderr_pipe[0]); ::close(stderr_pipe[1]);
        result.exit_code = 126;
        result.standard_error = std::string("Unable to start Workshop worker: ") + std::strerror(errno);
        return result;
    }

    if (pid == 0) {
        ::dup2(stdout_pipe[1], STDOUT_FILENO);
        ::dup2(stderr_pipe[1], STDERR_FILENO);
        ::close(stdout_pipe[0]); ::close(stdout_pipe[1]);
        ::close(stderr_pipe[0]); ::close(stderr_pipe[1]);

        std::vector<std::string> values;
        values.reserve(arguments.size() + 2);
        values.emplace_back("python3");
        values.push_back(worker_script_.string());
        values.insert(values.end(), arguments.begin(), arguments.end());

        std::vector<char*> argv;
        argv.reserve(values.size() + 1);
        for (std::string& value : values) {
            argv.push_back(value.data());
        }
        argv.push_back(nullptr);
        ::execvp("python3", argv.data());
        _exit(127);
    }

    ::close(stdout_pipe[1]);
    ::close(stderr_pipe[1]);

    // The worker emits small structured status output. Reading stdout and stderr
    // sequentially is safe for that contract and avoids shell invocation entirely.
    result.standard_output = read_all(stdout_pipe[0]);
    result.standard_error = read_all(stderr_pipe[0]);
    ::close(stdout_pipe[0]);
    ::close(stderr_pipe[0]);

    int status = 0;
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = 126;
    }
    return result;
}

} // namespace nougat::workshop
