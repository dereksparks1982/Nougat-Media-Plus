#!/usr/bin/env python3
from pathlib import Path
import sys

project = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()
path = project / "src/games/emulator_host.cpp"

if not path.is_file():
    raise SystemExit("STOP: src/games/emulator_host.cpp was not found.")

text = path.read_text(encoding="utf-8")
marker = "// NOUGAT_V53_XENIA_EDGE_EMBED_REPAIR"

if marker in text:
    print("PASS: v53 Xenia Edge host repair is already applied.")
    raise SystemExit(0)

replacements = []

needle = '''std::string lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

'''
replacement = '''std::string lower_ascii(std::string value) {
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

'''
replacements.append((needle, replacement, "identity helpers"))

needle = 'bool mark_private_emulator_window(Display* display, Window window, Window shell) {'
replacement = 'bool mark_private_emulator_window(Display* display, Window window, Window shell, bool set_transient = true) {'
replacements.append((needle, replacement, "private-window signature"))

needle = '    if (shell) XSetTransientForHint(display, window, shell);'
replacement = '    if (set_transient && shell) XSetTransientForHint(display, window, shell);'
replacements.append((needle, replacement, "transient guard"))

needle = '''        // Remove the emulator from the desktop/window-manager surface before it
        // becomes a Nougat child. The EWMH state prevents a separate GNOME
        // dock/task-switcher entry while the client is being captured.
        if (!mark_private_emulator_window(display, window, shell)) return false;

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
            XSetInputFocus(display, window, RevertToParent, CurrentTime);
            if (!trap.sync_ok()) return false;
        }
'''
replacement = '''        // Remove the emulator from the desktop/window-manager surface before it
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
'''
replacements.append((needle, replacement, "safe Xenia reparent sequence"))

needle = '''        const std::string backend_hint = lower_ascii(backend);
        const long long age = monotonic_ms() - started_ms;
'''
replacement = '''        const std::string backend_hint = lower_ascii(backend);
        const std::string backend_token = normalized_window_token(backend_hint);
        const bool xenia_backend = xenia_backend_family(backend_hint);
        const std::set<Window> managed_clients =
            xenia_backend ? ewmh_client_windows(display, root) : std::set<Window>{};
        const long long age = monotonic_ms() - started_ms;
'''
replacements.append((needle, replacement, "candidate family setup"))

needle = '''            if (attrs.map_state != IsViewable) continue;
            if (attrs.width < 160 || attrs.height < 100) continue;

            const pid_t owner_pid = x_window_pid(display, window);
            const bool process_owned = owner_pid > 1 && process_descends_from(owner_pid, child);
            const std::string identity = safe_window_identity(display, window);
            const bool backend_owned = !backend_hint.empty() && identity.find(backend_hint) != std::string::npos;
'''
replacement = '''            if (attrs.map_state != IsViewable) continue;
            if (attrs.width < 160 || attrs.height < 100) continue;

            // Xenia Edge's wxGTK hierarchy contains multiple mapped X11 child
            // windows. Only capture the EWMH client window itself when GNOME
            // publishes _NET_CLIENT_LIST. Reparenting an internal wx child can
            // crash the host or leave a detached top-level frame behind.
            if (xenia_backend && !managed_clients.empty() &&
                managed_clients.count(window) == 0U) {
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
'''
replacements.append((needle, replacement, "safe Edge candidate selection"))

needle = '''            long long score = static_cast<long long>(attrs.width) * attrs.height;
            if (process_owned) score += 2200000000LL;
            if (backend_owned) score += 900000000LL;
            if (attrs.override_redirect) score -= 10000000LL;
'''
replacement = '''            long long score = static_cast<long long>(attrs.width) * attrs.height;
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
'''
replacements.append((needle, replacement, "render-client scoring"))

working = text
for old, new, label in replacements:
    count = working.count(old)
    if count != 1:
        raise SystemExit(f"STOP: expected exactly one {label} pattern, found {count}. Nothing was written.")
    working = working.replace(old, new, 1)

path.write_text(working, encoding="utf-8")
print("PASS: v53 Xenia Edge emulator-host source repair applied.")
