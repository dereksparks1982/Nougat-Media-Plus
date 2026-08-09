# ReddMedia Changelog

## v0.0.4 - Time Layout, Skip Buttons, and Red Timeline Repair

This build repairs the visible time layout, adds bottom skip buttons, and updates the seek/volume bars after v0.0.3 acceptance.

### Changed

- Split the time display into current time before the seek bar and total duration after the seek bar.
- Removed the combined current/total time block from after the seek bar.
- Lowered the current time, seek bar, and total time row below the video area.
- Added vertical spacing between the video area and the seek/time row.
- Changed the seek/progress fill to red.
- Added chapter-style marks to the seek bar.
- Changed the volume bar fill to red.
- Added a visible Rewind 10s button to the bottom control bar.
- Added a visible Fast Forward 10s button to the bottom control bar.
- Set the bottom button order to Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- Rewind 10s button seeks backward 10 seconds.
- Fast Forward 10s button seeks forward 10 seconds.
- Updated window/app text to v0.0.4.

### Preserved

- Flicker repair from v0.0.3.
- Single left-click video pause/resume.
- Double left-click video fullscreen toggle.
- Right-click video options menu.
- Spacebar pause/resume.
- Up/Down Arrow volume controls.
- Left/Right Arrow 10-second seek controls.
- Mouse wheel volume-only behavior.
- Fullscreen video-only behavior.
- Escape exits fullscreen.
- Resume and Load Different File startup choices.
- Clean close behavior.
