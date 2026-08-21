# Nougat Media Suite v0.0.21 Validation Record

## Pre-handoff results

- PASS: v0.0.21 identity/palette/navigation source contract.
- PASS: exact Stream service set remains YouTube, Rumble, RuTube, VK, and OK; deferred services do not appear in the v0.0.21 source.
- PASS: new Nougat Media Suite icon asset/header inventory and desktop identity contract.
- PASS: retained deterministic v0.0.20 recommendation/watch-provider/metadata/diagnostics behavior harness.
- PASS: Nougat Search engine and native bridge tests.
- PASS: media-server graceful-stop, parent-death-stop, and independent-server-preservation harness.
- PASS: installer rollback contract preserves the accepted v0.0.20 touched state, generated runtimes, pinned-model location, Search data, media, and user data.
- PASS: C++17 deterministic stub build with `-Wall -Wextra -Werror`.
- PASS: stub `Nougat_Media_Suite_v21 --version` reports `Nougat Media Suite v0.0.21`.
- PASS: stub Discover AI self-test.
- PASS: bounded Xvfb live-window smoke finds the `Nougat Media Suite` window.
- PASS: interactive X11 probe clicked Library List/Grid controls and Search P2P, confirming the active view-button treatment changes and the P2P surface opens inside Search; Rumble selection changed Stream to the green service palette.
- PASS: warnings-as-errors stub build confirms the repaired navigation/UI source compiles; owner-machine visual confirmation remains required for exact rendered colors and icon placement.
- PASS: local captured-window inspection confirmed Stream switches among distinct YouTube red, Rumble green, RuTube violet/red/cyan, VK blue/cyan, and OK orange/chocolate palette surfaces.
- PASS: Search-facing v0.0.21 strings use renderer-safe ASCII separators/check marks, avoiding mojibake in the native X11 font path.
- PASS: a changed-files package extracted into a reconstructed accepted v0.0.20 source tree; its manifest verified every payload/base hash, the extracted payload reran the source/rollback suites, rebuilt the warnings-as-errors stub executable, passed version/AI self-test, and passed bounded Xvfb window smoke.
- PASS: final handoff ZIP integrity and SHA-256 are verified after the final manifest is generated; the accompanying `.sha256` file is the authoritative archive checksum.

## Environment-limited full-native gate

The build container cannot run the owner's full native lane because `pkg-config` does not provide `libtorrent-rasterbar` in this environment. Full native configuration therefore stops before compilation here. This is not reported as a PASS.

The owner-machine installer requires `libtorrent-rasterbar`, the accepted pinned llama.cpp runtime, the accepted Nomic model, and the integrated Jellyfin runtime before applying the candidate. It then performs the full native build, root-executable version gate, real pinned-model Discover AI self-test, final executable icon assignment/readback, and launcher/icon installation before it can print `FINAL PASS`.

## Owner-visible checks after installer FINAL PASS

- Window/application identity reads **Nougat Media Suite**.
- Top order is `Video Player | Library | Discover | Search | Stream | Debug`; P2P is available inside Search.
- The approved chocolate/nougat N + play icon appears on the launcher, running app/dock/app switcher, in-app version badge, and raw `Nougat_Media_Suite_v21` executable in Files/Nautilus.
- Common suite chrome is chocolate/cocoa/cream/caramel rather than the former ReddMedia red identity.
- Video Player is chocolate/cocoa/caramel; Library, Discover, Search, and Debug use their coordinated palettes; P2P is a navy/ice sub-surface inside Search.
- Stream visibly changes palette when switching among YouTube, Rumble, RuTube, VK, and OK.
- No additional Stream service appears in v0.0.21.
- Search shows `Search | Crawler | P2P`, peer/node controls live behind `Network...`, and the top-level P2P tab is absent.
- Library shows working List (three lines) and Grid (2x2 squares) icon controls after its heading, with no `Grid [x]` / `List [x]` labels.
- Existing playback, Library catalog, Discover, Search engine, Stream actions, P2P engine, Debug, server, TV autoplay, volume, persisted view choices, and text-input behavior remain intact.
