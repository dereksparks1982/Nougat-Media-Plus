# ReddMedia v0.0.4 Validation Record

## Package

- Package: `ReddMedia_v0_0_4_TIME_LAYOUT_SKIP_BUTTON_RED_BAR_REPAIR.zip`
- Target version: v0.0.4
- Required base: v0.0.3
- Required base commit: `3224003`

## Build validation performed before package delivery

- C++ compile command completed successfully with warnings treated as errors.
- Top-level executable `ReddMedia` exists.
- `ReddMedia` is an ELF 64-bit Linux executable.
- Dynamic dependency check confirms linkage to X11 and standard Linux runtime libraries.
- Package contains README, CHANGELOG, ROADMAP, LICENSE, THIRD_PARTY_NOTICES, source, build handshake, validation record, desktop file, and top-level executable.

## Derek-side manual validation required

Derek must verify the following with actual media:

- App launches by double-clicking the top-level `ReddMedia` executable.
- Current playback time appears before the seek bar.
- Total media duration appears after the seek bar.
- Current time, seek bar, and total time are lowered below the video area.
- There is visible spacing between the video area and the seek/time row.
- Seek/progress bar uses red fill.
- Seek bar shows chapter-style marks.
- Volume bar uses red fill.
- Bottom buttons appear in exact order: Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- Rewind 10s button rewinds 10 seconds.
- Fast Forward 10s button fast-forwards 10 seconds.
- Left Arrow rewinds 10 seconds.
- Right Arrow fast-forwards 10 seconds.
- Up Arrow raises volume.
- Down Arrow lowers volume.
- Spacebar pauses/resumes.
- Single left-click video pauses/resumes.
- Double left-click video toggles fullscreen.
- Right-click video opens options menu.
- Playback flicker from earlier builds remains gone.
- Fullscreen still shows video only.
- Escape exits fullscreen.
- Mouse wheel controls volume only.
- Resume / Load Different File startup choices still work.
- Window close exits cleanly.

## Known limitation

- Chapter marks are visual timeline tick marks. Embedded media chapter metadata reading is not implemented in this build.
