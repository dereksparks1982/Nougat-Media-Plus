# ReddMedia v0.0.10

ReddMedia is a standalone Linux desktop media player by Elderredd Softworks. It combines local video playback, yt-dlp downloading, and built-in P2P/P2P downloading and streaming in one application.

## Current build: v0.0.10 P2P Streaming Core

The current build adds a permanent P2P screen powered by libtorrent-rasterbar and connects torrent downloading directly to ReddMedia's VLC-based video player.

### Current P2P workflow

- Paste a magnet link or open a local `.torrent` file.
- Choose a download folder.
- ReddMedia retrieves torrent metadata and displays the files in the torrent.
- A single obvious video file is selected automatically; multi-file torrents can be selected manually.
- Press **Play** to begin playback before the torrent finishes downloading.
- ReddMedia serves the selected torrent file to VLC through a localhost-only HTTP Range stream.
- Playback requests drive libtorrent time-critical piece priorities automatically. The user does not choose a technical download strategy.
- The full torrent keeps downloading behind playback and can seed after completion.
- Active torrent resume data is stored under `~/.config/reddmedia/p2p/`.
- The P2P source field supports Ctrl+A and Cut / Copy / Paste.

### v0.0.10 stabilization carried under the same version

The v0.0.10 stabilization pass keeps the same feature version while repairing defects found during owner testing:

- Seek/time and volume partial updates are buffered offscreen before being copied to the X11 window, preventing the direct erase/redraw path that caused visible flashing.
- A new P2P HTTP range request supersedes obsolete stream workers so an old seek cannot continue fighting a newer seek for torrent pieces.
- Old time-critical torrent piece deadlines are cleared when VLC starts a new stream range request.
- HTTP suffix byte ranges such as `Range: bytes=-5000000` are supported for VLC/container probing.
- Stream sockets have bounded send/receive waits so abandoned seek connections cannot hang indefinitely.
- The installer reapplies and verifies the ReddMedia red-triangle custom icon on the versioned executable.

The versioned executable is `ReddMedia_v10`.

## Main ReddMedia features

- Native X11 desktop interface.
- VLC/libVLC local video playback.
- Open, Play/Pause, Stop, Rewind 10s, Fast Forward 10s, timeline seeking, volume, fullscreen, and resume support.
- Keyboard and mouse playback controls.
- Embedded audio-track selection.
- External and embedded subtitle controls, automatic matching `.srt` loading, subtitle folder selection, and subtitle delay controls.
- Embedded chapter discovery and chapter navigation when exposed by libVLC.
- Bundled yt-dlp downloader with URL entry and output-folder selection.
- Built-in P2P magnet and `.torrent` downloading with stream-while-downloading playback.
- Red ReddMedia branding, red controls, red seek/volume bars, and red-triangle window/launcher/executable identity.

## Dependencies

See [`DEPENDENCIES.md`](DEPENDENCIES.md) for the exact Ubuntu runtime and build requirements and one-command installation lines.

ReddMedia bundles its yt-dlp executable under `tools/yt-dlp/`. VLC/libVLC, FFmpeg, X11, Zenity, and libtorrent are currently supplied by the Linux system.

## Running the current development build

From the ReddMedia project folder:

```bash
./ReddMedia_v10
```

Version check:

```bash
./ReddMedia_v10 --version
```

Expected output:

```text
ReddMedia v0.0.10
```

# Release history

The entries below describe why each numbered ReddMedia build existed and what changed for the user. They are reconstructed from the versioned build handshakes, validation records, changelog, and source history in this repository.

## v0.0.1 — VLC-Style Base Video Player

**Purpose:** establish the first accepted standalone ReddMedia player instead of the earlier rejected prototypes.

What this build added:

- A top-level native ReddMedia executable.
- VLC/libVLC-backed local video playback.
- Open-file support.
- Play/Pause and Stop controls.
- Timeline seeking and current playback time.
- Volume control.
- Fullscreen playback.
- Cursor auto-hide during playback.
- Clean close behavior.
- Basic saved-session resume/load behavior.
- Initial README, changelog, roadmap, license, third-party notices, build handshake, and validation record.

Validation highlights:

- Native C++ build/package checks passed.
- Full graphical playback validation was reserved for the Ubuntu desktop because the build sandbox did not provide the live VLC/X11 environment.

## v0.0.2 — VLC-Style Player Repair 1

**Purpose:** make fullscreen, resume, and repaint behavior act like a real desktop media player.

What changed:

- Fullscreen became video-only, hiding the normal controls.
- Double-clicking the video toggled fullscreen.
- Escape exited fullscreen.
- Resume became a clickable video-area action.
- Resume reopened the saved media file and retried the saved seek after playback began.
- **Load Different File** opened normal file selection.
- Mouse wheel input changed volume only.
- Normal playback refresh stopped clearing the live video surface, reducing flicker.
- Close behavior continued to save session state and release VLC cleanly.

Validation highlights:

- C++17 `-Wall -Wextra -Werror` compile passed.
- X11/dynamic-libVLC source contracts for fullscreen, resume, volume, and repaint behavior passed.

## v0.0.3 — Mouse, Keyboard, Time Display, and Flicker Repair

**Purpose:** make direct player interaction quicker and repair the bottom playback layout.

What changed:

- Single left-click on video toggled pause/resume.
- Double left-click continued to toggle fullscreen without the single-click action firing first.
- Right-click on video opened a basic player options menu.
- Up/Down Arrow changed volume.
- Left/Right Arrow sought backward/forward by 10 seconds.
- Keyboard seek jumps were standardized to 10 seconds.
- The bottom layout reserved visible space for the time display.
- Playback repaint behavior continued avoiding full-window clearing to reduce flicker.

Validation highlights:

- C++17 warnings-as-errors compile passed.
- Mouse, keyboard, fullscreen, seek, and redraw source contracts passed.

## v0.0.4 — Time Layout, Skip Buttons, and Red Timeline Repair

**Purpose:** turn the bottom controls into the player layout used by later ReddMedia builds.

What changed:

- Current time moved to the left of the seek bar.
- Total duration moved to the right of the seek bar.
- The time/seek row was lowered to create space below the video.
- The seek/progress fill changed to ReddMedia red.
- Chapter-style tick marks were added to the timeline.
- The volume fill changed to ReddMedia red.
- Visible **Rewind 10s** and **Fast Forward 10s** buttons were added.
- The bottom button order became: Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- The new skip buttons used the same 10-second seek behavior as the arrow keys.

Validation highlights:

- Build/package checks passed.
- Owner-side playback testing was required for the final visual positioning and controls.

## v0.0.5 — Branding and Polish Flicker/Icon Repair

**Purpose:** establish ReddMedia's visual identity and move the parent UI to buffered drawing.

What changed:

- Added the red ReddMedia triangle icon assets in multiple sizes.
- Set the X11 `_NET_WM_ICON` window icon.
- Set the X11 window class to `ReddMedia` for launcher/dock matching.
- Updated the desktop launcher to use `Icon=reddmedia` and `StartupWMClass=ReddMedia`.
- Kept the native GNOME/window title bar under system control.
- Kept the in-app header red.
- Replaced direct/partial parent-window repainting with offscreen buffered drawing before onscreen copy.
- Preserved the v0.0.4 red timeline, red volume bar, controls, playback, fullscreen, and resume behavior.
- Removed a rejected custom MIME-icon approach that could interfere with double-click launching.
- Used safe local icon registration/custom-icon metadata attempts for the raw executable.

Validation highlights:

- Native warnings-as-errors compile and link passed.
- Buffered parent-window drawing, icon assets, window class, launcher identity, and safe icon path checks passed.

## v0.0.6 — Menu, Audio, Subtitle, Chapter, and Close Behavior Repair

**Purpose:** turn the top menus and media-track controls into functional player features.

What changed:

- File became a real menu with **Open File** and **Exit ReddMedia**.
- Audio menu gained embedded audio-track discovery and selection through libVLC.
- Subtitle menu gained subtitles on/off, subtitle-file loading, subtitle-folder loading, delay earlier/later, delay reset, and embedded subtitle-track selection.
- Matching `.srt` files could be detected automatically beside the video or inside common subtitle folders.
- A detected matching subtitle could be enabled automatically.
- Right-click video options gained subtitle control.
- Real embedded chapter information was used when libVLC exposed it.
- Timeline chapter marks used real chapter timestamps when available.
- Right-click chapter controls gained Previous Chapter, Next Chapter, and direct chapter jumps.
- The buffered UI and red-triangle branding from v0.0.5 were preserved.

Validation highlights:

- C++17 warnings-as-errors compile passed.
- Menu/audio/subtitle/chapter package contracts passed.
- Media-specific audio/subtitle/chapter behavior required owner testing with files that actually contain those tracks or metadata.

## v0.0.7 — Red Button Polish

**Purpose:** finish the visible red control-button direction.

What changed:

- Bottom player control buttons changed to red.
- Button lettering changed to white.
- The established button order and player behavior were preserved.

Validation highlights:

- Warnings-as-errors compile passed.
- Executable/package and version checks passed.
- Desktop testing verified the visual behavior before acceptance.

## v0.0.8 — Direct yt-dlp

**Purpose:** put the downloader inside ReddMedia as a permanent application screen.

What changed:

- Added the direct yt-dlp screen inside ReddMedia.
- Bundled the real Linux yt-dlp executable at `tools/yt-dlp/yt-dlp`.
- Added direct URL typing.
- Added Ctrl+V keyboard paste and right-click paste to the URL field.
- Kept output-folder selection separate from URL entry.
- Introduced the versioned executable name `ReddMedia_v8`.
- Preserved the v0.0.7 video-player behavior.
- The apply workflow attempted safe red-triangle custom-icon metadata for the raw executable and installed the matching desktop launcher identity.

Validation highlights:

- Bundled yt-dlp and build/package checks passed.
- Real desktop/download behavior required owner-side testing.

## v0.0.9 — URL Field Text Controls

**Purpose:** make the yt-dlp URL box behave like a normal editable text field.

What changed:

- Ctrl+A selects the entire yt-dlp URL.
- Full-field selection is visibly highlighted.
- Right-click opens **Cut / Copy / Paste**.
- Cut and Copy place the complete selected URL on the X11 clipboard.
- Paste replaces a full-field selection before inserting clipboard text.
- The versioned executable advanced to `ReddMedia_v9`.

Validation highlights:

- Warnings-as-errors native compile passed.
- `ReddMedia_v9 --version` and native ELF checks passed.
- An Xvfb-driven interaction test proved Ctrl+A, Copy, Cut, Paste, and clipboard round-trip behavior.
- Installer and forced rollback rehearsals passed.
- Owner-side live URL-field testing passed before acceptance.

## v0.0.10 — P2P Streaming Core

**Purpose:** add P2P directly to ReddMedia and make watching while downloading the normal P2P behavior.

What this build added:

- Permanent **P2P** application screen.
- libtorrent-rasterbar 2.x integration.
- Magnet-link loading.
- Local `.torrent` file loading.
- Torrent metadata retrieval and file listing.
- Automatic selection of a single obvious video file.
- Manual file selection for multi-file torrents.
- Download folder selection.
- Live torrent name, state, progress, downloaded amount, download/upload speed, peers, and seeds.
- Complete torrent downloading and seeding behind playback.
- Persistent P2P resume data.
- Ctrl+A and Cut / Copy / Paste in the P2P source field.
- A localhost-only HTTP Range bridge between the torrent engine and VLC.
- Time-critical libtorrent piece deadlines driven by what VLC needs for playback.
- Stream-while-downloading playback without exposing separate sequential-download controls.
- `DEPENDENCIES.md` for runtime and developer requirements.
- libtorrent BSD license/third-party notice records.
- Roadmap work for a future self-contained Linux distribution.

Owner-test results that established the milestone:

- Magnet-link intake and torrent downloading worked.
- Local `.torrent` intake and torrent downloading worked.
- Torrent metadata, file list, peer/seed status, and automatic video selection worked.
- Playback began while a torrent was still downloading.

Stabilization repairs in the current v0.0.10 candidate:

- Restores buffered seek/time and volume partial repainting to remove the flashing regression.
- Cancels obsolete P2P stream requests when VLC seeks to a new range.
- Clears obsolete torrent piece deadlines on a new stream range request.
- Adds legal HTTP suffix-range support used by media probing/seeking.
- Adds bounded stream-socket waits for abandoned requests.
- Reapplies and validates the red-triangle custom icon on `ReddMedia_v10`.
- Expands this README so every numbered ReddMedia release explains what it actually did.

Further owner testing is still the authority for seek behavior under slow or difficult swarms because peer availability can change how long an undownloaded seek takes to resume.

## Third-party software

See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and the license files under `licenses/`.

## Roadmap

See [`ROADMAP.md`](ROADMAP.md) for the next planned ReddMedia milestones.
