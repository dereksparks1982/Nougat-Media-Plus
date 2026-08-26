# Xenia Canary runtime source for Nougat Media Suite v0.0.48

Nougat v0.0.48 adds an Xbox 360 backend intended for Xenia Canary.

Upstream:
https://github.com/xenia-canary/xenia-canary

Current Xenia Canary documentation lists Linux with Proton as a supported minimum-OS path, while
native Linux work remains experimental. Nougat therefore accepts either:

1. `NOUGAT_XENIA` pointing to a native/experimental Xenia executable
2. `components/games/runtime/xenia/xenia_canary`
3. `components/games/runtime/xenia/xenia`
4. Xenia on PATH
5. `xenia_canary.exe` / `xenia.exe` in `components/games/runtime/xenia/`, launched through
   `NOUGAT_XENIA_RUNNER`, `umu-run`, `wine64`, or `wine`

For custom Proton setups, set `NOUGAT_XENIA_RUNNER` to an executable wrapper that accepts
`<xenia.exe> <game-path>`.

Nougat recognizes Xbox 360 `.iso` images and extracted `default.xex` titles. A `default.xex` title
uses its containing directory name in the Games library. Large Xbox 360 images are deliberately not
extracted from ZIP archives.

The first owner acceptance title is Grand Theft Auto IV. Emulator compatibility and frame rate are
Xenia/hardware dependent, so per-title owner testing should separately verify library detection, launch,
embedding, input, stop, resize/fullscreen behavior, and actual game compatibility.

This changed-files package does not redistribute Xenia. Consult the upstream license before
redistributing a runtime binary.

## v0.0.48 pinned Linux runtime

The v0.0.48 build installer pins the official Linux AppImage from Xenia Canary release `1e834f8`,
published 2026-08-24.

SHA-256:
`91df919a912bd305a214c535e0ab8abee43c18eb1bab1ef5e35991d16738b05e`

Nougat sets `APPIMAGE_EXTRACT_AND_RUN=1` for Xbox 360 sessions so the AppImage does not depend on a
system FUSE mount path.
