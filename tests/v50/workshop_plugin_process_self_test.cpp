#include "plugins/plugin_process_host.hpp"
#include "plugins/plugin_registry.hpp"

#include <X11/Xlib.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace {

bool set_path_environment(const char* name, const fs::path& path) {
    return ::setenv(name, path.c_str(), 1) == 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: workshop_plugin_process_self_test <plugin-root>\n";
        return 2;
    }

    const fs::path plugin_root = fs::absolute(argv[1]);
    const fs::path scratch = plugin_root.parent_path() / "nougat-host-state";
    if (!set_path_environment("NOUGAT_PLUGIN_ROOT", plugin_root) ||
        !set_path_environment("XDG_DATA_HOME", scratch / "data") ||
        !set_path_environment("XDG_CONFIG_HOME", scratch / "config") ||
        !set_path_environment("XDG_CACHE_HOME", scratch / "cache") ||
        !set_path_environment("XDG_STATE_HOME", scratch / "state")) {
        std::cerr << "FAIL: unable to establish isolated plugin test paths\n";
        return 1;
    }

    const nougat::plugins::PluginScanResult scan = nougat::plugins::scan_installed_plugins();
    const nougat::plugins::PluginManifest* workshop = nullptr;
    for (const auto& plugin : scan.plugins) {
        if (plugin.id == "workshop") {
            workshop = &plugin;
            break;
        }
    }
    if (workshop == nullptr || workshop->runtime_kind != "x11-process" ||
        workshop->entrypoint.filename() != "nougat-workshop-plugin") {
        std::cerr << "FAIL: strict plugin registry did not accept Workshop\n";
        for (const auto& diagnostic : scan.diagnostics) std::cerr << diagnostic << '\n';
        return 1;
    }

    Display* display = XOpenDisplay(nullptr);
    if (display == nullptr) {
        std::cerr << "FAIL: no X11 display for Workshop process-host test\n";
        return 1;
    }
    const int screen = DefaultScreen(display);
    Window parent = XCreateSimpleWindow(display, RootWindow(display, screen), 20, 20, 720, 480,
                                        0, BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, parent);
    XSync(display, False);

    nougat::plugins::PluginProcessHost host;
    std::string error;
    if (!host.start(*workshop, static_cast<unsigned long>(parent), 720, 480, error)) {
        std::cerr << "FAIL: Workshop process host could not launch plugin: " << error << '\n';
        XDestroyWindow(display, parent);
        XCloseDisplay(display);
        return 1;
    }

    bool child_seen = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline && host.running()) {
        Window root_return = 0;
        Window parent_return = 0;
        Window* children = nullptr;
        unsigned int child_count = 0;
        if (XQueryTree(display, parent, &root_return, &parent_return, &children, &child_count) != 0) {
            child_seen = child_count > 0;
        }
        if (children != nullptr) XFree(children);
        if (child_seen) break;
        std::string status;
        (void)host.poll(&status);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!child_seen || !host.running()) {
        std::cerr << "FAIL: Workshop process did not create a hosted X11 child surface\n";
        host.stop();
        XDestroyWindow(display, parent);
        XCloseDisplay(display);
        return 1;
    }

    host.stop();
    if (host.running()) {
        std::cerr << "FAIL: Workshop plugin process remained alive after scoped host shutdown\n";
        XDestroyWindow(display, parent);
        XCloseDisplay(display);
        return 1;
    }

    XDestroyWindow(display, parent);
    XCloseDisplay(display);

    std::error_code ec;
    fs::remove_all(plugin_root / "workshop", ec);
    if (ec) {
        std::cerr << "FAIL: unable to remove Workshop test plugin folder: " << ec.message() << '\n';
        return 1;
    }
    const nougat::plugins::PluginScanResult after_remove = nougat::plugins::scan_installed_plugins();
    for (const auto& plugin : after_remove.plugins) {
        if (plugin.id == "workshop") {
            std::cerr << "FAIL: Workshop remained discoverable after its physical folder was removed\n";
            return 1;
        }
    }

    std::cout << "PASS: strict registry accepted the native Workshop x11-process plugin\n";
    std::cout << "PASS: PluginProcessHost launched Workshop into a real X11 child surface\n";
    std::cout << "PASS: scoped shutdown stopped only the Workshop process\n";
    std::cout << "PASS: removing the Workshop folder removed the feature from registry discovery\n";
    return 0;
}
