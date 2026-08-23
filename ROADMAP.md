# Nougat Media Suite Roadmap

## v0.0.26 candidate — Systems, navigation, diagnostics, and TV end-of-episode UX
- Mouse side Button 8/9 navigate Back/Forward through Nougat internal history.
- Library removes the redundant root “MEDIA LIBRARY” label and places List/Grid controls at the far left.
- Clean the approved N icon perimeter app-wide so the tiny lower-edge light sliver is transparent.
- Upgrade Debug into the Nougat Media Suite Diagnostic Center with evidence-backed app/system/Jellyfin/library/playback/Search/current-P2P/AI/TMDb/Stream checks plus TXT, JSON, and redacted support-bundle export.
- Center the existing Volume label + 0-200% control + single correct percentage; remove the duplicate percentage and rejected speaker-square/triangle glyphs without changing the gain range.
- Keep N/name, Server status/dot, and version fixed beneath the horizontally scrolling top tabs so tabs roll over them cleanly.
- At natural TV episode end, resolve the actual next episode and show an Up Next overlay with a visible 10-second autoplay countdown, Play Next, Back to Series, and Replay. If resolution fails, show an explicit message rather than silently stopping.

## v0.0.27 accepted — Home, resume history, player polish, and seek previews
- Add Home as the first/default tab. Continue Watching is the only horizontal shelf; the mouse wheel moves it left/right while hovered and scrolls Home vertically everywhere else. No arrow buttons.
- Keep every useful unfinished local movie/episode in Continue Watching with a caramel progress bar and persistent resume position.
- Under `LOCAL`, show mixed movies/TV as a normal vertically scrolling card wall organized by useful genre/category groups plus watch-history-informed recommendations.
- Prefer wide high-resolution Jellyfin backdrop artwork with verified fallbacks and provide one-at-a-time muted hover previews from the real local file.
- Add persistent `Continue | Start Over | Cancel` reopen behavior and a stopped-player screen with `Resume | Restart | Load Different | Back to Library`.
- Keep the current media identity visible below windowed/maximized video and temporarily overlay it in true fullscreen on mouse activity.
- Round the video viewport in windowed/maximized mode while keeping true fullscreen square.
- Add actual-frame seek hover previews with timestamp and real chapter name when real chapter metadata applies; do not seek the active libVLC player to create previews.
- Repair mouse-motion flicker at the repaint source, repair the selected-tab notch/header divider layering, and remove the redundant Discover/Stream/Diagnostic headings.
- Preserve accepted v0.0.26 behavior outside this scope.

## v0.0.28 accepted — Candy Palette, Artwork, and UI State Polish
- Make the main page background carry each native area identity: purple Home, cocoa/chocolate Video Player, green Library, red Discover, cream Search, provider-reactive Stream, and charcoal/licorice Debug. Use roughly 2–3 coordinated colors per page; the background does most of the differentiation while cream remains selective trim/text/panel material.
- Replace the remaining light/white halo around windowed video with a cocoa/chocolate theater surround and restrained caramel trim. Preserve true fullscreen as edge-to-edge and square.
- Keep Home loaded when switching tabs. Returning to Home preserves its recommendation results, Continue Watching shelf, artwork, and scroll positions instead of rebuilding the page; reload only when watch/library data changed or the owner explicitly refreshes.
- Make Home cards poster-first at rest. Movies use their proper movie poster; TV Continue Watching resolves the matching season poster first and series poster as fallback. Silent hover video previews remain one-at-a-time and restore the poster when the pointer leaves.
- Repair the rounded-card clipping path so artwork and silent hover frames obey the top-left/top-right rounded mask instead of square pixels protruding through the curved border.
- Increase Home section/category and metadata readability and make the ordinary LOCAL recommendation wall responsive. At the owner's half-screen width around 650 px, at least three recommendation cards fit on each row; wider windows fit more automatically. Continue Watching remains its separate horizontal shelf.
- Repair the legacy X11 metadata separator path so intended bullets render cleanly instead of mojibake such as `â€¢`/a-cent glyphs.
- Upgrade Library poster sourcing and presentation: exact catalog TMDb IDs may supply a preferred high-resolution poster, Jellyfin Primary remains the local fallback, portrait-quality gating rejects tiny/landscape art, poster aspect is preserved, and missing acceptable art uses a deliberate `NO POSTER` state instead of stretching garbage.
- Remove the redundant standalone `SEARCH` page heading while retaining `Search | Crawler | P2P` and reclaim the vertical space.
- Preserve accepted v0.0.27 playback, resume, seek-preview, Discover, Search/P2P, diagnostics, licensing, and N identity outside this visual/state scope.

## v0.0.29 accepted — TV Playback, Navigation, and Carry-Forward UI Repair
- Resolve the next local episode from the current playback file's folder regardless of whether playback began from Home, Library, Open File, resume history, or another local path. Prefer parsed/verified season+episode identity; use natural filename order for catalog-confirmed episodes when explicit numbering tokens are absent.
- Resolve/cache the next episode when playback starts so the end-of-media path does not perform a slow folder/catalog discovery before showing Up Next.
- At natural episode completion, show the resolved Up Next overlay immediately with a visible 10-second countdown, `Play Next`, `Back to Series`, and `Replay`; autoplay at zero unless the owner chooses another action. Manual Stop continues to cancel autoplay.
- `Back to Series` resolves the actual Jellyfin series and returns to that series/season browsing context when the current episode can be mapped by series ID or exact local path, with TV Library as the honest fallback only when series identity cannot be recovered.
- Add executable behavior coverage for `S01E13 -> S01E14`, natural filename ordering, and the actual 10-second Up Next overlay state so source-token presence cannot falsely report runtime health.
- Repair Home wheel routing so the top navigation strip always receives wheel events first, including while Home is the active page. Continue Watching horizontal wheel behavior and ordinary Home vertical scrolling remain unchanged outside the header.
- Add Vimeo immediately after YouTube in Stream using the provider's current blue/black/white brand family and keep provider detection, homepage action, selected notch, and provider-reactive quilt/palette behavior consistent with the existing Stream services.
- Remove the partial brown Video Player rail/matte so the Video Player page background itself surrounds the video uniformly on all sides. Preserve the accepted page palette and player controls.
- Strengthen Home artwork for owner-observed black TV cards: exact episode Primary/still first, matching season poster second, series poster third, with exact path containment able to recover the owning series for resume records created outside Library navigation.
- Make Home movie/poster art fill the artwork region with aspect-preserving cover behavior instead of rendering as a tiny centered poster. Preserve rounded top clipping and silent hover previews.
- Preserve accepted v0.0.28 palette, Home state persistence, Library poster work, Search cleanup, resume/seek/player behavior, Discover, Search/P2P, diagnostics, licensing, and N identity outside this focused reliability/repair scope.

## v0.0.30 accepted — UI Cohesion, Library Performance, and Player Navigation
- Apply the Search-style rounded/inset primary panel treatment to the equivalent large Library, Discover, Stream, and Debug content surfaces while preserving each page palette.
- Keep every selected top-tab notch fully visible by keeping the busy/progress strip below the navigation pointer instead of painting across it.
- Use Library-style portrait 2:3 DVD/poster geometry for Home movies, series, and seasons; keep episode imagery landscape.
- Make Library Grid use the available vertical viewport so multiple poster rows are visible simultaneously, with normal vertical scrolling through the wall.
- Add a private persistent cache-first Library metadata path so known views can appear immediately while Jellyfin reconciliation continues asynchronously. Reuse verified cached artwork identifiers to avoid unnecessary repeat enrichment.
- Keep Jellyfin library scan phases indeterminate when no total is available, then show a real numeric percentage beside the gold progress bar for item/artwork phases with measured completed/total work. Clarify that Refresh Server refreshes server/process state while Refresh Library triggers Jellyfin library scanning/metadata reload.
- Add concise `Previous` and `Next` native-player controls backed by the v0.0.29 episode resolver, preserving the existing +/-10-second seek controls and disabling navigation at episode boundaries.
- Enlarge the compact volume slider knob so it fits its housing proportionally while preserving 0-200% behavior and the approved colors/readout.
- Preserve accepted v0.0.29 Search/P2P, licensing, diagnostics, Stream providers, TV Up Next/autoplay, resume history, and N identity outside this UI/Library/player scope.

## v0.0.31 candidate — Exact Approved UI Sheet Components
- Treat the owner-approved Nougat component sheet as the literal control-shape authority rather than a loose visual reference.
- Preserve every accepted v0.0.30 page/background palette exactly: purple Home, cocoa Video Player, green Library, red Discover, cream Search, provider-reactive Stream, and charcoal Debug.
- Replace ordinary buttons that still look like flat rounded rectangles with the sheet's layered raised control construction: lower shadow, dark outer rim, bright inner bevel, stitched/inset seam, top highlight, and pressed/disabled states.
- Replace top-level tabs plus Search/Discover/Stream selector tabs with the same sheet control family and the integrated downward selected point/notch.
- Replace text/input frames, large panels, compact icon buttons, watch-service checkboxes, progress surfaces, seek track, volume housing/track, and slider knobs with the sheet component geometry while retaining their existing page colors and behavior.
- Keep Home card proportions/artwork and all v0.0.30 media behavior unchanged; this release is UI-component fidelity only.
- Preserve licensing, Search/P2P transport, Jellyfin/catalog behavior, Discover logic, Stream playback, diagnostics, TV autoplay, metadata cache, and user data paths unchanged.

## v0.0.32 planned — Focused P2P streaming expansion
- Make P2P playback range-aware: map native-player HTTP byte-range demand to the selected torrent file and libtorrent pieces, using time-critical/deadline priorities for immediate playback data.
- Maintain moving immediate, near-future buffer, and background priority zones; clear stale playback deadlines when seeking and reprioritize the new position.
- Improve startup/seeking truthfulness with Finding Peers / Connecting / Buffering evidence rather than opaque stalls.
- Keep playback inside Nougat's native player and retain Search > P2P as the user-facing location.
- Add focused P2P transfer/peer/buffer evidence under Search > P2P and Debug without resurrecting a top-level P2P tab.

## Future platform proposal — native web player and free provider adapters, version assignment pending
- This future platform proposal is not part of v0.0.28 or v0.0.29 and must not displace the focused v0.0.32 P2P expansion without a later owner decision.
- Add a first-party Nougat web client/player so browser playback uses Nougat controls, Home, watch history, resume state, subtitles/audio selection, fullscreen, seek previews, and Up Next instead of exposing the Jellyfin web UI.
- Prefer direct browser playback of the original media when the browser supports it; use Nougat-controlled FFmpeg delivery/transcoding only when required by browser codec/container support.
- Keep future external-provider support behind a generic provider adapter rather than hard-coding one company into Home. Provider sections may appear beneath LOCAL as source headings, with mixed movie/TV cards and no redundant Movies/TV subheadings.
- External-provider integration must have a legitimate **no-cost** technical path. Do not plan paid provider partnerships, enterprise integration fees, commercial SDK costs, or licensing fees into Nougat.
- Preserve every provider's required advertising, DRM, attribution, and playback rules; do not bypass DRM or strip provider ads. If a provider requires a paid/private commercial deal or has no legitimate third-party playback path, skip native integration.
- Outside providers retain recognizable brand colors when active instead of being forced into Nougat-native tab colors.
- Xumo, Tubi, Pluto, or similar FAST/AVOD services are examples to research individually, not promised integrations.
- The Xumo-style genre/category browsing concept is independent of Xumo itself and may be used for LOCAL organization even if no external provider is ever integrated.

## v0.0.25 candidate - Stream Provider Theme, Persistent Selection, and Discover Native Play

- Build directly on accepted v0.0.24 commit `f66d35b671c9bceee6151dc63003dc3ec24578e8`.
- Make the selected Stream provider control the Stream interior buttons/accents and tint the exact concept-sheet quilt: YouTube red, Rumble green, RuTube purple, VK blue, OK orange/caramel.
- Give the selected Stream provider the same downward active notch language used by the selected top-level tab.
- Give Discover two independent persistent selector groups: `Usual | Random` and `Local Movie | Local TV | External Movie | External TV`; one selection in each group may remain notched simultaneously.
- Keep TMDb/service operations as ordinary action buttons without persistent selected notches.
- Repair local Discover `Play in Nougat...` so local movie/TV recommendations resolve through the Jellyfin catalog to an actual playable native-player target.
- For a local TV series recommendation, resume the most recently watched matching episode when history identifies one; otherwise begin with the first real episode.
- Preserve the accepted v0.0.24 literal concept-sheet N, exact quilt source, Search behavior, licensing, Library/P2P behavior, Crawler spacing, TV autoplay, and pointer-motion performance work.


## v0.0.24 same-version replacement repair - Exact Master Art

- Current v0.0.24 candidate remains rejected until the owner visually accepts this replacement.
- Use the N directly from the owner-uploaded full-resolution Nougat concept sheet, never a screenshot/crop derivative.
- Keep the icon canvas transparent outside the emblem so no white/cream border or square is visible in the dock, Files, window switcher, or executable metadata.
- Use the exact padded quilt material extracted from the concept sheet instead of the procedural line approximation.
- Keep one quilt material across the suite but visibly tint it by top-level area: Video Player caramel, Library sage, Discover lavender, Search cream/gold, Stream dusty blue, Debug taupe/gray.
- Preserve all other v0.0.24 behavior and previously repaired defects unchanged.

## v0.0.24 same-version repair - Search UI Stability

- Keep the approved v0.0.24 Search-page layout and Search-engine behavior unchanged.
- Move the Crawler log panel down so the status line is clear of the panel border while preserving selectable/highlighted output.
- Repair Ubuntu/GNOME application matching with the canonical reverse-DNS desktop ID, accepted WM_CLASS, approved N icon-theme asset, and compatibility launcher.
- Restore reliable TV next-episode autoplay across seasons by recovering Series context from Jellyfin episode metadata when needed, validating local libVLC startup, recognizing guarded natural-EOF states, and using bounded retry after a failed automatic transition.
- Remove the new UI sluggishness by rate-limiting full rich-theme repaints generated by raw pointer-motion traffic instead of repainting the entire app for every motion packet.
- Preserve manual Stop cancellation, current playback controls, Search engine/bridge files, protected licensing files, and user data unchanged.


## v0.0.24 candidate - Search Page UI Polish

- Preserve accepted v0.0.23 Search engine behavior; this release is visual polish only.
- Put the approved square N emblem at the far-left header position with `NOUGAT MEDIA SUITE` immediately beside it.
- Remove the duplicate N beside the version while preserving server state and version on the far right.
- Finish exact concept-sheet styling for Search, Crawler, and P2P sub-tabs, fields, buttons, result/log panels, and selected-tab point/notch.
- Align `Network...` and `SEARCH` to the same right-side column with identical width and horizontal position.
- Improve Search status/help contrast and replace the legacy dark result slab with the approved Nougat panel treatment.
- Strengthen GNOME launcher/window identity and use the approved N asset directly so the running application does not fall back to the generic gear icon.
- Preserve the protected PolyForm Noncommercial licensing files unchanged.


## v0.0.22 candidate - License Protection and Contribution Boundary

- Keep PolyForm Noncommercial License 1.0.0 as the controlling recipient license for Elderred Softworks LLC Original Materials.
- Correct the copyright/licensor identity and add explicit ownership, contribution, and licensing-policy records.
- Preserve all third-party licenses and prevent project-level notices from relicensing upstream components.
- Add an inbound contribution grant that preserves the owner's ability to maintain, sublicense, relicense, and commercially license Nougat Media Suite.
- Add deterministic license-boundary and rollback validation.
- Advance the root executable to `Nougat_Media_Suite_v22` with no media/UI behavior change.
- Move the previously planned UI-polish and Stream-service expansion work to v0.0.23.

## v0.0.21 accepted - Official Rename, Palette, and Navigation Repair

- Official application identity changes from **ReddMedia** to **Nougat Media Suite**.
- Versioned root executable becomes `Nougat_Media_Suite_v21`.
- Top-level order becomes `Video Player | Library | Discover | Search | Stream | Debug`; the former top-level Nougat label becomes **Search**.
- Search's ordinary internal sections are `Search | Crawler | P2P`; media/torrent P2P moves under Search and its former top-level tab is removed.
- Decentralized search-network peer/node controls move behind a smaller **Network...** advanced surface inside Search instead of remaining a normal tab.
- Replace the former red-tree identity with the owner-approved rounded-square chocolate/nougat **N + play triangle** icon across launcher, dock/app switcher, X11 window icon, raw executable custom-icon metadata, and in-app version badge.
- Replace the common red top chrome with candy-family identities: Video Player chocolate/cocoa/caramel, Library forest/sage, Discover plum/lavender, Search cocoa/nougat/caramel, and Debug graphite/amber.
- Make top-level tabs themselves use their area identity colors.
- Stream remains service-reactive for the existing v0.0.20 services only: YouTube, Rumble, RuTube, VK, and OK each recolor the Stream interior and its top-level tab when selected.
- Replace the dead `Grid [x]` / `List [x]` Library controls with working compact List (three lines) and Grid (2x2 squares) icon buttons immediately after the Library heading. Preserve independent Movies/TV view persistence.
- Preserve all accepted v0.0.20 media-engine, Library catalog, Discover, Search, Stream, server, AI, playback, and history behavior outside these approved UI identity/navigation repairs.


## v0.0.23 candidate - Exact Concept UI and Stream Direct Watch Repair

- Use the owner-uploaded Nougat concept sheet as the visual authority for buttons, tabs, fields, seek/volume controls, page material, and active-tab point/notch.
- Preserve the exact top-level order `Video Player | Library | Discover | Search | Stream | Debug` and the exact player-control order `Open | Rewind 10s | Play/Pause | Stop | Fast Forward 10s | Fullscreen`.
- Center top navigation and the bottom player-control group when width permits; preserve narrow-window scrolling.
- Remove the old red seek/volume fill and use the concept-sheet caramel/cream/chocolate palette.
- Keep volume at 0-200% with its 100% marker while shortening it to the approved compact concept geometry.
- Use the quilted page material with subtle per-area tinting.
- Replace the old candy executable/launcher artwork with the exact approved square N emblem.
- Keep only YouTube, Rumble, RuTube, VK, and OK in Stream. Use one shared Direct Play URL field, remove the redundant Stream Play button, and keep Direct Watch as the single native-player action.
- Repair the reported YouTube Direct Watch JavaScript-runtime/403 path without adding a new bundled dependency.
- Preserve the v0.0.22 licensing boundary unchanged.

### Later owner-approval lane

Additional Stream services, Web Player, Plex integration, and other feature expansion remain future work and are not part of v0.0.23.

## v0.0.20 accepted - final ReddMedia-branded baseline

- Accepted commit: `c3d2c60e5c36407b96a0eba72e2863f884aacd28`.
- Top-level order: `Video Player | Library | Discover | Nougat | Stream | P2P | Debug`.
- Stream platform selectors: YouTube, Rumble, RuTube, VK, OK.
- Full tab palettes, 0-200% volume, centered bottom controls when wide, independent Movies/TV Grid/List preferences, and visible text caret/focus state.

## v0.0.19 candidate - Nougat Search Integration

- Native Nougat tab between Discover and YouTube.
- Local SQLite FTS5 index with Ranked and RAW result views.
- Recursive clearnet/Tor-aware crawler and persisted provenance.
- Direct peer search with local node identity and peer configuration.
- Read-only crawler output that remains selectable and copyable.
- Candy-bar Nougat palette isolated to the Nougat content area.
- App-wide compact adjacent action buttons with horizontal mouse-wheel scrolling when button rows overflow.
- Video Player footer separated into seek/time, volume with live percentage, and compact player-control rows.
- TV next-episode autoplay across season boundaries while preserving Library series/season navigation context; manual Stop cancels autoplay.
- Interior color identities: Video Player red, Library forest green, Discover plum, Nougat candy-bar, YouTube red, P2P deep blue, Debug amber/yellow.
- Future Nougat work may add automatic peer discovery, distributed routing/sharding, larger public corpora, search-engine comparison research, and media-aware result actions only through separately approved builds.

## Previous candidate: v0.0.18 Intelligent Debug, Metadata, Watch Availability, and Responsive Library

- Show `SxxExx - verified title` on episode tiles, with technical format on a secondary line and honest unavailable states.
- Resolve artwork through item, parent/series, then exact TMDb fallback without inventing matches.
- Add an evidence-based Debug tab with green/yellow/red health, actionable findings, retry/refresh/test/log/report controls, and credential redaction.
- Keep one top-bar `Server:` indicator: green when ready, yellow while transitioning, and red when unavailable.
- Keep at least two Library rows visible at the normal non-fullscreen window size and use consistent wheel/arrow navigation.
- Preserve the beginning of long Discover descriptions and provide wrapped, scrollable details.
- Show exact United States subscription, free, ad-supported, rent, and buy listings from JustWatch via TMDb; support private local My Services selections and open only TMDb's supplied official watch-options page.
- Maintain progress reporting, server ownership, Movie/TV separation, private credentials, generated-runtime exclusions, and the canonical project Bible.
- Reapply and verify the red-tree identity on the final raw `ReddMedia_v18` executable after its last write.

## Completed candidate foundation: v0.0.17 Library, Discover, and Server Reliability

- Enforce Movie/TV separation at every recommendation boundary.
- Show and cache real Jellyfin and TMDb posters without blocking the X11 draw path.
- Support validated TMDb API keys and read access tokens with Test, Save/Replace, and Clear controls.
- Show full-width red progress for catalog, poster, recommendation, credential, and server work.
- Provide Start Server, Stop Server, and Refresh Server controls while keeping owned-process shutdown safe.
- Keep generated runtimes out of Git and maintain one ReddMedia-only project Bible.

## Completed foundation: v0.0.16 Native Library and Discover AI

- Present Movies and TV as native title grids rather than a flat file list.
- Navigate TV by series, season, and episode; navigate metadata-created movie box sets before individual films.
- Link and unlink multiple typed folders without modifying the owner's media files.
- Add Discover Usual and Discover Random with Local Movie, Local TV, External Movie, and External TV.
- Keep viewing history and embedding inference local with SQLite, llama.cpp, and Nomic Embed Text.
- Use TMDb only for optional External results, remove locally owned titles, and never substitute invented data.
- Preserve direct playback in the existing ReddMedia player and the red-tree identity.
- Keep the hidden catalog process tied to the ReddMedia process lifetime so normal or forced application close releases port 8096.

## Completed foundation: v0.0.15 Native Library and Hidden Media Catalog

- Bundle the stable Jellyfin 10.11.11 server and web runtime from pinned Ubuntu 26.04 packages.
- Start and supervise the server as a hidden ReddMedia catalog component with its web client disabled.
- Add media folders, scan titles, browse the native Library, and play cataloged local paths through ReddMedia's existing embedded player.
- Preserve one ReddMedia application identity, setup path, data layout, status surface, and player.
- Keep proven open-source components while they provide useful working machinery.
- Prepare TVHeadend as the future Live TV and DVR component behind the same ReddMedia experience.

## Accepted v0.0.14 Local Pause Stability & Red Tree Identity

- Keep local-file playback responsive after a multi-minute pause.
- Use explicit libVLC pause/resume state and cached paused playback timing.
- Stop recurring full chapter-description scans during paused playback and cache chapter metadata per media item.
- Keep close responsive during a long pause with cached session-save state and bounded final player teardown.
- Replace every active ReddMedia icon surface with the approved red-tree artwork.
- Show a small red tree immediately beside the top-right `v0.0.14` label.
- Preserve accepted v0.0.13 YouTube, P2P, subtitles, local playback, and seek behavior outside the pause-lifecycle repair.
- Owner validation target: local episode -> play -> pause 1 minute -> resume -> pause at least 5 minutes -> resume -> pause and close -> reopen/resume -> repeated pause/resume cycles.

## Archive

Create one **Archive** area for legitimate public, preservation, and historical sources instead of adding a top-level tab for every archive.

Initial targets:

- **Internet Archive**
  - Search and browse archive items where supported.
  - Open playable media in ReddMedia when the source permits it.
  - Hand supported P2P sources and metadata files to ReddMedia's existing P2P system.
- **MiNERVA Archive**
  - Browse/search preserved collections where practical.
  - Hand supported downloads or P2P sources into the appropriate ReddMedia path.

The Archive design should be source-neutral so additional legitimate archives can be added later.

## Stream

Replace the current YouTube-only top-level area in a future release with one **Stream** area for online video platforms. This is roadmap-only and is not part of v0.0.19.

Planned service investigations:

- **YouTube** — first priority; use official search/player integration where available.
- **Rumble**
- **RUTUBE**
- **VK Video**
- **OK.ru / Odnoklassniki**

ReddMedia should search/browse and play inside its own experience where a service officially permits that integration. Authentication, ads, DRM, and service rules remain with the service.

## Plex integration

Add Plex as a high-priority post-v0.0.19 library/server integration:

- Link a Plex account through the official PIN/device flow.
- Discover the user's linked Plex server or servers automatically.
- Browse Plex Movies, TV, and Music inside ReddMedia.
- Route playable Plex media through ReddMedia's native player where supported.
- Support multiple Plex servers where practical.
- Test direct play, transcoding, remote access, and Plex Pass-related capabilities where Plex exposes them through supported APIs.

This is roadmap-only and is not part of v0.0.19.

## Live TV / NextGen TV

Build a unified **Live TV** area around Linux tuner capabilities rather than one hardware brand. Playback stays inside Nougat's native player.

Initial hardware paths:

- **Hauppauge WinTV-HVR-955Q** and compatible local USB/PCIe tuners through Linux DVB/V4L2 when the kernel exposes the required receiver interfaces. The HVR-955Q is the first owner-hardware target for ATSC 1.0, clear QAM, and supported analog capture paths.
- **HDHomeRun** network tuner discovery, channel enumeration, stream selection, signal/status evidence, and native playback.
- **HDHomeRun FLEX 4K / other compatible ATSC 3.0 hardware** as the first network NextGen TV target.
- Generic local/direct antenna tuner support when Linux drivers expose standard DVB/V4L2 interfaces.

NextGen TV targets:

- **ATSC 3.0 / NextGen TV** is a first-class Nougat target, not an ATSC 1.0-only afterthought.
- Parse the service/signaling information required to identify playable ATSC 3.0 services and hand supported A/V streams to Nougat's native playback pipeline.
- Add HEVC and applicable NextGen audio/subtitle handling as supported by the bundled/system media stack.
- Report encrypted/DRM-only services honestly and use provider/hardware-supported access paths where available rather than inventing playback success.

Long-term Live TV features:

- Channel scan.
- Combined channel list across available tuners.
- Electronic program guide (EPG).
- Watch Live.
- Recording.
- DVR recordings/library.
- Signal strength, lock state, modulation/service information, and tuner diagnostics.
- One combined guide even when channels come from different tuner devices.

## Radio / SDR reception

Add a native **Radio** receiver path alongside Live TV, driven by the actual capabilities of attached tuner/SDR hardware.

- Broadcast **AM/FM** reception when the device/driver exposes it.
- **NOAA/weather-band** and other receive-only services when supported by attached hardware.
- A generic **SDR backend** for supported USB/network software-defined radios, with frequency tuning, modulation selection, gain, squelch, signal meter, presets/favorites, and spectrum/waterfall work as later polish.
- **CB radio reception** through suitable SDR/receiver hardware, including conventional AM and supported SSB reception.
- Keep radio audio inside Nougat's native playback/audio path and expose real tuner/signal diagnostics in Debug.
- Any future transmit capability is a separate explicitly approved hardware/module scope; the Radio roadmap here is reception-focused.

## Supported streaming-service integration investigation

Investigate services such as Netflix and Prime Video only through provider-supported integration paths. Where third-party playback is not permitted, ReddMedia may act as a unified library/front door or launcher instead of bypassing provider-controlled playback or DRM.

## P2P expansion

Build the streaming core into a complete P2P client while keeping streaming as the normal user-facing playback behavior:

- Transfer queue management and ordering.
- Per-transfer and global download/upload limits.
- Connection limits and ratio/seeding controls.
- Tracker list/status controls, reannounce, and scrape controls.
- DHT, PEX, and local-peer-discovery status and controls where useful.
- TCP/uTP connection status.
- UPnP, NAT-PMP/PCP port-mapping status and controls.
- IPv4 and IPv6 status.
- Protocol-encryption controls.
- Proxy controls.
- Web-seed status.
- Private-transfer handling/status.
- Force recheck.
- Move storage/downloaded files.
- Multi-file selection and file priorities.
- P2P metadata creation and seeding of user-created files.
- Initial/super-seeding controls where supported.
- Persistent transfer library with completed and active transfers.
- Stronger crash/shutdown recovery and session-state persistence.

## Linux distribution

- Produce a self-contained ReddMedia Linux distribution so the normal end-user path becomes download and run.
- Bundle appropriate runtime components and their license notices while keeping build-only compilers, headers, CMake, and development packages out of the user distribution.
- Replace the external Zenity dependency with a file/folder dialog solution that can ship with ReddMedia.
- Target a single-file AppImage-style release after the runtime layout is stable.

## v0.0.24 exact N app-wide replacement repair

This same-version repair is limited to the owner-rejected Nougat N identity. The single active N source is the owner-uploaded full-resolution concept sheet. All active application surfaces must use the corrected transparent master: in-app header badge, X11 `_NET_WM_ICON`, GNOME dock/app switcher, desktop launchers, raw executable custom-icon metadata, Files/Nautilus, and installed icon-theme aliases. The old rejected blurry screenshot-derived icon hashes are forbidden from active Nougat identity surfaces. The already-installed concept-sheet quilt and per-tab tints remain unchanged.


## v0.0.24 literal concept-sheet N identity repair

This same-version repair remains icon-only. The literal N cropped from the owner-supplied full-resolution concept sheet is the single active icon source. The in-app left header, embedded X11 window icon, GNOME dock/app-switcher identity, all Nougat desktop launcher aliases, Files/Nautilus executable metadata, and installed icon-theme aliases must all resolve to that same artwork. The rejected prior icon-family hashes are forbidden. Background/quilt tinting and every non-icon subsystem remain unchanged.
