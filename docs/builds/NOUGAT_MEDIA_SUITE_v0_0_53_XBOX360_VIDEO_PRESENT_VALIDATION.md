# Nougat Media Suite v0.0.53 Xbox 360 Video Present Repair

Scope: video presentation only. Existing Nougat player position, size, layout,
XWayland bridge, emulator launch path, and v0.0.53 root executable are unchanged.

## Current owner-tested baseline

- Xenia Edge launches GTA IV through Nougat.
- GTA IV intro audio plays.
- Nougat's native video player opens in the correct place.
- The remaining defect is a black embedded picture.

## Repair

Xenia Edge commit 7be830a loads `libvulkan.so.1` itself and obtains
`vkGetInstanceProcAddr` with `dlsym`. It then obtains
`vkCreateXcbSurfaceKHR` through that function pointer.

The previous bridge redirected GDK's X11 window ID. This repair additionally
intercepts Edge's actual Vulkan loader function-pointer chain and replaces only
the XCB window in `VkXcbSurfaceCreateInfoKHR` with the already embedded Nougat
render-surface XID immediately before `vkCreateXcbSurfaceKHR`.

The existing GDK redirect remains only as a secondary path.

Edge's own wxGTK top-level stays mapped off-screen and out of the taskbar/pager
so its render loop remains alive without exposing a separate emulator window.

## Validation

- Hook compiled as an x86-64 ELF shared object with `-Wall -Wextra -Werror`.
- A loader-path test used real `libvulkan.so.1` and proved that a handle-specific
  `dlsym("vkGetInstanceProcAddr")` returns the Nougat wrapper.
- The wrapped `vkGetInstanceProcAddr` path proved interception of
  `vkCreateXcbSurfaceKHR`.
- Static validation confirms this patch contains no Nougat player reparent,
  resize, or geometry mutation.
- Real GTA IV owner-machine video output remains the acceptance gate.
