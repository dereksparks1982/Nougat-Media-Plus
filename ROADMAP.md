## v0.0.62 - Complete Games and Emulator Overhaul

v0.0.62 is the dedicated complete Games overhaul.

- Audit every system currently exposed by Nougat Games.
- Build a complete supported-system emulator matrix with an explicit primary backend and fallback policy for each system.
- Add every emulator runtime required by the supported system matrix rather than leaving placeholder system entries.
- Require every configured emulator backend to pass executable discovery, dependency validation, launch validation, input validation, audio validation, video validation, stop/cleanup validation, and relaunch validation.
- NES and SNES must render inside Nougat's native Video Player instead of opening an unintended detached MesenCE desktop window.
- Xbox 360 must both render inside Nougat and successfully launch GTA IV again.
- Preserve Nougat-owned player containment wherever the system architecture requires embedded gameplay.
- Keep Grid and List views, collapsible console groups, large artwork cards, scrolling Systems navigation, metadata/artwork support, ZIP-aware libraries, persistent ROM folders, controller architecture, and per-system diagnostics.
- Do not treat emulator discovery alone as working support. A system is complete only after real launch and gameplay-path validation.
- Carry emulator failures truthfully in diagnostics instead of silently opening the wrong window or falling back to an unvalidated backend.


## v0.0.61 candidate - Drone Lab Mission Control

- Owner-test the recovered Mission Control UI against the approved Drone Lab reference.
- Owner-test waypoint editing, New Shot, Record Path, Clear Path, Save Shot, and animated Simulate Shot behavior.
- Owner-test telemetry, flight instruments, camera/gimbal simulation controls, and simulation-only payload controls.
- Real-aircraft arming/command transmission remains intentionally disabled.
- After owner acceptance, close v0.0.61 through the normal snapshot, commit, tag, GitHub push, and verification gate.

## v0.0.60 accepted - Drone Production Foundation

- Studio Drone workspace foundation completed.
- MAVLink / MAVSDK / PX4 / ArduPilot integration foundation completed.
- Director Shot model foundation completed.
- Simulation-only telemetry, camera, gimbal, and recorded-path scaffolding completed.
- Approved Nougat N identity repair carried forward permanently.
- v0.0.61 begins from this frozen baseline with deeper Drone simulation and Director Shot development.

## v0.0.59 accepted - Studio File Tools and Player Seek Repair

- Completed live Target MiB / Pieces / Recommended recalculation in File Splitter.
- Completed fixed first-open Splitter column layout.
- Completed separate Studio File Assembler with manifest-or-part discovery, verification, reconstruction, and final integrity checking.
- Completed stable shared seek-thumb rendering across the entire track in windowed and fullscreen playback.
- v0.0.59 is owner accepted; subsequent work begins from this frozen baseline.

## v0.0.53 planned - Rejected v0.0.51 carry-forward and alerts

The complete mandatory carry-forward list is recorded in `docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md`. v0.0.52 is Radio-only. v0.0.53 resumes the deferred File Splitter, HDHomeRun/full-scan, World TV, Games/emulator/artwork, navigation, identity, overlay/process, LAN and related repair work. AMBER Alerts and the broader official public-warning integration are also assigned to v0.0.53.

## v0.0.51 and forward: Controller, Radio, LAN Viewer, and Aerial Production

- **Unified Controller & Remote Input Framework:** app-wide controller navigation, not a drone-only input stack. Controller setup is in the **System tab**. Default vocabulary includes D-pad/left stick navigation, A/Cross Select, B/Circle Back/Cancel, tab/page movement, player seek/play, subtitles/audio and context actions, with remapping, dead zones, sensitivity and test input. Separate UI, Video Player, Games and **Drone Flight** contexts. Drone Flight receives exclusive flight-axis ownership while armed.
- **Radio:** top-level Mulberry Radio area with AM, FM, Shortwave, Weather, DAB/DAB+, DRM, Internet Radio, SDR, Favorites and Recordings. Capability-gated by actual hardware/providers. Reception controls roadmap includes Short Range, Normal and Long Range antenna modes where hardware supports or user metadata makes them meaningful.
- **Regional tuner profiles:** keep Live TV provider-neutral for the home HDHomeRun FLEX DUO and portable Hauppauge/other regional USB tuners, including future Turkey-compatible DVB hardware. Frequency plan and scan behavior belong to provider/region capability data, not WinTV-specific assumptions.
- **LAN Web Viewer:** versioned local catalog/history/artwork/media/Live-TV/device/session/pairing/health/browser contracts, LAN-only default, no cloud-login requirement, no automatic port forwarding, no external relay.
- **Studio Aerial Production:** DJI/open-autopilot ground-control integration, gamepad/PS5 DualSense control, repeatable cinematic flight/gimbal/camera paths, live video/telemetry and synchronized production metadata. Flight controls remain in Aerial Production; System owns controller configuration.

## Studio professional-production direction after v0.0.50

Studio remains Nougat's integrated production environment. After the v0.0.50 File Splitter foundation, the roadmap includes professional video/audio/photo editing, green-screen keying and compositing, animation, VFX, camera/object tracking, rotoscoping, motion capture and retargeting, production asset management, high-resolution rendering, and render-queue/farm workflows. Architecture should remain capable of scaling toward large live-action/VFX feature-film production rather than a consumer-only editor.

Future Studio work also includes **Nougat Studio Aerial Production** for movie shoots: a Nougat-owned computer ground station with live drone-camera ingest and telemetry, DualSense/gamepad plus keyboard/mouse control, aircraft/gimbal/camera mapping, waypoint missions, automated and repeatable cinematic flight/gimbal paths, shot presets, synchronized flight/gimbal/camera metadata, exact-take replay, and production ingest into Studio for stabilization, camera tracking, compositing, and VFX. The provider boundary should support open autopilots such as PX4/ArduPilot and supported proprietary platforms including DJI. Model-specific custom drone software/firmware swaps and deeper DJI integration may be supported after the exact aircraft, controller, flight controller, and firmware combination is verified. Safety-critical flight behavior stays on a verified flight-control layer with manual takeover, link-loss behavior, return-to-home, battery limits, and geofencing available to the production crew.

No Aerial Production implementation or LAN Web Viewer code is part of v0.0.50. v0.0.51 is assigned to remaining emulator support plus the LAN Web Viewer foundation/scaffolding.

## v0.0.46 planned — World TV Repair, Scanner Control, Kaaba Live, and LAN Media Foundation

- Repair World TV's X11/libVLC window ownership so the black video surface can never cover the station list.
- Repair World TV station activation and native playback. Double-click and Watch Live must either begin verified playback or report a truthful source/playback failure.
- Re-verify World TV broadcaster feeds, startup, reconnect, source switching, artwork, and status reporting rather than retaining dead or mislabeled feeds.
- Add the approved Nougat N shortcut: double-click the small N beside the application name to open the verified official live Kaaba / Masjid al-Haram broadcast in Nougat's native player, preferring official Saudi broadcast infrastructure and a verified fallback.
- Add a real Stop Scan control to Virus Scan.
- Scanner cancellation must terminate the complete Nougat-owned scan process group, including capa, ClamAV, and other scanner children. Closing Nougat must not leave orphan scanner processes.
- Preserve useful partial scan results when the owner stops a scan and clearly report that the scan was cancelled.
- Replace the ambiguous live `flagged` count with separate Threats and Suspicious counts.
- Redesign Full System Scan as a tiered bulk scan. Fast checks run broadly; expensive deep analysis such as capa runs only when justified instead of blindly analyzing every ELF/shared library.
- Improve ClamAV bulk scanning efficiency rather than launching heavyweight per-file work across hundreds of thousands of files.
- Repair the top-navigation right-edge scroll extent at narrow window sizes so every tab, including System, can be brought fully into the visible navigation lane.
- Start the LAN Media Service architecture for phones, tablets, laptops, and TV browsers on the same local network.
- LAN local-media access must continue working when WAN/Internet access is unavailable. No cloud login or Internet round trip may be required to play locally owned media across the LAN.
- Establish versioned LAN interfaces for catalog/history, direct media delivery, future HLS/transcoding, authentication/pairing, and the local web UI without moving those implementations into main.cpp.
- Default LAN service exposure to the local network only. Do not automatically expose Nougat through router port forwarding, UPnP, or a cloud relay.
- Plan friendly local discovery such as `nougat.local` while retaining direct local-IP access as a fallback.
- Preserve v0.0.45 Secure Search privacy invariants and all accepted player, Library, Live TV, Stream, Games, and diagnostics behavior.

## v0.0.45 candidate - Secure Search Foundation, Privacy Broker & Crawler Access Architecture

- Split user Search from crawler networking: local Search uses a no-network worker and receives plaintext queries only over private local stdin IPC, never argv, URL query strings, environment variables, ordinary logs, diagnostics, or privacy receipts.
- Remove automatic DuckDuckGo/live-discovery fallback and disable plaintext remote peer Search. Secure remote Search fails closed until the Privacy Broker can provide the required private transport; there is no direct fallback.
- Add versioned `SecureSearchController`, Privacy Policy, Privacy Receipt, Privacy Broker client/protocol, and Crawler Access Manager interfaces so future OHTTP, ODoH/ECH, PIR/HE, mix-network, post-quantum, renderer-containment, and relay-directory providers can be replaced without rewriting Search.
- Split `nougat_search_worker.py` and `nougat_crawler_worker.py`. The administrative node service becomes loopback-only and no longer accepts plaintext Search queries over `/nougat/v1/search`.
- Identify crawling truthfully as `NougatSearchCrawler/0.0.45`, respect robots restrictions, and expose access states for bot-policy block, rate limit, authentication, feeds, payment-required, and temporary unavailability. Nougat never spends automatically on HTTP 402.
- Add the Rust Privacy Broker v1 scaffold as an optional, isolated security component without making Rust a hard dependency of the v0.0.45 C++ application build.
- Keep Secure Search implementation outside `src/main.cpp`; main receives only controller wiring and truthful Secure Search loading/status text.
- Candidate remains uncommitted/untagged/unpushed until owner acceptance.

## v0.0.46 planned - World TV Repair, Verified Broadcasts & Kaaba Live

- Make World TV the primary v0.0.46 repair/polish focus. Fix the X11/libVLC black video-child overlay that can cover the station list when the World TV surface is active, and validate clean player/list window mapping during every view transition.
- Harden native broadcaster playback, reconnect/failover behavior, stream-health reporting, source verification, station selection, scrolling, status text, and correct station/network artwork so World TV behaves like a finished Nougat surface rather than an experimental catalog.
- Double-clicking the small approved Nougat `N` icon beside the application name opens Kaaba Live immediately in Nougat's native player.
- Prefer the official Saudi Quran TV / Saudi Broadcasting Authority / ALOULA Masjid al-Haram live broadcast path for Kaaba Live. Verify the actual native stream endpoint, quality, redirects, uptime, and libVLC behavior before shipping; use only verified official distribution/fallback paths rather than mystery IPTV mirrors.
- Keep a future Madinah / Saudi Sunnah TV companion path available for later owner approval without complicating the default N double-click action.
- Candidate work begins only after v0.0.45 is owner-accepted.

## v0.0.44 candidate - Direct World TV, Playable Games, Artwork, and Card Context Menus

- World TV is a top-level tab immediately after Live TV and uses direct non-YouTube international linear television feeds only.
- Direct World TV playback is capped at 1080p and gains faster startup/reconnect handling.
- Games installs pinned MesenCE 2.2.1, RMG 0.9.0, and Atari800 7.1.2 runtimes so bundled legal NES ROMs, compatible Nintendo ROMs including Nintendo 64, and Atari 5200/8-bit ROMs can launch without separate emulator installation.
- Games reuses the Media Library multi-row grid and scrollbar layout exactly, with visible-card artwork retrieval/caching and verified bundled-test artwork.
- Every Nougat card surface gains a relevant right-click context menu; local media/game cards include Open Source without deleting user files.
- Candidate remains uncommitted/untagged/unpushed until owner acceptance.

## v0.0.43 candidate — Games, World TV, and Responsive Grid Repair

- Build directly on accepted v0.0.42 commit `819ee82098f6c4c72269c4f07b3f899f9b0184cf`.
- Fix the carried Movies/TV responsive-grid one-row collapse at its geometry source.
- Finish the Games library lane with persistent linked folders, ZIP-contained ROM discovery, Grid/List views, system/source badges, sidecar artwork, double-click launch, and automatic selection among supported installed emulator backends.
- Add Atari 2600/5200/7800/8-bit/Lynx file recognition and backend discovery alongside the existing Nintendo systems.
- Keep legally redistributable bundled NES starter content/license records intact. Do not add copyrighted commercial ROMs.
- Expand World TV through broadcaster-owned internet feeds without using device location to construct or filter the catalog. Include Al Jazeera Arabic.
- Repair the Discover > My Services scrollbar hit geometry and reserve its own visual gutter so provider names do not collide with it.
- Keep the exact approved Nougat N icon everywhere with no exceptions.
- Candidate remains uncommitted/untagged/unpushed until owner acceptance.

## v0.0.34 accepted — Exact Sheet Tabs/Player Controls + Home/Discover UI Repair
- Exact concept-sheet top tabs, seek bar, and volume component.
- Left-shifted scrollable nav lane with right Server/version side frozen.
- Fixed Home card templates and direct scrollbar dragging.
- Live TV header overlap repair.
- Discover Live TV source and TMDb naming.
- Affected page-frame corner repair; Search and Video Player frame behavior preserved.
- Known v0.0.34 issues carried into v0.0.35: seek/volume sheet-fidelity mismatch and validation that could pass the invented geometry instead of proving the approved component.

# Nougat Media Suite Roadmap

## v0.0.41 candidate — Housekeeping, Archives, IMDb, Live TV and Player Activity Repair
- Add Search's `Archive` tab beside Network with direct browser links to the curated preservation/library directory, including Archive.org and Minerva Archive.
- Restore the shared Movies/TV Library card geometry so height changes cannot shrink cards out of alignment.
- Add exact IMDb links to Movies, TV series/episodes, and Home cards when Jellyfin exposes a verified IMDb ProviderId; hide the link when no exact IMDb title ID is known.
- Distribute Search-result card height across the available scroll viewport so a scrollable result page does not leave the cream bottom strip.
- Add `Stop Live` beside `Watch Live`; stopping releases the tuner and immediately permits a queued full guide sweep.
- Repair manual Live TV guide refresh state so an idle refresh starts as a fresh full sweep.
- Make the on-video media/episode description use the same three-second activity timer as the mouse pointer in fullscreen, maximized, and normal window sizes.
- Add release-integrity checks that keep version, executable, README, CHANGELOG, ROADMAP, launcher, and validation records aligned.

## v0.0.42 candidate — Persistence, Intelligence, Live TV, Security, and Games
- Preserve Movies/TV library mappings across build replacement and recover Jellyfin virtual-folder links from Nougat-owned persistent state.
- Improve metadata identification and recommendation performance with deterministic structure evidence, provider-ID matching, and reusable embedding caches.
- Repair Discover Live TV, My Services, responsive Home/Library card geometry, player Live TV-to-local handoff, and subtitle context-menu usability.
- Persist/maintain the ATSC guide automatically when the tuner is idle and add the owner-approved World TV internet-broadcaster surface.
- Expand Virus Scan with library, quick, and system scan modes.
- Add the top-level Games tab after Studio with persistent user ROM folders, legal bundled starter content, backend discovery, controller visibility, and real emulator launch.

## v0.0.43 planned — Home LAN Web Viewer / Streaming
- Add an opt-in LAN-only Nougat web service for phones and other browsers connected to the same home network.
- Provide mobile-friendly Home, Movies, TV, Continue Watching, artwork, and exact resume state from Nougat's existing catalog/history rather than a second media database.
- Start with direct browser-compatible streaming and establish an HLS/transcoding path for formats a phone browser cannot play directly.
- Add local pairing/PIN protection, clear server status/address controls, and no cloud-account requirement.
- Keep the first milestone focused on reliable phone-browser playback over the home LAN; Internet-facing remote access is not part of the first build.

## v0.0.39 candidate — Deep Diagnostics + Live TV Guide Reliability
- Rebuild Diagnostic Center around subsystem truth: Passed, Needs Attention, Problems, Not Tested, and Information.
- Add deep runtime, Library, server, player, Live TV/tuner, Search, P2P, Stream, AI/TMDb, storage/system evidence with observed/expected/action detail and retained exports/history.
- Use recursive Library diagnostic queries and stop treating missing metadata/posters or an idle Search node as suite-wide faults.
- Auto-load cached Live TV guide data, preserve/merge valid PSIP events, collect longer VCT/MGT/EIT cycles, and improve coverage across stored channels.
- Harvest current-multiplex PSIP through demux0 during playback; queue other-RF refresh work until the single DVB frontend is idle.
- Expand real bundled channel-logo mapping and use square logo cards.
- Repair only the explicitly carried visual defects: progress-bar pale bottom strip/stitch clarity, black Live TV timing text, and the green/yellow/red stitched Server status indicator.
- Preserve approved v0.0.38 positions, VOLUME control, player geometry, and unrelated artwork.


## v0.0.35 accepted — Code + Bug Cleanup, UI Alignment, Live TV Scan, and Studio Foundation
- Make v0.0.35 a stabilization-first release: repair confirmed bugs from the project-wide code review, reduce brittle/duplicated code where it directly raises regression risk, harden ownership/lifetime/process cleanup, and preserve accepted v0.0.34 behavior outside the approved repair scope.
- Replace false-positive source-token/constant checks with stronger behavioral validation where practical, including the approved concept-sheet seek/volume controls.
- Repair the Video Player seek bar and housed volume control against the approved component sheet's actual geometry and visual hierarchy rather than an approximation.
- Keep top-level page/background frames square while retaining rounded inner controls/panels; specifically remove the rounded outer Search page frame.
- Use Stream's current top-inner-control vertical position as the alignment reference. Move Search, Debug, and Live TV top-inner control rows to that same baseline without moving the global top navigation.
- Re-align the full app around the enlarged global tab/header geometry: extend the same Stream-derived baseline to Discover and Library, keep Library tools in one scrollable row with List/Grid at far right, recenter the full-width Video Player transport group, and preserve full horizontal reach in narrow layouts.
- On Live TV, keep the LIVE TV heading, remove the hardware-description subtitle, and align the action-button row to the Stream baseline.
- Advance the Linux DVB Live TV foundation from detection into a real owner-testable channel scan: enumerate the detected frontend, scan appropriate broadcast frequencies, expose tuning/lock/signal/progress evidence, discover services/channels, populate the channel list, and persist discovered channels for later Watch Live use. Fail honestly when lock/scanning is unavailable.
- Add a new top-level Studio tab between Stream and Debug. Give Studio a true yellow/gold palette with brown stitched borders; the page is branded Gold Studio internally. v0.0.35 establishes the navigation/page foundation only; the major media-processing/editor toolset remains roadmap work so stabilization is not displaced.

## v0.0.36 candidate — Library Hierarchy, Home Artwork, and Exact-Sheet Player/Header Repair
- Collapse recognized movie collections/franchises into a single top-level Library card by default. Member movies are hidden from the root while the collection remains directly navigable; standalone movies remain normal root items.
- Add the approved-sheet `Search` input directly below the Library green action row with the exact placeholder `Search` and live local filtering. Keep List/Grid together at the far right of the action row.
- Repair Home card artwork placement so Continue Watching uses centered proportional 16:9 cover artwork and Local/Recommended movie cards fill their portrait poster viewports without the current black-gap/offset defect.
- Replace the generic player seek rendering with a pixel-derived approved-sheet seek sprite family. Shorten/keep the bar at its sheet-derived width and place elapsed/total time on its left/right sides on the same line.
- Repair mouse-driven player clipping at the repaint root by making seek/time, VOLUME/percentage, and transport buttons one stable control repaint region. Preserve the accepted VOLUME component itself and remove only the rectangular sheet background around the rounded housing.
- Change the global header to the same tan sampled from the approved VOLUME housing, center both header clusters vertically, and replace the generic Server dot with the sheet-family circular status indicator.
- Preserve the current app lettering until a separately approved system-wide typography pass.
- Preserve v0.0.35's owner-validated ATSC 1.0 scan behavior unchanged except for regression checks.

## v0.0.38 candidate — Library + Live TV + Player Exact-Sheet Polish

- Finish Library Search visible typing/caret behavior and title-first card details without moving the approved Library layout.
- Keep the system-wide loading bar in its original top lane; use the literal approved-sheet progress component, sized to contain the percent, with the percent inside the caramel fill.
- Keep the approved seek/VOLUME art unchanged except for removing the specifically rejected pale underside crop-shadows; VOLUME percent remains black and its current geometry stays untouched.
- Unify pointer and media/program information under a single three-second player activity timer.
- Keep Guide as the sole Live TV guide-view control and the default Live TV surface; remove only the redundant Channels control.
- Move only Detect Tuner and Refresh Tuner to System.
- Use real local broadcaster/network logo assets where identified; use call-sign text when no matching logo is known.
- Add visible approved-sheet-style vertical scrollbars to existing vertically scrollable pages/panels while preserving mouse-wheel behavior.
- Retain remembered/keyboard Live TV channel selection, PSIP current-program identity, and aligned program timing.
- Characterize the already-working VLC pause buffer before any dedicated DVR/timeshift pipeline is considered.
- Make installer baseline validation fail-closed.
- Keep the Diagnostic Center interpretation redesign for v0.0.39 rather than mixing it into this repair candidate.

## v0.0.37 candidate — Native Live TV Watch + Classic Guide
- Make `Watch Live` on a persisted ATSC channel such as 3.1 tune the stored DVB frontend/frequency, select the correct MPEG transport-stream program, and feed the live broadcast into Nougat's existing native player. Double-clicking a selected channel uses the same path.
- Expose one logical tuner per independently usable DVB frontend instead of showing sibling V4L2/VBI device nodes as separate tuners.
- Add explicit tuner ownership state so channel scanning, guide harvesting, and live playback cannot compete for the same tuner.
- Harvest/cache ATSC PSIP EIT guide data and present the first old-school guide grid with channels down the left, time across the top, duration-sized program cells, current-time marker, current-program highlight, `Refresh Guide`, and `Now`.
- Keep ETT-rich descriptions, XMLTV supplementation, DVR/recording, timeshift, favorites, and multi-tuner scheduling as later Live TV stages after owner validation of v0.0.37 tuning and EIT grid behavior.
- Support clean Stop, retune, and channel changes without destabilizing the accepted v0.0.35 channel scan/persistence path.
- Validate first on the owner's already working tuner/channel database before expanding into guide, recording, timeshift, favorites, or channel-surfing features.

## Games / Unified Emulation — promoted into v0.0.42 by owner direction
- Add a top-level `Games` tab immediately after `Studio`. Games is a Nougat media category, not a separate application.
- Present one unified Nougat game-library/emulator experience while allowing different mature emulator cores/backends underneath per system.
- Target the broad practical emulation range available on Linux at implementation time: Nintendo, Sega, PlayStation, Xbox where mature emulation exists, Atari, arcade/MAME, classic computers/handhelds, and other supported systems.
- Provide unified library artwork, system filters/search, controller setup, fullscreen launch, saves, save states, recent games, favorites, play history, and per-game backend overrides when needed.
- Users supply their own game dumps/ROMs/ISOs and any legally required BIOS/firmware. Nougat does not distribute copyrighted commercial ROM sets.
- Re-verify and preserve upstream licensing before bundling any redistributable test content. Candidate playable test: `2048-nes` (Unlicense/public-domain-style). Candidate NES diagnostic ROM: `NES Waveforms` (MIT).
- This work begins only after v0.0.39 is repaired and owner-accepted.

## Studio / Nougat Media Processing Engine — post-v0.0.35
- Build one reusable native media-processing engine around FFmpeg/libav rather than reinventing codecs. Share it across Convert, Audio Lab, Quick Edit, Batch, and the eventual timeline editor.
- Core engine: persistent job queue, progress/ETA/cancel, logs, recoverable jobs, safe temporary/output handling, presets, advanced codec/container controls, and hardware acceleration where available. Prefer lossless remuxing when streams already fit the destination container.
- Video conversion: MP4, MKV, WebM, MOV, AVI and other supported containers/codecs; resolution/FPS/bitrate/quality conversion; target-file-size compression; crop, resize, rotate, flip, trim, split, join/merge, still-frame extraction, thumbnails/contact sheets, GIF/WebP creation.
- Subtitle/chapter/metadata tools: extract, convert, embed, remove, burn-in, chapter editing, metadata/tag editing, and artwork handling.
- Audio Lab: MP3, FLAC, AAC, Opus, WAV, M4A, OGG and supported formats; extract audio from video; trim/join; split by timestamp, chapter, silence, or equal pieces; fade; sample-rate/bit-depth/bitrate conversion; loudness analysis/normalization; stereo/mono conversion; channel splitting/recombination; tags and album art.
- Quick Edit: mark in/out, trim, crop, rotate, resize, join, replace/extract audio, subtitles, and fast export without opening the full timeline.
- Batch: run reusable recipes across large file sets with naming templates, output rules, retry/skip behavior, collision handling, and queue persistence.
- Full Studio timeline: project files, autosave/recovery, undo/redo, non-destructive editing, multiple video/audio tracks, razor/cut, ripple editing, trim/move/slip, snapping, markers, timeline zoom, transitions, fades, titles/text, overlays, picture-in-picture, opacity/compositing, speed changes, keyframes, filters/effects, color controls/scopes, audio mixer/waveforms/gain/pan/mute/solo, proxy media, render cache, source/program preview, and serious export controls.
- Suite integration: Library → Convert/Quick Edit/Studio; Video Player → edit current media or create clip; legitimately obtained Stream downloads → processing; Live TV/DVR recordings → Studio; completed Studio exports → optional automatic Library import.
- Media import: investigate lawful DVD/Blu-ray media extraction/import paths without bypassing DRM or access controls.

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

## v0.0.31 accepted — Exact Approved UI Sheet Components
- Treat the owner-approved Nougat component sheet as the literal control-shape authority rather than a loose visual reference.
- Preserve every accepted v0.0.30 page/background palette exactly: purple Home, cocoa Video Player, green Library, red Discover, cream Search, provider-reactive Stream, and charcoal Debug.
- Replace ordinary buttons that still look like flat rounded rectangles with the sheet's layered raised control construction: lower shadow, dark outer rim, bright inner bevel, stitched/inset seam, top highlight, and pressed/disabled states.
- Replace top-level tabs plus Search/Discover/Stream selector tabs with the same sheet control family and the integrated downward selected point/notch.
- Replace text/input frames, large panels, compact icon buttons, watch-service checkboxes, progress surfaces, seek track, volume housing/track, and slider knobs with the sheet component geometry while retaining their existing page colors and behavior.
- Keep Home card proportions/artwork and all v0.0.30 media behavior unchanged; this release is UI-component fidelity only.
- Preserve licensing, Search/P2P transport, Jellyfin/catalog behavior, Discover logic, Stream playback, diagnostics, TV autoplay, metadata cache, and user data paths unchanged.

## v0.0.32 candidate — Native P2P Media + Nougat Security Analysis
- Keep the existing v0.0.32 native P2P work: magnets, local `.torrent` files, media selection, `Watch Now`, pause/resume/remove, localhost HTTP Range playback, selected-media progress, start-buffer evidence, playback-aware priority windows, and Search-to-P2P handoff.
- Make completed torrents clearly report whether the local client is a complete seed, whether it is available/idle/uploading/paused, remote/known seed and peer evidence, and swarm availability where libtorrent exposes it.
- Add `Virus Scan` beside `Search | Crawler | P2P` as a general manual file/folder scanner.
- Add the one-shot Nougat Security Analysis scaffold with SHA-256/type checks, scan history, WARN-ME-FIRST behavior, optional external one-shot `clamscan`, community-telemetry hooks, and pinned integration points for YARA-X/capa/Magika. Full engine-runtime hardening follows after the scaffold is proven.
- Hash scanned files with SHA-256, detect real content type, flag executable/media extension mismatches and suspicious double extensions, keep recent scan history, and allow Scan Again.
- Support free/community MalwareBazaar and ThreatFox reputation lookups when the owner supplies a free abuse.ch Auth-Key; local scanning remains fully usable without a key.
- Enforce **WARN ME FIRST**: Nougat reports evidence and never automatically quarantines, deletes, moves, renames, or opens a suspicious file. Scanner workers terminate after each manual/automatic scan.
- Automatically run the same one-shot analysis when a selected P2P download reaches complete/seeding state; do not install a filesystem watcher.
- Remove the stray ordinary Search `Node <id>` text while keeping the Node ID in Network/Advanced. Keep the Crawler layout unchanged and move only `Ready. Crawl a site or add a peer, then search.` upward so it sits correctly between the controls and results panel.
- Retain the already-built autoplay no-flash repair, contained 0–200% volume geometry, brown Search stitching, and bottom-only Stream provider panel accent across YouTube, Vimeo, Rumble, RuTube, VK, and OK.
- Preserve accepted v0.0.31 Home cards, page palettes, UI-sheet component family, Library/Discover/Stream behavior, licensing, Jellyfin ownership, and user-data boundaries outside this scope.

- Add a dedicated right-side Home vertical scrollbar plus a sheet-style horizontal Continue Watching scrollbar; clip all Home scrolling content below the fixed top header.

## v0.0.33 candidate — P2P Plus + Security Hardening + Persistent Server + Live TV Foundation
- Add the first mature P2P Plus management layer: speed limits, seed ratio/time rules, file priorities, queue movement, tracker visibility, Force Reannounce and Force Recheck. More advanced multi-transfer scheduling, peer/piece maps, RSS and remote control remain later work.
- Harden Virus Scan with a pinned one-shot YARA-X/capa/Magika runtime and truthful ANALYSIS INCOMPLETE semantics; retain optional external clamscan and free abuse.ch telemetry only.
- Add system-wide page viewport/border containment excluding Video Player, plus Home/Library scrollbar and top-navigation clipping repairs.
- Make the Nougat-owned media server persist after the desktop UI closes until Stop Server is explicitly used.
- Add Live TV as a top-level tab between Discover and Search, with Linux DVB/V4L2 discovery and first-hardware targeting for the Hauppauge WinTV-HVR-955Q. Real channel tuning/playback follows after owner hardware probing.
- Keep future HDHomeRun, ATSC 3.0/NextGen TV, Radio/SDR and CB reception behind replaceable Nougat-owned backend interfaces.

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

## Studio / Media Processing

Build Nougat into a complete native media workshop, not only a player/library. The Studio family should be powered by one reusable **Nougat Media Processing Engine** around FFmpeg/libav and related proven native libraries rather than reimplementing codecs.

Core processing engine:

- Shared job model for convert, remux, edit, audio, batch, and final Studio renders.
- Queueing, pause/cancel where technically safe, progress/ETA, logs, failure recovery, output-conflict handling, and persistent job history.
- Hardware-acceleration discovery/use where supported, with honest software fallback.
- Safe temporary-file handling, crash-safe output writes, overwrite protection, and explicit source/output paths.
- Reusable export presets plus an Advanced mode exposing codec/container/bitrate/resolution/frame-rate/audio/subtitle controls.
- Fast lossless remux/copy paths when re-encoding is unnecessary.

Video and container tools:

- Convert/transcode among common containers and formats including MP4, MKV, WebM, MOV, AVI, and other formats supported by the bundled media stack.
- Remux between compatible containers without re-encoding.
- Trim/cut, split, join/merge, crop, resize, rotate, and flip.
- Frame-rate, resolution, codec, bitrate, quality, and target-file-size conversion.
- Extract clips and still frames; create thumbnails/contact sheets.
- Create GIF/WebP animations from video clips.
- Subtitle extraction, conversion, embedding, removal, language/default-track selection, and burn-in rendering.
- Chapter viewing/editing/import/export.
- Media metadata/tag viewing and editing.
- Legally accessible DVD/Blu-ray media extraction/import where the source can be read through supported lawful paths.

Audio Lab:

- Convert among MP3, FLAC, AAC, Opus, WAV, M4A, OGG, and other supported audio formats.
- Extract audio from video without unnecessary video processing.
- Split audio by timestamps, chapters, detected silence, cue information, or equal-length pieces.
- Join/merge audio, trim clips, fade in/out, and change sample rate/bit depth/bitrate where supported.
- Loudness analysis and normalization.
- Stereo/mono conversion plus left/right or multichannel stem/channel splitting and recombination.
- Metadata/tag editing and album-art handling where the format supports it.

Quick Edit:

- A fast non-timeline workspace for the common jobs: mark in/out, trim, crop, rotate, resize, join, audio replacement/extraction, subtitle work, and export.
- Allow direct handoff from Video Player, Library, Stream acquisitions, and Live TV recordings into Quick Edit without manually re-browsing for the source file.

Batch:

- Drop many files into a queue and apply one conversion/edit recipe to all of them.
- Per-item status, retry/skip, output naming templates, destination rules, and collision handling.
- Save reusable batch recipes/presets.

Full Studio timeline editor:

- Native project files with autosave/recovery, undo/redo, and non-destructive editing.
- Multiple video and audio tracks.
- Razor/cut, ripple-style editing, clip move/trim/slip behavior, snapping, markers, and timeline zoom.
- Transitions, fades, titles/text, overlays, picture-in-picture, opacity/compositing, and speed changes.
- Keyframes for transform/effect/audio parameters.
- Video filters/effects and basic color controls, with room for scopes/color work as the editor matures.
- Audio mixing, gain/pan, fades, waveform display, mute/solo, normalization, and track-level effects where supported.
- Proxy media, render/cache strategy, and responsive preview for difficult source media.
- Fullscreen/program preview plus source preview.
- Export presets and advanced render settings using the shared media-processing engine.

Suite integration:

- Library item -> Convert / Quick Edit / Open in Studio.
- Video Player -> edit the current file or create a clip from the current position.
- Stream -> send a legitimately obtained local media file into Studio/Convert.
- Live TV/DVR -> edit or convert recordings directly.
- Studio/Convert output -> optionally add or refresh the result in Library.

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

## Complete Media Suite / Nero Capability Program

The long-term classic/current/future Nero capability-parity program and Nougat expansion roadmap is maintained in:

`docs/COMPLETE_MEDIA_SUITE_ROADMAP.md`

This is roadmap documentation only. Individual implementation still requires explicit owner build approval.
