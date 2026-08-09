# ReddMedia

## v0.0.2 - VLC-Style Player Repair 1

This build does the following:

- Provides a top-level executable named `ReddMedia`.
- Launches as a standalone Linux desktop app.
- Opens local media files.
- Plays video through VLC/libVLC.
- Provides play, pause, stop, seek, time display, fullscreen, and volume controls.
- Shows only the video surface while fullscreen is active.
- Toggles fullscreen when the video area is double-clicked.
- Exits fullscreen with Escape.
- Hides the mouse pointer after 3 seconds of idle time over the video area.
- Shows the mouse pointer again when moved.
- Uses the mouse wheel for volume up and volume down only.
- Saves the last watched file and playback position locally.
- Shows a clickable Resume or Load Different File choice on startup when saved media exists.
- Closes playback and exits the app when the window is closed.
