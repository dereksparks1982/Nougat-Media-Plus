# ReddMedia

## v0.0.4 - Time Layout, Skip Buttons, and Red Timeline Repair

This build does the following:

- Provides a top-level executable named `ReddMedia`.
- Launches as a standalone Linux desktop app.
- Opens local media files.
- Plays video through VLC/libVLC.
- Provides play, pause, stop, seek, time display, fullscreen, and volume controls.
- Shows the current playback time before the seek bar.
- Shows the total media duration after the seek bar.
- Lowers the current time, seek bar, and total time row below the video area.
- Adds spacing between the video area and the seek/time row.
- Uses a red seek/progress bar.
- Shows chapter-style marks on the seek bar.
- Uses a red volume bar.
- Provides bottom buttons in this order: Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- Rewinds 10 seconds with the Rewind 10s button.
- Fast-forwards 10 seconds with the Fast Forward 10s button.
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
- Saves the last watched file and playback position locally.
- Shows a clickable Resume or Load Different File choice on startup when saved media exists.
- Closes playback and exits the app when the window is closed.
