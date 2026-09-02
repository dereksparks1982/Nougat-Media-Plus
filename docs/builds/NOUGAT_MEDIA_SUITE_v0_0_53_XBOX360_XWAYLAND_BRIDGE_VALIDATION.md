# Nougat Media Suite v0.0.53 Xbox 360 XWayland Bridge Validation

Scope: Xbox 360 / Xenia Edge viewport integration only.

## Failure corrected

The rejected direct-surface packages attempted to put an absolute hook path
under the Nougat project directory into LD_PRELOAD. The project directory is
named `Nougat Media Suite`. Linux dynamic-loader preload lists use whitespace
and colons as separators, so that absolute path was split and the surface hook
could fail to load.

This repair never puts the project path into LD_PRELOAD. The native bridge
creates a private no-space symlink under XDG_RUNTIME_DIR (or /tmp) for the
lifetime of Edge and uses that path for LD_PRELOAD.

## XWayland integration design

1. Nougat v53 launches `xenia_canary`, as already supported.
2. `xenia_canary` execs the native bridge, so the bridge has the exact PID
   tracked by Nougat's EmulatorHost.
3. The bridge creates a mapped, override-redirect X11 window named/classed
   `xenia_canary`, initially far off-screen.
4. The unmodified v53 EmulatorHost sees this as a process-owned backend window
   and reparents it into the Games/player viewport.
5. The bridge detects the successful reparent before Xenia Edge is started.
6. Edge is launched with the XCB/Vulkan surface hook.
7. The hook redirects Xenia Edge's direct `gdk_x11_window_get_xid` request from
   its Linux surface factory to the already-embedded bridge XID.
8. Edge's own wxGTK top-level is kept off-screen, opacity 0, and marked
   skip-taskbar / skip-pager. It remains alive only to satisfy wxGTK lifecycle;
   the game surface is the Nougat child window.

## Validation performed before packaging

- Both native files compiled with `-Wall -Wextra -Werror`.
- Exact original v53 `src/games/emulator_host.cpp` from the supplied v53 source
  material was compiled into a test harness.
- That exact host entered HostState::Embedded with the override-redirect bridge
  and reparented it under the player viewport.
- A GTK/X11 Xenia-shaped mock was then launched through the complete bridge.
- The mock's requested XCB surface XID exactly equaled the child window that
  the exact v53 host had embedded.
- The complete test was repeated with the hook and mock stored beneath a path
  containing `Nougat Media Suite`; the private no-space preload path loaded the
  hook successfully.
- GTK top-level validation showed `_NET_WM_WINDOW_OPACITY = 0` and
  `_NET_WM_STATE_SKIP_TASKBAR`, `_NET_WM_STATE_SKIP_PAGER`.

No claim is made that a real Xbox title has passed owner-machine validation.
That remains the acceptance gate.
