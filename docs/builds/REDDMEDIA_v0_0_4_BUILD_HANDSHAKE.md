# ReddMedia v0.0.4 Build Handshake

## Project and version

- Project: ReddMedia
- Target version: v0.0.4
- Build title: Time Layout, Skip Buttons, and Red Timeline Repair
- Package filename: `ReddMedia_v0_0_4_TIME_LAYOUT_SKIP_BUTTON_RED_BAR_REPAIR.zip`

## Required base

- Required accepted base: ReddMedia v0.0.3
- Required commit: `3224003`
- Required tag: `v0.0.3`
- Required branch: `main`
- Required remote: `origin/main`
- Required tree state before apply: clean

## Completed changes

- Split the time display into current time before the seek bar and total duration after the seek bar.
- Removed the combined `current / total` time block from after the seek bar.
- Lowered the current time, seek bar, and total duration row.
- Added vertical spacing between the video area and the seek/time row.
- Changed the seek/progress fill to red for the company colour direction.
- Added chapter-style marks to the seek bar.
- Changed the volume bar fill to red for the company colour direction.
- Added visible Rewind 10s and Fast Forward 10s buttons to the bottom control bar.
- Set the bottom button order to Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- Wired Rewind 10s button to seek backward 10 seconds.
- Wired Fast Forward 10s button to seek forward 10 seconds.
- Preserved Left Arrow and Right Arrow 10-second seek behavior.
- Preserved single left-click video pause/resume.
- Preserved double left-click video fullscreen toggle.
- Preserved right-click video options menu.
- Preserved v0.0.3 playback flicker repair.
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
- No subtitle work.
- No broad styling redesign.
- No GitHub operation during build packaging.

## Changed files

- `ReddMedia`
- `src/main.cpp`
- `README.md`
- `CHANGELOG.md`
- `ROADMAP.md`
- `docs/builds/REDDMEDIA_v0_0_4_BUILD_HANDSHAKE.md`
- `docs/builds/REDDMEDIA_v0_0_4_VALIDATION.md`

## Validation results

See `docs/builds/REDDMEDIA_v0_0_4_VALIDATION.md`.

## Known failures or risks

- Chapter marks are visual chapter-style tick marks on the seek bar. Reading embedded media chapter metadata is future work.
- Bottom controls are wider now and must be verified visually on Derek's desktop.
- Real video playback and button behavior must be verified by Derek with actual media.
- Volume controls remain on the bottom bar and must not obscure the requested button order.

## Rollback point

- ReddMedia v0.0.3
- Commit: `3224003`
- Tag: `v0.0.3`
- Accepted snapshot: `/home/dereksparks1982/DKLab/Archive/ReddMedia_v0_0_3_ACCEPTED_20260809_160722`

## Current continuation point

- Candidate v0.0.4 package is ready for Derek-side apply and testing.
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

- If v0.0.4 passes, close out in the staged order above.
- Future builds may move to right-click menu polish, subtitle/audio selection, or media-library planning only after separate proposal and approval.
