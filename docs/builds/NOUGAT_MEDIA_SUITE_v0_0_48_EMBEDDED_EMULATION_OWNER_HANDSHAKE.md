# Nougat Media Suite v0.0.48 Owner Handshake
## DOS + Xbox 360 + Embedded Emulator Host

Baseline: v0.0.47 at `b23b146f0df2b7c68c3e0b32152074d78f8ea1c3`

### Build scope

v0.0.48 extends the existing Games library rather than replacing it.

- DOS games are folder-based library entries.
- DOSBox Staging / DOSBox is the DOS backend.
- Xbox 360 `.iso` and extracted `default.xex` titles are library entries.
- Xenia Canary is the Xbox 360 backend.
- Existing NES/SNES/Game Boy/N64/Atari backend selection is retained.
- All emulator backends are routed through one Nougat X11 embedding host.
- Nougat refuses an external-window fallback when an embeddable X11/XWayland game window cannot be found.
- Existing linked game-folder persistence is retained.
- No copyrighted game files are bundled.

### Apply and build

From the Nougat Media Suite project root:

```bash
python3 tools/apply_v48_embedded_emulation.py
python3 tools/build_v48.py
```

The patcher verifies the exact v0.0.47 Git HEAD and Git blob IDs for `src/main.cpp` and
`CMakeLists.txt` before changing them. If those source files already have local edits, it stops
without overwriting them.

### Owner acceptance lane A: DOS

Use the owner's linked copies of GTA 1, Prince of Persia, and the Mario DOS title. Check library
discovery, the detected launcher, embedded picture, input, fullscreen/resize, Stop, and clean return
to Games.

### Owner acceptance lane B: Xbox 360

Use the owner's GTA IV Xbox 360 copy. Check `.iso` or extracted `default.xex` discovery, Xenia
backend selection, embedded picture, input, resize/fullscreen, Stop, and clean return to Games.

GTA IV playability is a separate compatibility result from whether Nougat's integration works.

### Linux/X11 note

Nougat itself is X11-native. The embedded host forces SDL toward X11 and Qt toward XCB so emulators
on a Wayland desktop can normally expose an XWayland window that X11 can reparent. If an emulator
exposes only a native Wayland surface, generic X11 reparenting cannot own that surface. v0.0.48
fails closed in that case rather than opening a separate emulator window.

### Release status

v0.0.48 was accepted after a clean native warnings-as-errors build and owner-verified embedded
NES/Mesen playback. DOS, Atari, Xbox 360, and additional emulator/title compatibility testing
continues as follow-up work without rewriting the accepted v0.0.48 checkpoint.
