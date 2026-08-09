# ReddMedia

## v0.0.7 - Red Button Polish

This build does the following:

- Provides a top-level executable named `ReddMedia`.
- Launches as a standalone Linux desktop app.
- Opens local media files.
- Plays video through VLC/libVLC.
- Provides play, pause, stop, seek, time display, fullscreen, and volume controls.
- Uses red bottom control buttons with white lettering.
- Provides bottom buttons in this order: Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- Rewinds 10 seconds with the Rewind 10s button.
- Fast-forwards 10 seconds with the Fast Forward 10s button.
- Opens a working File menu from the red in-app top bar.
- Provides Open File and Exit ReddMedia actions in the File menu.
- Opens a working Audio menu from the red in-app top bar.
- Lists embedded audio tracks when the playing media provides them.
- Allows embedded audio track selection from the Audio menu.
- Opens a working Subtitle menu from the red in-app top bar.
- Allows subtitles to be turned on or off from the Subtitle menu.
- Allows subtitle files to be loaded from the Subtitle menu.
- Allows a subtitle folder to be selected from the Subtitle menu.
- Auto-detects nearby `.srt` subtitle files that match the opened video.
- Turns matching detected subtitles on by default.
- Allows subtitle track changes when subtitle tracks are available.
- Provides subtitle delay earlier, subtitle delay later, and reset subtitle delay controls.
- Provides subtitle on/off from the right-click video menu.
- Provides previous chapter, next chapter, and chapter jump choices from the right-click video menu when real embedded chapters exist.
- Uses real embedded chapter timestamps for seek bar marks when real chapters exist.
- Shows default chapter-style marks on the seek bar when real embedded chapters are not present.
- Keeps the current playback time before the seek bar.
- Keeps the total media duration after the seek bar.
- Uses a red seek/progress bar.
- Uses a red volume bar.
- Uses a red in-app top menu/header bar.
- Uses buffered drawing for the parent-window controls to keep the header, seek bar, volume bar, and buttons stable during playback.
- Uses a red ReddMedia triangle icon for the app window.
- Includes red triangle icon files for local Linux launcher and dock registration.
- Uses a red triangle icon in `ReddMedia.desktop`.
- Shows only the video surface while fullscreen is active.
- Pauses or resumes when the video area is single left-clicked.
- Toggles fullscreen when the video area is double left-clicked.
- Opens the video options menu when the video area is right-clicked.
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
- Closes playback and exits the app when the window is closed or Exit ReddMedia is chosen.
