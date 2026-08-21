# Nougat Media Suite Roadmap

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

## Live TV

Build a unified **Live TV** area around tuners rather than one hardware brand.

Initial hardware paths:

- **HDHomeRun** network tuner discovery and playback.
- **Local/direct antenna tuner** connected to the computer, subject to Linux driver support for the specific device.

Long-term Live TV features:

- Channel scan.
- Combined channel list across available tuners.
- Electronic program guide (EPG).
- Watch Live.
- Recording.
- DVR recordings/library.
- One combined guide even when channels come from different tuner devices.

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
