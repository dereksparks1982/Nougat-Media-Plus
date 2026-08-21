# ReddMedia v0.0.20

ReddMedia is an integrated Linux media suite by Elderredd Softworks. It combines native local playback, a hidden local Jellyfin catalog foundation, local recommendation AI, optional TMDb discovery, Nougat decentralized search, multi-platform Stream URL handling, and built-in P2P transfer/streaming in one desktop application.

## Current candidate: v0.0.20 Stream, Full Tab Palettes, Volume Boost, and Library Views

v0.0.20 builds directly on the owner-accepted v0.0.19 baseline. The top-level order is **Video Player | Library | Discover | Nougat | Stream | P2P | Debug**. The former YouTube top-level area becomes **Stream**, with platform selectors for **YouTube, Rumble, RuTube, VK, and OK**. Stream keeps the existing URL-based Download and Play workflow, adds **Direct Watch** through ReddMedia's native player, and adds **Open Webpage** through the user's default browser. The same yt-dlp engine remains the URL playback/download backend where the selected public URL is supported.

Each major area now has a coordinated interior palette instead of a black/gray surface with one accent color. Video Player uses the ReddMedia red/burgundy family, Library uses forest/sage, Discover uses plum/lavender, Nougat preserves its cocoa/tan/caramel/cream identity, Stream uses deep-ocean teal/cyan, P2P uses navy/steel blue, and Debug uses charcoal/amber. The common top navigation remains ReddMedia red.

Video volume now defaults to **100%** and can be amplified from **0% through 200%**, with the live percentage and a visible 100% marker. The compact bottom player-control group centers when the window has enough room and retains horizontal mouse-wheel scrolling when narrow.

Movies and TV now each remember their own **Grid** or **List** Library view. List mode presents compact rows with identifying metadata; Grid preserves the existing poster layout. Editable in-app text fields show a visible caret when focused.

Poster-quality/aspect-ratio overhaul, rounded-corner visual polish, shorter/balanced volume-bar geometry, wide-window top-navigation centering, the optional local-only ReddMedia Web Player button, and Plex account/server integration are intentionally reserved for v0.0.21 or later.

The final versioned executable for this candidate is `ReddMedia_v20`. This candidate remains unaccepted until owner installation and visual/functional testing.

## Previous candidate: v0.0.18 Intelligent Debug, Metadata, Watch Availability, and Responsive Library

v0.0.18 makes the native Library and Discover screens explain more and guess less. Episode tiles show verified `SxxExx - title` identity with technical format on a secondary line. Artwork resolves through the item's Jellyfin image, parent/series artwork, and then exact TMDb movie/series/season artwork. Missing titles or posters remain explicitly unavailable when no verified match exists.

The new **Debug** tab runs evidence-based checks against the integrated server, port 8096, generated runtimes, local paths, the current Library level, artwork failures, TMDb configuration, and background work. Results use green, yellow, or red health with a concrete next action. **Run Checks**, **Retry**, **Refresh Metadata**, **Test TMDb**, **Refresh Server**, **Open Logs**, and **Copy Report** invoke real actions; copied reports omit credentials.

Discover now preserves and wraps the beginning of long descriptions. External results show exact United States subscription, free, ad-supported, rental, and purchase listings returned by JustWatch through TMDb, with a refresh time and explicit no-listing state. **My Services** privately marks providers the owner uses; it does not sign into a provider. **Open Watch Options** opens only the official link supplied by TMDb.

The top bar has one `Server:` light: green when ready, yellow during a transition, and red when unavailable. The duplicate Library status was removed. The Library grid now derives drawing, wheel scrolling, and arrow movement from the same responsive layout and shows at least two rows at the normal 1000-by-650 non-fullscreen size.

The final versioned executable is `ReddMedia_v18`. Its installer writes the executable first, assigns the approved red-tree custom-icon metadata directly to that raw executable, reads the assignment back, and refreshes Files/Nautilus before owner visual confirmation. This candidate is not accepted or tagged.

## Previous candidate: v0.0.17 Library, Discover, and Server Reliability

v0.0.17 is one reliability build over the technically working v0.0.16 checkpoint. Movie and TV recommendation requests now pass a final strict type gate, including repeated Random selections. Jellyfin poster responses use a supported source format, are normalized through FFmpeg for the native X11 renderer, and are cached locally; external TMDb results use and cache their real TMDb poster paths. A full-width red loading bar reports Library, poster, Discover, credential, and server work.

Discover now accepts either a TMDb 32-character API key or a TMDb read access token. The screen exposes **Test TMDb**, **Save / Replace**, and **Clear TMDb**. A replacement is validated before it can overwrite a working credential, the saved file remains owner-only, and a rejected credential produces a clear 401 message without exposing the value.

The Library screen now exposes **Start Server**, **Stop Server**, and **Refresh Server**. Closing ReddMedia still stops only the integrated Jellyfin process ReddMedia launched; an independently started Jellyfin process is preserved. Generated Jellyfin and AI runtime trees are excluded from Git. The repository root also contains the one canonical ReddMedia-only `COMPANY_BIBLE.md`.

The final versioned executable was `ReddMedia_v17`. Its installer wrote the executable first, assigned the approved red-tree custom-icon metadata directly to that raw executable, read the assignment back, and refreshed Files/Nautilus before owner visual confirmation.

## Previous build: v0.0.16 Native Library and Discover AI

v0.0.16 replaces the temporary flat Library file list with a native poster grid and two explicit entry points: **Movies** and **TV**. Movies show real cataloged movie titles and metadata-created box sets. TV shows series first, then seasons, then episodes. No sample or invented titles are inserted. Each media type supports more than one linked folder, unlinking removes only the catalog link, and playable items continue through ReddMedia's existing embedded player using their real local file paths.

The new top-level **Discover** tab contains the two approved modes, **Usual** and **Random**. Its heading changes between **DISCOVER USUAL** and **DISCOVER RANDOM**, and each mode exposes exactly **Local Movie**, **Local TV**, **External Movie**, and **External TV**. Every request returns one recommendation. Usual uses private SQLite viewing history and local Nomic embeddings; Random does not read the viewing profile. External results come from TMDb, exclude titles already identified in the local catalog, and require a user-supplied TMDb read-access token stored with mode `0600`.

Embeddings run locally on the CPU with pinned llama.cpp source and the bundled `nomic-embed-text-v1.5` Q4_K_M GGUF model. Metadata sent to TMDb is limited to catalog requests; viewing history and embeddings remain local.

The v0.0.16 owner-test repair builds only llama.cpp's required shared libraries. The hidden catalog server now lives for exactly the ReddMedia process lifetime: closing ReddMedia stops and reaps its owned Jellyfin process, and a parent-death safeguard stops it if ReddMedia is forcibly terminated. Repair 2 also corrects the failed-candidate rollback so tracked files return atomically to the committed v0.0.15 baseline while the accepted untracked Jellyfin runtime is preserved. It can recognize, back up, and recover the exact hash-verified leftovers produced by the rejected original v0.0.16 candidate; it will not overwrite any different dirty state.

## Previous build: v0.0.15 Native Library and Hidden Media Catalog Repair

v0.0.15 adds the stable Jellyfin 10.11.11 service as hidden catalog machinery behind ReddMedia. Its web client is disabled, first-run setup is completed privately through the local API, and remote access is disabled. The native ReddMedia window now has a Library tab for adding media folders, scanning, selecting titles, and playing them. Library selections resolve to their real local file paths and enter the same embedded libVLC player used by Open File, P2P, and YouTube; they do not open a browser or an external player and do not pass local playback through Jellyfin transcoding.

The installer extracts pinned Ubuntu 26.04 packages instead of rebuilding Jellyfin with Node and .NET. Jellyfin remains a separately licensed GPL process with matching source and licenses preserved inside the project.

## Previous build: v0.0.14 Local Pause Stability & Red Tree Identity

v0.0.14 hardens ordinary local-file pause/resume behavior so a long pause cannot strand the X11 event loop behind repeated libVLC polling. Paused playback uses cached time/length state, chapter metadata is discovered once per media item instead of being queried every few seconds, pause/resume uses explicit libVLC pause state, and final player teardown has a bounded close safeguard. The ReddMedia identity now uses the approved red-tree artwork across the launcher, dock/app switcher, raw executable, MIME icons, X11 window icon, and the small tree badge beside the top-right version label.

### Current P2P workflow

- Paste a magnet link or open a local P2P metadata file.
- Choose a download folder.
- ReddMedia retrieves P2P transfer metadata and displays the files in the P2P transfer.
- A single obvious video file is selected automatically; multi-file P2P transfers can be selected manually.
- Press **Play** to begin playback before the P2P transfer finishes downloading.
- ReddMedia serves the selected P2P transfer file to VLC through a localhost-only HTTP Range stream.
- Playback requests drive libtorrent time-critical piece priorities automatically. The user does not choose a technical download strategy.
- The full P2P transfer keeps downloading behind playback and can seed after completion.
- Active P2P transfer resume data is stored under `~/.config/reddmedia/p2p/`.
- The P2P source field supports Ctrl+A and Cut / Copy / Paste.

### v0.0.10 stabilization carried under the same version

The v0.0.10 stabilization pass keeps the same feature version while repairing defects found during owner testing:

- Seek/time and volume partial updates are buffered offscreen before being copied to the X11 window, preventing the direct erase/redraw path that caused visible flashing.
- A new P2P HTTP range request supersedes obsolete stream workers so an old seek cannot continue fighting a newer seek for P2P transfer pieces.
- Old time-critical P2P transfer piece deadlines are cleared when VLC starts a new stream range request.
- HTTP suffix byte ranges such as `Range: bytes=-5000000` are supported for VLC/container probing.
- Stream sockets have bounded send/receive waits so abandoned seek connections cannot hang indefinitely.
- The installer reapplies and verifies the ReddMedia red-triangle custom icon on the versioned executable.

The v0.0.16 versioned executable was `ReddMedia_v16`.

### v0.0.13 YouTube seek and close stability repair

- Preserves the growing-cache fix that carried long-form YouTube playback beyond the original 4-to-5-second wall.
- Every YouTube rewind, fast-forward, and seek-bar request now restarts the feeder at the requested timestamp with keyframe-aware cuts. ReddMedia no longer uses libVLC `set_time()` inside a growing YouTube cache.
- Seek replacement returns control to the UI immediately and finishes startup from the normal event-loop poll once enough replacement media has buffered.
- A newer seek replaces the previous pending seek by shutting down its localhost bridge and feeder before starting the newer timestamp.
- YouTube shutdown disconnects the localhost bridge/client sockets before stopping and releasing libVLC, preventing the network reader from holding application close hostage.
- YouTube format selection excludes AV1 and prefers H.264/AVC video with AAC audio, with a non-AV1 fallback, at a maximum of 1080p.
- Keeps the 512 KiB startup target, 5000 ms VLC network cache, red-star identity, P2P behavior, and v0.0.13 version identity.

### v0.0.12 YouTube seekable cache bridge

- Same-version identity repair: the application/window/launcher/raw-executable identity now uses the **red ReddMedia star** instead of the former triangle.
- The GNOME/X11 window title is `★ ReddMedia` with no version number in that title, while the in-app top-right version surface shows only `v0.0.12`.
- Same-version UI repair: the creator-facing tab, screen heading, activity-log heading, and status messages now use **YouTube**. The technical `yt-dlp` name remains only where it identifies the bundled engine, executable, command options, or implementation details.
- YouTube **Play** remains capped at 1080p by default.
- ReddMedia asks the bundled yt-dlp engine for the video's duration so the normal seek timeline has a stable full-video time scale.
- The bundled yt-dlp/FFmpeg pipeline writes the active playback segment into a private temporary cache under `/tmp`.
- ReddMedia serves that growing cache only on `127.0.0.1` through an internal HTTP server with byte-range support.
- Seeking within material already reached by the current cached segment uses the local HTTP source.
- Seeking beyond the current cached segment cancels the obsolete feeder/server and restarts the bundled yt-dlp engine at the requested timestamp using `--download-sections` and keyframe-aware cuts.
- Stop, replacement playback, and clean shutdown terminate the active feeder and remove its temporary cache file.
- The embedded libVLC player continues to render inside ReddMedia.

### v0.0.11 transfer and playback controls

- YouTube Play streams directly into ReddMedia through the bundled yt-dlp/FFmpeg pipeline, capped at 1080p by default.

- **Stop Download** pauses the active P2P transfer, stops active P2P playback, and preserves partial data/resume state.
- The same control becomes **Resume Download** while paused and continues the existing transfer.
- YouTube now has **Play** beside **Download**. Play resolves a network media location and hands it to ReddMedia's embedded VLC player without performing the normal saved-file download first.
- YouTube Download remains the normal save-to-disk path.

### v0.0.11 same-version repair

Owner testing proved P2P Stop/Resume but exposed two release defects before acceptance. The v0.0.11 repair keeps the same version number and corrects them:

- YouTube Play was repaired to stream the bundled yt-dlp/FFmpeg output into embedded libVLC, which proved that supported YouTube playback works inside ReddMedia at up to 1080p.
- libVLC Play startup is checked instead of silently treating a failed start as success.
- The red-triangle executable icon is assigned after the final binary write and GNOME Files/Nautilus is refreshed when available; owner-side visual confirmation remains an acceptance gate.
- The visible top-bar version surface is corrected to `ReddMedia v0.0.11`.

## Main ReddMedia features

- Native X11 desktop interface.
- Native media Library with folder selection, local catalog scanning, title selection, and direct playback in the existing embedded player.
- Native Movies/TV hierarchy with real poster metadata, movie box sets, series, seasons, and episodes.
- Responsive multi-row Library tiles with verified episode numbers/titles and separate technical format.
- Discover Usual/Random recommendations across Local Movie, Local TV, External Movie, and External TV.
- United States JustWatch availability through TMDb, private My Services markings, and official watch-option links.
- Evidence-based Debug and system-health checks with actionable green/yellow/red findings.
- Local SQLite history and offline llama.cpp/Nomic metadata embeddings.
- Hidden local Jellyfin 10.11.11 catalog service with no exposed Jellyfin setup/player web interface.
- VLC/libVLC local video playback.
- Open, Play/Pause, Stop, Rewind 10s, Fast Forward 10s, timeline seeking, volume, fullscreen, and resume support.
- Keyboard and mouse playback controls.
- Embedded audio-track selection.
- External and embedded subtitle controls, automatic matching `.srt` loading, subtitle folder selection, and subtitle delay controls.
- Embedded chapter discovery and chapter navigation when exposed by libVLC.
- YouTube download/playback screen with URL entry and output-folder selection, powered by the bundled yt-dlp engine.
- Built-in P2P magnet and local metadata-file downloading with stream-while-downloading playback.
- Red ReddMedia branding, red controls, red seek/volume bars, and red-tree window/launcher/executable identity.

## Dependencies

See [`DEPENDENCIES.md`](DEPENDENCIES.md) for the exact Ubuntu runtime and build requirements and one-command installation lines.

ReddMedia bundles its yt-dlp executable under `tools/yt-dlp/`. VLC/libVLC, FFmpeg, X11, Zenity, and libtorrent are currently supplied by the Linux system.

## Running the current development build

From the ReddMedia project folder:

```bash
./ReddMedia_v18
```

Version check:

```bash
./ReddMedia_v18 --version
```

Expected output:

```text
ReddMedia v0.0.18
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

## v0.0.8 — Direct YouTube

**Purpose:** put the downloader inside ReddMedia as a permanent application screen.

What changed:

- Added the direct YouTube screen inside ReddMedia, powered by the bundled yt-dlp engine.
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

**Purpose:** make the YouTube URL box behave like a normal editable text field.

What changed:

- Ctrl+A selects the entire YouTube URL.
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

**Purpose:** add built-in P2P file transfer to ReddMedia and make watching while downloading the normal P2P behavior.

What this build added:

- Permanent **P2P** application screen.
- libtorrent-rasterbar 2.x integration.
- Magnet-link loading.
- Local P2P metadata-file loading.
- P2P transfer metadata retrieval and file listing.
- Automatic selection of a single obvious video file.
- Manual file selection for multi-file P2P transfers.
- Download folder selection.
- Live P2P transfer name, state, progress, downloaded amount, download/upload speed, peers, and seeds.
- Complete P2P transfer downloading and seeding behind playback.
- Persistent P2P resume data.
- Ctrl+A and Cut / Copy / Paste in the P2P source field.
- A localhost-only HTTP Range bridge between the P2P transfer engine and VLC.
- Time-critical libtorrent piece deadlines driven by what VLC needs for playback.
- Stream-while-downloading playback without exposing separate sequential-download controls.
- `DEPENDENCIES.md` for runtime and developer requirements.
- libtorrent BSD license/third-party notice records.
- Roadmap work for a future self-contained Linux distribution.

Owner-test results that established the milestone:

- Magnet-link intake and P2P transfer downloading worked.
- Local P2P metadata-file intake and transfer downloading worked.
- P2P transfer metadata, file list, peer/seed status, and automatic video selection worked.
- Playback began while a P2P transfer was still downloading.

Stabilization repairs accepted in v0.0.10:

- Restores buffered seek/time and volume partial repainting to remove the flashing regression.
- Cancels obsolete P2P stream requests when VLC seeks to a new range.
- Clears obsolete P2P transfer piece deadlines on a new stream range request.
- Adds legal HTTP suffix-range support used by media probing/seeking.
- Adds bounded stream-socket waits for abandoned requests.
- Reapplies and validates the red-triangle custom icon on `ReddMedia_v10`.
- Expands this README so every numbered ReddMedia release explains what it actually did.

Seek behavior under slow or difficult P2P swarms can still take time because peer availability controls how quickly an undownloaded region arrives.

## v0.0.13 — YouTube Seek and Close Stability Repair

**Purpose:** keep long-form YouTube playback alive and make rewind/fast-forward/seek replacement and application close stable under the growing-cache architecture.

- Repaired the localhost bridge so an open-ended VLC byte request no longer freezes the cache size at request time.
- Added chunked indeterminate-length range delivery for the growing cache.
- Increased startup and libVLC network buffering for steadier playback.
- Preserved the 1080p ceiling and timestamp-restart seek behavior from v0.0.12.
- Added a slow-growing stream regression specifically designed to catch the 4-to-5-second freeze.
- Same-version stability repair: every YouTube seek now restarts the feeder, seek buffering is non-blocking to the main UI, shutdown disconnects the bridge before libVLC release, and AV1 is excluded in favor of H.264/AVC + AAC preference with a non-AV1 fallback.

## v0.0.12 — YouTube Seekable Cache Bridge

**Purpose:** turn the proven v0.0.11 YouTube playback path into a restartable, seek-aware embedded stream.

What changed:

- Replaced direct libVLC stdin playback for YouTube with a temporary yt-dlp/FFmpeg cache served by a ReddMedia localhost-only HTTP bridge.
- Kept the default YouTube playback ceiling at 1080p.
- Added duration probing so ReddMedia's normal seek timeline represents the full video.
- Added HTTP `HEAD`, full `GET`, byte-range, suffix-range, and invalid-range handling for the local cache source.
- Added timestamp restarts for seeks beyond the current cached playback segment.
- Added stale feeder cancellation and temporary-cache cleanup for seek replacement, Stop, and shutdown.
- Same-version UI/identity repair renames creator-facing network-video labels to **YouTube**, changes the window title to `★ ReddMedia`, shows only `v0.0.12` at the in-app top right, and replaces the application/launcher/raw-executable triangle with the red ReddMedia star.

Validation targets:

- C++17 warnings-as-errors build.
- Localhost-only listener and HTTP range fixture tests.
- yt-dlp 1080p selector and timestamp-restart contract checks.
- Real libtorrent linkage, embedded libVLC location playback, version identity, YouTube labels, title/version split, and red-star application/executable icon gates on the Ubuntu target machine.

## v0.0.11 — Playback & Transfer Controls

**Purpose:** give the two network-media paths the controls needed for everyday use.

What this build adds:

- **Stop Download / Resume Download** on the P2P screen.
- Stopping a P2P transfer also stops active P2P playback and seeding/uploading while preserving partial files and resume state.
- Resume continues the same P2P transfer without discarding completed data.
- A **Play** button on the YouTube screen that streams the bundled yt-dlp/FFmpeg output into ReddMedia's embedded VLC player.
- The existing YouTube **Download** path remains available for saving media normally.
- The roadmap records future Archive, Online Video, Live TV, and supported streaming-service integration work.

Validation target:

- P2P Stop/Resume preserves partial progress.
- YouTube Play starts embedded playback through the bundled yt-dlp/FFmpeg stream path.
- `ReddMedia_v11` retains the red-triangle executable icon.

## Third-party software

See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and the license files under `licenses/`.

## Roadmap

See [`ROADMAP.md`](ROADMAP.md) for the next planned ReddMedia milestones.

### Viewing-history completion repair
This replacement v0.0.19 candidate includes the recommendation/viewing-history source changes required by TV natural-end autoplay. Existing SQLite history databases are migrated in place by adding a `completed` column when needed.
