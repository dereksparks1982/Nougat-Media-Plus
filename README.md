# ReddMedia v0.0.8

ReddMedia is a standalone Linux desktop video player by Elderredd Softworks.

## v0.0.8 Direct yt-dlp Working Repair

This build keeps ReddMedia as a video player and adds direct yt-dlp downloading inside the app.

## What this build does

- Provides a visible versioned executable named `ReddMedia_v8`.
- Keeps the Video Player as the base system.
- Adds a top-level `yt-dlp` screen directly inside ReddMedia.
- Packages the real bundled yt-dlp engine at `tools/yt-dlp/yt-dlp`.
- Uses only the bundled yt-dlp engine.
- Provides an inline URL field for typing or pasting video URLs.
- Supports Ctrl+V paste and right-click paste in the URL field.
- Keeps the output folder picker separate from the URL field.
- Shows visible download status and log output.
- Preserves the accepted v0.0.7 playback controls, red UI, subtitles, audio, chapters, fullscreen, and close behavior.
