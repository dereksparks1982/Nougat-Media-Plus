Nougat Media Suite v0.0.48 accepted release
==========================================

BASELINE
  Git HEAD: b23b146f0df2b7c68c3e0b32152074d78f8ea1c3 (v0.0.47)

SCOPE
  - DOS folder discovery + DOSBox
  - Xbox 360 .iso/default.xex discovery + Xenia Canary
  - one X11/XWayland emulator host embedded inside Nougat Video Player
  - existing NES/SNES/Game Boy/N64/Atari backend selection retained
  - pinned official DOSBox Staging + Xenia Canary Linux runtimes installed by the build tool

SAFE APPLY
  Extract this package OVER the Nougat Media Suite project root, then run:

    python3 tools/apply_v48_embedded_emulation.py
    python3 tools/build_v48.py

The patcher stops if v47 main.cpp/CMakeLists.txt are not the exact verified Git blobs or if those
files already have local edits. No command in this package commits or pushes to GitHub.

See V48_TESTING.md and docs/builds/NOUGAT_MEDIA_SUITE_v0_0_48_EMBEDDED_EMULATION_OWNER_HANDSHAKE.md.

CORRECTION 2026-08-25
  Replaces the first v48 ZIP. DOS scan integration is now indentation-independent.

RELEASE STATUS 2026-08-26
  Accepted after a clean native warnings-as-errors build and owner-verified
  embedded NES/Mesen playback. Further per-system game compatibility testing
  continues as follow-up work without rewriting the accepted v0.0.48 release.
