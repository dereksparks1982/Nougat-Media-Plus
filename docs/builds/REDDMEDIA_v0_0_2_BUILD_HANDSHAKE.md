# ReddMedia v0.0.2 Build Handshake

## Project and version

- Project: ReddMedia
- Target version: v0.0.2
- Build title: VLC-Style Player Repair 1
- Package: ReddMedia_v0_0_2_VLC_STYLE_PLAYER_REPAIR_1.zip

## Required base

- Required accepted base: ReddMedia v0.0.1
- Required branch: main
- Required tag before build: v0.0.1
- Required current base after README cleanup: 662c38b

## Completed changes

- Fullscreen now presents the video surface only.
- Non-video controls are hidden while fullscreen is active.
- Double-click video area toggles fullscreen.
- Escape exits fullscreen.
- Resume is a clickable video-area action.
- Resume loads the saved file and retries seeking to the saved position after playback begins.
- Load Different File opens normal file selection.
- Mouse wheel changes volume only.
- Video surface is not cleared during normal playback refresh to reduce flicker.
- Close path still saves session, stops playback, releases VLC, and exits.
- README, CHANGELOG, ROADMAP, build handshake, and validation record were updated.

## Excluded work

- No P2P.
- No YT-DLP.
- No media library.
- No music, photos, playlists, or module loader.
- No GitHub action inside this package.

## Changed files

- ReddMedia
- src/main.cpp
- CMakeLists.txt
- README.md
- CHANGELOG.md
- ROADMAP.md
- docs/builds/REDDMEDIA_v0_0_2_BUILD_HANDSHAKE.md
- docs/builds/REDDMEDIA_v0_0_2_VALIDATION.md

## Rollback point

- Rollback to accepted ReddMedia v0.0.1 tag and accepted README-clean snapshot.

## Current continuation point

- Derek must install and test the v0.0.2 candidate.
- Only after Derek accepts it should snapshot, local Git commit/tag, and GitHub push happen.
