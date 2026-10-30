#pragma once

// Pure, testable X11 emulator-window ownership policy.
// No X11 calls or process creation occur here.
namespace nougat::games::window_policy {

struct Evidence {
    bool newly_seen = false;
    bool already_in_player = false;
    bool process_tree_owned = false;
    bool process_group_owned = false;
    bool backend_identity_matches = false;
    bool reports_other_process = false;
    bool xenia_backend = false;
    bool window_manager_list_available = false;
    bool window_manager_client = false;
};

constexpr bool eligible(const Evidence& e) noexcept {
    // An existing child of Nougat's own player is already contained.
    if (e.already_in_player) return true;
    // Never steal a desktop window that existed before this game launched.
    if (!e.newly_seen) return false;
    // If _NET_WM_PID identifies a different process, reject that client
    // regardless of title or class: another window can call itself "Xenia".
    if (e.reports_other_process) return false;
    // Real ownership is preferred and also works when exec/wrappers change
    // parentage but preserve the isolated emulator process group.
    if (e.process_tree_owned || e.process_group_owned) return true;
    // Runtimes without _NET_WM_PID need a backend-specific title/class,
    // never merely a new window of suitable size.
    if (!e.backend_identity_matches) return false;
    // When Xenia has an authoritative EWMH client list, do not capture a
    // random child or toolkit decoration frame.
    if (e.xenia_backend && e.window_manager_list_available &&
        !e.window_manager_client) return false;
    return true;
}

} // namespace nougat::games::window_policy
