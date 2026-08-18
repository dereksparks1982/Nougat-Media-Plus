# ReddMedia Changelog

## v0.0.16 - Native Library and Discover AI

- Same-version installer repair 2: corrected the failed-candidate rollback so all tracked files are restored atomically, the untracked patch manifest is removed separately, the accepted Jellyfin runtime is preserved, and rollback errors cannot be reported as passes. The installer recognizes and backs up the exact hash-verified leftovers from the rejected original candidate before recovering the baseline automatically.
- Same-version owner-test repair: restricted the pinned llama.cpp build to the required shared-library target, preventing the unrelated `llama-app` missing-header failure.
- The integrated Jellyfin process is now owned for the exact ReddMedia process lifetime. Normal close performs a bounded graceful stop, forced close triggers a Linux parent-death signal, and shutdown reaps the owned server.
- Added executable validation proving both normal ReddMedia shutdown and forced parent death release the hidden server and port 8096.
- Replaced the flat episode/file list with a native poster grid and explicit Movies and TV libraries.
- Added metadata-driven movie box sets and TV series -> season -> episode navigation; episodes do not appear at the TV root.
- Added typed multi-folder linking and unlinking for Movies and TV. Unlinking changes the catalog only and never deletes media files.
- Added catalog-only migration when an existing v0.0.15 untyped folder is reselected under Movies or TV, so the same files do not need to be moved or re-added from scratch.
- Preserved direct local-path playback through ReddMedia's existing embedded libVLC player.
- Added the top-level Discover tab with Usual and Random modes and exactly four actions in each mode: Local Movie, Local TV, External Movie, and External TV.
- Added one-result-at-a-time recommendations, private SQLite viewing history, local llama.cpp inference, and the pinned Nomic Embed Text v1.5 Q4_K_M model.
- Added optional TMDb external catalog discovery, owner-only token storage, owned-title filtering, explicit service-failure status, and no invented fallback titles.
- Added deterministic validation for all eight Discover paths, Random without history, SQLite persistence, TMDb failure, source/type separation, hierarchy, multi-folder behavior, non-destructive unlinking, embedded playback routing, and red-tree/version identity.
- Advanced the active executable and desktop entry to `ReddMedia_v16` from committed v0.0.15 base `d67cf6e5e0e3ce3036adae5d9695147a7aa771e8`.

## v0.0.15 - Native Library and Hidden Media Catalog Repair

- Repaired the real Jellyfin 10.11.11 scan-completion race without advancing the version. Jellyfin's `204` scan response means the scan was queued, so ReddMedia now polls catalog results until a video from the requested folder is actually indexed or a bounded verification timeout is reached.
- Applied the same indexed-path completion rule to native **Add Media Folder** and the installer's generated-video proof. Look-alike path prefixes are rejected at folder boundaries.
- Repaired Jellyfin 10.11.11 startup readiness without advancing the version: ReddMedia now waits for the real `/Startup/User` API instead of treating the temporary setup server's camel-case public-status response as a ready catalog.
- Updated the installer proof to gate on the same real API before catalog setup, preventing the false `unreadable status` failure seen in owner testing.
- Added a native **Library** tab inside the existing ReddMedia window.
- Added **Add Media Folder**, **Refresh**, a scrollable native video list, keyboard selection, and **Play Selected**.
- Library selections use the cataloged local file path and call ReddMedia's existing embedded `open_media` path, preserving the same libVLC controls, subtitles, audio tracks, chapters, resume behavior, and fullscreen surface.
- Disabled Jellyfin's web client with `--nowebclient`; ReddMedia never opens the Jellyfin setup wizard or browser player.
- Added private local API setup, local-only remote-access settings, ReddMedia-owned authentication state, folder registration, full scans, and video enumeration.
- Added an end-to-end installer proof that starts the hidden service, confirms the web client is unavailable, catalogs a generated video, and finds its direct-play path through the compiled ReddMedia client.
- Repaired the rejected first v0.0.15 foundation without advancing the version. v0.0.15 remains unaccepted until owner validation.

### Foundation retained

- Bundled the stable Jellyfin 10.11.11 Ubuntu 26.04 server and web packages with matching source, GPL licenses, hashes, and exact provenance.
- Extract the upstream prebuilt runtime into ReddMedia without requiring Node, npm, or the .NET SDK during installation.
- Repaired the rejected master-source candidate whose server build was cancelled by a .NET SDK worker-node failure; v0.0.15 remains unaccepted until this repaired candidate passes owner validation.
- Added native server startup, loopback health checks, failure detection, restart handling, and persistent ReddMedia-owned server paths.
- Added live `Server: Starting`, `Server: Ready`, `Server: Fault`, and missing-runtime status to the ReddMedia top bar.
- Kept the hidden service alive when the player window closes so catalog duties can continue.
- Preserved existing local, YouTube, P2P, subtitle, seek, pause, and red-tree behavior.

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

## v0.0.14 - Local Pause Stability & Red Tree Identity

- Local-file Play/Pause now uses explicit libVLC pause state instead of a blind toggle.
- ReddMedia snapshots playback time and duration when pausing and uses cached values for paused seek-bar/time refresh.
- Paused UI refresh no longer performs recurring libVLC time, length, or full chapter-description queries.
- Chapter metadata is cached once per media item after playback metadata becomes available.
- Closing while paused saves the cached resume position without first querying libVLC again.
- Final player shutdown has a bounded teardown safeguard so a stuck libVLC stop/release cannot leave the ReddMedia window alive indefinitely.
- Replaced active ReddMedia icon assets with the approved red-tree artwork.
- The tree identity is used for the launcher, dock/app switcher, raw executable, MIME icons, X11 window icon, and top-right version badge.
- The window title is `ReddMedia`; the in-app top-right identity is a small red tree followed by `v0.0.14`.
