Nougat Media Suite v0.0.53 - Xbox 360 Xenia Edge Direct Child Surface Repair

Scope:
- Runtime-only Xbox 360 embedding repair.
- Does not rebuild or replace Nougat_Media_Suite_v53.
- Uses the NOUGAT_EMBED_XID already supplied by local v53 EmulatorHost.
- Creates one real X11 InputOutput child directly inside Nougat's Games/video viewport.
- Xenia Edge's XCB/Vulkan surface is redirected to that child before swapchain creation.
- Edge's GTK top-level window is realized but not mapped as a desktop window.

This replaces the rejected parent-surface/proxy/reparent experiments.
The existing xenia_edge_linux.AppImage remains required in the same runtime directory.
