# ReddMedia

## v0.0.1 - VLC-Style Base Video Player

This build does the following:

- Provides a top-level executable named `ReddMedia`.
- Launches as a standalone Linux desktop app.
- Opens local media files.
- Plays video through VLC/libVLC.
- Provides play, pause, stop, seek, time display, fullscreen, and volume controls.
- Toggles fullscreen when the video area is double-clicked.
- Exits fullscreen with Escape.
- Hides the mouse pointer after 3 seconds of idle time over the video area.
- Shows the mouse pointer again when moved.
- Saves the last watched file and playback position locally.
- Shows a Resume or Load Different File choice on startup when saved media exists.
- Closes playback and exits the app when the window is closed.
