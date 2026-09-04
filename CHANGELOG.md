# v0.0.60 - OWNER-ACCEPTED RELEASE

- Added Studio Drone Production foundation.
- Added Director Shot planning architecture.
- Added MAVLink, MAVSDK, PX4, and ArduPilot integration foundations.
- Added telemetry, camera, gimbal, simulation, and recorded-path scaffolding.
- Added reproducible Drone dependency pins and source-fetch tooling.
- Added FFmpeg/GStreamer camera-ingest detection.
- Added Director Shot schema.
- Kept real-aircraft control disabled in the simulation-only foundation.
- Added permanent approved Nougat N identity installation support.
- Advanced the root executable to Nougat_Media_Suite_v60.

# v0.0.59 - OWNER-ACCEPTED RELEASE

- File Splitter live Target MiB recalculation with exact target-based piece recommendations.
- Manual Pieces override behavior and no silent Target MiB clamping.
- Split remains blocked above the 476 MiB safe ceiling while analysis/recommendation remains available.
- Fixed first-open Pieces / Target MiB / Recommended layout.
- Separate Studio File Assembler tool.
- Manifest-or-numbered-part discovery, part verification, reconstruction, and final integrity verification.
- Reassemble / Verify removed from File Splitter itself.
- Stable independent round Player seek thumb across the full playback range in normal and fullscreen modes.
- Root executable advanced to Nougat_Media_Suite_v59.

# v0.0.58 - OWNER-ACCEPTED RELEASE

- Cache-first Home/Library startup and Continue Watching persistence/path repair.
- Older resume-record migration and artwork/provider identity recovery.
- Expanded artwork fallback handling.
- Nougat-native multi-candidate Fix Match workflow with persistent owner-controlled matches.
- Player seek/thumb endpoint repair and unloaded-player click-to-open behavior.
- Persistent preferred physical Live TV tuner handling.
- HDHomeRun physical-device grouping and guide/cache recovery.
- Universal Radio receiver/scanner service matrix expansion.
- Additional Radio SIGNAL/status spacing.
- Cellular Lab 2G GSM / future 1G interface and architecture foundation.
- File Splitter 450 MiB target and maximum-size handling.
- Server-aware diagnostics.
- Canonical GNOME/Resources application identity.
- Exact restoration of the accepted stitched Server status indicator.
- Root executable advanced to Nougat_Media_Suite_v58.

v0.0.59 carries forward File Splitter live Target MiB recalculation, the first-open Splitter layout repair, and a separate File Assembler Studio tool.

# v0.0.57 — CANDIDATE (owner testing required)

- Movies is the default Library root and Library/Home are pre-warmed so ordinary navigation does not visibly rebuild cards.
- Cached Home/Library refreshes stay in the background while finished content remains visible.
- Added tolerant real-world movie filename cleanup for periods, underscores, release tags, years, codec/resolution noise and long catalog/DVD prefixes while preserving legitimate numeric titles.
- Added persistent owner Fix Match / Clear Manual Match controls for Home and Library cards.
- Manual matches lock title/year/media type/TMDb identity/poster path and override automatic identity on later refreshes.
- Deep Diagnostic now writes private history snapshots to the Nougat config tree and no longer calls a failed history save a complete success.
- Server diagnostics use the real integrated health probe and can clear a stale transition/busy state after the server is actually healthy.
- Removed Child Safe status/gating from the System page.
- Added the owner-approved Plex-class capability program to the long-term roadmap.
- Repaired Live TV playback allocation/retry, World TV source recovery, Pro-only Radio layout, collapsible console-grouped Games, server health/startup state, and GNOME application identity.

# Nougat Media Suite Change Log

## v0.0.55 candidate — First-Party LAN Web Player

- Repairs the rejected foreground-only Web Player architecture: the Nougat browser player now owns LAN port 8096 as a persistent background companion service supervised with the Nougat-owned media server, so closing the desktop UI does not remove phone/tablet access.
- Moves the hidden Jellyfin `--nowebclient` backend to loopback-only port 8098. Jellyfin remains the catalog/API engine but is no longer the page exposed at the LAN address.
- Migrates an already-running Nougat-owned legacy/rejected v55 Jellyfin process from 8096 to the corrected 8098 + Web Player 8096 stack without claiming or killing an independently started Jellyfin process.
- Serves responsive Nougat-owned HTML/CSS/JavaScript with no cloud login, external relay, UPnP, or automatic Internet exposure.
- Adds private-LAN health/catalog contracts, catalog-ID-only media selection, direct HTTP byte-range delivery, and FFmpeg fragmented-MP4 browser compatibility streaming.
- Repairs the playback placeholder so its text cannot remain painted over active video and makes transient playback-status text clear after successful playback.
- Adds a short-viewport desktop layout gate so the player fits a 1366×768-class display at 100% browser zoom instead of requiring browser zoom-out.
- Leaves PS2 deferred and preserves the accepted Xbox/Xenia emulator-host source byte-for-byte.
- Does not include the pending File Splitter recommendation repair or unrelated Games/runtime cleanup.
- Preserves the five-button true-fullscreen transport overlay: `<<` rewind 10s, `<` previous item, `^` play/pause, `>` next item, and `>>` forward 10s, with larger drawn glyphs.

# Changelog

## v0.0.52 candidate - Professional Radio Receiver

- Focuses v0.0.52 exclusively on replacing the rejected Radio shell with a functional receiver/scanner architecture.
- Adds a simple RADIO view for ordinary listening and a PRO view for frequency, modulation, tuning step, gain, squelch, device selection, scanning, recording, spectrum-engine/runtime status and TX-chain testing.
- Adds truthful hardware discovery across Linux radio/DVB devices plus SoapySDR-class, RTL-SDR, HackRF, LimeSDR, UHD/USRP and Airspy tooling when present.
- Adds real asynchronous RTL-SDR receive/scanning paths, persistent radio Favorites/Recordings, weather/emergency/ISS/shortwave presets, Internet Radio playback through Nougat's existing libVLC engine, and a TV Antenna Scan bridge to the existing Live TV tuner path.
- Adds receive-only public-safety/P25 capability hooks through OP25 and preserves encrypted-system boundaries.
- Keeps RF transmit disabled by default while adding RX/TX hardware capability reporting and a non-radiating generated-IQ TX-chain self-test so the architecture is not receiver-only.
- Vendors/pins the owner-supplied radio open-source foundations and preserves their upstream licenses behind Nougat-owned boundaries.
- Moves every non-Radio rejected-v0.0.51 repair into the v0.0.53 carry-forward document. No other rejected subsystem is repaired in v0.0.52.
- Candidate remains uncommitted and untagged until owner testing and acceptance.


## v0.0.51 candidate

- Repairs v0.0.50 File Splitter, HDHomeRun grouping/full-scan status, World TV guide/reliability, version identity and transient overlay clipping/opacity.
- Keeps World TV source verification off the X11 event thread and prevents application teardown from blocking on long-running World TV resolver/guide/artwork network workers.
- Adds the Mulberry Radio top-level foundation as an independent root view that cannot expose the Video Player resume child simply by opening Radio.
- Repairs top-level navigation so its full horizontal scroll extent is derived from the actual rendered final System tab rather than a stale hard-coded count.
- Replaces the old N with the owner-approved new N everywhere, repairs the rounded icon alpha silhouette at both bottom corners, and uses the approved cursive brand lockup in the application header.
- Enforces replacement-style root promotion so v50 cannot remain beside v51 after a successful candidate apply, while preserving v50 in rollback evidence.
- Records the Nougat-wide controller framework roadmap with configuration under System and separate Drone Flight context.
- Candidate stays uncommitted/unpushed until owner acceptance.


## v0.0.50 - Studio File Splitter and Unified Tuners

- Adds the Studio File Splitter / Reassembler with configurable part sizes, per-part SHA-256 verification, byte-exact reconstruction, folder packaging, corruption refusal, and Zenity-backed Studio actions.
- Keeps the area named Studio; the old Gold Studio wording is removed from the active Studio screen.
- Adds HDHomeRun LAN tuner discovery alongside the accepted Linux DVB / Hauppauge WinTV backend without using an external viewer.
- Exposes HDHomeRun physical tuners independently so a FLEX DUO can keep playback on one tuner while a channel scan uses the other when free.
- Merges HDHomeRun lineup/scan results into Nougat's existing Live TV channel model and sends HDHomeRun MPEG-TS streams through Nougat's embedded libVLC player.
- Keeps all LAN Web Viewer code out of v0.0.50. v0.0.51 is assigned to remaining emulator support plus the LAN Web Viewer foundation/scaffolding.


## v0.0.49 - Games Runtime, Artwork, ZIP Library, and World TV Repair

- Adds a managed Stella runtime for Atari 2600 and keeps Atari gameplay embedded inside Nougat's native Video Player with no separate emulator taskbar/pager entry.
- Strengthens emulator-window ownership/capture while preserving the top-edge options bridge and accepted NES/SNES presentation.
- Repairs persistent Atari artwork preparation and preservation-set filename matching with cached Libretro indexes and conservative aliases.
- Prefers USA, then other English, then foreign-only same-game variants, with the newest final revision winning inside the chosen region tier.
- Removes Games wheel-event coasting and stale scrollbar-drag redraw backlog.
- Adds Sega Genesis / Mega Drive, Master System, and Game Gear discovery inside ZIP libraries with managed BlastEm support.
- Treats DOS ZIP packages as one game, safely extracts the required package on launch, selects a safe launcher, and reuses the private extraction cache when unchanged.
- Repairs Russia-24 World TV probing so accepted candidates must expose both video and audio.
- Repairs visible/diagnostic v0.0.49 identity and preserves accepted v0.0.48 behavior outside this repair scope.
- Keeps the LAN Web Player deferred beyond v0.0.49.

## v0.0.48 - Embedded Emulation, DOS, Xbox 360, and Atari Repair

- Adds one Nougat-owned embedded X11/XWayland emulator host inside the native Video Player.
- Adds DOS folder discovery with pinned DOSBox Staging 0.82.2 and repaired per-game launch/config handling.
- Adds Xbox 360 `.iso` / `default.xex` discovery with pinned Xenia Canary Linux support while refusing large Xbox ZIP extraction.
- Hardens foreign X11 window operations against `BadWindow` races and emulator shutdown crashes.
- Repairs emulator sizing so the embedded child follows the final Video Player geometry.
- Repairs Mesen fullscreen presentation; owner testing verified correct embedded NES playback and top-edge menu access.
- Adds Atari 2600 `.bin` recognition, restores ordinary `.xex` to Atari 8-bit, reserves `default.xex` for Xbox 360, and ignores `.sta` save states.
- Preserves accepted v0.0.47 subsystems and the no-copyrighted-ROM distribution policy.

## v0.0.47 - World TV Channels, Playback Reliability, and Desktop Identity

- Expands World TV with Al Quran Al Kareem TV, a larger Russian channel group, and additional Turkish and Asian channels without YouTube catalog entries.
- Requires real station artwork for exposed World TV channels and uses available current/next guide data.
- Adds visible-video probing, alternate-source resolution, stall detection, and bounded reconnect for black/dead/unstable streams.
- Shows World TV station logo/identity in the native player activity overlay and adds Previous/Next channel navigation.
- Restores the true-fullscreen [<] [^] [>] mouse transport overlay using the approved sheet button surfaces.
- Gives World TV a distinct orange interface family.
- Repairs top navigation so the System tab is fully reachable.
- Installs and validates the canonical GNOME desktop identity and approved N icon for the running app/dock.

## v0.0.46 - World TV Repair, Scanner Control, and LAN Media Foundation

- Repairs World TV video-window ownership and verifies actual libVLC startup before reporting playback.
- Adds real scanner cancellation with process-group cleanup, partial-result preservation, separate Threat/Suspicious counters, and a tiered bulk scanning path.
- Establishes WAN-independent versioned LAN media interfaces without cloud login, cloud relay, or automatic router exposure.
- Makes root-executable cleanup and final Nougat N custom-icon verification permanent release gates.
- Enforces README ordering: suite identity/introduction, current version, then older history.

## v0.0.45 - Secure Search Foundation, Privacy Broker & Crawler Access Architecture

- Moved plaintext Search query delivery from child-process argv to private local stdin IPC.
- Removed automatic DuckDuckGo/live-discovery Search fallback and disabled plaintext remote peer Search.
- Split local Search and crawler networking into separate workers.
- Added fail-closed Secure Search controller, Privacy Policy, Privacy Receipt, Privacy Broker client/protocol scaffold, and Crawler Access Manager.
- Made the legacy administrative node loopback-only and refused plaintext `/nougat/v1/search` requests.
- Added truthful `NougatSearchCrawler/0.0.45` identity, robots handling, crawler access classification, and no-auto-payment handling for HTTP 402.
- Added privacy-canary validation proving a unique query does not appear in worker argv, environment, output, or persistent Nougat Search files.

## v0.0.44 - World TV, Playable Games, Artwork, and Global Card Actions

- Promote World TV to its own top-level tab after Live TV and remove all YouTube catalog entries.
- Use direct foreign linear HLS broadcaster/CDN sources, libVLC adaptive 1080p ceiling, faster live caching, HTTP reconnect, and bounded unexpected-end recovery.
- Add pinned MesenCE 2.2.1 for immediately playable bundled NES tests plus SNES/GB/GBC/GBA, RMG 0.9.0 for Nintendo 64, and Atari800 7.1.2 for Atari 5200/8-bit.
- Reuse the exact Library multi-row grid/scroll geometry for Games, with automatic cached box-art lookup and verified bundled art for the legal test ROMs.
- Add right-click context menus to all card-based Nougat surfaces with relevant Play/Open, source, information, refresh, broadcaster, and artwork actions.
- Preserve v0.0.43 Library/My Services repairs, the native player, licensing boundaries, and the permanent approved Nougat N icon gate.

## v0.0.43 - Games, World TV, and Responsive Grid Repair

- Repair the shared Movies/TV Library grid so a height-safe two-row layout cannot collapse to one row after card-width redistribution.
- Expand Games with Grid/List views, system/source badges, double-click launch, automatic emulator selection, Atari formats/backends, ZIP-aware ROM indexing/extraction, persistent ROM folders, and local sidecar box artwork.
- Preserve bundled legal NES starter assets and their source/license records unchanged.
- Show the tested Manta USB controller identity when connected while preserving backend-owned button mapping.
- Expand World TV with language-agnostic broadcaster-owned live sources including Al Jazeera Arabic and DW News, without location-based catalog filtering.
- Repair Discover > My Services with a reserved scrollbar gutter, wider draggable scrollbar, and uncluttered service-row spacing.
- Preserve the permanent approved Nougat N icon release gate and accepted v0.0.42 behavior outside this scope.

## v0.0.42 - Persistent Libraries, Live TV Maintenance, Security, Intelligence, and Games

- Adds persistent Movies/TV mapping recovery and startup-resilient private Jellyfin session behavior.
- Adds metadata title-structure correction, provider matching groundwork, and persistent recommendation embedding caches.
- Repairs Discover to Live TV UI deadlock, My Services scrolling/header layout, and responsive Library/Home card fill.
- Adds persistent/automatic Live TV guide maintenance, exact `Refreshing` active state, and `World TV`.
- Expands Virus Scan with Movies, TV, Quick, and System scanning paths.
- Adds a functional Games tab with persistent ROM folders, system/backend detection, controller visibility, and emulator launch.
- Repairs Live TV to local-media player ownership handoff and the full-row subtitle On/Off context-menu toggle.
- Moves the player volume and transport rows upward slightly without changing the seek bar.
- Same-version repair: restores the mandatory Nougat identity gate after the rejected generic-root-executable icon regression. The exact N comes from the owner-supplied approved UI sheet, and installer validation now proves Files/Nautilus custom-icon metadata, icon-theme assets, canonical launchers, and embedded X11 icon data before handoff.

## v0.0.41 - Housekeeping, Archives, IMDb, Live TV and Player Activity Repair

- Adds the Search `Archive` directory with Archive.org, Minerva Archive, and the curated archive/library links.
- Restores the shared Movies/TV Library card geometry and prevents height-driven card shrinking.
- Adds exact IMDb links to Movies, TV series/episodes, and Home cards when a verified IMDb ProviderId is present.
- Repairs the scrollable Search-card bottom-gap geometry.
- Adds `Stop Live`, immediate tuner release, and queued-guide continuation.
- Repairs idle manual Live TV guide refresh state.
- Makes the on-video media identity share the pointer's three-second activity timer in fullscreen and windowed/maximized playback.
- Moves the Home LAN web viewer/streaming project to v0.0.42 in the roadmap.

## v0.0.40 - System Loading, Live TV Tuner Navigation, Search Repair, and Crawler UI

- Replaces the oversized percentage loading surface with the 3 px caramel loading sliver.
- Restores Detect Tuner and Refresh Tuner to Live TV and adds the dedicated Tuners page.
- Applies and verifies the approved Nougat N artwork on the actual root executable.
- Repairs the Nougat Search FTS index and improves multi-word result matching.
- Adds clearnet bootstrap discovery when local and peer results are insufficient.
- Arranges Search as field, SEARCH, RAW.
- Makes Crawler Max Pages a visible minus, value, plus control.

## v0.0.39 - Diagnostic Center and Live TV Reliability Repair
- Replaces obsolete diagnostic severity with Passed / Needs Attention / Problem / Not Tested / Information.
- Adds evidence, expected/observed results, repair guidance, history, and Quick/Deep diagnostics.
- Preserves guide cache data, queues full guide sweeps until a single tuner is idle, and forbids text artwork fallbacks.
- Preserves the accepted exact-sheet player VOLUME asset unchanged.


## v0.0.39 - Deep Diagnostics and Live TV Guide Reliability

- Replaces the old flat diagnostic interpretation with subsystem health summaries using Passed, Needs Attention, Problems, Not Tested, and Information.
- Adds evidence/expected/action detail, recursive Library counts, correct Search idle handling, Live TV/tuner awareness, and diagnostic history snapshots while retaining TXT/JSON/support-bundle exports.
- Treats metadata/artwork completeness as information rather than catastrophic suite health and distinguishes individual Library path warnings from core failures.
- Expands tuner evidence with Linux DVB frontend/demux/dvr/net access, delivery-system details, tuner-use state, signal lock/quality when exposed, current channel/program, guide coverage, guide freshness, and queued-refresh state.
- Loads cached Live TV guide data automatically, preserves valid cached events, waits longer for PSIP tables, collects additional EIT tables, and merges source-ID/VCT evidence instead of wiping rows on an incomplete broadcast cycle.
- During Live TV playback, harvests PSIP from the currently tuned multiplex through demux0 without retuning; a full cross-multiplex refresh is queued until the single tuner becomes idle.
- Expands bundled real channel/network artwork and uses square logo cards as explicitly approved.
- Keeps the accepted player layout unchanged while removing only the rejected pale bottom strip from the progress bar and retaining its approved-sheet stitched detail.
- Keeps Live TV program start/end timing and uses the approved dark timing text.
- Rebuilds the round Server status light as one smooth stitched design shared by green/yellow/red states; only the state color changes.

## v0.0.38 - Library, Live TV and Player Exact-Sheet Polish

- Continue Watching adds local movies/episodes after 10 seconds of actual playback, avoiding accidental-open clutter.
- Repaired Library Search focus/caret/typing visibility and empty-query reset semantics without moving the approved Search row.
- Reworked Library card text so titles win visible space and technical media details move to stitched hover/focus information.
- Restored the system-wide loading/progress surface to its original lane directly beneath the top tabs. Determinate loading uses the literal approved-sheet PROGRESS BAR, sized only high enough to contain the percentage, with the percentage carried inside the moving caramel fill.
- Unified player pointer and media/Live TV information under one three-second activity timer.
- Preserved the literal approved-sheet seek and VOLUME artwork. The seek keeps its stitched body while only the separate pale underside crop-shadow is omitted; VOLUME keeps its current geometry/artwork and only its small underside crop-shadow is omitted. The VOLUME percentage is black.
- Added visible approved-sheet-style vertical scrollbars to existing vertically scrollable surfaces, including Discover, Live TV Guide, Search results/Crawl/Peers, and System diagnostics, while preserving all existing mouse-wheel scrolling.
- Live TV Guide remains the single guide destination: the redundant Channels control is removed, Guide stays in place, and the guide grid remains the default Live TV surface.
- Moved only Detect Tuner and Refresh Tuner from Live TV to System; Scan Channels, Watch Live, Guide, Refresh Guide, and Record remain on Live TV.
- Replaced duplicate numbered channel-logo badges with locally packaged real broadcaster/network artwork where identified (including NBC, PBS, PBS Kids, Telemundo, ABC, CBS, FOX, CW, MeTV, ION, ION Plus, Create, Bounce, Busted, and ShopLC); unknown stations fall back to call-sign text rather than another channel number.
- Live TV retains remembered/keyboard channel selection, current-program identity, and aligned broadcast program timing.
- Made installer baseline hash validation fail-closed.
- Preserved the accepted Live TV playback/timeshift path while documenting further timeshift characterization for later DVR work.

## v0.0.37 - Native Live TV Watch + Classic Guide + System/Visual Repair
- Renamed Debug to System and moved server Start/Stop/Refresh controls out of Library.
- Unified Continue Watching card size with normal Home cards.
- Made the exact-sheet seek responsive/full-width with side timestamps and removed seek/volume halo artifacts.
- Replaced the server indicator with the stitched whole-face state circle.
- Deduplicated owner-visible tuner devices to logical DVB tuners.
- Added channel selection, double-click/Watch Live ATSC playback through Nougat's native libVLC player, and tuner ownership guards.
- Added ATSC PSIP EIT guide refresh/cache plus the first classic channel-by-time guide grid and Now navigation.
- Preserved accepted v0.0.36 Library/Search/collection behavior, P2P, security, diagnostics, licensing, and the validated ATSC scan.

# Nougat Media Suite v0.0.36 - Library Hierarchy, Home Artwork, and Exact-Sheet Player/Header Repair

- Adds a sheet-exact Library `Search` input on its own row below the green actions with live local filtering.
- Enforces collection-first Movie roots so BoxSet member films do not duplicate beside their collection card; collection children remain directly navigable/playable and are ordered by production year/name.
- Fixes Home card artwork black-gap/vertical-offset rendering by isolating offscreen image drawing from the page/shelf X11 clipping GC.
- Adds a 101-frame, pixel-derived seek sprite family from the literal approved `SEEKBAR (PROGRESS)` component and keeps elapsed/total timestamps on the same line to the left/right of the shorter bar.
- Replaces competing seek/volume partial redraw rectangles with one stable full-height player-control repaint region, eliminating mouse-state clipping between the seek times, VOLUME housing, percentage, and transport row.
- Preserves the accepted exact VOLUME sprite while masking the rectangular source-sheet corners outside its rounded housing.
- Recolors the global header to the approved VOLUME-housing tan, vertically centers both header clusters, and uses a sheet-family circular Server state indicator.
- Preserves the working v0.0.35 Linux DVB ATSC channel scan and puts persisted-channel native `Watch Live` tuning/playback on the next-build agenda.
- Defers the requested lettering/font redesign to a dedicated future system-wide typography pass.

# Nougat Media Suite v0.0.35 - Code + Bug Cleanup, UI Alignment, Live TV Scan, and Studio Foundation

- Stabilization-first cleanup release on the accepted v0.0.34 line.
- Fix App-owned Nougat Search/Crawler shutdown lifetime by joining workers instead of detaching them across owner destruction.
- Replace the false-positive v0.0.34 seek/volume assumptions with stronger v0.0.35 checks; Volume now uses a packaged sheet-pixel-derived 335x47 sprite family whose 100% frame is the canonical approved VOLUME component.
- Search outer page frame is square; rounded treatment remains on inner controls/panels.
- Standardize top inner control placement app-wide using Stream's accepted baseline, including Search, Debug, Live TV, Discover, and Library.
- Recenter the Video Player's full 8-button transport row as one group at normal/full widths after the larger-tab layout change; the same row and Debug's 10-button action strip scroll completely to their final actions at narrow widths. Library no longer wraps its header controls: tools remain one scrollable row and List/Grid stay fixed at the far right.
- Live TV removes the hardware-description subtitle and gains a native Linux DVB ATSC 1.0 RF 2-36 scan with frontend lock/signal evidence, PSIP VCT channel discovery, progress reporting, cancellation, and persisted channels.
- Adds Studio between Stream and Debug with a dedicated yellow/gold palette with brown stitched borders and an internal Gold Studio identity.
- Enlarges the selected top-tab pointer to match the taller/wider tab bodies and paints it after page content/loading chrome so it remains visible.
- Repairs partial player repaints so undefined pixmap pixels cannot produce the black horizontal band near the seek/volume area; preserve the owner-confirmed no-flash Up Next behavior.
- Expands the roadmap for the Nougat Media Processing Engine, conversion/audio tools, Quick Edit, Batch, and eventual full timeline Studio.

# v0.0.34 candidate - Exact Sheet Tabs/Player Controls + Home/Discover UI Repair

- Rebuilt the global top navigation from the approved concept-sheet TAB BUTTONS geometry/material treatment.
- Shifted only the scrollable tab lane left toward the app name; fixed right Server/version chrome is unchanged.
- Rebuilt seek and volume visuals from the actual sheet while preserving Nougat playback/0-200% volume behavior.
- Fixed Home-wide card geometry/alignment, direct scrollbar dragging, Live TV header overlap, affected page-frame corners, Discover Live TV selection, and TMDb source naming.
- Retains accepted v0.0.33 server/security/P2P/tuner foundations without source changes.

# v0.0.33 candidate - P2P Plus + Security Hardening + Persistent Server + Live TV Foundation

- Added system-wide bordered/hard-clipped content viewports to normal pages while leaving Video Player untouched.
- Hard-clipped Home/Continue Watching and added Library containment, vertical scrolling, and toolbar wrapping.
- Clipped the top navigation between fixed Nougat branding and fixed Server/version chrome; inserted Live TV between Discover and Search.
- Hardened Security Analysis verdict truthfulness and pinned YARA-X 1.19.0, capa 9.4.0/rules, and Magika 1.0.3 through a generated one-shot runtime. Added URLhaus to free abuse.ch reputation hooks and renamed Community Key to Threat Intel Key.
- Added P2P Plus speed limits, seed ratio/time rules, file priority, queue movement, tracker status, Force Reannounce, and Force Recheck behind Nougat-owned interfaces.
- Made the Nougat-owned Jellyfin server persistent across UI close/reopen after Start Server; Stop Server remains explicit and external Jellyfin is never killed.
- Added Live TV scaffolding with Linux DVB/V4L2 discovery, channel storage/probe interfaces, and WinTV-HVR-955Q as the first hardware target. No fake tuning/playback claims.

# v0.0.32 same-version replacement — P2P + Nougat Security Analysis

- Retained the already-built native P2P media workflow, playback-aware seek scheduling, autoplay no-flash repair, Search seam contrast, and Stream bottom-only provider panel treatment. Replaced the oversized volume housing with the same sheet-style track used by Seek, only shorter.
- Added `Virus Scan` beside Search/Crawler/P2P with manual file/folder scanning, Scan Again, scan history, and optional free community telemetry key configuration.
- Added the one-shot Nougat Security Analysis scaffold: SHA-256, file/content identification fallback, extension/type checks, built-in rule plumbing, hooks for pinned YARA-X/capa/Magika, optional external one-shot `clamscan`, and optional MalwareBazaar/ThreatFox community lookups. Full pinned engine runtime installation is intentionally deferred to the security-hardening pass.
- Locked security behavior to **WARN ME FIRST**. Nougat does not automatically quarantine, delete, move, rename, or open suspicious files, and it installs no resident security daemon or filesystem watcher.
- Added automatic one-shot analysis when the selected P2P transfer becomes complete/seeding, using the same engine as manual Virus Scan.
- Added explicit local-seed / available-idle-uploading-paused information, known/connected peer and remote-seed evidence, and libtorrent swarm availability where meaningful.
- Removed the stray normal Search `Node <id>` display while retaining the Node ID in Network/Advanced. Moved only the unchanged Crawler status sentence upward so the existing results box no longer crosses the text.
- Added dedicated Home scrolling controls: a right-side vertical page scrollbar, a bottom horizontal Continue Watching scrollbar, and hard clipping below the fixed top header so posters cannot scroll over the menu.
- Assigned advanced qBittorrent-class management to v0.0.33 under the owner-approved name **P2P Plus**.

# Changelog

## v0.0.31 candidate — Exact Approved UI Sheet Components

- Replaced the generic rounded-button approximation with a shared approved-sheet component renderer using a lower shadow, dark rim, raised inner bevel, stitched/inset seam, top highlight, pressed state, hover state, and disabled state.
- Routed top-level navigation plus Search, Discover, and Stream selector tabs through the same component family with the integrated downward selected point/notch.
- Rebuilt large panels and text/input fields around the sheet's double-rim/inset construction while retaining each page's accepted v0.0.30 palette.
- Rebuilt Library List/Grid icon controls as actual sheet-family icon buttons instead of flat rectangles.
- Replaced textual watch-service `[x]` markers with sheet-family checkbox controls.
- Rebuilt the Video Player seek track, volume housing/track, slider knobs, and loading/progress surface with the sheet component geometry; slider knobs are now proportioned at 24 px against their compact housings.
- Preserved Home cards, accepted page/background palettes, metadata/cache behavior, playback logic, Search/P2P transport, Stream behavior, TV autoplay, diagnostics, licensing, and user-data paths outside the visual component layer.
- Reassigned the focused P2P streaming expansion to v0.0.32 so v0.0.31 remains a clean UI-sheet fidelity release.
- Expanded the future Live TV roadmap with the owner's Hauppauge WinTV-HVR-955Q, HDHomeRun, ATSC 3.0/NextGen TV, and a receive-focused Radio/SDR path including AM/FM/weather and CB reception through suitable hardware.

## v0.0.30 accepted — UI Cohesion, Library Performance, and Player Navigation

- Standardized the large Library, Discover, Stream, and Debug content surfaces on Search's rounded/inset Nougat panel language while preserving page-specific palettes.
- Moved the gold busy/progress strip below the top-tab notch so selected Home/Library/Debug pointers are not clipped by busy-page paint order.
- Changed Home movie/series/season cards to portrait 2:3 DVD/poster geometry while retaining landscape episode stills and rounded artwork clipping.
- Reworked Library Grid sizing from both available width and height so multiple poster rows are visible at once instead of one row occupying the whole vertical viewport.
- Added a private persistent Library metadata cache. Known library views can paint cached metadata first while the current Jellyfin view refreshes asynchronously, and verified cached artwork metadata avoids unnecessary repeat enrichment.
- Added determinate Library/artwork progress reporting based only on real completed/total work. Jellyfin scan phases with no measurable total remain indeterminate rather than using a fake percentage.
- Clarified refresh semantics: Refresh Server updates server/process state; Refresh Library performs Jellyfin library scanning and metadata reload.
- Added `Previous` and `Next` player buttons using the v0.0.29 episode queue/resolver, separate from the existing 10-second seek controls.
- Enlarged the compact volume knob to fit its rounded housing proportionally while retaining 0-200% gain and the existing percentage readout.
- Preserved accepted licensing, Search/P2P implementation, diagnostics, Stream providers, v0.0.29 TV reliability, and N identity outside this scope. Focused P2P streaming expansion moves intact to v0.0.32.

## v0.0.29 candidate — TV Playback, Navigation, and Carry-Forward UI Repair

- Repaired TV next-episode resolution at the player level: local episodes prepare a same-folder queue regardless of launch route, prefer parsed `SxxEyy` / `NxNN` identity, and retain natural filename ordering for catalog-confirmed episodes without explicit tokens.
- Pre-resolved the next episode when playback starts so natural EOF can present Up Next immediately instead of waiting on a late folder/catalog scan.
- Restored a visible 10-second Up Next countdown with `Play Next`, `Back to Series`, and `Replay`, plus autoplay at zero and bounded retry if the resolved local file cannot start immediately.
- Repaired `Back to Series` so series ID or exact episode-path catalog resolution returns to the actual series browsing context instead of the generic Library root whenever the catalog can identify the series.
- Added a real v0.0.29 executable behavior self-test covering `S01E13 -> S01E14`, natural filename ordering, series-folder inference, and creation of the actual 10-second Up Next overlay state.
- Repaired Home wheel routing so the top tab strip scrolls horizontally even while Home is selected; Home's Continue Watching shelf and vertical page wheel handling no longer swallow header events.
- Added Vimeo immediately after YouTube in Stream, including URL detection, provider homepage action, selected provider control, provider-reactive quilt/palette, and Vimeo blue/black/white treatment.
- Removed the partial brown Video Player rail so the page background surrounds the video uniformly.
- Strengthened Continue Watching TV artwork recovery: exact episode Primary/still first, matching season poster second, series poster third, with exact path containment able to recover the owning series for direct/open-file resume records.
- Changed Home poster rendering from contain/postage-stamp presentation to aspect-preserving cover fill while retaining rounded top clipping and silent hover preview clipping.
- Preserved accepted v0.0.28 licensing, Search/P2P implementation, diagnostics, palette, Library poster system, Search cleanup, resume/seek behavior, and approved N identity outside this scope.

## v0.0.28 — Candy Palette, Artwork, and UI State Polish

- Page backgrounds now carry the Nougat candy identity: purple Home, cocoa/chocolate Video Player, green Library, red Discover, cream Search, provider-reactive Stream, and charcoal Debug, with restrained supporting colors.
- Home remains loaded across tab switches unless watch/library data changed or an explicit refresh is requested.
- Home resting artwork is poster-first. TV Continue Watching resolves season poster then series poster fallback; silent hover previews are retained and top rounded-corner clipping prevents square pixels protruding through the card.
- Home section/category and metadata text are more readable, the X11 metadata bullet path is repaired, and the LOCAL grid fits at least three cards per row around 650-pixel half-screen width.
- Library poster loading prefers exact-ID TMDb artwork, falls back to Jellyfin Primary, requests 480x720 display-useful art, rejects tiny/landscape poster sources, and preserves portrait aspect instead of stretching.
- Video Player removes the remaining pale/white windowed-video rim in favor of a cocoa/chocolate theater surround with caramel trim.
- Search removes the redundant standalone `SEARCH` heading and reclaims the vertical space while keeping `Search | Crawler | P2P`.
- TV Up Next/autoplay and Back-to-Series reliability are deliberately reserved for v0.0.29; the major P2P expansion moves to v0.0.30.
- Accepted v0.0.27 Search/P2P implementation, licensing boundary, diagnostics, resume/player behavior, and N identity are preserved outside this scope.

## v0.0.27 - Home, Resume History, Player Polish, and Seek Previews

- Added **Home** as the first/default top-level tab.
- Added persistent **Continue Watching** with all unfinished local movies/episodes, caramel progress bars, and horizontal mouse-wheel navigation only while hovering that shelf.
- Added a normal vertically scrolling **LOCAL** card wall below Continue Watching, organized by genre/category and a watch-history-informed recommendation group.
- Added wide 16:9 Home artwork selection preferring higher-resolution Jellyfin backdrops with verified fallbacks, plus one-at-a-time muted card hover previews extracted from the real local video.
- Added persistent per-title resume history and `Continue | Start Over | Cancel` reopen behavior.
- Added a stopped-playback overlay with `Resume | Restart | Load Different | Back to Library`.
- Added permanent windowed/maximized now-playing identity below video and temporary true-fullscreen identity on mouse activity.
- Added rounded video corners in windowed/maximized playback while preserving square true fullscreen.
- Added seek-hover real-frame preview, timestamp, and real chapter label when available without seeking the active libVLC instance.
- Repaired pointer-motion flicker by eliminating raw-motion whole-window repaint scheduling.
- Repaired the header divider cutting through the selected tab notch.
- Removed redundant Discover state heading, Stream `Direct Play URL` heading, and Debug `DIAGNOSTIC CENTER` heading.
- Changed the Stream URL placeholder to exactly `Paste URL Then Press Direct Watch / Rumble / RuTube / VK / OK`.
- Preserved the accepted v0.0.26 diagnostics, Up Next, Search/P2P, Stream, Library, Discover, licensing, icon identity, mouse Back/Forward, and 0-200% volume behavior.
- Logged the owner-approved later palette/background and video-surround proposal without applying it to v0.0.27.

### v0.0.27 same-version installer validation repair
- First owner-machine v0.0.27 install attempt applied the candidate, then correctly rolled back to accepted v0.0.26 when the post-apply validation lane invoked the historical v0.0.26 release-identity test and reported `FAIL: CMake v26 identity missing`.
- Removed only that contradictory post-apply `tools/test_nougat_media_suite_v26.py` invocation.
- Kept `tools/test_nougat_media_suite_retained_v26.py` as the accepted-v26 compatibility gate under v0.0.27 identity.
- Added an installer regression guard that rejects reintroducing the old v26 identity contract into the v27 post-apply source-test lane.
- No Home, player, Library, Discover, Search, Stream, Debug, diagnostics, P2P, licensing, media-server, or runtime feature code changed in this same-version repair.


# Changelog

## v0.0.26 — Candidate: Systems, Navigation, Diagnostics, and TV Up Next
- Added X11 mouse side-button Back/Forward navigation history.
- Removed redundant Library root heading and moved List/Grid controls to the far left.
- Cleaned the approved N icon lower perimeter and regenerated app-wide icon assets/embedded icon data.
- Rebuilt Debug as an evidence-based Diagnostic Center with TXT, JSON, and redacted support-bundle exports.
- Cleaned and centered the intentional 0-200% volume control, retaining only one percentage readout.
- Fixed header z-order so scrolling top tabs cover fixed branding, server status/dot, and version text.
- Added TV Up Next overlay with a visible 10-second countdown, Play Next, Back to Series, Replay, and explicit no-next/failure messaging.
- P2P feature expansion remains deferred; v0.0.26 only reports current P2P evidence in diagnostics.

# Nougat Media Suite Changelog

## v0.0.25 - Stream Provider Theme, Persistent Selection, and Discover Native Play

- Made the selected Stream provider drive the full Stream interior palette and exact concept-sheet quilt tint: YouTube red, Rumble green, RuTube purple, VK blue, and OK orange/caramel.
- Made common Stream actions such as Download, Direct Watch, Open Webpage, and Clear Log inherit the selected provider palette.
- Added the concept-sheet downward selected notch to the active Stream provider.
- Added persistent Discover selection indicators for the independent `Usual | Random` mode group and `Local Movie | Local TV | External Movie | External TV` target group, allowing two active notches simultaneously.
- Kept TMDb/service controls as momentary action buttons rather than persistent selectors.
- Repaired local Discover `Play in Nougat...` so Jellyfin movie/series results resolve to a real playable local file before entering the native player.
- Series-level local Discover play now chooses the most recently watched matching episode when available from local history, otherwise the first real episode in season/episode order.
- Added deterministic provider-theme/selector-state tests plus a fake-Jellyfin behavior gate for local-series playback resolution.
- Preserved the accepted v0.0.24 concept-sheet N, quilt source, Search engine/bridge behavior, protected licensing files, Crawler spacing, TV autoplay repair, and pointer-motion performance repair.
- Same-version owner-test repair: removed the redundant `STREAM` / `Online video: <provider>` text above the provider row. The Stream top tab and selected provider downward notch already communicate both states, so the extra label is no longer rendered. Provider colors, quilt tinting, provider notches, Stream actions, Discover behavior, and playback logic remain unchanged.
- Same-version owner-test repair: removed the now-unnecessary cream/white Stream provider container and border so the YouTube/Rumble/RuTube/VK/OK selectors render directly on the active provider-tinted concept-sheet quilt. Provider geometry, colors, selected notch, Stream actions, Discover behavior, and playback logic remain unchanged.


## v0.0.24 same-version installer repair - Legacy v23 launcher accepted and replaced

- Repaired the exact-N app-wide installer preflight after owner testing found an existing untracked `NougatMediaSuite_v23.desktop` legacy launcher in the rejected v0.0.24 working tree.
- The legacy v23 launcher is now an explicitly supported icon-identity surface: it is rollback-captured if present, overwritten with the same corrected exact-sheet N launcher identity, installed into the user applications directory, and restored on installer rollback.
- No icon artwork, background/quilt, Search, playback, Library, Discover, Stream, diagnostic, or licensing behavior changed in this installer-only repair.

## v0.0.24 same-version repair - Exact Master Icon and Quilt Background

- Owner rejected the preceding v0.0.24 candidate because the dock/application N was generated from a blurry screenshot crop and the quilted page background was an approximation instead of the full-resolution concept-sheet material.
- Replaced every active Nougat icon size and embedded X11/top-bar icon with the exact N extracted from the owner-uploaded full-resolution concept sheet.
- Removed the concept-sheet canvas from the icon asset: the area outside the emblem is transparent, with no white/cream square or halo around the N.
- Replaced the procedural diagonal-line quilt approximation with an exact 128x128 crop of the padded quilt material from the concept sheet, rendered as a mirrored X11 tile for seamless page coverage.
- Applied visible per-area dye/tints to that same quilt material: warm caramel for Video Player, sage for Library, lavender for Discover, cream/gold for Search, dusty blue for Stream, and taupe/gray for Debug.
- Preserved the already-passing v0.0.24 Search-page polish, Crawler spacing repair, GNOME identity chain, TV autoplay repair, pointer-motion lag repair, Search-engine behavior, and protected licensing files unchanged.

## v0.0.24 same-version repair - Search UI Stability

- Fixed the Crawler status/log overlap by moving the log panel below the status baseline with a visible gap.
- Preserved Crawler text selection/highlight behavior unchanged.
- Repaired Ubuntu/GNOME dock matching with canonical application ID `com.elderredsoftworks.NougatMediaSuite`, the accepted `WM_CLASS=NougatMediaSuite`, a canonical desktop-file hint, and the approved N icon-theme asset.
- Installed `com.elderredsoftworks.NougatMediaSuite.desktop` while retaining the compatibility launcher.
- Repaired TV next-episode autoplay so an episode can reconstruct its Jellyfin Series parent, natural EOF accepts libVLC Ended plus a guarded near-EOF Stopped fallback, local playback startup is checked, and a failed next-episode start is retried a bounded number of times. Manual Stop still cancels autoplay.
- Reduced UI lag by throttling full-window concept-UI repaints during raw X11 pointer motion while preserving a final pending hover repaint and existing Crawler drag-selection behavior.
- Search-engine behavior and protected licensing files remain unchanged.


## v0.0.24 - Search Page UI Polish

- Moved the approved N emblem to the far-left header and placed `NOUGAT MEDIA SUITE` immediately beside it.
- Removed the duplicate right-side N badge while retaining server status and version on the right.
- Restyled Search/Crawler/P2P sub-tabs with the approved stitched/beveled Nougat controls and active-tab point.
- Converted Search inputs, action buttons, result cards, peer lists, crawl log, and embedded P2P fields/panels to the concept-sheet family.
- Aligned `Network...` and `SEARCH` exactly in one right-side column.
- Improved Search text contrast and replaced the legacy dark results slab with a light Nougat panel.
- Added a stable GNOME X11 application-ID hint and direct launcher icon path to eliminate the generic gear fallback.
- Preserved Search-engine behavior and all protected licensing files unchanged.

## v0.0.23 - Exact Concept UI and Stream Direct Watch Repair

- Applied the owner-approved concept-sheet control language to the existing UI without changing the established top-tab or player-control order.
- Added the integrated downward point/notch to the active top-level tab and centered the top navigation group when width permits while preserving narrow-window scrolling.
- Centered the six-button Video Player control row when width permits while preserving narrow-window wheel scrolling.
- Replaced the old red seek/progress and volume fill colors with the concept-sheet caramel/cream/chocolate treatment.
- Shortened the 0-200% volume control to the approved compact concept-sheet proportions while retaining its live percentage and 100% marker.
- Added the approved quilted page material with subtle area tints for Video Player, Library, Discover, Search, Stream, and Debug.
- Replaced the old candy icon asset with the exact square chocolate/caramel N emblem from the approved concept sheet across project icon sizes and embedded X11 icon data.
- Simplified Stream to one shared Direct Play URL field and removed the redundant Stream Play button; Direct Watch remains the single native-player playback action.
- Preserved the existing Stream services only: YouTube, Rumble, RuTube, VK, and OK. No additional services were added.
- Updated YouTube Direct Watch to auto-detect supported Deno, Node, or QuickJS runtimes for yt-dlp challenge solving and to prefer a YouTube playback fallback path intended to avoid the reported 403 failure.
- Preserved the v0.0.22 PolyForm Noncommercial licensing files unchanged.

## v0.0.22 - License Protection and Contribution Boundary

- Corrected the project licensor/copyright notice to **Elderred Softworks LLC**.
- Kept **PolyForm Noncommercial License 1.0.0** as the controlling recipient license for owner-controlled Original Materials.
- Added `COPYRIGHT.md` to state the ownership boundary and preserve the owner's ungranted commercial/relicensing rights.
- Added `CONTRIBUTING.md` and a pull-request confirmation template so outside contributions grant the project owner broad inbound rights without silently changing the recipient license.
- Added `docs/LICENSING_POLICY.md` to separate Original Materials from third-party material and prohibit accidental relicensing.
- Rewrote `THIRD_PARTY_NOTICES.md` under the Nougat Media Suite identity and clarified Jellyfin, FFmpeg, libVLC, libtorrent, yt-dlp, llama.cpp, Nomic, TMDb, and system-component boundaries.
- Added deterministic license-protection and installer-rollback tests.
- Same-version runtime-path repair: the installer now proves the accepted v0.0.21 executable against the relocated AI runtime after the project-directory rename, and v0.0.22 embeds a relocatable `$ORIGIN` llama.cpp RPATH instead of an absolute project path.
- Advanced only release/version plumbing to `Nougat_Media_Suite_v22`; media/UI behavior remains the accepted v0.0.21 behavior.
- Moved the planned UI-polish/service-expansion lane to v0.0.23 so v0.0.22 remains a clean licensing release.

## v0.0.21 - Official Rename, Candy Palette, and Navigation Repair

- Renamed the visible application identity from **ReddMedia** to **Nougat Media Suite**.
- Renamed the top-level **Nougat** tab to **Search** while preserving the integrated decentralized search engine and behavior.
- Removed top-level **P2P** and moved media/torrent P2P under Search, giving the ordinary Search subnavigation exactly **Search | Crawler | P2P**.
- Moved decentralized search peer/node administration behind a smaller **Network...** advanced surface instead of presenting Peers as a normal Search tab.
- Changed the versioned root executable target to `Nougat_Media_Suite_v21` and the X11/window/launcher identity to Nougat Media Suite.
- Replaced the red-tree icon system with the owner-approved rounded-square chocolate/nougat **N + play triangle** artwork.
- Replaced common red navigation chrome with the candy-family identity and made the top-level tabs themselves carry their area colors.
- Changed Video Player from the former cinema-red identity to **chocolate/cocoa/caramel**, while preserving Library forest/sage, Discover plum/lavender, Search nougat/caramel, and Debug graphite/amber.
- Added service-reactive Stream palettes for existing YouTube, Rumble, RuTube, VK, and OK selectors.
- Replaced nonworking `Grid [x]` / `List [x]` Library text controls with working compact List (three lines) and Grid (four-square) icon controls beside the Library heading. Movie and TV preferences remain independently persistent.
- Preserved the v0.0.20 Stream service set with no new services in this version.
- Deferred the post-rebrand UI polish, additional Stream services, and the other feature work to v0.0.23 after the v0.0.22 licensing release.


## Historical ReddMedia changelog


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
## v0.0.24 same-version exact N app-wide replacement repair

- Owner rejected the preceding v0.0.24 icon result because the blurry screenshot-derived N remained active on application surfaces.
- Replaced the complete active Nougat icon family again from the owner-uploaded full-resolution concept sheet, with a clean transparent silhouette and no cream/white exterior halo.
- Regenerated the embedded top-bar and `_NET_WM_ICON` pixel data from the corrected master so the in-app header, running window, dock/app switcher, and executable all share the same artwork.
- Changed active desktop launchers to a fresh `nougat-media-suite-exact-n` icon-theme key to bypass stale GNOME icon-cache identity while also overwriting legacy/current Nougat icon aliases with the same corrected artwork.
- The installer refreshes installed icon-theme assets, desktop entries, GNOME favorites identity, Files/Nautilus metadata, and verifies old rejected blurry icon hashes are absent from every active Nougat icon surface it owns.
- No Search-engine, quilt/background, playback, licensing, Library, Discover, Stream, or diagnostic behavior is changed by this repair.


## v0.0.24 same-version literal concept-sheet N app-wide repair

- Owner rejected the preceding icon result because the in-app left header still visibly showed the old blurry screenshot-derived N and the dock identity was not reliably replaced.
- The active icon family is regenerated from a literal crop of the N emblem in the owner-supplied full-resolution concept sheet; the surrounding sheet canvas and exterior cream/gray sheet shadow are removed to transparent alpha while the emblem pixels remain from the sheet.
- The previous rejected 14/16/32/48/64/128/256/512 icon family is explicitly forbidden by hash.
- `src/nougat_media_suite_icon_data.hpp` is regenerated from the new 14/16/32/64 assets, so the far-left in-app header badge and `_NET_WM_ICON` no longer use the old embedded pixels.
- Every project launcher from unversioned through v22/v23/v24 plus the canonical reverse-DNS launcher uses the fresh cache-busting key `nougat-media-suite-concept-sheet-v24`.
- The installer writes the exact same icon under the fresh key and every legacy/current Nougat icon alias used by GNOME, refreshes the dock favorite binding when present, refreshes desktop/icon caches, and reapplies raw executable custom-icon metadata only after the final v24 executable is written.
- Existing concept-sheet quilt/background tinting and all non-icon behavior remain unchanged.

- v0.0.33 repair: libtorrent 2.0.12 tracker compatibility repair.
- v0.0.33 server-stop repair: persistent Nougat-owned Jellyfin sessions carry a per-session ownership token; Stop Server terminates the complete owned process tree and verifies port 8096 is released without killing Jellyfin by name.
