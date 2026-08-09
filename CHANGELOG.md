# ReddMedia Changelog

## v0.0.3 - Mouse, Keyboard, Time Display, and Flicker Repair

This build repairs player input and bottom-control behavior from v0.0.2.

### Changed

- Single left-click on the video area now pauses or resumes playback.
- Double left-click on the video area remains the fullscreen toggle.
- Right-click on the video area now opens a basic options menu.
- Up Arrow raises volume.
- Down Arrow lowers volume.
- Left Arrow rewinds 10 seconds.
- Right Arrow fast-forwards 10 seconds.
- The default skip interval is 10 seconds.
- Bottom controls reserve room for the time display so it is not cut off.
- Normal redraw avoids clearing the video surface while media is playing.

### Preserved

- Spacebar pause/resume.
- Escape exits fullscreen.
- Mouse wheel controls volume only.
- Fullscreen shows only the video surface.
- Resume and Load Different File startup choices.
- Clean close behavior.
