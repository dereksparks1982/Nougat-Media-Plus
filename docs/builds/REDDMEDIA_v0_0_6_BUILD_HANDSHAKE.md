# ReddMedia v0.0.6 Build Handshake

## Project and version

- Project: ReddMedia
- Version: v0.0.6
- Build name: Menu, Audio, Subtitle, Chapter, and Close Behavior Repair

## Required base and target

- Required base version: ReddMedia v0.0.5
- Required base commit: c9d09de
- Required base tag: v0.0.5
- Target version: ReddMedia v0.0.6

## Package filename

- ReddMedia_v0_0_6_MENU_AUDIO_SUBTITLE_CHAPTER_CLOSE_REPAIR.zip

## Completed changes

- Repaired the top File menu so it opens a menu instead of directly triggering the file picker.
- Added File menu actions for Open File and Exit ReddMedia.
- Repaired Audio menu behavior.
- Added embedded audio track discovery through libVLC when available.
- Added embedded audio track selection through the Audio menu.
- Repaired Subtitle menu behavior.
- Added Subtitle menu actions for subtitles on/off, load subtitle file, open subtitle folder, subtitle delay earlier, subtitle delay later, reset subtitle delay, and subtitle track selection.
- Added automatic matching `.srt` subtitle detection beside the opened video and inside common subtitle subfolders.
- Added default-on subtitle behavior when a matching subtitle is detected.
- Added subtitles on/off to the right-click video menu.
- Added real embedded chapter handling when libVLC exposes chapter descriptions.
- Updated seek bar chapter marks to use real embedded chapter timestamps when present.
- Preserved default decorative chapter-style marks when real embedded chapters are not present.
- Added right-click chapter controls for Previous Chapter, Next Chapter, and direct chapter jumps when real embedded chapters are present.
- Preserved the native grey GNOME/window title bar.
- Preserved v0.0.5 buffered parent-window drawing so the red in-app header, seek bar, and volume bar stay stable during playback.
- Preserved the red triangle app/window/launcher icon assets.

## Excluded work

- yt-dlp is not included in v0.0.6.
- p2p is not included in v0.0.6.
- Module/mod installer work is not included in v0.0.6.
- Media library work is not included in v0.0.6.
- Playlist work is not included in v0.0.6.
- Installer packaging is not included in v0.0.6.

## Changed files

- ReddMedia
- src/main.cpp
- CMakeLists.txt
- README.md
- CHANGELOG.md
- ROADMAP.md
- ReddMedia.desktop
- docs/builds/REDDMEDIA_v0_0_6_BUILD_HANDSHAKE.md
- docs/builds/REDDMEDIA_v0_0_6_VALIDATION.md

## Validation results

- Native C++ compile with `-Wall -Wextra -Werror`: PASS
- Top-level executable named `ReddMedia` exists: PASS
- Version updated to v0.0.6 in the app title and docs: PASS
- README updated to describe v0.0.6 behavior only: PASS
- yt-dlp, p2p, and module work excluded from build docs: PASS
- Package ZIP created: PASS

## Known failures or risks

- Embedded audio and subtitle track availability depends on the media file and libVLC exposing track descriptions.
- Real chapter marks require libVLC chapter descriptions. When real embedded chapters are not exposed, ReddMedia keeps default decorative marks.
- Subtitle folder selection loads a matching subtitle file when one can be found for the current video name.
- External file/folder picker tools such as zenity or tkinter may block while the picker dialog itself is open.

## Rollback point

- Roll back to accepted ReddMedia v0.0.5 if v0.0.6 fails manual testing.
- Accepted v0.0.5 snapshot: /home/dereksparks1982/DKLab/Archive/ReddMedia_v0_0_5_ACCEPTED_20260809_170200
- Accepted v0.0.5 commit: c9d09de
- Accepted v0.0.5 tag: v0.0.5

## Current continuation point

- Apply the v0.0.6 candidate ZIP to `~/DKLab/Projects/ReddMedia`.
- Derek manually tests File, Audio, Subtitle, right-click chapter/subtitle options, subtitles, close behavior, and no-flicker behavior.
- Do not snapshot, Git commit/tag, or push to GitHub until Derek accepts.

## Next planned work

- If v0.0.6 is accepted, close out using the staged rule: snapshot first, Git second, GitHub third.
- After v0.0.6 is accepted, revisit module/add-on system planning before yt-dlp or p2p work.

## Required input files/artifacts

- ReddMedia v0.0.5 accepted project tree.
- Media files with multiple embedded audio tracks for manual Audio menu testing.
- Media files with external `.srt` subtitle files for subtitle auto-load/manual load testing.
- Media files with embedded chapters for real chapter mark and chapter jump testing.
