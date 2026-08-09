# ReddMedia v0.0.3 Validation Record

## Package checks

- PASS: Package contains top-level folder `ReddMedia/`.
- PASS: Package contains top-level executable `ReddMedia/ReddMedia`.
- PASS: README.md exists.
- PASS: README.md states only what the build does.
- PASS: CHANGELOG.md exists.
- PASS: ROADMAP.md exists.
- PASS: LICENSE exists.
- PASS: THIRD_PARTY_NOTICES.md exists.
- PASS: Build handshake exists.
- PASS: Validation record exists.

## Compile checks

- PASS: `src/main.cpp` compiled with `g++ -std=c++17 -Wall -Wextra -Werror`.
- PASS: Executable linked with X11 and dl.
- PASS: libVLC is loaded dynamically at runtime.

## Source checks

- PASS: Single left-click on the video area schedules pause/resume.
- PASS: Double left-click on the video area cancels the pending single-click action and toggles fullscreen.
- PASS: Right-click on the video area opens a basic options menu.
- PASS: Spacebar still toggles pause/resume.
- PASS: Up Arrow routes to volume up.
- PASS: Down Arrow routes to volume down.
- PASS: Left Arrow routes to rewind 10 seconds.
- PASS: Right Arrow routes to fast-forward 10 seconds.
- PASS: Mouse wheel Button4/Button5 still route to volume only.
- PASS: Escape still exits fullscreen.
- PASS: Fullscreen redraw still hides normal controls.
- PASS: Bottom layout reserves visible time-display space beside the seek bar.
- PASS: Normal redraw avoids full-window clearing during playback refresh to reduce flicker risk.
- PASS: Build handshake records the staged apply/test/snapshot/Git/GitHub workflow rule.

## Deferred owner-side checks

These require Derek's desktop session, VLC/libVLC runtime, and real media files:

- Double-click launch.
- Video playback.
- Flicker behavior during real playback.
- Single left-click pause/resume in windowed mode.
- Single left-click pause/resume in fullscreen.
- Double left-click fullscreen toggle in windowed mode.
- Double left-click fullscreen toggle in fullscreen.
- Right-click options menu in windowed mode.
- Right-click options menu in fullscreen.
- Up/Down Arrow volume behavior.
- Left/Right Arrow 10-second seek behavior.
- Time display visibility.
- Resume / Load Different File behavior.
- Clean close with no stuck process.
