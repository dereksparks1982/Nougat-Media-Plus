# Nougat Media Suite v0.0.30 — UI Cohesion, Library Performance, and Player Navigation — Build Handshake

## Project / release identity
- Project: Nougat Media Suite
- Company: Elderred Softworks LLC
- Required accepted base: v0.0.29
- Required base commit: `91ced5349a71de34cbc7d3d95d32c4de56645c7b`
- Target version: v0.0.30
- Target root executable: `Nougat_Media_Suite_v30`
- Changed-files package: `Nougat_Media_Suite_v0_0_30_UI_LIBRARY_PLAYER_POLISH_CHANGED_FILES_ONLY.zip`
- Candidate status: unaccepted until owner real-machine testing and explicit approval

## Approved v0.0.30 work
1. Apply Search's rounded/inset primary-panel treatment to equivalent large Library, Discover, Stream, and Debug content surfaces while retaining each page palette.
2. Repair selected top-tab pointer clipping by keeping the gold activity/progress strip below the complete navigation notch.
3. Use proper portrait 2:3 DVD/poster geometry on Home for movies, series, and seasons; retain landscape geometry for episode stills.
4. Make Library Grid use available vertical height so multiple portrait poster rows are visible at once and scrolling moves naturally through the full wall.
5. Add a private persistent cache-first Library metadata path. Cached known views may paint immediately while Nougat reconciles the live Jellyfin view in the background; verified cached artwork metadata may avoid unnecessary repeat enrichment.
6. Show a numeric percentage beside the existing gold activity bar only when real completed/total work is available. Jellyfin scan/server phases without measurable totals remain indeterminate.
7. Clarify refresh semantics: Refresh Server refreshes server/process state; Refresh Library requests the Jellyfin library scan and reloads/enriches library metadata.
8. Add `Previous` and `Next` player controls using the accepted v0.0.29 episode queue/resolver. Keep them distinct from `Rewind 10s` and `Fast Forward 10s`, and disable unavailable episode directions.
9. Enlarge the compact volume knob so it fits its existing rounded housing proportionally while preserving 0–200% volume behavior and the existing readout/colors.
10. Move the focused P2P streaming expansion intact to v0.0.31.

## Changed implementation surfaces
- `src/main.cpp`
- `src/media_server/library_metadata_cache.hpp` (new)
- `src/media_server/library_metadata_cache.cpp` (new)
- `CMakeLists.txt`
- current/unversioned and compatibility desktop launchers through `NougatMediaSuite_v30.desktop`
- `README.md`, `ROADMAP.md`, `CHANGELOG.md`, `APPLY_COMMAND.txt`
- v0.0.30 installer, manifest, retained-v29 test, v30 contract test, X11 smoke test, installer/rollback test
- this handshake and the v0.0.30 validation record

## Protected boundaries
The v0.0.30 package preserves unchanged:
- PolyForm Noncommercial owner-controlled licensing files and third-party license separation
- Nougat Search engine / bridge
- current P2P implementation and P2P stream server
- diagnostics engine
- accepted v0.0.29 TV Up Next/autoplay/Back-to-Series resolver behavior
- accepted Stream provider set and native playback paths
- approved N icon artwork and application identity

## Primary risks
- Cache-first presentation can expose stale metadata if identity/invalidation is wrong. Cache keys are view-specific and live Jellyfin results replace cached state; the cache stores no credentials and is private to the user.
- Grid geometry changes can regress hit testing or scrolling. The executable self-test verifies at least two visible portrait rows in a realistic tall Library viewport.
- A numeric progress display can become dishonest if used during server-side work with no known total. Determinate mode is enabled only after Nougat has a real item count.
- Previous/Next must never masquerade as ordinary seek. Episode navigation uses the v0.0.29 queue/resolver and is disabled at boundaries.
- Busy-bar/notch repair changes paint geometry; X11 smoke plus owner visual testing verifies the top navigation remains intact.

## Rollback point
- Exact accepted base: v0.0.29 commit `91ced5349a71de34cbc7d3d95d32c4de56645c7b`
- Installer creates a rollback snapshot before modifying the project and restores the accepted touched state on validation failure.
- Existing accepted root executable `Nougat_Media_Suite_v29` is retained until the final v30 executable has passed validation and is ready to replace it.

## Validation plan
- exact Git/base/hash manifest gate before changes
- protected licensing/Search/P2P checks
- retained-v0.0.29 behavior test
- v0.0.30 source/runtime contract test
- executable v0.0.30 UI/Library/player self-test, including cache round-trip and multi-row geometry
- warnings-as-errors stub build
- X11 application identity/UI smoke
- full native libtorrent build on owner machine, unchanged implementation but native link preserved
- real llama.cpp/Nomic model load on owner machine
- `$ORIGIN` relative AI runtime verification
- final root executable/version checks
- approved N icon metadata applied after final executable write and read back
- owner real-use validation of rounded panels, Library load behavior/grid, progress display, Previous/Next, volume knob, and tab pointers

## Owner acceptance checks
- Confirm large Library/Discover/Stream/Debug content panels match Search's rounded family.
- Trigger busy work on Home/Library/Debug and confirm the selected downward tab point remains fully visible.
- Confirm Home movies use portrait DVD/poster cards without stretching; episode stills remain landscape.
- In Library fullscreen/tall mode, confirm multiple poster rows are simultaneously visible before scrolling.
- Return to a previously loaded Library view and confirm cached metadata appears promptly while live reconciliation continues.
- Run Refresh Library and confirm unmeasurable scan work is indeterminate, then real item/artwork phases show a truthful percentage.
- Run Refresh Server and confirm it is identified as server status work rather than metadata scanning.
- During episodic playback, confirm `Previous` / `Next` navigate episodes and disable appropriately at boundaries.
- Confirm the larger volume knob fits the compact housing while 0–200% behavior/readout remains correct.
- Confirm accepted v0.0.29 behavior remains intact.

## Continuation point
If the owner accepts v0.0.30: create accepted snapshot, local Git commit/tag, then perform separate SSH GitHub push/remote verification. The next focused release is v0.0.31 P2P streaming expansion.
