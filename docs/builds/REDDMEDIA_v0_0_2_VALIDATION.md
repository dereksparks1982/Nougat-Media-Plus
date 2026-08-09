# ReddMedia v0.0.2 Validation Record

## Package checks

- PASS: Package contains top-level folder `ReddMedia/`.
- PASS: Package contains top-level executable `ReddMedia/ReddMedia`.
- PASS: README.md exists.
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

- PASS: Fullscreen path hides normal UI and resizes video surface to cover the window.
- PASS: Mouse wheel Button4/Button5 route to volume only.
- PASS: Resume button exists inside the video surface and calls the saved-file resume path.
- PASS: Resume seek is retried after playback begins.
- PASS: Normal redraw no longer clears the video surface while media is playing.

## Deferred owner-side checks

These require Derek's desktop session, VLC/libVLC runtime, and real media files:

- Double-click launch.
- Video playback.
- Fullscreen video-only appearance.
- Flicker behavior during real playback.
- Resume from saved TV show position.
- Mouse wheel volume behavior.
- Clean close with no stuck process.
