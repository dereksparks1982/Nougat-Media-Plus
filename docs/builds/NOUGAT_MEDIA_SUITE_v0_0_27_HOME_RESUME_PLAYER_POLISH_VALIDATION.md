# Nougat Media Suite v0.0.27 Validation Record

## Baseline
- Accepted base: v0.0.26
- Accepted Git commit: `406c910e606ba58fa8943b1973e45d135ac15eae`
- Target: v0.0.27 candidate

## Builder-environment validation completed before handoff
- Warnings-as-errors CMake configure/build with `REDDMEDIA_AI_STUB=ON` and `REDDMEDIA_P2P_STUB=ON`: PASS.
- Stub executable `--version`: PASS (`Nougat Media Suite v0.0.27`).
- Retained provider/Discover state self-test: PASS.
- v0.0.27 deterministic source/contract test: PASS.
- v0.0.26 retained-behavior/protected-boundary test: PASS.
- Xvfb/X11 window identity smoke (WM_CLASS, application ID, desktop hint, `_NET_WM_ICON`, window title): PASS.


## Same-version owner-machine installer repair
- First owner-machine install attempt: expected rollback PASS, candidate not installed.
- Failure occurred in the post-apply source/regression lane at `FAIL: CMake v26 identity missing`.
- Root cause: the v27 installer incorrectly invoked the historical `tools/test_nougat_media_suite_v26.py` identity contract after applying v27 sources. That contract intentionally requires v0.0.26 CMake/executable identity and therefore cannot be a valid post-upgrade compatibility test.
- Repair: removed that obsolete invocation; retained `tools/test_nougat_media_suite_retained_v26.py` for accepted-v26 behavioral compatibility under v27 identity.
- Added a static installer regression assertion that fails if `tools/test_nougat_media_suite_v26.py` is ever reintroduced into `run_source_tests()` for v27.
- Runtime/player/Home source is unchanged by this same-version repair. Full owner native validation still reruns from accepted v0.0.26.

## Native owner-machine gates in installer
The installer must still prove these on the owner Ubuntu workstation before printing FINAL PASS:
- exact clean `main` at accepted v0.0.26 commit;
- exact v0.0.26 root executable/runtime identity;
- exact manifest base hashes and required-absent state;
- license/Search/P2P preservation;
- deterministic v27 and retained-v26 tests;
- warnings-as-errors stub build and X11 UI smoke;
- full native libtorrent build;
- full native llama.cpp/model load and Discover AI self-test using the pinned runtime/model;
- final root `Nougat_Media_Suite_v27` install, executable/version/RPATH verification;
- launcher aliases and raw executable approved-N metadata readback after the final executable write.

## Honest validation boundary
The builder environment does not replace the owner's real library, real media metadata/artwork, native GPU/display behavior, real video catalog, or Ubuntu launcher/dock. Home recommendation quality, real Jellyfin artwork selection, hover video previews, resume persistence, fullscreen/windowed shape transitions, seek previews, flicker repair, and the TV/movie-specific owner UX remain owner acceptance gates after installer FINAL PASS.

## Builder-environment limits observed during final packaging
- A fresh warnings-as-errors stub build completed successfully after the implementation was finalized.
- Full native configure was attempted in the builder environment and stopped before compilation because that environment does not provide the `libtorrent-rasterbar` development package. This is an environment limitation, not reported as a native-build PASS. The installer requires and rebuilds against the owner's real libtorrent installation before FINAL PASS.
- The standalone historical licensing regression script was invoked against the reconstructed three-part upload and stopped because that reconstructed upload does not contain `.github/PULL_REQUEST_TEMPLATE.md`. Core protected licensing hashes are still checked by the v0.0.27 deterministic/protected-boundary test, and the installer runs the full historical licensing test against the owner's complete accepted project before FINAL PASS.

## Package gates
- Changed-files payload inventory checked against the v0.0.27 manifest: PASS.
- Exact accepted-v0.0.26 base hash records and required-absent paths checked against the reconstructed accepted baseline: PASS.
- Changed-files ZIP integrity test: PASS.
- Package contains source/docs/tests/installer/manifest only; no build tree, runtime tree, model, credential, cache, transfer ZIP, or generated candidate executable is included.
