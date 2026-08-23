# Nougat Media Suite v0.0.32 Native P2P Media + Player/UI Repair Handshake

## Base
- Accepted/published v0.0.31 commit: `b22d11d4989784dc6df56abbde08344720064790`.
- Accepted v0.0.31 root executable: `Nougat_Media_Suite_v31`.

## Approved scope
- Expand the existing libtorrent 2.x P2P core under `Search > P2P` only.
- Preserve magnet and local `.torrent` intake, file selection, persistent resume data, full background download/seeding, and native-player-only playback.
- Add moving immediate / near-future / rewind piece-priority windows around native-player byte-range demand.
- Add real selected-media progress and contiguous start-buffer evidence, Watch Now, Pause/Resume, non-destructive Remove, Search magnet handoff, and richer Debug P2P evidence.
- Repair owner-reported Up Next countdown flashing, volume housing/track/knob containment, low-contrast Search cream stitching, and the Stream activity panel left-side accent strip across YouTube, Vimeo, Rumble, RuTube, VK and OK.
- Preserve accepted v0.0.31 page/service palettes and UI-sheet component construction outside the approved contrast repair.

## Protected boundaries
- No licensing files change.
- No Nougat decentralized Search engine/bridge behavior changes except UI handoff of a returned `magnet:?` result into the existing P2P intake.
- No Home card redesign, Library behavior change, Discover behavior change, Stream provider change, or Jellyfin ownership change.
- P2P Remove leaves already-downloaded files on disk.

## Validation
- C++17 `-Wall -Wextra -Werror` deterministic stub build.
- Retained v0.0.29/v0.0.30/v0.0.31 executable self-tests.
- v0.0.32 player/UI/P2P contract self-test.
- Deterministic HTTP Range bridge test against a fake P2P engine, including new playback-window scheduling calls and seek supersession.
- Exact Search seam source regression, Up Next offscreen-composition regression, volume containment regression, Search magnet handoff and Remove contract checks.
- Target-machine full native libtorrent 2.x + llama.cpp rebuild, relative `$ORIGIN` RPATH verification, final root executable, launcher/N identity, rollback and protected-license tests.

## Package
`Nougat_Media_Suite_v0_0_32_NATIVE_P2P_MEDIA_AND_PLAYER_UI_REPAIR_CHANGED_FILES_ONLY.zip`
