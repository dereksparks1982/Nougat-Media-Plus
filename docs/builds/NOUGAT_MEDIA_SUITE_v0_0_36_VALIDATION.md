# Nougat Media Suite v0.0.36 Candidate Validation

Base: accepted/published v0.0.35 commit `8c2b8fb63254eb4b17045688ffbd50c1bba2075b`.

## Candidate scope

- Library collection-first hierarchy and a sheet-style `Search` field below the green action row.
- Home card artwork destination/clip repair.
- Pixel-derived approved-sheet seek sprite with elapsed/total times on the bar's left/right sides.
- Stable full-height player repaint region for seek, VOLUME, percentage, and transport controls.
- Exact accepted VOLUME sprite preserved with only the rectangular source-sheet corner area masked away.
- Header changed to the VOLUME-housing tan with vertically centered chrome and sheet-family circular Server indicator.
- Working v0.0.35 native ATSC scan code preserved.
- v0.0.37 roadmap agenda records native persisted-channel `Watch Live` tuning/playback.

## Validation completed in the packaging environment

- CMake stub configuration PASS with `REDDMEDIA_AI_STUB=ON` and `REDDMEDIA_P2P_STUB=ON`.
- Full stub compile PASS under the project's `-Wall -Wextra -Werror` gate.
- Executable identity PASS: `Nougat Media Suite v0.0.36`.
- Retained executable self-tests PASS: Discover AI, v25, v28, v29, v30, v31, v32, v33, v34, v35.
- New `--v36-library-ui-player-self-test` PASS.
- `tools/test_nougat_media_suite_v36.py` PASS, including seek/volume asset hashes, Library search, collection-hierarchy source contract, Home offscreen-GC isolation, player repaint unification, header/status treatment, retained ATSC scan tokens, and README description placement.
- `tools/test_installer_rollback_v36.py` PASS.
- P2P stream server v32 regression test PASS.
- Diagnostic Center v26 regression test PASS.
- Nougat engine and bridge v19 regression tests PASS.
- `INSTALL_NOUGAT_MEDIA_SUITE_v0_0_36.sh` syntax PASS via `bash -n`.
- Protected licensing files were not changed from the v0.0.35 source baseline.

## Owner-machine gates intentionally performed by the installer

The packaging environment does not provide `libtorrent-rasterbar`, so it cannot reproduce the owner's full native link. The installer therefore repeats source/regression tests, verifies/rebuilds the retained security runtime when needed, performs the warnings-as-errors stub build, performs the full native llama.cpp + libtorrent build on the owner workstation, verifies the relative `$ORIGIN` AI RPATH, copies the runnable v36 executable to the project root, and verifies the root executable before reporting FINAL PASS.

The owner visual check remains authoritative for pixel placement under the real X11 desktop: Home card fill, Library collection/search UX, seek/VOLUME appearance, mouse-movement clipping, and header/status appearance.
