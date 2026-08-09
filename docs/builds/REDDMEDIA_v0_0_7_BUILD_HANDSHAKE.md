# ReddMedia v0.0.7 Build Handshake

## Project and version

- Project: ReddMedia
- Version: v0.0.7
- Build name: Red Button Polish

## Required base

- Required base version: ReddMedia v0.0.6
- Required base commit: 82c103c
- Required base tag: v0.0.6
- Required branch: main

## Target version

- Target version: ReddMedia v0.0.7

## Package filename

- ReddMedia_v0_0_7_RED_BUTTON_POLISH.zip

## Completed changes

- Updated the visible bottom player control buttons to red.
- Updated bottom player control button lettering to white.
- Preserved the exact button order: Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- Updated app version text to v0.0.7.
- Updated README, CHANGELOG, ROADMAP, build handshake, and validation records.

## Excluded work

- yt-dlp is not included in v0.0.7.
- p2p is not included in v0.0.7.
- Module/mod installer work is not included in v0.0.7.
- Native GNOME titlebar changes are not included in v0.0.7.
- New icon experiments are not included in v0.0.7.
- Media library and playlist work are not included in v0.0.7.

## Changed files

- ReddMedia
- src/main.cpp
- CMakeLists.txt
- README.md
- CHANGELOG.md
- ROADMAP.md
- docs/builds/REDDMEDIA_v0_0_7_BUILD_HANDSHAKE.md
- docs/builds/REDDMEDIA_v0_0_7_VALIDATION.md

## Validation results

- C++ compile with warnings as errors: PASS
- Top-level executable named `ReddMedia` created: PASS
- Version updated to v0.0.7 in the app title and docs: PASS
- README updated to describe v0.0.7 behavior only: PASS
- Package contains changed project files and executable: PASS

## Known failures or risks

- Manual desktop testing on Derek's machine is still required.
- Button recoloring must be checked during video playback to ensure no flicker returns.

## Rollback point

- Roll back to accepted ReddMedia v0.0.6 if v0.0.7 fails manual testing.

## Current continuation point

- Apply the v0.0.7 candidate ZIP to `~/DKLab/Projects/ReddMedia`.
- Derek manually tests launch, playback, button colors, menus, fullscreen, and close behavior.

## Next planned work

- If v0.0.7 is accepted, close out using the staged rule: snapshot first, Git second, GitHub third.
- After v0.0.7 is accepted, return to module/add-on system planning before yt-dlp or p2p work.

## Required input files/artifacts

- Accepted ReddMedia v0.0.6 project tree.
- v0.0.7 candidate package.

## Workflow reminder

- Do not auto-launch the app.
- Do not open GitHub in a browser.
- After acceptance, close out in stages: snapshot first, Git second, GitHub third.
