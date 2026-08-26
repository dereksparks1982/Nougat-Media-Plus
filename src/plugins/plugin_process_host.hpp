#pragma once

#include "plugin_registry.hpp"

#include <string>
#include <sys/types.h>

namespace nougat::plugins {

class PluginProcessHost {
public:
    PluginProcessHost() = default;
    ~PluginProcessHost();

    PluginProcessHost(const PluginProcessHost&) = delete;
    PluginProcessHost& operator=(const PluginProcessHost&) = delete;

    // Launch one plugin as a child process. The plugin creates its UI as an X11
    // child of parent_xid. Only one plugin process is hosted by an instance.
    bool start(const PluginManifest& plugin,
               unsigned long parent_xid,
               int width,
               int height,
               std::string& error);

    // Ask the active plugin to stop. Escalates only that plugin process group
    // when it does not exit after a short grace period.
    void stop();

    // Reap a plugin that exited on its own. Returns true while it is alive.
    bool poll(std::string* status = nullptr);

    bool running() const { return child_ > 1; }
    pid_t pid() const { return child_; }
    const std::string& plugin_id() const { return plugin_id_; }

private:
    pid_t child_ = -1;
    pid_t process_group_ = -1;
    std::string plugin_id_;
};

} // namespace nougat::plugins
