# Changelog

## v0.0.5 - Branding and Polish

- Added a red ReddMedia triangle icon asset set.
- Set the ReddMedia window icon to the red triangle.
- Updated `ReddMedia.desktop` to use the red triangle icon name and ReddMedia window class.
- Added local Linux icon files for launcher and dock registration.
- Changed the in-app top menu/header bar from grey to red.
- Repaired v0.0.5 repaint handling with buffered parent-window control drawing.
- Removed the rejected direct partial repaint path that made the seek bar flash.
- Excluded the custom MIME override approach so double-click opening remains intact.
- Preserved v0.0.4 playback, timeline, skip button, fullscreen, mouse, keyboard, and resume behavior.

## v0.0.4 - Time Layout, Skip Buttons, and Red Timeline Repair

- Moved current time before the seek bar.
- Moved total duration after the seek bar.
- Lowered the seek/time row below the video area.
- Added spacing between the video area and the seek/time row.
- Added red seek/progress bar.
- Added chapter-style marks to the seek bar.
- Added red volume bar.
- Added visible Rewind 10s and Fast Forward 10s buttons.
- Preserved v0.0.3 playback behavior.

## v0.0.3 - Mouse, Keyboard, Time Display, and Flicker Repair

- Fixed video playback flashing/flicker.
- Added single left-click video pause/resume.
- Preserved double left-click fullscreen toggle.
- Added right-click video options menu.
- Added keyboard volume and seek controls.

## v0.0.2 - VLC-Style Player Repair 1

- Repaired fullscreen so only the video surface appears.
- Preserved video playback, basic controls, resume prompt, and close behavior.

## v0.0.1 - VLC-Style Base Video Player

- Added a standalone Linux desktop executable named `ReddMedia`.
- Added local media opening and VLC/libVLC playback.
