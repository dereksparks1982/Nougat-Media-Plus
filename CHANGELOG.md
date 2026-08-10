# ReddMedia Changelog

## v0.0.13

### YouTube Growing Cache Stream Repair

- Fixed the real-world YouTube freeze where playback stopped after only the bytes that existed when VLC first opened the localhost cache bridge.
- Open-ended VLC range requests are now served as an indeterminate-length growing HTTP stream instead of advertising the current cache frontier as the complete media length.
- Added chunked live-range delivery so the bridge waits for newly appended cache bytes and keeps feeding VLC while yt-dlp/FFmpeg continues downloading.
- Kept bounded and suffix byte-range support for cached probes while reporting unknown total length until the feeder is complete.
- Increased the YouTube startup cache target to 512 KiB and embedded VLC network caching to 5000 ms for a steadier start.
- Kept YouTube playback capped at 1080p and preserved timestamp-restart seeking, stale-stream replacement, Stop cleanup, P2P behavior, and the red-star ReddMedia identity.
- Added a deterministic slow-growing stream regression that proves a single VLC-style open-ended request continues receiving bytes beyond the cache size that existed when the request began.

## v0.0.12

### YouTube Seekable Cache Bridge

- Same-version identity repair: replaced the ReddMedia triangle with a red star for the application/window/launcher/raw-executable identity.
- Changed the GNOME/X11 window title to `★ ReddMedia` with no version number and changed the in-app top-right identity surface to version-only `v0.0.12`.
- Same-version UI repair: renamed the creator-facing tab, page heading, activity-log heading, and visible status messages to **YouTube** while retaining `yt-dlp` only as the technical engine/tool name.
- Replaced the accepted v0.0.11 YouTube stdout-to-libVLC pipe with a ReddMedia-managed temporary cache and localhost HTTP playback bridge.
- Kept YouTube Play capped at 1080p by default.
- Added yt-dlp duration probing so the normal ReddMedia seek timeline can represent the complete video.
- Added localhost-only HTTP `HEAD`, full `GET`, byte-range, suffix-range, and invalid-range handling for cached playback.
- Added seek restart support using yt-dlp `--download-sections` with keyframe-aware cuts when the requested position is outside the current cached segment.
- Added cancellation of obsolete seek feeders and cleanup of temporary YouTube cache files on replacement playback, Stop, and shutdown.
- Added the dedicated `YtDlpStreamServer` source module and versioned `ReddMedia_v12` executable/desktop identity.

## v0.0.11

### Same-version repair after owner testing

- Repaired yt-dlp Play so the yt-dlp/FFmpeg stream's HTTP request headers are carried into embedded libVLC playback.
- Added libVLC Play startup-result checking for direct yt-dlp playback.
- Reworked the executable-icon release gate so the final binary receives the red-triangle icon assignment after rebuild and GNOME Files/Nautilus is refreshed when available.
- Tightened the repository-wide P2P terminology gate across tracked text and historical release descriptions.
- Corrected the visible application top bar to v0.0.11.
- P2P Stop/Resume remained owner-validated from the first v0.0.11 candidate.

### Playback & Transfer Controls

- Added **Stop Download / Resume Download** to P2P transfers.
- Stopping a P2P transfer stops active P2P playback and transfer activity while preserving partial data and resume state.
- Added YouTube **Play** for direct network playback through ReddMedia's embedded VLC player while preserving the existing Download path.
- Standardized public-facing feature wording on **P2P** across README, roadmap, dependency, and third-party documentation.
- Added Archive, Online Video, Live TV, and supported streaming-service integration directions to the roadmap.

## v0.0.10

### Stabilization repair carried under v0.0.10

- Restored buffered offscreen repainting for seek/time and volume partial updates to remove the flashing regression.
- Added P2P stream-request generations so obsolete VLC HTTP range workers stop when a newer seek request arrives.
- Clear obsolete libtorrent piece deadlines when VLC starts a new stream range request.
- Added HTTP suffix byte-range support for media probing and seeking.
- Added bounded P2P stream socket waits so abandoned seek connections cannot block indefinitely.
- Reapplied and validated the red-triangle custom icon metadata on the versioned `ReddMedia_v10` executable.
- Rewrote README release history so every ReddMedia build from v0.0.1 through v0.0.10 explains its purpose and user-visible changes.

### P2P Streaming Core

- Added the permanent P2P Streaming screen powered by libtorrent-rasterbar.
- Added magnet-link and `.torrent` loading with torrent metadata and file selection.
- Added live torrent status for progress, downloaded data, transfer rates, peers, and seeds.
- Added stream-while-downloading playback through a localhost-only HTTP Range bridge to VLC.
- Added playback-aware libtorrent piece deadlines so startup and seeks automatically prioritize the data VLC needs next.
- Added P2P resume-state persistence under `~/.config/reddmedia/p2p/`.
- Added Ctrl+A and Cut / Copy / Paste behavior to the P2P magnet field.
- Added `DEPENDENCIES.md` with runtime and developer dependency requirements.
- Added libtorrent BSD licensing notice and expanded third-party notices.
- Added the self-contained Linux distribution target and full P2P client expansion to the roadmap.

## v0.0.9

- Ctrl+A now selects the full URL in the YouTube URL field.
- Right-click inside the URL field now opens Cut / Copy / Paste.

## v0.0.8

- Added direct YouTube screen to ReddMedia, powered by the bundled yt-dlp engine.
- Bundled the real yt-dlp Linux engine inside `tools/yt-dlp/yt-dlp`.
- Added versioned executable name `ReddMedia_v8`.
- Added inline URL entry with keyboard paste and right-click paste support.
- Preserved v0.0.7 video player behavior.

- YouTube Play now streams through the bundled yt-dlp/FFmpeg pipeline into embedded libVLC and caps playback at 1080p by default.

## v0.0.13 - YouTube Growing Cache and Seek Stability

- Improved sustained YouTube playback through the growing localhost cache bridge.
- YouTube playback progressed beyond the previous few-second playback limit.
- Seek-bar selection can restart playback near the requested position.
- YouTube feeder replacement is used for seeking.
- Improved playback and shutdown stability.
- Preserved embedded YouTube playback and the 1080p maximum target.

Known limitations:
- Actual selected playback resolution still needs direct verification.
- Seeking can take approximately 10 to 15 seconds to resume.
- Playback may alternate between buffering and playing after a seek.
- Further sustained-buffering and quality improvements are planned for v0.0.14.
