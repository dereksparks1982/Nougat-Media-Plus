# Nougat Media Plus: v0.0.69 Same-Version Roadmap Gap Review

**Reviewed:** 2026-10-30  
**Status:** PLANNING / VERIFICATION ONLY, not an accepted release  
**Project version:** v0.0.69 (unchanged by this review)  
**Sources of planned scope:** [ROADMAP.md](../../ROADMAP.md), [Complete Media Suite Roadmap](../COMPLETE_MEDIA_SUITE_ROADMAP.md)

This review prioritizes unresolved work against the current repository. A committed source file does not establish a working feature. Never claim acceptance or parity without real hardware, runtime, or end-to-end evidence. Do not use GitHub Actions; run targeted checks manually on the internal development drive. The external `3T EXT` drive remains out of scope.

## Priority 0: First-run notice acceptance (implementation present, release validation outstanding)

- **Source present:** `src/legal_notice.cpp`, `src/main.cpp`, `legal-notice.js`, `components/web_player/legal-notice.js`; a notice version receipt is stored locally.
- **Requested refusal behavior:** initially unchecked acknowledgement; first Reject shows "You must accept the Terms of Service"; second Reject/Exit closes the native GUI, while the browser attempts to close or blanks its interface.
- **Required runtime tests:** new-profile native launch, consent disabled until checkbox is selected, first refusal followed by acceptance, second refusal closes, Escape/window-close path, failure to save the receipt, restart with existing receipt, changed notice version.
- **Web tests:** fresh browser profile on both published and component viewers, narrow/mobile layout, keyboard-only use, refusal with app access blocked, browser restrictions on script-close, stale service-worker caches, offline/reconnect, and receipt persistence.
- **Hard limitation:** a public static browser client's local acknowledgement cannot be made impossible to bypass by users who modify or disable client-side code. If strict enforcement must cover HTTP media/API access, add a server-verified authenticated session acceptance gate and test it against direct URL/API requests. Do not present the current client-side UI gate as server-side authorization.
- **Status:** SOURCE COMMITTED, native binary rebuild and end-to-end runtime proof still outstanding. Do not advance version automatically.

## Priority 1: LAN media viewer and playback reliability

- **Source present:** `src/lan/lan_media_service.cpp`, `src/lan/lan_viewer_service.cpp`, `src/lan/web_player_daemon.cpp`, root `index.html` and `app.js`, `components/web_player/`.
- **Must verify:** one local host streams to browser on phone, tablet and desktop; media metadata, posters, episode selection, video startup, seeking/range responses, supported container formats, and truthful errors.
- **Operational cases:** LAN works without WAN, no automatic router port-forwarding, stable same-LAN pairing/auth, no empty catalog after refresh, no duplicate media requests, and clean reconnect after host restarts.
- **Status:** CODE EXISTS, end-to-end acceptance NOT VERIFIED by this repository review.

## Priority 1: Games / emulator correctness

- **Source present:** `src/games/emulator_host.cpp`, `emulator_registry.cpp`, `src/console/runtime_manager.cpp`, and `components/games/`.
- **Next step:** audit exposed systems against an explicit emulator backend matrix: install/discovery, dependencies, launch, controller input, video/audio, app-owned display containment, stopping, cleanup and relaunch.
- **Known roadmap focus:** NES/SNES must stay embedded in Nougat's player; Xbox 360/GTA IV startup must be verified on real runtime. Do not count a backend as playable solely because its executable is detected.
- **Status:** INTEGRATION COMPONENTS EXIST, system-by-system acceptance NOT VERIFIED.

## Priority 2: Shared media-processing engine before Studio expansion

- Build or validate a reusable FFmpeg/libav job foundation before additional Studio tools: safe paths, output collision protection, temporary-file recovery, remux fast path, progress/error logs, cancellation, a persistent queue, and presets.
- Prioritize convert/remux/audio extraction and batch jobs before a full multitrack video editor, DAW or disc-authoring pipeline.
- **Status:** ROADMAP / ENGINE IMPLEMENTATION TO BE AUDITED. Do not infer parity with Nero from roadmap descriptions.

## Priority 2: Radio, Live TV and World TV verification

- Hardware-specific end-to-end checks for USB and network tuners, HDHomeRun, available FM/AM/SDR paths, guide refresh, broadcaster stream availability, and native player-window ownership.
- Confirm that scanner cancel closes all scan children and maintains honest threat/suspicious counts.
- **Status:** PROVIDER AND UI SCAFFOLDING EXISTS, hardware/stream validation still needed.

## Priority 3: Future feature lanes requiring owner scope approval

Nougat Weather (radar, satellite and alerts), multi-calendar views (Gregorian, Hijri, Hebrew and liturgical calendars), professional music-production DAW, Plex-class integration, P2P extensions, Linux one-click distribution, backup/recovery, and long-term optical-media production. These remain roadmap work until explicitly prioritized for implementation.

## Repository and release discipline

1. Keep Nougat at **v0.0.69** unless the owner authorizes another version.
2. Do not move or replace owner-approved UI geometry. Validate runtime behavior before claiming completion.
3. Do not run GitHub Actions. No build artifacts or downloads are prerequisites to source-only GitHub updates.
4. Verify the current `CMakeLists.txt` naming/version values against the owner-accepted executable before doing any build-system alignment; never silently rename the executable or advance the version.
5. Update README intro, roadmap and changelog with the truth of each change. Source commit alone does not mean owner acceptance, tagging or release.
