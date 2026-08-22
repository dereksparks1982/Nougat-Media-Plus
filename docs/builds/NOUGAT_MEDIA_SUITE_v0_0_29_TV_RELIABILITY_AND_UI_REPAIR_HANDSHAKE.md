# Nougat Media Suite v0.0.29 — TV Playback, Navigation, and Carry-Forward UI Repair — Build Handshake

## Project / release identity
- Project: Nougat Media Suite
- Company: Elderred Softworks LLC
- Required accepted base: v0.0.28
- Required base commit: `80b12f2b47959f4482e65238b9c0b6033f9785a2`
- Target version: v0.0.29
- Target root executable: `Nougat_Media_Suite_v29`
- Changed-files package: `Nougat_Media_Suite_v0_0_29_TV_RELIABILITY_AND_UI_REPAIR_CHANGED_FILES_ONLY.zip`
- Candidate status: unaccepted until owner real-machine testing and explicit approval

## Approved v0.0.29 work
1. Make TV Up Next independent of how the current local episode was opened. Home, Library, Open File/file picker, and resume playback all use the current file path as an input to the resolver.
2. Search the current episode folder for later playable episodes. Prefer parsed episode identity such as `S01E13 -> S01E14` or `1x13 -> 1x14`; use natural filename ordering only as a bounded fallback for confirmed episode/catalog contexts.
3. Resolve and cache the next local episode before end-of-media so the EOF transition does not wait on a late folder/metadata scan.
4. Restore the Up Next overlay with the actual resolved episode, a visible 10-second countdown, `Play Next`, `Back to Series`, and `Replay`, followed by automatic playback at zero unless the owner acts.
5. Manual Stop cancels pending TV autoplay. Starting the next episode uses a bounded retry rather than an unbounded loop.
6. `Back to Series` returns to the actual series/season context when resolvable; generic TV Library is only a truthful fallback when the owning series cannot be resolved.
7. Repair Home wheel routing so the top navigation strip scrolls horizontally even while Home is selected. Continue Watching and Home-page scrolling retain their own pointer-region behavior below the header.
8. Add Vimeo immediately after YouTube in Stream. Vimeo uses its provider identity colors while retaining Nougat's surrounding UI structure.
9. Remove the partial brown Video Player rail so the Video Player page background surrounds the player uniformly.
10. Improve Home TV artwork recovery for resume records created outside Library navigation: exact episode Primary/still first, then matching season poster, then series poster, then Nougat fallback.
11. Make movie artwork fill the Home artwork region with aspect-preserving cover behavior instead of rendering as a tiny centered poster.
12. Strengthen v0.0.29 regression testing with a real executable state-machine check for same-folder episode progression and the 10-second Up Next state.

## Explicitly deferred / excluded
- Focused P2P expansion remains v0.0.30.
- No licensing-model changes.
- No repository-root archival/cleanup migration.
- No new external Stream providers beyond Vimeo in this release.
- No redesign of the accepted v0.0.28 candy palette, Home state persistence, Library poster-quality system, Search layout cleanup, resume history, Stop screen, seek previews, diagnostics, or N identity.

## Changed implementation surfaces
- `src/main.cpp`
- `CMakeLists.txt`
- current/unversioned and versioned desktop launchers through `NougatMediaSuite_v29.desktop`
- `README.md`, `ROADMAP.md`, `CHANGELOG.md`, `APPLY_COMMAND.txt`
- v0.0.29 installer, manifest, retained-v28 test, v29 contract test, X11 smoke test, installer/rollback test
- this handshake and the v0.0.29 validation record

## Protected boundaries
The v0.0.29 package must preserve unchanged:
- PolyForm Noncommercial owner-controlled licensing files and third-party license separation
- Nougat Search engine / bridge
- existing P2P implementation
- diagnostics implementation
- accepted v0.0.28 page palette and Home state foundations
- accepted resume/Stop/seek-preview/player-navigation foundations
- approved N icon and desktop identity

## Primary risks
- Ambiguous filenames can be misordered. Explicit season/episode identity is preferred; natural filename fallback is bounded to episode contexts.
- An Open File episode may not have Jellyfin series metadata. Same-folder playback must still work, while `Back to Series` must fall back honestly if no catalog series can be proven.
- Pre-resolving the next episode must not misclassify normal movie folders as TV queues.
- Home cover-fill behavior may crop poster edges; it must preserve aspect ratio and never stretch artwork.
- Vimeo styling must use provider identity without implying a commercial partnership.

## Rollback point
- Exact accepted base: v0.0.28 commit `80b12f2b47959f4482e65238b9c0b6033f9785a2`
- Installer creates a rollback snapshot before modifying the project and restores the accepted touched state on validation failure.
- Existing accepted root executable `Nougat_Media_Suite_v28` is retained until the final v29 executable has passed validation and been installed.

## Validation plan
- exact Git/base/hash manifest gate before changes
- protected licensing/Search/P2P checks
- retained-v0.0.28 behavior test
- v0.0.29 source/runtime contract test
- warnings-as-errors stub build
- X11 application identity/UI smoke
- full native libtorrent build on owner machine
- real llama.cpp/Nomic model load on owner machine
- `$ORIGIN` relative AI runtime verification
- final root executable/version checks
- approved N icon metadata applied after final executable write and read back
- owner real-use testing of TV EOF flow and carry-forward UI/artwork fixes

## Owner acceptance checks
- Load a middle TV episode from more than one route, including direct Open File when practical, and confirm the same-folder next episode is found.
- Confirm the Up Next panel appears promptly at natural EOF with the actual next episode and visible 10-second countdown.
- Confirm countdown reaches zero and starts the next episode automatically; confirm manual Stop does not trigger autoplay.
- Confirm `Back to Series` returns to the actual show/season when catalog identity exists.
- On Home, hover the top tab bar and confirm the wheel scrolls navigation while Home is selected.
- Confirm Vimeo appears immediately after YouTube and uses Vimeo provider styling.
- Confirm the Video Player background is continuous around the player without the partial brown rail.
- Confirm TV Home cards recover episode/season/series artwork instead of black cards where artwork exists.
- Confirm movie artwork fills the Home card artwork region without distortion.
- Confirm accepted v0.0.28 behavior remains intact.

## Continuation point
If the owner accepts v0.0.29: create accepted snapshot, local Git commit/tag, then perform separate SSH GitHub push/remote verification. Focused P2P expansion remains the planned v0.0.30 lane.
