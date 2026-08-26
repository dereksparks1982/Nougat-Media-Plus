Nougat Media Suite v0.0.49 Atari Owner-Test Repair
===================================================

Repairs the rejected v0.0.49 runtime candidate after owner screenshots showed:
1. Stella remained a separate GNOME/X11 window instead of becoming the Nougat player child.
2. GNOME exposed a separate Stella application/dock/task-switcher icon.
3. Atari artwork did not resolve for preservation-set filenames, including the tested
   2 Pak Special - Challenge, Surfing (1990) (HES) (PAL) [a] cartridge.
4. The Nougat header and diagnostic version identity still contained v0.0.48 text.

Repair behavior:
- recursively discovers the actual emulator client below XWayland/window-manager frames;
- only captures windows belonging to the launched emulator process or backend identity;
- sets _NET_WM_STATE_SKIP_TASKBAR and _NET_WM_STATE_SKIP_PAGER before reparenting;
- keeps the no-separate-window policy and existing top-edge Stella Options bridge;
- caches Libretro directory indexes persistently and performs conservative normalized matching;
- uses Named_Boxarts first, then Named_Titles/Named_Snaps when no box exists;
- includes a verified exact front-cover fallback for the owner-tested 2 Pak Special cartridge;
- preserves prepared artwork across Nougat builds;
- repairs visible and diagnostic v0.0.49 identity;
- preserves v49 wheel/drag, USA/English, revision filtering, NES/SNES, DOS, Xenia and Atari800 work.

No Git action is performed by this package.
