# Nougat Media Suite v0.0.30 — UI Cohesion, Library Performance, and Player Navigation — Validation Record

## Candidate
- Base: accepted v0.0.29 commit `91ced5349a71de34cbc7d3d95d32c4de56645c7b`
- Target: v0.0.30
- Root executable target: `Nougat_Media_Suite_v30`
- Package: `Nougat_Media_Suite_v0_0_30_UI_LIBRARY_PLAYER_POLISH_CHANGED_FILES_ONLY.zip`
- Status: candidate only; owner acceptance required

## Builder-environment results
- Warnings-as-errors C++ stub build: PASS.
- Runtime version: PASS (`Nougat Media Suite v0.0.30`).
- Retained v0.0.28 UI/artwork executable self-test: PASS.
- Retained v0.0.29 TV/UI reliability executable self-test: PASS.
- v0.0.30 UI/Library/player executable self-test: PASS.
  - realistic tall Library viewport produces multiple visible 2:3 poster rows
  - Home movie/series poster geometry remains portrait while episodes remain landscape
  - Previous/Next episode-boundary availability is exercised with real temporary files
  - persistent metadata cache round-trips tabs/newlines/genres/numeric episode identity safely
- retained-v0.0.29 regression test: PASS.
- v0.0.30 source/runtime contract test: PASS.
- Search/P2P/licensing protected SHA-256 boundaries: PASS.
- X11 identity/UI smoke: rerun on final builder binary/package.

## Builder-environment native boundary
This builder environment does not provide the `libtorrent-rasterbar` development package, so it cannot truthfully substitute for the owner's full native Ubuntu validation. The installer therefore requires `pkg-config --exists libtorrent-rasterbar`, performs the full native build, verifies the relative AI RPATH, loads the real llama.cpp/Nomic model, reruns X11 smoke and runtime regression tests, verifies the final root executable, and reads back the N custom-icon metadata before it may print `FINAL PASS`.

## Owner-machine required gates
1. Clean `main` at accepted v0.0.29 commit `91ced5349a71de34cbc7d3d95d32c4de56645c7b`.
2. Existing accepted `Nougat_Media_Suite_v29` reports v0.0.29.
3. Exact package manifest and accepted-base hashes match.
4. Rollback snapshot is written before changes.
5. Protected licensing/Search/P2P boundaries pass.
6. Retained-v29 and v30 regression suites pass.
7. Warnings-as-errors stub build and X11 smoke pass.
8. Full native libtorrent build passes.
9. Real llama.cpp/Nomic model load passes from relative `$ORIGIN` runtime layout.
10. Final `Nougat_Media_Suite_v30` is copied to project root and reports v0.0.30.
11. Final v30 runtime regression/X11 tests pass.
12. N custom-icon metadata is applied after the final executable write and read back successfully.

## Owner-visible checks after installer FINAL PASS
- rounded large primary content panels across remaining square pages
- complete active top-tab arrows during idle and busy states
- portrait Home movie cards and landscape episode cards
- multiple Library poster rows visible simultaneously
- cache-first return to known Library views
- truthful determinate/indeterminate Library progress
- Refresh Server versus Refresh Library status semantics
- Previous/Next episode controls and boundary disabling
- larger volume knob with retained 0–200% behavior
- retained v0.0.29 TV reliability, Stream, Search/P2P, diagnostics, licensing and N identity

A successful installer run creates a validated candidate, not owner acceptance. Acceptance still requires Derek's real-use approval.
