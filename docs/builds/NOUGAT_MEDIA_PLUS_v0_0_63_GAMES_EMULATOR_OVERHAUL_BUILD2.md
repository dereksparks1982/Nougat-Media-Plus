# Nougat Media Plus v0.0.63 - Games & Emulator Overhaul Build 2

Build 2 replaces rejected Build 1.

## Build 1 failure fixed

Build 1 failed before compilation because its patcher attempted to structurally parse a large C++ function and could not reliably locate the closing brace.

Build 2 removes that parser completely. It uses exact accepted-v0.0.62 source anchors and stops before writing files unless every required anchor is present exactly once.

## PlayStation 2 / PCSX2

- Official PCSX2 2.8.1 Linux Qt AppImage.
- SHA-256 verified before installation.
- Managed runtime location: `components/games/runtime/pcsx2/`.
- `NOUGAT_PCSX2` override supported.
- Launch form: `-batch -fullscreen -- <game>`.
- EmulatorHost remains responsible for frontend lifecycle.
- No PS2 BIOS or commercial game content is included.

## Games work

- Managed runtime discovery expanded for PlayStation, PS2, PSP, PS3, GameCube/Wii, Wii U, Arcade, Switch, and Xbox 360.
- Mesen no longer receives forced desktop-fullscreen arguments from Nougat.
- Xbox 360 and DOS are restored to the visible Systems audit list.
- Context-aware optical-disc recognition is expanded for additional emulator image formats.

## Validation

The installer verifies v0.0.62, creates a rollback snapshot, syntax-checks the patcher, applies exact anchors, SHA-verifies PCSX2, configures CMake, compiles with warnings-as-errors, verifies the v63 executable identity, and runs the retained tactical-identity self-test. Any failed gate restores the prebuild snapshot.
