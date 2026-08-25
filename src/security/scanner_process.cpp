#include "scanner_process.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>

namespace reddmedia::security {

ScannerProcess::~ScannerProcess() { cancel(); }

bool ScannerProcess::running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pid_ > 0;
}

void ScannerProcess::cancel() {
    pid_t pgid = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cancel_requested_ = true;
        pgid = pgid_;
    }
    if (pgid <= 0) return;

    (void)::kill(-pgid, SIGTERM);
    for (int i = 0; i < 20; ++i) {
        if (::kill(-pgid, 0) != 0 && errno == ESRCH) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    (void)::kill(-pgid, SIGKILL);
}

ScannerRunResult ScannerProcess::run(const std::string& program,
                                     const std::vector<std::string>& arguments,
                                     const LineCallback& on_line) {
    ScannerRunResult result;
    int output_pipe[2] = {-1, -1};
    if (::pipe(output_pipe) != 0) return result;

    const pid_t child = ::fork();
    if (child < 0) {
        ::close(output_pipe[0]);
        ::close(output_pipe[1]);
        return result;
    }

    if (child == 0) {
        ::close(output_pipe[0]);
        if (::setpgid(0, 0) < 0) _exit(126);
        if (::dup2(output_pipe[1], STDOUT_FILENO) < 0 ||
            ::dup2(output_pipe[1], STDERR_FILENO) < 0) {
            _exit(126);
        }
        ::close(output_pipe[1]);

        std::vector<char*> argv;
        argv.reserve(arguments.size() + 2U);
        argv.push_back(const_cast<char*>(program.c_str()));
        for (const std::string& argument : arguments)
            argv.push_back(const_cast<char*>(argument.c_str()));
        argv.push_back(nullptr);

        if (program.find('/') != std::string::npos)
            ::execv(program.c_str(), argv.data());
        else
            ::execvp(program.c_str(), argv.data());
        _exit(127);
    }

    ::close(output_pipe[1]);
    // Close the fork/cancel race by establishing the child process group from
    // the parent too. EACCES is harmless if exec won the race.
    if (::setpgid(child, child) != 0 && errno != EACCES) {
        (void)::kill(child, SIGTERM);
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pid_ = child;
        pgid_ = child;
        cancel_requested_ = false;
    }
    result.started = true;

    FILE* stream = ::fdopen(output_pipe[0], "r");
    if (stream) {
        char buffer[8192];
        while (::fgets(buffer, sizeof(buffer), stream)) {
            std::string line(buffer);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
                line.pop_back();
            if (on_line) on_line(line);
        }
        ::fclose(stream);
    } else {
        ::close(output_pipe[0]);
    }

    int status = 0;
    pid_t waited;
    do { waited = ::waitpid(child, &status, 0); }
    while (waited < 0 && errno == EINTR);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        result.cancelled = cancel_requested_;
        if (pid_ == child) {
            pid_ = -1;
            pgid_ = -1;
        }
    }

    if (waited == child) {
        if (WIFEXITED(status)) result.exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) {
            result.exit_code = 128 + WTERMSIG(status);
            if (WTERMSIG(status) == SIGTERM || WTERMSIG(status) == SIGKILL)
                result.cancelled = true;
        }
    }
    return result;
}

}  // namespace reddmedia::security
