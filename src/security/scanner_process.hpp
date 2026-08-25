#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <sys/types.h>

namespace reddmedia::security {

struct ScannerRunResult {
    int exit_code = -1;
    bool cancelled = false;
    bool started = false;
};

class ScannerProcess {
public:
    using LineCallback = std::function<void(const std::string&)>;
    ScannerProcess() = default;
    ~ScannerProcess();
    ScannerProcess(const ScannerProcess&) = delete;
    ScannerProcess& operator=(const ScannerProcess&) = delete;

    ScannerRunResult run(const std::string& program,
                         const std::vector<std::string>& arguments,
                         const LineCallback& on_line);
    void cancel();
    bool running() const;

private:
    mutable std::mutex mutex_;
    pid_t pid_ = -1;
    pid_t pgid_ = -1;
    bool cancel_requested_ = false;
};

}  // namespace reddmedia::security
