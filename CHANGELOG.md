# ReddMedia Changelog

## v0.0.2 - VLC-Style Player Repair 1

This build repairs the base video player behavior from v0.0.1.

### Changed

- Fullscreen now resizes the video surface to cover the player window.
- Normal player controls are hidden while fullscreen is active.
- Mouse wheel input now controls volume only.
- README wording is limited to what the build does.

### Fixed

- Fixed the Resume prompt so Resume is a clickable action inside the video area.
- Fixed Resume so it loads the saved file and retries seeking to the saved position after playback starts.
- Reduced playback flicker by no longer clearing the video surface during normal playback refresh.
- Preserved Escape as fullscreen-exit behavior.
- Preserved clean shutdown path when closing the window.
