# Nougat Media Suite v0.0.53 Xbox 360 Edge Direct-Surface Repair

Scope: Xbox 360 embedding only. No Nougat version bump, no installer, no Git operation, no controller work.

## Repair architecture

The rejected approaches attempted to move, reparent, proxy, or capture an already-created Xenia GTK/Vulkan window. This repair changes the boundary instead:

1. `nougat_x11_parent_probe` locates the active Nougat v53 X11 top-level from the launcher process ancestry and selects the large mapped native child used as the Games/video viewport.
2. `xenia_canary` exports that exact X11 window ID as `NOUGAT_X11_PARENT` before Xenia Edge starts.
3. `nougat_xenia_embed.so` interposes Edge's Linux `gdk_x11_window_get_xid` call so Xenia's `XcbWindowSurface` is constructed directly against the Nougat viewport before the Vulkan surface/swapchain is created.
4. The same interposer suppresses mapping/presenting Edge's GTK top-level window while still realizing it for wxGTK bookkeeping.
5. If the Nougat viewport cannot be identified, the launcher fails closed and does not fall back to a detached Xenia window.

## Upstream source evidence

Pinned Xenia Edge release: `7be830a2dd9c7336dedea5f134d0f79adcefb46e`.

At that revision:
- `src/xenia/app/emulator_window.cc` constructs the emulator window with `ui::Window::Create(...)`.
- `src/xenia/ui/window_wx.cc` creates a wx top-level frame and child render panel.
- `WxWindow::CreateSurfaceImpl` passes the GTK render panel handle to `GtkSurfaceFactory::Create`.
- `src/xenia/ui/surface_gnulinux.cc` obtains the X11 XID with `gdk_x11_window_get_xid` and constructs `XcbWindowSurface(connection, window)`.
- `XcbWindowSurface::GetSizeImpl` queries geometry from that X11 window, so the direct target remains the Nougat viewport rather than a proxy top-level.

## Build validation performed before handoff

- `nougat_x11_parent_probe.c`: GCC `-O2 -Wall -Wextra -Werror` PASS.
- `nougat_xenia_embed.c`: GCC shared-object build with `-O2 -Wall -Wextra -Werror -fPIC -shared` PASS.
- Synthetic X11 hierarchy test: probe returned the exact expected large mapped child XID PASS.
- Synthetic GTK/X11 interposition test under Xvfb: surface XID redirected to the supplied external X11 window and GTK top-level remained unmapped (`TOP_MAPPED=0`) PASS.
- No Xenia source compilation is required on the owner's machine.
- No Nougat source or root executable is replaced by this package.

## Owner validation still required

The candidate is not accepted until tested in the real Nougat v53 process with the pinned Xenia Edge AppImage already present at:
`components/games/runtime/xenia/xenia_edge_linux.AppImage`.

Required visual result: launching an Xbox 360 title produces game rendering only inside Nougat's Games/player viewport and no separate Xenia/host/proxy top-level window.
