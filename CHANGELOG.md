# ReddMedia Changelog

## v0.0.20 - Stream, Full Tab Palettes, Volume Boost, and Library Views (candidate)

## v0.0.20 small-handoff repair

- Reissued the accepted-base candidate without re-shipping the unchanged 84,106,624-byte Nomic model, reducing transfer size and avoiding repeated large-file download corruption.
- The installer now verifies the exact pinned v0.0.19 model in place before changing any source. Missing or altered model state causes a clean preflight stop.
- No v0.0.20 feature or behavior changed in this handoff repair.


- Replaced the top-level YouTube area with **Stream** in the exact order `Video Player | Library | Discover | Nougat | Stream | P2P | Debug`.
- Added Stream platform selectors for YouTube, Rumble, RuTube, VK, and OK while preserving URL Download/Play, activity output, and the existing yt-dlp playback path.
- Added **Direct Watch** to send a supplied supported video URL directly to ReddMedia's native player and **Open Webpage** to open the supplied/platform webpage in the user's default browser.
- Added coordinated full interior palettes: Video Player red/burgundy, Library forest/sage, Discover plum/lavender, Nougat cocoa/tan/caramel/cream, Stream teal/cyan, P2P navy/steel, Debug charcoal/amber.
- Expanded volume from 0-100% to **0-200%**, with a 100% default, live percent display, and visible normal-volume marker.
- Centered the compact bottom player-control group when enough width exists while preserving horizontal wheel scrolling when narrow.
- Added persistent independent **Grid/List** Library view choices for Movies and TV.
- Added visible focus carets to the custom editable text fields used by Nougat, Stream, and P2P.
- Preserved v0.0.19 TV next-episode autoplay, Library navigation continuity, Nougat integration, existing media/server behavior, and app-wide compact scrolling control rows.
- Deferred poster-quality overhaul, rounded corners, shorter volume geometry, wide-window top-nav centering, local-only Web Player, and Plex integration to the next polish/feature lane.

## v0.0.19 - Nougat Search Integration

- Repaired the rejected first v0.0.19 candidate under the same version: the installer now installs the missing Ubuntu X11 smoke-test packages (`xvfb` and `x11-utils`) when required before rerunning validation.
- Standardized normal action buttons across ReddMedia to the compact top-bar footprint and placed button rows directly adjacent; overflowing top navigation and action rows scroll horizontally with the mouse wheel instead of wrapping or hiding controls.
- Reworked the Video Player footer into separate seek, volume, and compact-button rows so Fullscreen cannot be covered by the volume bar at narrow/half-screen widths.
- Added a live numeric volume percentage that updates for slider changes, mouse-wheel volume changes, and keyboard volume changes.
- Added TV episode autoplay across the current series, including season boundaries, while preserving the Library series/season navigation state when playback switches to the Video Player. Natural episode completion updates viewing history; manual Stop cancels autoplay.
- Added distinct interior themes while preserving the red ReddMedia top bar: Video Player red, Library forest green, Discover plum, Nougat cocoa/tan/caramel, YouTube red, P2P deep blue, and Debug amber/yellow.
- Added Nougat as a native top-level ReddMedia tab in the exact order **Video Player | Library | Discover | Nougat | YouTube | P2P | Debug** while preserving Discover and existing ReddMedia subsystems.
- Integrated a headless Nougat engine under `components/nougat/` with SQLite FTS5 local indexing, Ranked and RAW search, recursive HTML crawling, direct peer querying, persisted node identity/peer configuration, and Tor/onion retrieval support through the local Tor service when available.
- Added a native C++ Nougat bridge so ReddMedia owns the UI and does not embed the standalone Tk prototype. The active integrated data directory is `~/.local/share/reddmedia/nougat/`; no standalone Nougat project folder is required.
- Preserved the approved Nougat candy-bar palette inside the Nougat tab without recoloring the rest of ReddMedia.
- Added selectable/copyable crawler output with mouse selection, Ctrl+C, Ctrl+A, right-click Copy, and right-click Select All while keeping the output read-only.
- Rejected bare crawler words such as `google` with a clear domain/URL prompt while accepting and normalizing real domains and HTTP(S) URLs.
- Added deterministic Nougat local/RAW/ranked/peer/persistence tests and retained the full v0.0.18 metadata, provider, diagnostic, and recommendation regression lane under v0.0.19.
- Advanced the root executable to `ReddMedia_v19` and retained the approved red-tree ReddMedia executable/launcher identity.
- Applied PolyForm Noncommercial 1.0.0 to owner-created ReddMedia/Nougat material for recipients; third-party components retain upstream licenses, and the copyright owner retains all ungranted rights including commercial use/licensing of the owner's own work.

## v0.0.18 - Intelligent Debug, Metadata, Watch Availability, and Responsive Library

- Added verified season/episode identity and a separate technical-format line to episode tiles, with explicit unavailable labels rather than filename guesses.
- Added ordered artwork fallback from the item's Jellyfin image to parent/series artwork and then exact TMDb movie/series/season artwork.
- Added exact TMDb episode-title and overview fallback when Jellyfin is incomplete and a verified series ID is available.
- Added an evidence-based Debug tab with green/yellow/red health, filesystem and port evidence, metadata counts, credential-redacted reports, and real retry/refresh/test/log actions.
- Replaced duplicate Library server text with one top-bar `Server:` light: green ready, yellow transitioning, red unavailable.
- Reworked the native Library grid to show multiple rows at normal non-fullscreen size and share its responsive geometry across drawing, scrolling, and arrow navigation.
- Reworked Discover details to preserve and wrap the beginning of the synopsis and to scroll when content exceeds the available area.
- Added complete United States watch-provider categories from JustWatch via TMDb, local private My Services selections, refresh timestamps, explicit no-listing messages, mandatory attribution, and TMDb-supplied official watch-options links.
- Advanced the executable and launcher identity to `ReddMedia_v18`; the installer reapplies and reads back the approved red-tree metadata after the executable's final write.
- Added deterministic v0.0.18 tests for provider categories, service preferences, credential safety, Movie/TV separation, episode identity, artwork fallback, and diagnostic truth.

## v0.0.17 - Library, Discover, and Server Reliability

- Added the canonical ReddMedia-only project `COMPANY_BIBLE.md` and removed unrelated project material from that record.
- Added strict Movie/TV filtering immediately before selection and a final result-type assertion for Local and External Usual/Random recommendations.
- Added TMDb API-key and read-access-token recognition, validation-before-replacement, Test, Save/Replace, Clear, owner-only persistence, and explicit 401 guidance.
- Repaired local posters by requesting a supported Jellyfin source format, normalizing it for the native renderer through FFmpeg, and caching successful images by item/image tag.
- Added real TMDb poster retrieval and local caching for External recommendation results.
- Moved Library poster retrieval off the X11 draw path and added a full-width red progress bar for Library, poster, Discover, TMDb, and server work.
- Added visible Start Server, Stop Server, and Refresh Server controls while preserving the rule that ReddMedia stops only the Jellyfin process it owns.
- Added Git exclusions for generated Jellyfin/AI runtimes, build trees, and Python caches.
- Advanced the executable and launcher identity to `ReddMedia_v17`; final installer validation reapplies and reads back the approved red-tree icon metadata on the raw executable after its last write.
- Added deterministic v0.0.17 regression coverage for repeated type separation, both TMDb credential forms, failed replacement preservation, 401 handling, Clear, local/external posters, caches, progress/UI controls, and the project Bible.

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

## v0.0.19 Viewing-history completion repair

- Added the missing `RecommendationEngine::record_completed` and `ViewingHistory::record_completed` API to the changed-files payload so the native build receives the same completion support used by TV autoplay.
- Viewing history now persists a `completed` flag with a non-destructive SQLite migration for existing history databases.
- Starting playback records `completed=0`; natural episode completion records `completed=1`. Manual Stop does not call the completion path.
- Deterministic validation now compiles the real recommendation/history sources and verifies an ordinary started movie remains incomplete while a completed TV episode persists as complete.
- Same target remains v0.0.19 because the preceding candidate failed compilation and rolled back to v0.0.18.


## v0.0.19 Pinned AI model/runtime-layout repair

- Corrected the full native Discover AI gate after the prior candidate compiled successfully but failed its AI self-test.
- Verified the owner-supplied component archive contains the pinned `nomic-embed-text-v1.5-Q4_K_M.gguf` model at exactly 84,106,624 bytes with SHA-256 `d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac`.
- Bundles that exact model in the repair package because the deployed v0.0.18 changed-file baseline does not contain the model file.
- The installer preserves an already-correct model, safely replaces a non-matching model with rollback backup, or installs the verified model when absent.
- Corrected the real-AI build gate so the temporary `/tmp` build executable receives a project-equivalent `components/ai/models` path before `--discover-ai-self-test`; the previous gate incorrectly searched relative to the temporary build executable.
- Added a second real Discover AI self-test after the final `ReddMedia_v19` executable is copied to the project root, so FINAL PASS proves the installed root executable can actually load and use the pinned model.
- Same target remains v0.0.19 because the previous candidate failed and rolled back to v0.0.18.

### v0.0.20 candidate handoff repair — accepted v0.0.19 base manifest
- Corrected the installer/package base gate to the final accepted v0.0.19 `REDDMEDIA_PATCH_MANIFEST_v19.json` hash.
- No v0.0.20 feature behavior changed.
