# ReddMedia v0.0.5 Validation

## Build validation

- C++ compile: PASS
- Warning policy: PASS with `-Wall -Wextra -Werror`
- Link target: PASS
- Top-level executable: PASS
- Executable bit: PASS

## Static validation

- App title updated to `ReddMedia v0.0.5`: PASS
- Native grey window title bar left under system control: PASS
- In-app top/menu bar uses red: PASS
- Parent-window UI draws to an offscreen buffer before being copied onscreen: PASS
- Seek/time row no longer uses the rejected direct partial repaint path: PASS
- Volume row no longer uses the rejected direct partial repaint path: PASS
- Red triangle icon code is present in the X11 window icon path: PASS
- Transparent red triangle PNG files included: PASS
- X11 window class is set to `ReddMedia`: PASS
- `ReddMedia.desktop` uses `Icon=reddmedia`: PASS
- `ReddMedia.desktop` uses `StartupWMClass=ReddMedia`: PASS
- Custom MIME override is not included: PASS
- README v0.0.5 describes only what the build does: PASS
- Changelog updated for v0.0.5: PASS
- Roadmap confirms yt-dlp and p2p are not part of v0.0.5: PASS

## Manual validation required by Derek

- Double-click the top-level `ReddMedia` executable.
- Confirm the app window shows v0.0.5.
- Confirm the in-app top/menu bar is red.
- Confirm the native grey title bar is left alone.
- Confirm the red top/menu bar does not flash during playback.
- Confirm the seek bar does not flash during playback.
- Confirm the volume bar does not flash during playback.
- Confirm the red triangle appears for the window/dock/app switcher where Linux applies the icon.
- Confirm the top-level executable still opens by double-click.
- Confirm the executable in Files uses the red triangle if GNOME accepts safe metadata.
- Confirm red seek bar is preserved.
- Confirm red volume bar is preserved.
- Confirm chapter-style seek marks are preserved.
- Confirm bottom buttons remain in this order: Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- Confirm video playback still works.
- Confirm fullscreen still shows video only.
- Confirm mouse and keyboard controls still work.
- Confirm app closes cleanly.
