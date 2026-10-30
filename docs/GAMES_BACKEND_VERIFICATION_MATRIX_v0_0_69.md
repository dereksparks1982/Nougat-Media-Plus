# Nougat Games Backend Verification Matrix

**Prepared:** 2026-10-30  
**Project:** Nougat Media Plus v0.0.69  
**State:** Source-only planning. No console/system listed here has been newly runtime-tested in this batch.

Discovery merely checks whether a matching executable (and, for PCSX2, a plausible BIOS candidate) exists. It **does not** validate actual gameplay, controllers, audiovisual output, containment, licensing, or speed. The available-backend function has a legacy name; its output must not be described as an accepted runtime.

| System | Candidate backend(s) from source registry | Required future proof |
|---|---|---|
| NES | Mesen, FCEUX, Nestopia | Embedded video, 1P/2P controller input, save/load, exit |
| SNES | Mesen, Snes9x | Embedded video, input mapping, sound, save/load, exit |
| Game Boy | SameBoy, mGBA, Mesen | Game display sizing, input, save/load |
| Game Boy Color | SameBoy, mGBA, Mesen | Color/rendering, input, save/load |
| Game Boy Advance | mGBA, Mesen | Embedded renderer, input, audio |
| Nintendo 64 | RMG, Mupen64Plus | Video plugin/window ownership, controller |
| Sega Genesis | BlastEm | Input, sound, embedded output |
| Sega Master System | BlastEm | Backend system mode and actual gameplay |
| Sega Game Gear | BlastEm | Backend system mode and actual gameplay |
| Atari 2600 | Stella | Frame ownership, input, exit |
| Atari 5200 | Atari800 | System flags, input, ROM launch |
| Atari 7800 | a7800 | Configuration/ROM launch, controller |
| Atari 8-bit | Atari800 | Model flags, media loading, embedded window |
| Atari Lynx | Mednafen | Explicit console selection, controller |
| PlayStation | DuckStation | BIOS, hardware renderer, game boot |
| PlayStation 2 | PCSX2 | User-provided BIOS, game boot, X11 containment |
| PlayStation Portable | PPSSPP | ISO boot, controls, aspect, audio |
| PlayStation 3 | RPCS3 | Firmware, game boot, input, GPU, containment |
| GameCube | Dolphin | Disc image boot, gamepad, renderer |
| Wii | Dolphin | Controls, remote input, video/audio |
| Wii U | Cemu | Compatibility/runtime dependencies, X11 embedding |
| Arcade | MAME | Machine ROM and legal BIOS dependencies |
| Nintendo Switch | Ryujinx/Suyu/Yuzu | Legal game/keys/firmware, backend availability, containment |
| DOS | DOSBox Staging/DOSBox | Executable launch, input, filesystem containment |
| Xbox 360 | Xenia Canary / configured compatibility runner | GTA IV title launch, performance, X11 containment, exit |

## Required per-backend state transitions

1. No backend installed: explain missing executable, do not imply playability.
2. Backend installed: report **Detected, not tested**, not green/complete.
3. Firmware needed: explain separate user-provided legal dependencies. Do not distribute copyrighted firmware.
4. Launch: one process group per emulator, process start detection, visible result or bounded failure.
5. Embed: capture only process-owned or clearly backend-identified new X11 windows; never arbitrary desktop windows.
6. Playback: honor the preapproved Nougat Video Player position and size, no external desktop popup fallback.
7. Input: controllers, keys, start/select/back mappings; confirm two-player behavior where relevant.
8. End: close button/Escape, disappearing window, launch timeout and failed processes all clean up child group.
9. Repeat: relaunch same title, switch title, exit Nougat while emulator is running, ensure no lingering orphan.
10. Logs: distinguish executable missing, exit 127, other exit codes, termination by signal and embed timeout.

## Owner approval boundaries

Do not change owner-approved UI geometry, game artwork, systems navigation, original working games, or emulator-specific launch arguments as part of this source-only batch. Existing Linux compatibility runners remain supported. The main CMake source target still contains an older v0.0.68 version label even though the approved project line is v0.0.69; this discrepancy is **not silently changed** by the Games source batch.

The emulator host currently relies on X11/XWayland for in-player containment. Wayland native-window attachment is not claimed. All backend/hardware acceptance tests are pending and intentionally deferred at the owner's request.
