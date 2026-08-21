# Nougat Media Suite v0.0.23 — Exact Concept UI and Stream Repair Handshake

## Project and release
- Project: Nougat Media Suite
- Company/licensor: Elderred Softworks LLC
- Target version: v0.0.23
- Release lane: new-version UI/Stream build after the separate license-only Git update
- Required Git branch: `main`
- Required Git HEAD before apply: `755c1ac` (`licensing: protect Nougat original materials under PolyForm Noncommercial`)
- Required current root executable: `Nougat_Media_Suite_v22`
- Required v22 executable SHA-256: `651ba56069f97170b7f94cce2d019de703c18fa7f8d8e51775a61caf92e843a5`
- Target root executable: `Nougat_Media_Suite_v23`
- Package: `Nougat_Media_Suite_v0_0_23_EXACT_CONCEPT_UI_STREAM_REPAIR_CHANGED_FILES_ONLY.zip`

## Approved completed changes
1. Treat the owner-approved Nougat concept sheet as the visual authority for the v0.0.23 UI pass.
2. Preserve the exact top-level order: `Video Player | Library | Discover | Search | Stream | Debug`.
3. Restyle the existing top tabs with the Nougat concept treatment without rearranging the row.
4. Give only the active top tab the integrated centered downward point/notch shown in the approved concept art.
5. Center the top navigation and the six existing player controls when width permits; retain horizontal scrolling when width is narrow.
6. Preserve the exact player-control order: `Open | Rewind 10s | Play/Pause | Stop | Fast Forward 10s | Fullscreen`.
7. Replace the old red seek fill with the Nougat caramel/golden-brown progress, cream track, chocolate/gold detailing, and rounded knob treatment.
8. Replace the old red volume fill with the Nougat caramel/cream/chocolate treatment and shorten the volume control to concept-art proportions while retaining 0–200%, live percentage, and the 100% midpoint marker.
9. Use the quilted Nougat page material across the main page surfaces with the approved restrained tab tints: warm cream/caramel Video Player, pale sage Library, pale lavender Discover, cream Search, pale dusty blue Stream, warm gray/taupe Debug.
10. Replace the old candy application identity with the approved square chocolate/caramel N emblem in the active icon assets and embedded X11 window icon data.
11. Preserve the five approved Stream selectors: YouTube, Rumble, RuTube, VK, OK.
12. Use one shared Direct Play URL field for the Stream page.
13. Remove the redundant Stream `Play` button; `Direct Watch` remains the single Stream native-playback action. The main Video Player `Play/Pause` control remains unchanged.
14. Preserve the existing Stream `Download`, `Direct Watch`, `Open Webpage`, and `Clear Log` actions.
15. Improve the YouTube Direct Watch yt-dlp path by detecting an already-installed supported JavaScript runtime and preferring a playable HLS fallback before ordinary YouTube formats.
16. Preserve the already-pushed licensing files byte-for-byte.
17. Keep the relative `$ORIGIN/components/ai/runtime/lib[64]` runtime path and validate the final root executable without manually setting `LD_LIBRARY_PATH`.

## Explicitly excluded from this release
- Additional Stream services such as Vimeo, Twitch, Kick, TikTok, Dailymotion, Bilibili, or Niconico.
- A new Web Player or browser-embedded player.
- Plex integration or other media-server expansion.
- Any licensing-text rewrite. The separately pushed v0.0.22 license-only state is protected unchanged.
- Any change to the accepted top-tab order or main player-control order.

## Files changed or added
- `APPLY_COMMAND.txt`
- `CHANGELOG.md`
- `CMakeLists.txt`
- `COMPANY_BIBLE.md`
- `DEPENDENCIES.md`
- `NougatMediaSuite.desktop`
- `NougatMediaSuite_v23.desktop`
- `README.md`
- `ROADMAP.md`
- `assets/icons/nougat-media-suite.png`
- `assets/icons/nougat-media-suite-14.png`
- `assets/icons/nougat-media-suite-16.png`
- `assets/icons/nougat-media-suite-32.png`
- `assets/icons/nougat-media-suite-48.png`
- `assets/icons/nougat-media-suite-64.png`
- `assets/icons/nougat-media-suite-128.png`
- `assets/icons/nougat-media-suite-256.png`
- `assets/icons/nougat-media-suite-512.png`
- `docs/NOUGAT_MEDIA_SUITE_BRAND_PALETTE.md`
- `src/main.cpp`
- `src/nougat_media_suite_icon_data.hpp`
- `src/ytdlp_stream_server.cpp`
- `INSTALL_NOUGAT_MEDIA_SUITE_v0_0_23.sh`
- `NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v23.json`
- `docs/builds/NOUGAT_MEDIA_SUITE_v0_0_23_EXACT_CONCEPT_UI_STREAM_REPAIR_HANDSHAKE.md`
- `docs/builds/NOUGAT_MEDIA_SUITE_v0_0_23_EXACT_CONCEPT_UI_STREAM_REPAIR_VALIDATION.md`
- `tools/test_installer_rollback_v23.py`
- `tools/test_nougat_media_suite_ui_smoke_v23.py`
- `tools/test_nougat_media_suite_v23.py`

## Protected licensing state
The following files must remain byte-identical to the already-pushed license-only commit:
- `LICENSE`
- `COPYRIGHT.md`
- `CONTRIBUTING.md`
- `THIRD_PARTY_NOTICES.md`
- `docs/LICENSING_POLICY.md`

## Validation plan
1. Package and base-file SHA-256 verification.
2. Git branch/HEAD/worktree preflight.
3. Protected licensing hash verification.
4. Existing license, Search/Nougat bridge, retained-behavior, media-server lifecycle, and v0.0.23 regression tests.
5. Installer rollback contract test.
6. Warnings-as-errors deterministic stub build.
7. CLI version and Discover AI self-test.
8. X11 UI smoke test.
9. Full native libtorrent + real llama.cpp build on the owner's workstation.
10. Relative `$ORIGIN` RPATH verification and final root executable version/self-test with `LD_LIBRARY_PATH` removed.
11. Raw executable `metadata::custom-icon` write/readback after the final executable is copied.
12. Owner visual acceptance of the exact-concept presentation, active-tab point, centered wide layouts, Nougat seek/volume colors, compact volume, quilted/tinted pages, and N identity.
13. Owner Stream acceptance with one shared URL field, no redundant Stream Play button, and the previously failing YouTube Direct Watch URL.

## Rollback
The installer creates a timestamped exact touched-state snapshot under:

`$HOME/DKLab/Archives/ReddMedia Archive/Nougat_Media_Suite_pre_v0_0_23_<timestamp>`

On any post-apply validation failure, the installer restores the exact pre-v0.0.23 touched files and the root `Nougat_Media_Suite_v22`. It preserves the separately committed licensing files, runtime/model data, media/server data, and owner-uploaded source ZIPs.

## Known risks / owner gates
- Pixel-level visual acceptance requires the owner's real desktop because this build environment cannot reproduce the owner's exact Ubuntu compositor/font rendering.
- The reported YouTube 403 is network/service dependent; the build can validate the yt-dlp compatibility path structurally, but the owner must retest the actual URL on the workstation.
- No new JavaScript runtime is silently installed. Direct Watch uses an already-installed supported runtime when available; yt-dlp behavior without one remains dependent on the current YouTube challenge path.

## Continuation point after candidate installation
Do not accept, snapshot, tag, or push v0.0.23 until the installer prints `FINAL PASS` and the owner completes the visual and Stream checks above.

## Next planned work
Only after owner acceptance of this candidate: close out v0.0.23 with the normal acceptance snapshot, local commit/tag, SSH push, and remote verification. Any additional Stream services or Web Player work remains a separate future proposal.

## Required owner artifacts / inputs
- Current Nougat Media Suite project at the required base above.
- Existing AI runtime and pinned Nomic model already present in the project.
- Integrated Jellyfin runtime already present in the project.
- The approved Nougat concept sheet and exact N emblem already incorporated into this candidate.
