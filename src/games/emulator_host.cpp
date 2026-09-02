#include "emulator_host.hpp"

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace nougat::games {
namespace {

long long monotonic_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::mutex g_x_error_mutex;
int g_x_error_code = 0;

int trapped_x_error(Display*, XErrorEvent* event) {
    if (event) g_x_error_code = event->error_code;
    return 0;
}

class XErrorTrap {
public:
    explicit XErrorTrap(Display* display)
        : lock_(g_x_error_mutex), display_(display) {
        g_x_error_code = 0;
        old_handler_ = XSetErrorHandler(trapped_x_error);
    }

    ~XErrorTrap() {
        if (display_) XSync(display_, False);
        XSetErrorHandler(old_handler_);
    }

    bool sync_ok() {
        if (display_) XSync(display_, False);
        return g_x_error_code == 0;
    }

private:
    std::unique_lock<std::mutex> lock_;
    Display* display_ = nullptr;
    XErrorHandler old_handler_ = nullptr;
};

bool safe_window_attributes(Display* display, Window window, XWindowAttributes& attrs) {
    if (!display || !window) return false;
    XErrorTrap trap(display);
    const Status status = XGetWindowAttributes(display, window, &attrs);
    return status != 0 && trap.sync_ok();
}

bool safe_window_children(Display* display, Window window, std::vector<Window>& children_out) {
    children_out.clear();
    if (!display || !window) return false;
    Window root_return = 0;
    Window parent_return = 0;
    Window* children = nullptr;
    unsigned int child_count = 0;
    Status ok = 0;
    {
        XErrorTrap trap(display);
        ok = XQueryTree(display, window, &root_return, &parent_return,
                        &children, &child_count);
        if (!trap.sync_ok()) ok = 0;
    }
    if (ok && children) {
        children_out.assign(children, children + child_count);
    }
    if (children) XFree(children);
    return ok != 0;
}

std::string lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

// NOUGAT_V53_XENIA_EDGE_EMBED_REPAIR
// Treat Canary/Edge spelling and separator differences as one Xenia family
// while retaining the existing unrelated-window rejection gate.
std::string normalized_window_token(std::string value) {
    value = lower_ascii(value);
    std::string out;
    out.reserve(value.size());
    for (unsigned char c : value) {
        if (std::isalnum(c) != 0) {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

bool xenia_backend_family(const std::string& value) {
    return normalized_window_token(value).find("xenia") != std::string::npos;
}

std::set<Window> ewmh_client_windows(Display* display, Window root) {
    std::set<Window> out;
    if (!display || !root) return out;

    const Atom client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
    if (client_list == None) return out;

    Atom actual_type = None;
    int actual_format = 0;
    unsigned long count = 0;
    unsigned long bytes_after = 0;
    unsigned char* bytes = nullptr;
    int rc = BadWindow;
    {
        XErrorTrap trap(display);
        rc = XGetWindowProperty(display, root, client_list, 0, 8192, False,
                                XA_WINDOW, &actual_type, &actual_format,
                                &count, &bytes_after, &bytes);
        if (!trap.sync_ok()) rc = BadWindow;
    }

    if (rc == Success && bytes && actual_format == 32) {
        auto* windows = reinterpret_cast<Window*>(bytes);
        for (unsigned long i = 0; i < count; ++i) {
            if (windows[i]) out.insert(windows[i]);
        }
    }
    if (bytes) XFree(bytes);
    return out;
}

std::string safe_window_identity(Display* display, Window window) {
    if (!display || !window) return {};
    std::string identity;
    {
        XErrorTrap trap(display);
        char* name = nullptr;
        if (XFetchName(display, window, &name) != 0 && name) {
            identity.append(name);
            identity.push_back(' ');
        }
        if (name) XFree(name);
        XClassHint hint{};
        if (XGetClassHint(display, window, &hint) != 0) {
            if (hint.res_name) { identity.append(hint.res_name); identity.push_back(' '); }
            if (hint.res_class) identity.append(hint.res_class);
            if (hint.res_name) XFree(hint.res_name);
            if (hint.res_class) XFree(hint.res_class);
        }
        if (!trap.sync_ok()) return {};
    }
    return lower_ascii(identity);
}

bool mark_private_emulator_window(Display* display, Window window, Window shell, bool set_transient = true) {
    if (!display || !window) return false;
    const Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    const Atom skip_taskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
    const Atom skip_pager = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
    const Atom states[] = {skip_taskbar, skip_pager};
    XErrorTrap trap(display);
    XChangeProperty(display, window, wm_state, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(states), 2);
    if (set_transient && shell) XSetTransientForHint(display, window, shell);
    return trap.sync_ok();
}

std::vector<Window> root_candidates(Display* display, Window root) {
    std::vector<Window> out;
    if (!display || !root) return out;

    const Atom client_list = XInternAtom(display, "_NET_CLIENT_LIST", True);
    if (client_list != None) {
        Atom actual_type = None;
        int actual_format = 0;
        unsigned long count = 0;
        unsigned long bytes_after = 0;
        unsigned char* bytes = nullptr;
        int rc = BadWindow;
        {
            XErrorTrap trap(display);
            rc = XGetWindowProperty(display, root, client_list, 0, 8192, False,
                                    XA_WINDOW, &actual_type, &actual_format,
                                    &count, &bytes_after, &bytes);
            if (!trap.sync_ok()) rc = BadWindow;
        }
        if (rc == Success && bytes && actual_format == 32) {
            auto* windows = reinterpret_cast<Window*>(bytes);
            for (unsigned long i = 0; i < count; ++i) out.push_back(windows[i]);
        }
        if (bytes) XFree(bytes);
    }

    Window root_return = 0;
    Window parent_return = 0;
    Window* children = nullptr;
    unsigned int child_count = 0;
    Status ok = 0;
    {
        XErrorTrap trap(display);
        ok = XQueryTree(display, root, &root_return, &parent_return,
                        &children, &child_count);
        if (!trap.sync_ok()) ok = 0;
    }
    if (ok) {
        for (unsigned int i = 0; i < child_count; ++i) out.push_back(children[i]);
    }
    if (children) XFree(children);

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::vector<Window> window_tree_candidates(Display* display, Window root) {
    std::vector<Window> out = root_candidates(display, root);
    std::vector<std::pair<Window, int>> queue;
    queue.reserve(out.size() * 2U + 16U);
    std::set<Window> seen;
    for (Window window : out) {
        if (window && seen.insert(window).second) queue.push_back({window, 0});
    }
    constexpr std::size_t kMaxCandidates = 4096U;
    for (std::size_t index = 0; index < queue.size() && queue.size() < kMaxCandidates; ++index) {
        const Window current = queue[index].first;
        const int depth = queue[index].second;
        if (depth >= 8) continue;
        std::vector<Window> children;
        if (!safe_window_children(display, current, children)) continue;
        for (Window child : children) {
            if (!child || !seen.insert(child).second) continue;
            out.push_back(child);
            if (queue.size() < kMaxCandidates) queue.push_back({child, depth + 1});
        }
    }
    return out;
}

bool is_descendant_window(Display* display, Window candidate, Window ancestor) {
    if (!display || !candidate || !ancestor) return false;
    if (candidate == ancestor) return true;

    Window current = candidate;
    for (int depth = 0; depth < 32 && current; ++depth) {
        Window root_return = 0;
        Window parent_return = 0;
        Window* children = nullptr;
        unsigned int child_count = 0;
        Status ok = 0;
        {
            XErrorTrap trap(display);
            ok = XQueryTree(display, current, &root_return, &parent_return,
                            &children, &child_count);
            if (!trap.sync_ok()) ok = 0;
        }
        if (children) XFree(children);
        if (!ok || !parent_return || parent_return == current) return false;
        if (parent_return == ancestor) return true;
        current = parent_return;
    }
    return false;
}

pid_t x_window_pid(Display* display, Window window) {
    if (!display || !window) return -1;
    const Atom pid_atom = XInternAtom(display, "_NET_WM_PID", True);
    if (pid_atom == None) return -1;

    Atom actual_type = None;
    int actual_format = 0;
    unsigned long count = 0;
    unsigned long bytes_after = 0;
    unsigned char* bytes = nullptr;
    int rc = BadWindow;
    {
        XErrorTrap trap(display);
        rc = XGetWindowProperty(display, window, pid_atom, 0, 1, False,
                                XA_CARDINAL, &actual_type, &actual_format,
                                &count, &bytes_after, &bytes);
        if (!trap.sync_ok()) rc = BadWindow;
    }

    pid_t result = -1;
    if (rc == Success && bytes && count >= 1 && actual_format == 32) {
        const auto raw = *reinterpret_cast<unsigned long*>(bytes);
        if (raw > 0) result = static_cast<pid_t>(raw);
    }
    if (bytes) XFree(bytes);
    return result;
}

pid_t proc_parent_pid(pid_t pid) {
    if (pid <= 1) return -1;
    std::ifstream in("/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    if (!in || !std::getline(in, line)) return -1;
    const std::size_t right = line.rfind(')');
    if (right == std::string::npos || right + 2 >= line.size()) return -1;
    std::istringstream fields(line.substr(right + 2));
    char state = '\0';
    pid_t parent = -1;
    fields >> state >> parent;
    if (!fields || state == '\0') return -1;
    return parent;
}

bool process_descends_from(pid_t pid, pid_t ancestor) {
    if (pid <= 1 || ancestor <= 1) return false;
    pid_t current = pid;
    for (int depth = 0; depth < 48 && current > 1; ++depth) {
        if (current == ancestor) return true;
        const pid_t parent = proc_parent_pid(current);
        if (parent <= 1 || parent == current) return false;
        current = parent;
    }
    return false;
}

bool process_alive(pid_t pid) {
    if (pid <= 1) return false;
    if (kill(pid, 0) == 0) return true;
    return errno == EPERM;
}

std::string state_message(HostState state,
                          const std::string& title,
                          const std::string& backend,
                          const std::string& detail = {}) {
    const std::string pretty_backend = backend.empty() ? "emulator" : backend;
    switch (state) {
        case HostState::WaitingForWindow:
            return "Starting " + title + " with " + pretty_backend + " inside Nougat...";
        case HostState::Embedded:
            return "Playing " + title + " inside Nougat with " + pretty_backend + ".";
        case HostState::Failed:
            return detail.empty() ? "The emulator could not be embedded inside Nougat." : detail;
        case HostState::Exited:
            return detail.empty() ? title + " closed." : detail;
        case HostState::Idle:
            return {};
    }
    return {};
}

}  // namespace

struct EmulatorHost::Impl {
    Display* display = nullptr;
    Window shell = 0;
    Window parent = 0;
    Window root = 0;
    Window embedded_window = 0;
    int width = 1;
    int height = 1;
    pid_t child = -1;
    pid_t process_group = -1;
    HostState state = HostState::Idle;
    HostState reported_state = HostState::Idle;
    std::string backend;
    std::string title;
    long long started_ms = 0;
    long long last_scan_ms = 0;
    long long last_geometry_ms = 0;
    int timeout_ms = 45000;
    std::set<Window> preexisting;

    void clear_runtime_state() {
        embedded_window = 0;
        child = -1;
        process_group = -1;
        preexisting.clear();
        started_ms = 0;
        last_scan_ms = 0;
        last_geometry_ms = 0;
    }

    bool window_still_exists(Window window) const {
        XWindowAttributes attrs{};
        return safe_window_attributes(display, window, attrs);
    }

bool force_geometry() {
        if (!display || !embedded_window) return false;
        XWindowAttributes attrs{};
        if (!safe_window_attributes(display, embedded_window, attrs)) return false;

        const int target_width = std::max(1, width);
        const int target_height = std::max(1, height);
        if (attrs.x == 0 && attrs.y == 0 &&
            attrs.width == target_width && attrs.height == target_height) {
            return true;
        }

        XErrorTrap trap(display);
        XMoveResizeWindow(display, embedded_window, 0, 0,
                          static_cast<unsigned int>(target_width),
                          static_cast<unsigned int>(target_height));
        return trap.sync_ok();
    }

    bool embed(Window window) {
        if (!display || !parent || !window) return false;

        XWindowAttributes attrs{};
        if (!safe_window_attributes(display, window, attrs)) return false;

        // A cooperating runtime may already have attached itself to Nougat
        // before its first map / swapchain creation. Adopt it without a second
        // XReparentWindow transaction.
        if (is_descendant_window(display, window, parent)) {
            embedded_window = window;
            last_geometry_ms = monotonic_ms();
            (void)force_geometry();

            XErrorTrap trap(display);
            XRaiseWindow(display, window);
            XSetInputFocus(display, window, RevertToParent, CurrentTime);
            (void)trap.sync_ok();

            state = HostState::Embedded;
            return true;
        }

        // Remove the emulator from the desktop/window-manager surface before it
        // becomes a Nougat child. Xenia Edge uses a wxGTK top-level client.
        // Do not mark that client transient to Nougat before reparenting because
        // the GTK/X11 window manager relationship may otherwise be rebuilt while
        // the client is moving into the native Games viewport.
        const bool xenia_backend = xenia_backend_family(backend);
        if (!mark_private_emulator_window(display, window, shell, !xenia_backend)) return false;

        // Perform the structural move first and synchronize it before focus.
        // This keeps wxGTK focus handling out of the reparent transaction.
        {
            XErrorTrap trap(display);
            XUnmapWindow(display, window);
            XAddToSaveSet(display, window);
            XSetWindowBorderWidth(display, window, 0);
            XReparentWindow(display, window, parent, 0, 0);
            XMoveResizeWindow(display, window, 0, 0,
                              static_cast<unsigned int>(std::max(1, width)),
                              static_cast<unsigned int>(std::max(1, height)));
            XMapRaised(display, window);
            if (!trap.sync_ok()) return false;
        }
        {
            XErrorTrap trap(display);
            XSetInputFocus(display, window, RevertToParent, CurrentTime);
            (void)trap.sync_ok();
        }

        embedded_window = window;
        last_geometry_ms = monotonic_ms();
        state = HostState::Embedded;
        return true;
    }

    Window choose_candidate() {
        if (!display || !root) return 0;
        const std::vector<Window> candidates = window_tree_candidates(display, root);

        struct Scored {
            Window window = 0;
            long long score = -1;
        } best;

        const std::string backend_hint = lower_ascii(backend);
        const std::string backend_token = normalized_window_token(backend_hint);
        const bool xenia_backend = xenia_backend_family(backend_hint);
        const std::set<Window> managed_clients =
            xenia_backend ? ewmh_client_windows(display, root) : std::set<Window>{};
        const long long age = monotonic_ms() - started_ms;
        for (Window window : candidates) {
            if (!window || window == shell || window == parent) continue;
            if (preexisting.count(window) != 0U) continue;
            const bool preembedded =
                is_descendant_window(display, window, parent);
            if (is_descendant_window(display, window, shell) && !preembedded)
                continue;

            XWindowAttributes attrs{};
            if (!safe_window_attributes(display, window, attrs)) continue;
            if (attrs.map_state != IsViewable) continue;
            if (attrs.width < 160 || attrs.height < 100) continue;

            // Xenia Edge's wxGTK hierarchy contains multiple mapped X11 child
            // windows. Only capture the EWMH client window itself when GNOME
            // publishes _NET_CLIENT_LIST. Reparenting an internal wx child can
            // crash the host or leave a detached top-level frame behind.
            if (xenia_backend && !managed_clients.empty() &&
                managed_clients.count(window) == 0U && !preembedded) {
                continue;
            }

            const pid_t owner_pid = x_window_pid(display, window);
            const bool process_owned = owner_pid > 1 && process_descends_from(owner_pid, child);
            const std::string identity = safe_window_identity(display, window);
            const std::string identity_token = normalized_window_token(identity);
            const bool direct_backend_match =
                !backend_token.empty() &&
                identity_token.find(backend_token) != std::string::npos;
            const bool xenia_family_match =
                xenia_backend &&
                identity_token.find("xenia") != std::string::npos;
            const bool backend_owned = direct_backend_match || xenia_family_match;

            // Do not ever grab an arbitrary unrelated desktop window. The real
            // emulator client must either belong to the launched process tree or
            // identify itself as the requested backend. The short age allowance
            // lets wrappers publish their PID/class shortly after mapping.
            if (!process_owned && !backend_owned && !preembedded) {
                if (age < 1500) continue;
                continue;
            }

            long long score = static_cast<long long>(attrs.width) * attrs.height;
            if (preembedded) score += 5000000000LL;
            if (process_owned) score += 2200000000LL;
            if (backend_owned) score += 900000000LL;
            // Edge's actual game/render client includes the graphics backend in
            // its title. Prefer it over the library/front-end frame if both are
            // briefly visible during the transition into gameplay.
            if (xenia_backend && identity.find("vulkan") != std::string::npos)
                score += 1800000000LL;
            if (xenia_backend && identity.find("xenia") != std::string::npos)
                score += 400000000LL;
            if (attrs.override_redirect) score -= 10000000LL;

            // A WM decoration frame may contain the real SDL/X11 client. The
            // process-owned descendant wins over that frame even when the frame
            // has a larger rectangle or Stella text in its title.
            if (score > best.score) best = {window, score};
        }
        return best.window;
    }
};

EmulatorHost::EmulatorHost() : impl_(std::make_unique<Impl>()) {}
EmulatorHost::~EmulatorHost() { stop(); }

bool EmulatorHost::start(Display* display,
                         Window shell_window,
                         Window parent_window,
                         int width,
                         int height,
                         const LaunchRequest& request,
                         std::string& error) {
    error.clear();
    stop();

    if (!display || !shell_window || !parent_window) {
        error = "Nougat emulator host does not have a valid X11 player surface.";
        return false;
    }
    if (request.argv.empty() || request.argv.front().empty()) {
        error = "No emulator executable was selected.";
        return false;
    }

    impl_->display = display;
    impl_->shell = shell_window;
    impl_->parent = parent_window;
    impl_->root = DefaultRootWindow(display);
    impl_->width = std::max(1, width);
    impl_->height = std::max(1, height);
    impl_->backend = request.backend;
    impl_->title = request.title;
    impl_->timeout_ms = std::max(5000, request.window_timeout_ms);
    impl_->preexisting.clear();
    for (Window window : window_tree_candidates(display, impl_->root)) impl_->preexisting.insert(window);

    const pid_t child = fork();
    if (child < 0) {
        error = "Nougat could not create the emulator process.";
        impl_->state = HostState::Failed;
        return false;
    }

    if (child == 0) {
        setpgid(0, 0);

        // NOUGAT_V53_XENIA_PRE_VULKAN_EMBED
        // Let cooperating native emulator runtimes attach before their first
        // top-level map / graphics swapchain creation.
        const std::string nougat_embed_xid =
            std::to_string(static_cast<unsigned long long>(parent_window));
        setenv("NOUGAT_EMBED_XID", nougat_embed_xid.c_str(), 1);

        for (const auto& entry : request.environment) {
            if (!entry.first.empty()) setenv(entry.first.c_str(), entry.second.c_str(), 1);
        }

        if (!request.log_path.empty()) {
            const int fd = open(request.log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                if (fd > STDERR_FILENO) close(fd);
            }
        }

        std::vector<char*> argv;
        argv.reserve(request.argv.size() + 1U);
        for (const std::string& value : request.argv) argv.push_back(const_cast<char*>(value.c_str()));
        argv.push_back(nullptr);
        execvp(argv.front(), argv.data());
        _exit(127);
    }

    setpgid(child, child);
    impl_->child = child;
    impl_->process_group = child;
    impl_->embedded_window = 0;
    impl_->started_ms = monotonic_ms();
    impl_->last_scan_ms = 0;
    impl_->last_geometry_ms = 0;
    impl_->state = HostState::WaitingForWindow;
    impl_->reported_state = HostState::Idle;
    return true;
}

HostEvent EmulatorHost::poll() {
    HostEvent event;
    event.state = impl_->state;

    if (impl_->state == HostState::Idle ||
        impl_->state == HostState::Failed ||
        impl_->state == HostState::Exited) {
        if (impl_->reported_state != impl_->state) {
            event.changed = true;
            event.message = state_message(impl_->state, impl_->title, impl_->backend);
            impl_->reported_state = impl_->state;
        }
        return event;
    }

    if (impl_->state == HostState::Embedded) {
        if (!impl_->embedded_window || !impl_->window_still_exists(impl_->embedded_window)) {
            impl_->embedded_window = 0;
            impl_->state = HostState::Exited;
        } else {
            int status = 0;
            if (impl_->child > 1) {
                const pid_t waited = waitpid(impl_->child, &status, WNOHANG);
                if (waited == impl_->child) impl_->child = -1;
            }

            const long long now = monotonic_ms();
            if (now - impl_->last_geometry_ms >= 150) {
                impl_->last_geometry_ms = now;
                if (!impl_->force_geometry()) {
                    impl_->embedded_window = 0;
                    impl_->state = HostState::Exited;
                }
            }
        }
    }

    if (impl_->state == HostState::WaitingForWindow) {
        int status = 0;
        if (impl_->child > 1) {
            const pid_t waited = waitpid(impl_->child, &status, WNOHANG);
            if (waited == impl_->child) {
                impl_->child = -1;
                if (impl_->process_group <= 1 || kill(-impl_->process_group, 0) != 0) {
                    impl_->state = HostState::Failed;
                    event.message = "The emulator exited before Nougat could embed its game window.";
                }
            }
        }

        const long long now = monotonic_ms();
        if (impl_->state == HostState::WaitingForWindow && now - impl_->last_scan_ms >= 20) {
            impl_->last_scan_ms = now;
            const Window candidate = impl_->choose_candidate();
            if (candidate && impl_->embed(candidate)) {
                event.message = state_message(HostState::Embedded, impl_->title, impl_->backend);
            }
        }

        if (impl_->state == HostState::WaitingForWindow &&
            now - impl_->started_ms > impl_->timeout_ms) {
            stop();
            impl_->state = HostState::Failed;
            event.message =
                "The emulator started but did not expose an embeddable X11/XWayland game window. "
                "Nougat refused to fall back to a separate desktop window.";
        }
    }

    event.state = impl_->state;
    if (impl_->reported_state != impl_->state) {
        event.changed = true;
        if (event.message.empty()) {
            event.message = state_message(impl_->state, impl_->title, impl_->backend);
        }
        impl_->reported_state = impl_->state;
    }
    return event;
}

void EmulatorHost::resize(int width, int height) {
    impl_->width = std::max(1, width);
    impl_->height = std::max(1, height);
    if (!impl_->display || !impl_->embedded_window) return;

    impl_->last_geometry_ms = monotonic_ms();
    (void)impl_->force_geometry();
}

void EmulatorHost::focus() {
    if (!impl_->display || !impl_->embedded_window) return;

    XErrorTrap trap(impl_->display);
    XRaiseWindow(impl_->display, impl_->embedded_window);
    XSetInputFocus(impl_->display, impl_->embedded_window,
                   RevertToParent, CurrentTime);
    (void)trap.sync_ok();
}

bool EmulatorHost::pointer_position(int& x, int& y) const {
    x = 0;
    y = 0;
    if (!impl_->display || !impl_->embedded_window) return false;
    Window root_return = 0;
    Window child_return = 0;
    int root_x = 0;
    int root_y = 0;
    unsigned int mask = 0;
    XErrorTrap trap(impl_->display);
    const Bool ok = XQueryPointer(impl_->display, impl_->embedded_window,
                                  &root_return, &child_return,
                                  &root_x, &root_y, &x, &y, &mask);
    return ok != False && trap.sync_ok();
}

bool EmulatorHost::send_key(KeySym keysym) {
    if (!impl_->display || !impl_->embedded_window || keysym == NoSymbol) return false;
    const KeyCode code = XKeysymToKeycode(impl_->display, keysym);
    if (code == 0) return false;

    XWindowAttributes attrs{};
    if (!safe_window_attributes(impl_->display, impl_->embedded_window, attrs)) return false;

    XKeyEvent key{};
    key.display = impl_->display;
    key.window = impl_->embedded_window;
    key.root = impl_->root;
    key.subwindow = None;
    key.time = CurrentTime;
    key.x = 1;
    key.y = 1;
    key.x_root = attrs.x + 1;
    key.y_root = attrs.y + 1;
    key.same_screen = True;
    key.keycode = code;

    XErrorTrap trap(impl_->display);
    XEvent event{};
    key.type = KeyPress;
    event.xkey = key;
    if (XSendEvent(impl_->display, impl_->embedded_window, True, KeyPressMask, &event) == 0) return false;
    key.type = KeyRelease;
    event.xkey = key;
    if (XSendEvent(impl_->display, impl_->embedded_window, True, KeyReleaseMask, &event) == 0) return false;
    XFlush(impl_->display);
    return trap.sync_ok();
}

void EmulatorHost::stop() {
    if (!impl_) return;

    const pid_t child = impl_->child;
    const pid_t process_group = impl_->process_group;
    if (process_group > 1 && (kill(-process_group, 0) == 0 || errno == EPERM)) {
        kill(-process_group, SIGTERM);
        for (int i = 0; i < 20; ++i) {
            if (kill(-process_group, 0) != 0 && errno == ESRCH) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
        if (kill(-process_group, 0) == 0 || errno == EPERM) kill(-process_group, SIGKILL);
    } else if (child > 1 && process_alive(child)) {
        kill(child, SIGTERM);
    }
    if (child > 1) {
        int status = 0;
        waitpid(child, &status, WNOHANG);
    }

    if (impl_->display && impl_->embedded_window) {
        XErrorTrap trap(impl_->display);
        XUnmapWindow(impl_->display, impl_->embedded_window);
        XRemoveFromSaveSet(impl_->display, impl_->embedded_window);
        (void)trap.sync_ok();
    }

    impl_->clear_runtime_state();
    impl_->state = HostState::Idle;
    impl_->reported_state = HostState::Idle;
}

bool EmulatorHost::active() const {
    return impl_->state == HostState::WaitingForWindow || impl_->state == HostState::Embedded;
}

bool EmulatorHost::embedded() const {
    return impl_->state == HostState::Embedded && impl_->embedded_window != 0;
}

HostState EmulatorHost::state() const {
    return impl_->state;
}

}  // namespace nougat::games
