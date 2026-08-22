# Nougat Media Suite v0.0.28 Validation Record

## Baseline
- Accepted base: v0.0.27
- Accepted Git commit: `9e3b1820d8a0374b100911b5d2e5146e0688983c`
- Target: v0.0.28 candidate

## Builder-environment validation completed before handoff
- Fresh warnings-as-errors CMake configure/build with `REDDMEDIA_AI_STUB=ON` and `REDDMEDIA_P2P_STUB=ON`: PASS.
- Stub executable `--version`: PASS (`Nougat Media Suite v0.0.28`).
- Discover AI deterministic stub self-test: PASS.
- Retained provider/Discover state self-test: PASS.
- v0.0.28 palette/responsive-grid/poster-quality/metadata-encoding runtime self-test: PASS.
- v0.0.28 deterministic source/contract test: PASS.
- retained-v0.0.27 protected-behavior test: PASS, while explicitly reporting the known TV Up Next runtime path as deferred to v0.0.29 rather than falsely calling it healthy.
- Xvfb/X11 identity smoke (WM_CLASS, application ID, desktop hint, `_NET_WM_ICON`, window title): PASS.
- installer/rollback static contract and terminal-safety validation: PASS.
- historical Search engine/bridge, retained-v22, media-server lifecycle, and diagnostics tests available in the reconstructed full tree: PASS.

## Builder-environment limitations observed
- The historical standalone licensing regression script was invoked against the reconstructed project and stopped because that reconstruction does not contain `.github/PULL_REQUEST_TEMPLATE.md`. Protected licensing hashes are still checked by the retained-v27/v28 contract tests, and the v0.0.28 installer runs the complete historical licensing test against the owner's accepted repository before it can print FINAL PASS.
- A full native configure was attempted and stopped before compilation because this builder environment does not provide the `libtorrent-rasterbar` development package. This is not reported as a native-build PASS. The owner-machine installer requires libtorrent, performs the real native build, verifies the relative llama.cpp runtime RPATH, loads the pinned Nomic model, and reruns the native validation before installation succeeds.

## Native owner-machine gates in installer
The installer must still prove these on the owner Ubuntu workstation before printing FINAL PASS:
- exact clean `main` at accepted v0.0.27 commit;
- exact v0.0.27 root executable/runtime identity;
- exact manifest base hashes and required-absent state;
- full licensing/Search/P2P protected-state validation;
- retained-v27 and v28 source/runtime tests;
- warnings-as-errors stub build and X11 UI smoke;
- full native libtorrent build;
- full native llama.cpp/model load and Discover AI self-test using the pinned runtime/model;
- final root `Nougat_Media_Suite_v28` install, executable/version/RPATH verification;
- launcher aliases and approved-N custom-icon metadata readback after final executable write.

## Honest visual/use validation boundary
The builder environment cannot replace the owner's real catalog, TMDb credential, exact local poster inventory, X11/GNOME desktop, or real media files. Poster quality/matching, season-poster fallback, Home visual persistence, exact half-screen three-column presentation, category/metadata legibility, rounded top clipping, and removal of the visible player halo remain owner visual/use acceptance gates after installer FINAL PASS.

The owner-observed v0.0.27 TV Up Next/autoplay and `Back to Series` regressions are intentionally not claimed as fixed here. They are isolated to planned v0.0.29 so v0.0.28 remains an artwork/UI/state build.

## Package gates
- Changed-files payload inventory matches the v0.0.28 manifest.
- Existing changed files are recorded with exact accepted-v0.0.27 base hashes; new v28 paths are required absent.
- Package ZIP integrity is verified after creation.
- Package contains source/docs/tests/installer/manifest only; no build tree, runtime tree, model, credential, cache, transfer ZIP, or generated candidate executable is included.
