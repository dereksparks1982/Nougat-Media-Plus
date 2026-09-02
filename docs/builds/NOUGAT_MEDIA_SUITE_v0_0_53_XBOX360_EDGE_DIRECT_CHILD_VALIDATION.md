# Nougat Media Suite v0.0.53 Xbox 360 Direct Child Validation

## Scope
Runtime-only integration for the existing Xenia Edge Linux AppImage. No Nougat executable rebuild or replacement.

## Local v53 evidence
The owner's local `src/games/emulator_host.cpp` sets `NOUGAT_EMBED_XID` to `parent_window` before `execvp`, and the previously applied v53 host repair contains `preembedded` candidate/adoption logic. The host records `preexisting` before forking the emulator.

## Repair design
1. `xenia_canary` receives `NOUGAT_EMBED_XID` from Nougat.
2. `nougat_xenia_direct_child_host` creates a mapped InputOutput child of that exact window after the host's preexisting-window snapshot.
3. The child is identified as `xenia_canary` and publishes `_NET_WM_PID` from a process descended from Nougat's emulator child.
4. Nougat can therefore adopt the already-descendant child as its embedded emulator window.
5. `nougat_xenia_direct_surface.so` returns that child XID from Xenia Edge's `gdk_x11_window_get_xid` surface call, so Vulkan/XCB presents directly into the child.
6. The hook prevents Edge's GTK top-level from mapping onto the desktop while still realizing it for wxGTK internals.

## Build validation
- `nougat_xenia_direct_child_host`: GCC `-O2 -Wall -Wextra -Werror`, PASS.
- `nougat_xenia_direct_surface.so`: GCC shared build `-fPIC -O2 -Wall -Wextra -Werror`, PASS.
- `ldd`: no unresolved shared libraries.

## Synthetic X11 validation
Under Xvfb:
- mock Nougat shell + viewport created;
- helper created a 900x500 mapped InputOutput child directly under the supplied viewport;
- WM_CLASS = `xenia_canary`, WM_NAME = `xenia_canary Nougat direct render surface`;
- `_NET_WM_PID` present;
- child ancestry under the supplied viewport verified.

## Interposer validation
A fake GTK/GDK symbol harness named `xenia_surface_test` verified:
- Xenia-side `gdk_x11_window_get_xid` returned the supplied direct child XID;
- top-level `gtk_widget_show` was suppressed;
- GTK realize path still executed.

## Wrapper validation
With a fake Edge executable under Xvfb, the packaged launcher:
- received Nougat's viewport XID;
- created the direct child;
- passed that exact child XID to the Edge process;
- kept the child mapped under the Nougat viewport for the emulator lifetime.

## Owner validation still required
Actual Xenia Edge + Xbox 360 game rendering inside the real Nougat v53 viewport must be verified on the owner's machine before acceptance.
