# Nougat Media Suite v0.0.32 Native P2P Media + Player/UI Repair Validation

## Build-side status
- Deterministic C++17 warnings-as-errors **stub build: PASS** in the packaging environment.
- v0.0.29 TV reliability self-test: **PASS**.
- v0.0.30 Library/player self-test: **PASS**.
- v0.0.31 UI-sheet component self-test: **PASS**.
- v0.0.32 P2P/player/UI repair self-test: **PASS**.
- P2P localhost HTTP Range fixture, including explicit/suffix ranges, HEAD, 416 handling, seek supersession and playback-window priority: **PASS**.
- Installer rollback/static safety test: **PASS**.
- Exact package inventory/hash verification: performed again on the finalized ZIP before handoff.
- Full native libtorrent/llama.cpp build: **target-machine gate**, intentionally not claimed from the packaging environment.

## Target-machine gates
The installer must compile against the owner's installed libtorrent-rasterbar 2.x and pinned llama.cpp runtime with warnings as errors, verify the relative AI `$ORIGIN` RPATH, execute the retained and v0.0.32 self-tests, perform X11 identity/UI smoke, write the final `Nougat_Media_Suite_v32` to the project root, then apply/read back the approved N custom-icon metadata.

## Owner checks
1. TV natural end reaches Up Next; the countdown changes once per second without visible screen flashing; autoplay still starts the resolved next episode.
2. Volume housing visibly contains the full track and 24 px knob at 0%, 100% and 200%.
3. Search cream buttons/tabs/fields/panels retain the accepted cream palette but their stitched/inset seams are visibly chocolate brown.
4. Stream activity panels for YouTube, Vimeo, Rumble, RuTube, VK and OK have no provider-colored left strip and retain the clean lower panel border.
5. Magnet and `.torrent` intake work, metadata/file list appears, selected media can Watch Now while incomplete, Pause/Resume works, Remove leaves downloaded files intact, and seeking reprioritizes the new playback region.
6. A `magnet:?` Search result can be handed directly into P2P.
