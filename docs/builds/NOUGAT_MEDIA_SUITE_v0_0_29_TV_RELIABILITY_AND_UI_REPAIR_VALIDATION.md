# Nougat Media Suite v0.0.29 — TV Playback, Navigation, and Carry-Forward UI Repair — Validation Record

## Candidate
- Base: accepted v0.0.28 commit `80b12f2b47959f4482e65238b9c0b6033f9785a2`
- Target: v0.0.29
- Root executable target: `Nougat_Media_Suite_v29`
- Package: `Nougat_Media_Suite_v0_0_29_TV_RELIABILITY_AND_UI_REPAIR_CHANGED_FILES_ONLY.zip`
- Status: candidate only; owner acceptance required

## Builder-environment results
- Warnings-as-errors C++ stub build: PASS.
- Runtime version: PASS (`Nougat Media Suite v0.0.29`).
- v0.0.29 executable TV/UI reliability self-test: PASS.
  - synthetic same-folder `S01E13 -> S01E14` resolution
  - visible 10-second Up Next state-machine contract
  - natural filename fallback for confirmed episode context
  - Vimeo provider order/homepage contract
  - Home artwork acceptance contract
- Accepted v0.0.28 UI-state self-test: PASS.
- Accepted provider/Discover state self-test: PASS.
- Retained-v0.0.28 regression test: PASS.
- v0.0.29 source/runtime contract test: PASS.
- X11 identity/UI smoke under Xvfb: PASS.
- Search engine/bridge, media-server lifecycle, diagnostics and protected hash checks are rerun during final package validation.

## Builder-environment native boundary
This builder environment does not substitute for the owner's full native Ubuntu machine. The installer therefore requires and reruns the full native libtorrent build, relative AI RPATH check, real llama.cpp/Nomic model load, final X11 smoke, root-executable verification and N-icon metadata readback before it can print `FINAL PASS`.

## Owner-machine required gates
1. Clean `main` at accepted v0.0.28 commit `80b12f2b47959f4482e65238b9c0b6033f9785a2`.
2. Existing accepted `Nougat_Media_Suite_v28` reports v0.0.28.
3. Exact package manifest and accepted-base hashes match.
4. Rollback snapshot is written before changes.
5. Protected licensing/Search/P2P boundaries pass.
6. Retained-v28 and v29 regression suites pass.
7. Warnings-as-errors stub build and X11 smoke pass.
8. Full native libtorrent build passes.
9. Real llama.cpp/Nomic model load passes from relative `$ORIGIN` runtime layout.
10. Final `Nougat_Media_Suite_v29` is copied to project root and reports v0.0.29.
11. Final v29 runtime regression/X11 tests pass.
12. N custom-icon metadata is applied after the final executable write and read back successfully.

## Owner-visible checks after installer FINAL PASS
- same-folder next-episode resolution from different launch routes
- prompt arrival time at natural EOF
- visible 10-second countdown and autoplay
- manual Stop cancellation
- Back to Series destination
- Home top-navigation wheel routing
- Vimeo placement/branding
- uniform Video Player background
- TV Home artwork hierarchy
- movie artwork fill/crop quality
- retained v0.0.28 behavior

A successful installer run creates a validated candidate, not owner acceptance. Acceptance still requires Derek's real-use approval.

## Additional builder regression results
- Nougat Search engine test: PASS.
- Nougat native Search bridge test: PASS.
- retained-v0.0.22 deterministic behavior test: PASS.
- media-server lifecycle test: PASS.
- diagnostics v0.0.26 regression test: PASS.
- installer/rollback v0.0.29 static contract: PASS.
- Historical licensing test could not complete in this reconstructed builder tree because `.github/PULL_REQUEST_TEMPLATE.md` is not present in the supplied/reconstructed source set. The protected licensing-file SHA-256 boundaries remain unchanged, and the v0.0.29 installer reruns the complete licensing test against Derek's accepted repository before making changes.
- Full native builder configure was attempted and stopped at CMake because this environment does not provide the `libtorrent-rasterbar` development package. This is an environment limitation, not reported as a native PASS. The owner-machine installer requires `pkg-config --exists libtorrent-rasterbar` and performs the full native build before installing the candidate.
