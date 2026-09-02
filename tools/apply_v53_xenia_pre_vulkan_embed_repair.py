#!/usr/bin/env python3
from pathlib import Path
import sys

project = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()
path = project / "src/games/emulator_host.cpp"

if not path.is_file():
    raise SystemExit("STOP: src/games/emulator_host.cpp was not found.")

text = path.read_text(encoding="utf-8")
marker = "// NOUGAT_V53_XENIA_PRE_VULKAN_EMBED"

if marker in text:
    print("PASS: v53 pre-Vulkan Xenia embed host repair is already applied.")
    raise SystemExit(0)

def replace_once(old, new, label):
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"STOP: expected exactly one {label} pattern, found {count}. Nothing was written."
        )
    text = text.replace(old, new, 1)

replace_once(
'''    if (child == 0) {
        setpgid(0, 0);

        for (const auto& entry : request.environment) {
''',
'''    if (child == 0) {
        setpgid(0, 0);

        // NOUGAT_V53_XENIA_PRE_VULKAN_EMBED
        // Let cooperating native emulator runtimes attach before their first
        // top-level map / graphics swapchain creation.
        const std::string nougat_embed_xid =
            std::to_string(static_cast<unsigned long long>(parent_window));
        setenv("NOUGAT_EMBED_XID", nougat_embed_xid.c_str(), 1);

        for (const auto& entry : request.environment) {
''',
"child launch"
)

replace_once(
'''    bool embed(Window window) {
        if (!display || !parent || !window) return false;

        XWindowAttributes attrs{};
        if (!safe_window_attributes(display, window, attrs)) return false;

''',
'''    bool embed(Window window) {
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

''',
"embed"
)

replace_once(
'''            if (preexisting.count(window) != 0U) continue;
            if (is_descendant_window(display, window, shell)) continue;

            XWindowAttributes attrs{};
''',
'''            if (preexisting.count(window) != 0U) continue;
            const bool preembedded =
                is_descendant_window(display, window, parent);
            if (is_descendant_window(display, window, shell) && !preembedded)
                continue;

            XWindowAttributes attrs{};
''',
"candidate descendant gate"
)

old_filter = '''            if (xenia_backend && !managed_clients.empty() &&
                managed_clients.count(window) == 0U) {
                continue;
            }
'''
new_filter = '''            if (xenia_backend && !managed_clients.empty() &&
                managed_clients.count(window) == 0U && !preembedded) {
                continue;
            }
'''
if old_filter in text:
    text = text.replace(old_filter, new_filter, 1)

replace_once(
'''            if (!process_owned && !backend_owned) {
''',
'''            if (!process_owned && !backend_owned && !preembedded) {
''',
"candidate ownership gate"
)

replace_once(
'''            long long score = static_cast<long long>(attrs.width) * attrs.height;
            if (process_owned) score += 2200000000LL;
''',
'''            long long score = static_cast<long long>(attrs.width) * attrs.height;
            if (preembedded) score += 5000000000LL;
            if (process_owned) score += 2200000000LL;
''',
"candidate scoring"
)

path.write_text(text, encoding="utf-8")
print("PASS: v53 pre-Vulkan Xenia embed host repair applied.")
