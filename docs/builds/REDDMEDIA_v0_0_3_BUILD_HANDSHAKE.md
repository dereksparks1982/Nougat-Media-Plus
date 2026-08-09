# ReddMedia v0.0.3 Build Handshake

## Project and version

- Project: ReddMedia
- Target version: v0.0.3
- Build title: Mouse, Keyboard, Time Display, and Flicker Repair
- Package filename: `ReddMedia_v0_0_3_MOUSE_KEYBOARD_TIME_DISPLAY_FLICKER_REPAIR.zip`

## Required base

- Required accepted base: ReddMedia v0.0.2
- Required commit: `10b147f`
- Required tag: `v0.0.2`
- Required branch: `main`
- Required remote: `origin/main`
- Required tree state before apply: clean

## Completed changes

- Added single left-click pause/resume on the video area.
- Preserved double left-click fullscreen toggle.
- Added a basic right-click options menu on the video area.
- Added Up Arrow volume up.
- Added Down Arrow volume down.
- Added Left Arrow rewind 10 seconds.
- Added Right Arrow fast-forward 10 seconds.
- Standardized keyboard seek jumps to 10 seconds.
- Repaired bottom control layout so time display has reserved visible space.
- Reduced normal playback flicker risk by avoiding full-window clearing during redraw.
- Preserved fullscreen video-only behavior.
- Preserved Escape fullscreen exit.
- Preserved mouse wheel volume-only behavior.
- Preserved Resume / Load Different File behavior.
- Updated README, CHANGELOG, ROADMAP, build handshake, and validation record.

## Excluded work

- No P2P.
- No YT-DLP.
- No media library.
- No music library.
- No photos.
- No playlists.
- No module loader.
- No web app.
- No styling redesign.
- No GitHub operation during build packaging.

## Changed files

- `ReddMedia`
- `src/main.cpp`
- `CMakeLists.txt`
- `README.md`
- `CHANGELOG.md`
- `ROADMAP.md`
- `docs/builds/REDDMEDIA_v0_0_3_BUILD_HANDSHAKE.md`
- `docs/builds/REDDMEDIA_v0_0_3_VALIDATION.md`

## Validation results

See `docs/builds/REDDMEDIA_v0_0_3_VALIDATION.md`.

## Known failures or risks

- Real playback flicker must be verified by Derek on the actual desktop with actual media.
- Right-click options are intentionally basic for this repair build.
- Resume behavior is preserved from v0.0.2 and still requires owner-side media-file testing.

## Rollback point

- ReddMedia v0.0.2
- Commit: `10b147f`
- Tag: `v0.0.2`
- Accepted snapshot: `ReddMedia_v0_0_2_ACCEPTED_20260809_154347`

## Current continuation point

- Candidate v0.0.3 package is ready for Derek-side apply and testing.
- It is not accepted until Derek tests and says it passes.

## Required post-download workflow

Every ReddMedia build must use this staged workflow unless Derek explicitly says otherwise:

1. Assistant gives only the apply-to-project-folder commands for the downloaded candidate ZIP.
2. Derek applies the candidate to `~/DKLab/Projects/ReddMedia` and tests it.
3. If Derek accepts it, assistant gives snapshot commands only.
4. Derek shows snapshot output.
5. Assistant gives local Git commit/tag commands only.
6. Derek shows Git output.
7. Assistant gives GitHub push/verify commands only.
8. Derek shows GitHub output.

Hard rules:

- Never auto-launch the app.
- Never append `./ReddMedia` to apply command blocks.
- Never auto-open GitHub or a browser.
- Never use `gh repo view --web` unless Derek explicitly requests it.
- Never combine apply, snapshot, Git, and GitHub into one giant block unless Derek explicitly commands it.

## Next planned work after acceptance

- If v0.0.3 passes, close out in the staged order above.
- Future builds may move to subtitle/audio selection repairs only after a separate proposal and approval.
