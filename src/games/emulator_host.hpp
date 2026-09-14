#pragma once

#include <X11/Xlib.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nougat::games {

enum class HostState {
    Idle,
    WaitingForWindow,
    Embedded,
    Failed,
    Exited
};

struct LaunchRequest {
    std::vector<std::string> argv;
    std::vector<std::pair<std::string, std::string>> environment;
    std::string backend;
    std::string title;
    std::string log_path;
    // Optional per-title working directory. Ordinary emulators leave this empty
    // so their accepted launch behavior is unchanged; bundled native games may
    // use their own directory for relative assets.
    std::string working_directory;
    int window_timeout_ms = 45000;
    bool overlay_window = false;
};

struct HostEvent {
    HostState state = HostState::Idle;
    bool changed = false;
    std::string message;
};

class EmulatorHost {
public:
    EmulatorHost();
    ~EmulatorHost();

    EmulatorHost(const EmulatorHost&) = delete;
    EmulatorHost& operator=(const EmulatorHost&) = delete;

    bool start(Display* display,
               Window shell_window,
               Window parent_window,
               int width,
               int height,
               const LaunchRequest& request,
               std::string& error);

    HostEvent poll();
    void resize(int width, int height);
    void focus();
    bool pointer_position(int& x, int& y) const;
    bool send_key(KeySym keysym);
    void stop();

    bool active() const;
    bool embedded() const;
    bool owns_window(Window window) const;
    HostState state() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace nougat::games
