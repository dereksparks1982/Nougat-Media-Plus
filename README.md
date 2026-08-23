# Nougat Media Suite v0.0.34

Nougat Media Suite is the new official identity of the Linux media application previously released as ReddMedia through accepted v0.0.20. It combines native local playback, a hidden local Jellyfin catalog foundation, local recommendation AI, optional TMDb discovery, decentralized Search, multi-platform Stream URL handling, and built-in P2P transfer/streaming in one desktop application.

## v0.0.34 - Exact Sheet Tabs/Player Controls + Home/Discover UI Repair

v0.0.34 builds directly on accepted v0.0.33 commit `6763a42bf5c125974e5a2882234fb2ee2e04c512`. It is a focused owner-visible UI/Discover release. The approved Nougat concept sheet is the literal component authority for the global top tabs, player seek bar, and player volume control.

### Owner-approved v0.0.34 changes
- Global top navigation uses the actual concept-sheet tab proportions and construction instead of thin pill buttons: squarer rounded body, double bevel/inset seam, tight inter-tab spacing, and the selected downward pointer. Existing per-page colors remain.
- The entire scrollable top-tab lane is shifted left to sit close to `NOUGAT MEDIA SUITE`; the fixed right-side `Server / dot / version` area is deliberately unchanged.
- Video Player seek bar now follows the actual sheet seek component: slim cream track, caramel progress, round knob, timestamps below.
- Video Player volume now follows the actual sheet volume component: compact cream housing, left/right speaker glyphs, inset track, round knob, and percentage readout. Existing 0-200% Nougat gain behavior is retained.
- Home cards use fixed section geometry rather than media-type-driven heights. Continue Watching is a consistent landscape preview shelf; LOCAL recommendations use a consistent portrait-grid template.
- Home and Library scrollbar dragging consumes only the latest X11 motion and stops when Button1 is released, removing delayed/coasting drag behavior.
- Live TV header text and buttons no longer overlap.
- Discover adds a `Live TV` source selector so `Usual + Live TV` and `Random + Live TV` are valid combinations. Until channel/EPG data exists, the UI reports that truthfully instead of inventing recommendations.
- Discover renames `External Movie` / `External TV` to `TMDb Movie` / `TMDb TV`; backend behavior remains TMDb.
- Page-frame corner repair applies to Home, Library, Discover, Live TV, Stream, and Debug. Search and Video Player retain their accepted frame behavior.
- v0.0.33 persistent server, security runtime, P2P Plus, and tuner-detection foundations are retained unchanged.


## v0.0.33 - P2P Plus, Security Hardening, Persistent Server, Page Viewports, and Live TV Foundation

v0.0.33 builds directly on accepted v0.0.32 commit `084ee7ccd82be3a578f738b3bcb6ac8570a573dd`. Normal pages now use a Nougat-owned bordered clipping viewport so scrolling content cannot bleed under fixed chrome; **Video Player is intentionally unchanged**. Home and Library gain hard containment, Library gains a dedicated vertical scrollbar and wrapping toolbar, and the top navigation is clipped between the fixed Nougat brand and fixed Server/version area.

The top-level order is **Home | Video Player | Library | Discover | Live TV | Search | Stream | Debug**. Live TV is a truthful hardware-discovery foundation with a generic `NougatTunerBackend`, Linux DVB/V4L2 probing, channel database/scan interfaces, and first-hardware targeting for the Hauppauge WinTV-HVR-955Q. It does not claim channel tuning/playback yet.

P2P Plus adds Nougat-owned management interfaces for speed limits, seeding ratio/time rules, per-file priority, queue movement, tracker status, Force Reannounce and Force Recheck while preserving v0.0.32 native Watch Now/seek-aware streaming.

Nougat Security Analysis now requires the pinned one-shot runtime (YARA-X 1.19.0, capa 9.4.0 + matching rules, Magika 1.0.3). Missing required/relevant engines produce **ANALYSIS INCOMPLETE**, never a false clean result. Free abuse.ch MalwareBazaar/ThreatFox/URLhaus reputation remains optional through the owner's free **Threat Intel Key**. ClamAV remains an optional external one-shot second opinion. No resident scanner, filesystem watcher, automatic quarantine, deletion, move, rename, or open action is installed.

The integrated Jellyfin server is now persistent after **Start Server**: closing the Nougat UI leaves a Nougat-owned server running, reopening adopts it, and **Stop Server** is the explicit shutdown path. Independently started Jellyfin is never claimed or killed.



## v0.0.32 - Native P2P Media + Nougat Security Analysis

v0.0.32 keeps the native `Search > P2P` media workflow and adds a fourth Search subsection: `Virus Scan`. P2P accepts magnets and local `.torrent` files, selects playable media, streams through Nougat's localhost HTTP Range bridge into the native player, reprioritizes pieces around playback/seeks, and now reports complete-seed/availability evidence more clearly.

Nougat Security Analysis is deliberately on-demand rather than resident. v0.0.32 establishes the scaffold: manual **Scan File** / **Scan Folder** actions and completed P2P downloads launch a one-shot worker for SHA-256 hashing, file/content identification, extension/type checks, built-in rule plumbing, scan history, optional external one-shot ClamAV, and optional free/community reputation hooks. Pinned YARA-X/capa/Magika integration points are present, while installation of their full runtime is intentionally deferred to the next security-hardening pass. The worker exits after the requested scan. Nougat installs no antivirus daemon and no always-on filesystem watcher.

The owner policy is **WARN ME FIRST**. A finding never causes Nougat to automatically quarantine, delete, move, rename, or open a file. The report distinguishes `NO THREATS DETECTED`, `SUSPICIOUS`, and `THREAT DETECTED`, shows which engine produced the evidence, and keeps private scan history under the user config tree. Generated scanner runtimes and credentials are excluded from Git.

This same-version replacement also removes the stray normal-page Node ID, moves only the existing Crawler status sentence upward without altering the Crawler layout, keeps the autoplay no-flash/Search-seam/Stream-border repairs, replaces the oversized volume housing with the shorter Seek-style track, clips Home content below the fixed header, and adds explicit vertical Home and horizontal Continue Watching scrollbars. Advanced torrent-management work is assigned to **v0.0.33 P2P Plus**.

## v0.0.31 - Exact Approved UI Sheet Components

v0.0.31 is deliberately a UI-component-only release on the accepted/published v0.0.30 baseline. The exact approved sheet is stored as `docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png` so the visual authority travels with the source and build records. The owner-approved Nougat component sheet is the literal authority for control construction. If an ordinary Nougat control still reads as a generic rounded rectangle instead of the sheet's raised stitched/beveled component family, it is replaced in this release.

The component renderer also uses a sheet-derived runtime surface texture sampled from the approved PRIMARY component, then recolors that lighting/depth pattern with each page's existing palette. The accepted page palettes do **not** change. Home remains purple, Video Player cocoa/chocolate, Library green, Discover red, Search cream, Stream provider-reactive, and Debug charcoal. v0.0.31 changes component geometry, bevel/depth, shadows, inset seams, selected tab pointers, fields, panels, icon buttons, checkbox treatment, progress surfaces, seek/volume tracks, and slider knobs while preserving those colors and v0.0.30 behavior.

Home card proportions and artwork remain the accepted v0.0.30 design. Media playback, Library metadata/cache behavior, Discover logic, Search/P2P transport, Stream extraction/playback, TV autoplay, diagnostics, licensing, and user-data paths are outside this release's implementation scope. The focused P2P streaming expansion moves intact to v0.0.32.

The longer roadmap also includes native Live TV/NextGen TV and Radio/SDR work: owner HVR-955Q support, HDHomeRun, ATSC 3.0-capable hardware such as FLEX 4K, and receive-only AM/FM/weather/CB paths through compatible tuner or SDR hardware.

## v0.0.30 - UI Cohesion, Library Performance, and Player Navigation

v0.0.30 builds on the owner-accepted/published v0.0.29 baseline. It makes the native pages feel like one visual system, speeds the Library's perceived metadata load with a persistent cache-first path, makes Library Grid use multiple visible DVD-poster rows, and adds concise `Previous` / `Next` episode navigation to the native player.

Large primary content surfaces on Library, Discover, Stream, and Debug now use the same rounded/inset panel language already established by Search. The active top-tab pointer remains fully visible above page work because the gold busy/progress strip is positioned below the navigation notch rather than painting across it. Home movie/series/season artwork uses portrait 2:3 poster geometry like Library, while episode stills remain landscape.

Library metadata views can display a private persistent cached snapshot immediately, then reconcile against Jellyfin in the background. `Refresh Library` remains the operation that asks Jellyfin to scan the library; its server-side scan phase is honestly indeterminate because the current Jellyfin request does not provide completed/total counts. Once Nougat has an actual item total for metadata/artwork enrichment, the existing gold bar shows a real completed/total-derived percentage. `Refresh Server` refreshes server/process status and does not pretend to be a library metadata scan.

The player control row is `Open | Rewind 10s | Previous | Play/Pause | Next | Fast Forward 10s | Stop | Fullscreen`. Previous/Next use the accepted v0.0.29 episode queue/resolver and disable at boundaries. The compact 0-200% volume slider is retained with a larger, properly proportioned knob.

The focused P2P streaming expansion is intentionally v0.0.32. v0.0.30 does not modify the accepted Search/P2P engine or transport implementation.

## v0.0.29 - TV Playback, Navigation, and Carry-Forward UI Repair

v0.0.29 builds on the owner-accepted/published v0.0.28 baseline. TV playback now resolves the next local episode from the current file's own folder regardless of whether the episode was opened from Home, Library, Open File, resume history, or another local route. Nougat prefers parsed season/episode identity such as `S01E13 -> S01E14`; for catalog-confirmed episodes without those tokens it uses natural filename order. The next-episode queue is prepared when playback starts instead of waiting for end-of-media.

At natural episode completion, the Up Next overlay appears immediately with the resolved next episode and a visible 10-second countdown. `Play Next` starts it immediately, the countdown autoplays it at zero, `Replay` restarts the completed episode, and `Back to Series` resolves the actual Jellyfin series/season context when available instead of dropping to the generic Library root. Manual Stop continues to cancel autoplay. A real executable self-test now exercises same-folder episode ordering, natural filename fallback, and the 10-second Up Next state rather than merely checking that source tokens exist.

This release also carries forward the owner-visible v0.0.28 polish defects intentionally deferred at acceptance. Home no longer swallows top-header wheel events when Home is selected, so the top tab strip scrolls horizontally just as it does on other pages. Vimeo is added immediately after YouTube in Stream using Vimeo's current blue/black/white brand family while retaining Nougat's provider-reactive Stream behavior. The Video Player page background now surrounds the video uniformly without the partial brown rail/matte.

Home artwork is strengthened again: Continue Watching TV episodes prefer an exact episode Primary/still image, then the matching season poster, then series artwork, including resume records created outside the Library hierarchy when the series can be recovered by exact path containment. Movie/poster artwork fills the card artwork region with aspect-preserving cover behavior rather than appearing as a tiny centered stamp. Existing rounded top clipping and one-at-a-time silent hover previews remain intact.

The focused P2P streaming expansion is now v0.0.32.


## v0.0.28 - Candy Palette, Artwork, and UI State Polish

v0.0.28 builds on the owner-accepted/published v0.0.27 baseline. The main quilted page background now carries each native area identity instead of every page reading as cream: Home is purple, Video Player uses cocoa/chocolate/caramel, Library is green, Discover is red, Search remains the only cream-background native page, Stream stays provider-reactive, and Debug is charcoal/licorice. Each area stays restrained to roughly two or three coordinated colors with cream used selectively for readable trim, text, tracks, and panels.

Home now behaves as a persistent desktop surface. Switching to another tab and returning does not rebuild the feed when its data is still current, preserving loaded recommendations, Continue Watching state, artwork, and scrolling. Home cards are poster-first at rest; movie cards use the movie poster, while TV Continue Watching resolves the matching season poster and then the series poster as fallback. The existing silent local hover preview remains and returns to poster art when the pointer leaves. Rounded card artwork and hover frames are clipped through the same top-corner mask so square image pixels cannot protrude through the curved border.

The LOCAL wall uses larger section/category lettering and more readable card metadata. Its responsive layout guarantees at least three recommendation cards across at the owner's roughly 650-pixel half-screen width and fits more as the window widens. The legacy X11 text path translates metadata bullets safely so `2012 • Comedy • Romance` no longer becomes mojibake.

Library poster presentation now prefers proper portrait artwork from an exact catalog TMDb ID when available, falls back to Jellyfin Primary artwork, requests display-useful poster resolution, rejects tiny or landscape results, and preserves portrait aspect instead of stretching arbitrary images into the tile. A deliberate `NO POSTER` state is used when no acceptable art exists.

The Video Player replaces the remaining pale/white windowed-video halo with a dark cocoa/chocolate theater surround and restrained caramel trim. Search removes the redundant standalone `SEARCH` heading and moves its real controls upward while retaining `Search | Crawler | P2P`.

The owner-observed TV Up Next/autoplay and `Back to Series` regressions are intentionally split into v0.0.29 so this release remains a UI/artwork/state build. The focused P2P streaming expansion follows in v0.0.31.

## v0.0.27 - Home, Resume History, Player Polish, and Seek Previews

v0.0.27 builds on the accepted/published v0.0.26 baseline. A new **Home** tab is first in the top navigation and is the default landing page. Home starts with a horizontally scrollable **Continue Watching** shelf driven by persistent per-title resume history. The shelf uses the mouse wheel for left/right movement only while the pointer is over it, shows caramel progress bars, and keeps all unfinished movies/episodes available rather than limiting the list to only a few recent items. Below it, **LOCAL** presents mixed movies and TV in a normal vertically scrolling card wall organized into useful genre/category groups plus a personalized recommendation group that learns from local watch history.

Home uses wide 16:9 cards and prefers higher-resolution Jellyfin backdrop artwork, with primary/TMDb artwork as fallback. Card hover waits for intentional pointer dwell, then shows one muted FFmpeg-extracted moving preview at a time; leaving the card restores its artwork.

Local playback now has durable resume state. Reopening unfinished media can offer `Continue | Start Over | Cancel`, while **Stop** presents an intentional stopped state with `Resume | Restart | Load Different | Back to Library`. Resume positions persist across other files and application restarts and completed media is removed from Continue Watching. Windowed/maximized playback gets rounded video corners; true video fullscreen remains square. The current movie or TV identity is kept visible in the compact strip below windowed video and appears temporarily with mouse activity in true fullscreen.

The seek bar now supports an actual-frame hover preview with timestamp and real chapter name when real chapter metadata applies. Preview extraction runs separately from the active libVLC player and uses a bounded cache. The v0.0.26 pointer-motion flicker path is repaired by repainting only on meaningful hover-state changes instead of scheduling whole-window redraws for raw X11 motion packets. The selected top-tab notch now paints cleanly over the header divider. Redundant `DISCOVER USUAL / DISCOVER RANDOM`, `Direct Play URL`, and `DIAGNOSTIC CENTER` headings are removed; the Stream field placeholder is exactly `Paste URL Then Press Direct Watch / Rumble / RuTube / VK / OK`.

The page-background/artwork/state polish discussed during owner testing is implemented in v0.0.28. The TV Up Next reliability repair is split into v0.0.29 and the focused P2P expansion into v0.0.32.


## v0.0.26 - Systems, Navigation, Diagnostics, and TV Up Next

v0.0.26 builds on the owner-accepted v0.0.25 release. Mouse side buttons now navigate Nougat's internal history (Button 8 Back, Button 9 Forward), the Library root header drops the redundant `MEDIA LIBRARY` label and places the List/Grid view controls at the far left, and the fixed header identity/status layer remains anchored beneath the horizontally scrolling top tabs.

The approved Nougat N artwork receives a perimeter cleanup that removes the tiny lower-edge light sliver while preserving the approved emblem itself across the icon family and embedded window/header icon data. The Video Player keeps its intentional 0-200% volume range, but the Volume label, existing slider, and single correct percentage are centered as one group; the duplicate percentage and rejected square/triangle speaker glyphs are removed.

Debug becomes the **Nougat Media Suite Diagnostic Center**. It gathers evidence from Nougat, the host system, Jellyfin, Library metadata/paths, libVLC playback, Search, the existing P2P core, local AI/TMDb, and Stream. Reports can be copied or exported as human-readable TXT, structured JSON, or a redacted support bundle. Green is reserved for checks that returned healthy evidence; missing or unavailable evidence is kept explicit rather than invented.

At natural TV episode completion, Nougat resolves the real next episode before presenting an **Up Next** overlay. The overlay shows a visible 10-second countdown and `Play Next`, `Back to Series`, and `Replay`; manual choices cancel the countdown, and an unresolved next episode produces an explicit message instead of silently dead-ending.

v0.0.27 added seek-bar hover thumbnail previews while preserving aesthetic fallback chapter marks. During owner testing, v0.0.28 was reassigned to UI/artwork/state polish, v0.0.29 to TV playback/navigation reliability, and the larger P2P-management expansion to v0.0.32.


## v0.0.25 - Stream Provider Theme, Persistent Selection, and Discover Native Play

v0.0.25 builds directly on the owner-accepted v0.0.24 release. The selected Stream provider now drives the entire Stream interior: provider-colored controls, accents, and the exact concept-sheet quilt tinted to YouTube red, Rumble green, RuTube purple, VK blue, or OK orange/caramel. The selected provider also receives the concept-sheet downward active notch.

Discover now exposes its persistent state visually. `Usual | Random` is one selector group and `Local Movie | Local TV | External Movie | External TV` is a second independent group, so one active notch from each group may appear at the same time. Action controls such as TMDb tests and credential/service actions remain ordinary buttons.

`Play in Nougat...` for local Discover results now resolves the selected Jellyfin catalog entry to a real playable local movie or episode before starting the native Nougat player. A series-level TV result resumes the most recently watched matching episode when local history identifies one; otherwise it starts the first real episode in season/episode order.


## v0.0.24 - Search Page UI Polish

v0.0.24 is an owner-approved Search-page-only visual polish release on the accepted v0.0.23 base. It moves the approved N emblem and NOUGAT MEDIA SUITE identity to the far-left header position, removes the duplicate right-side N badge, finishes concept-sheet styling for Search/Crawler/P2P controls and panels, aligns Network... and SEARCH on the same right-side column, and strengthens GNOME launcher/window icon association so the approved N replaces the generic gear fallback. Search-engine behavior is intentionally unchanged in this release.


## v0.0.23 - Exact Concept UI and Stream Direct Watch Repair

v0.0.23 is the owner-approved post-license UI and Stream repair build. The uploaded Nougat Media Suite concept sheet is the visual authority for the application theme: rounded stitched/beveled candy-style controls, cream/caramel/chocolate materials, area-specific tab accents, the active-tab point, quilted page surfaces, concept-style seek/volume controls, and the exact square chocolate/caramel N emblem. Existing control order and media behavior are preserved unless explicitly listed below.

- Top-level order remains `Video Player | Library | Discover | Search | Stream | Debug`.
- The active top tab receives the concept-sheet downward point/notch; the navigation group centers when width permits and remains scrollable when narrow.
- The main player row remains `Open | Rewind 10s | Play/Pause | Stop | Fast Forward 10s | Fullscreen` and centers when width permits.
- The seek and volume fills no longer use the old red theme; they use the concept-sheet caramel/cream/chocolate palette.
- Volume remains 0-200% with the 100% marker, but uses the approved compact concept-sheet geometry.
- Video Player, Library, Discover, Search, Stream, and Debug use the quilted material with the approved subtle page tinting.
- Stream keeps only YouTube, Rumble, RuTube, VK, and OK, uses one shared Direct Play URL field, removes the redundant Stream `Play` button, and keeps `Direct Watch` as the native-player action.
- YouTube Direct Watch now detects supported external JavaScript runtimes for current yt-dlp YouTube challenge solving and uses the updated YouTube extraction/playback fallback path.
- The old candy executable/launcher artwork is replaced by the exact N emblem from the approved concept sheet.

## v0.0.22 - License Protection and Contribution Boundary

v0.0.22 is a legal/release-infrastructure build over the owner-accepted v0.0.21 baseline. It does not add or redesign media behavior. It strengthens the project license boundary around Elderred Softworks LLC Original Materials, preserves third-party licensing, adds explicit copyright/ownership notices, establishes inbound contributor terms, and adds automated release checks intended to prevent accidental relicensing.

The versioned root executable advances to `Nougat_Media_Suite_v22` only so the accepted legal release remains versioned consistently with Nougat Media Suite's release rules. The application's media, Search, Stream, Library, Discover, P2P, server, playback, and palette behavior is retained from v0.0.21.

## v0.0.21 - Official Rename, Candy Palette, and Navigation Repair

v0.0.21 builds directly on the owner-accepted ReddMedia v0.0.20 baseline at commit `c3d2c60e5c36407b96a0eba72e2863f884aacd28`. The release establishes the new product identity and candy color system, then folds two owner-approved UI organization repairs into the same still-unaccepted candidate: P2P moves under Search, and the nonworking Library Grid/List text controls become real view-toggle icons. No media engine, recommendation engine, server engine, or additional Stream service is added by this version.

The visible top-level order is **Video Player | Library | Discover | Search | Stream | Debug**. The former top-level **Nougat** label becomes **Search** because Nougat now names the entire suite. Search exposes **Search | Crawler | P2P** as its ordinary internal sections. Decentralized search-node peer controls are tucked behind **Network...** inside Search instead of occupying a normal user-facing tab.

The application/window/launcher identity becomes **Nougat Media Suite**. The versioned root executable is `Nougat_Media_Suite_v21`. The approved rounded-square chocolate/nougat **N + play triangle** artwork replaces the former ReddMedia red-tree icon for the launcher, dock/app switcher, X11 window, raw executable metadata, and the in-app version badge.

The suite-wide visual system is rebuilt around the candy identity. **Video Player is chocolate/cocoa/caramel**, Library is forest/sage, Discover is plum/lavender, Search is chocolate/nougat/caramel, and Debug is graphite/amber. The top navigation itself uses those area identities instead of one inherited ReddMedia-red strip. Stream is service-reactive: selecting **YouTube, Rumble, RuTube, VK, or OK** changes the Stream interior and its tab color to that service's recognizable palette.

In Library, the old `Grid [x]` / `List [x]` text buttons are removed. At the far left of the Library page are two compact view icons: **three horizontal lines** for List and a **2x2 four-square grid** for Grid. Clicking either changes the actual Library layout and the selected icon is highlighted. Movie and TV view preferences remain independent and persistent.

Existing Stream services remain exactly **YouTube | Rumble | RuTube | VK | OK** in v0.0.21. Vimeo, Dailymotion, Twitch, Kick, TikTok, Bilibili, and Niconico are deferred to v0.0.23. The approved nougat-specific UI polish, poster/artwork overhaul, rounded-corner polish, top-navigation centering, volume-bar geometry changes, the optional local Web Player, and Plex integration are also deferred.

The existing Git repository directory and backward-compatible user-data/config paths remain unchanged so the product rename does not silently become a filesystem/data migration. User-visible application identity is Nougat Media Suite.


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

## Licensing and third-party software

Nougat Media Suite Original Materials are made available to recipients under the **PolyForm Noncommercial License 1.0.0**. Elderred Softworks LLC retains all rights not granted to recipients, including commercial use and separate commercial licensing of its own Original Materials. See [`LICENSE`](LICENSE), [`COPYRIGHT.md`](COPYRIGHT.md), and [`docs/LICENSING_POLICY.md`](docs/LICENSING_POLICY.md).

Outside contributions are accepted only under [`CONTRIBUTING.md`](CONTRIBUTING.md), which grants the project owner sufficient rights to continue maintaining, sublicensing, relicensing, and commercially licensing the combined project.

Third-party components keep their upstream licenses and terms. See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and the preserved files under `licenses/`.

## Roadmap

See [`ROADMAP.md`](ROADMAP.md) for the next planned ReddMedia milestones.

### Viewing-history completion repair
This replacement v0.0.19 candidate includes the recommendation/viewing-history source changes required by TV natural-end autoplay. Existing SQLite history databases are migrated in place by adding a `completed` column when needed.


## v0.0.26 candidate

This candidate adds internal mouse Back/Forward navigation, cleans the Library header and Nougat icon perimeter, upgrades Debug into an evidence-based Diagnostic Center with TXT/JSON/redacted support-bundle exports, cleans and centers the intentional 0-200% volume control, fixes top-header layering during horizontal tab scrolling, and adds a 10-second TV Up Next overlay with Play Next / Back to Series / Replay. P2P feature expansion was later moved to v0.0.32 after the owner split v0.0.28/v0.0.29 into UI/artwork and TV-reliability releases.
- v0.0.33 server-stop repair: persistent Nougat-owned Jellyfin sessions carry a per-session ownership token; Stop Server terminates the complete owned process tree and verifies port 8096 is released without killing Jellyfin by name.
