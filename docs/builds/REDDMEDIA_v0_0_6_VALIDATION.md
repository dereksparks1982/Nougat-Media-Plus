# ReddMedia v0.0.6 Validation

## Build identity

- Version: v0.0.6
- Build name: Menu, Audio, Subtitle, Chapter, and Close Behavior Repair
- Required base: v0.0.5

## Automated/package validation

- C++ compile with `-std=c++17 -Wall -Wextra -Werror`: PASS
- Linked against X11 and dl: PASS
- Top-level executable exists: PASS
- Package includes README.md: PASS
- Package includes CHANGELOG.md: PASS
- Package includes ROADMAP.md: PASS
- Package includes ReddMedia.desktop: PASS
- Package includes red triangle icon assets from v0.0.5: PASS
- Package excludes yt-dlp/p2p/module implementation: PASS

## Manual validation checklist for Derek

- Double-click `ReddMedia` opens the app.
- File menu opens without freezing.
- File menu Open File action opens media selection.
- File menu Exit ReddMedia closes the app cleanly.
- Window close button closes the app cleanly.
- Audio menu opens without freezing.
- Audio menu lists embedded audio tracks when the file has them.
- Selecting an embedded audio track changes audio track.
- Subtitle menu opens without freezing.
- Subtitle menu can turn subtitles on and off.
- Subtitle menu can load a subtitle file.
- Subtitle menu can select a subtitle folder.
- Matching English `.srt` beside the video auto-loads and starts on.
- Subtitle menu delay earlier moves subtitles earlier.
- Subtitle menu delay later moves subtitles later.
- Reset subtitle delay returns delay to 0.
- Right-click video menu can turn subtitles on and off.
- Right-click video menu shows Previous Chapter and Next Chapter when real embedded chapters exist.
- Right-click chapter choices jump to real embedded chapter timestamps.
- Seek bar marks line up with real embedded chapters when real chapters exist.
- Seek bar shows default decorative marks when real embedded chapters do not exist.
- Red in-app top bar does not flash.
- Red seek bar does not flash.
- Red volume bar does not flash.
- Playback remains stable.
- Fullscreen remains video-only.
- Escape exits fullscreen.
- Spacebar pauses/resumes.
- Left/Right arrows seek 10 seconds.
- Up/Down arrows adjust volume.
