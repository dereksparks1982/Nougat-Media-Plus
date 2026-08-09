# Changelog

## v0.0.6 - Menu, Audio, Subtitle, Chapter, and Close Behavior Repair

- Reworked the red in-app top bar so File, Audio, and Subtitle open working menus.
- Added File menu actions for Open File and Exit ReddMedia.
- Added embedded audio track discovery and selection through the Audio menu.
- Added Subtitle menu actions for on/off, loading a subtitle file, selecting a subtitle folder, track selection, delay earlier, delay later, and delay reset.
- Added automatic nearby `.srt` subtitle detection for matching video files.
- Added default-on behavior when matching subtitles are detected.
- Added right-click video subtitle on/off.
- Added right-click chapter navigation using real embedded chapters when present.
- Updated seek bar chapter marks to use real embedded chapter timestamps when present.
- Preserved default decorative chapter-style marks when real embedded chapters are not present.
- Preserved the v0.0.5 buffered drawing approach for stable red header, seek, and volume controls.
- Preserved v0.0.5 player controls, red branding, red bars, fullscreen behavior, and resume behavior.

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
