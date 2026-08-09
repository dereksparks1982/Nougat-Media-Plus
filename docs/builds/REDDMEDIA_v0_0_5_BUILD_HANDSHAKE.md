# ReddMedia v0.0.5 Build Handshake

## Project and version

- Project: ReddMedia
- Version: v0.0.5
- Title: Branding and Polish Flicker/Icon Repair

## Required base

- Required base version: ReddMedia v0.0.4
- Required base commit: 5583f1d
- Required base tag: v0.0.4
- Required branch: main
- Required remote state: origin/main updated through v0.0.4

## Package

- Package filename: ReddMedia_v0_0_5_BRANDING_POLISH_NO_FLASH_SAFE_ICON_REPAIR.zip
- Package type: changed project folder with top-level executable
- Top-level executable: ReddMedia

## Completed changes

- Updated the application to ReddMedia v0.0.5 labels.
- Added a red ReddMedia triangle icon asset set under `assets/icons/`.
- Set the application window icon through `_NET_WM_ICON`.
- Set the X11 window class to `ReddMedia` for launcher and dock matching.
- Updated `ReddMedia.desktop` to use `Icon=reddmedia` and `StartupWMClass=ReddMedia`.
- Kept the native GNOME/window title bar under system control.
- Kept the in-app top menu/header strip red.
- Replaced the v0.0.5 direct/partial repaint path with a buffered parent-window UI draw before copying it onscreen.
- Preserved v0.0.4 playback, timeline, skip button, fullscreen, mouse, keyboard, and resume behavior.
- Preserved the v0.0.4 red seek bar, red volume bar, and chapter-style marks.
- Removed the rejected custom MIME override approach from the apply workflow so double-click launch is not broken.
- Kept only safe local icon registration and safe `gio` custom-icon metadata attempts for the raw executable.

## Excluded work

- No custom MIME type or MIME glob override is included.
- yt-dlp is not included in v0.0.5.
- p2p is not included in v0.0.5.
- The future top menu order `File Audio Subtitle yt-dlp p2p` is not added in v0.0.5.
- No media library work is included.
- No playlist work is included.
- No subtitle feature work is included.
- No installer system is included.
- No app auto-launch is included.
- No browser/GitHub auto-open is included.

## Changed files

- `ReddMedia`
- `src/main.cpp`
- `CMakeLists.txt`
- `ReddMedia.desktop`
- `README.md`
- `CHANGELOG.md`
- `ROADMAP.md`
- `assets/icons/reddmedia.png`
- `assets/icons/reddmedia-16.png`
- `assets/icons/reddmedia-32.png`
- `assets/icons/reddmedia-48.png`
- `assets/icons/reddmedia-64.png`
- `assets/icons/reddmedia-128.png`
- `assets/icons/reddmedia-256.png`
- `docs/builds/REDDMEDIA_v0_0_5_BUILD_HANDSHAKE.md`
- `docs/builds/REDDMEDIA_v0_0_5_VALIDATION.md`

## Validation results

- Native compile with `-Wall -Wextra -Werror`: PASS
- Link target: PASS
- Top-level executable exists: PASS
- Top-level executable is marked executable: PASS
- Version text updated to v0.0.5: PASS
- README v0.0.5 truth updated: PASS
- Red triangle icon assets included: PASS
- Transparent icon background generated: PASS
- No yt-dlp or p2p menu addition in v0.0.5: PASS
- Parent-window controls draw through an offscreen buffer before onscreen copy: PASS
- Rejected custom MIME override excluded from package/apply workflow: PASS

## Known failures or risks

- Flicker repair requires Derek manual confirmation because flicker is visual and machine-specific.
- GNOME Files can still choose a generic gear for raw executable files. This build avoids breaking double-click launch while safely registering the app/window/launcher icon and applying safe file metadata.
- The native Linux title bar color is controlled by the window manager theme; v0.0.5 leaves it alone by Derek direction.
- Dock/launcher icon matching depends on local `.desktop` registration and `StartupWMClass=ReddMedia`.

## Rollback point

- Roll back to accepted ReddMedia v0.0.4.
- Accepted v0.0.4 snapshot: `/home/dereksparks1982/DKLab/Archive/ReddMedia_v0_0_4_ACCEPTED_20260809_162447`
- Accepted v0.0.4 commit: 5583f1d
- Accepted v0.0.4 tag: v0.0.4

## Current continuation point

- Apply v0.0.5 candidate to `~/DKLab/Projects/ReddMedia`.
- Derek manually tests by double-clicking the top-level `ReddMedia` executable.
- If accepted, close out in strict order: snapshot first, Git second, GitHub third.

## Next planned work

- If Derek approves after v0.0.5, likely next planning target is yt-dlp foundation.
- Future top menu plan, not in v0.0.5: `File   Audio   Subtitle   yt-dlp   p2p`.

## Required input files/artifacts

- ReddMedia v0.0.4 accepted repository/project folder.
- Local VLC/libVLC available on Derek's Linux machine for playback testing.
