# ReddMedia

## v0.0.3 - Mouse, Keyboard, Time Display, and Flicker Repair

This build does the following:

- Provides a top-level executable named `ReddMedia`.
- Launches as a standalone Linux desktop app.
- Opens local media files.
- Plays video through VLC/libVLC.
- Provides play, pause, stop, seek, time display, fullscreen, and volume controls.
- Shows only the video surface while fullscreen is active.
- Pauses or resumes when the video area is single left-clicked.
- Toggles fullscreen when the video area is double left-clicked.
- Opens a basic options menu when the video area is right-clicked.
- Exits fullscreen with Escape.
- Pauses or resumes with Spacebar.
- Raises volume with Up Arrow.
- Lowers volume with Down Arrow.
- Rewinds 10 seconds with Left Arrow.
- Fast-forwards 10 seconds with Right Arrow.
- Uses 10 seconds as the default seek jump.
- Hides the mouse pointer after 3 seconds of idle time over the video area.
- Shows the mouse pointer again when moved.
- Uses the mouse wheel for volume up and volume down only.
- Keeps the time display visible in the bottom controls.
- Saves the last watched file and playback position locally.
- Shows a clickable Resume or Load Different File choice on startup when saved media exists.
- Closes playback and exits the app when the window is closed.
