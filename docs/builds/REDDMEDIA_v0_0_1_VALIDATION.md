# ReddMedia v0.0.1 Validation Record

## Package validation

PASS:

- Package contains top-level folder `ReddMedia/`.
- Package contains top-level executable `ReddMedia/ReddMedia`.
- Executable bit is set on `ReddMedia/ReddMedia`.
- README.md exists and states what the build did.
- CHANGELOG.md exists.
- ROADMAP.md exists.
- LICENSE exists.
- THIRD_PARTY_NOTICES.md exists.
- Build handshake exists.
- Validation record exists.
- Package excludes P2P, YT-DLP, modules, media library, music, photos, playlists, LAN/server mode, and web app features.

## Compile validation

PASS:

- Source compiled with `g++ -std=c++17 -O2 -Wall -Wextra -Werror`.
- Output binary is an x86_64 Linux ELF executable.

## Runtime validation limits

The sandbox does not provide a graphical desktop display or VLC/libVLC runtime, so full video playback launch validation must be performed on Derek's Ubuntu desktop.

Expected owner-side checks:

- Unzip package.
- Open `ReddMedia/` folder.
- Double-click `ReddMedia` executable.
- Confirm app opens.
- Confirm Open loads a media file.
- Confirm playback works when VLC/libVLC is installed.
- Confirm close button exits cleanly.
