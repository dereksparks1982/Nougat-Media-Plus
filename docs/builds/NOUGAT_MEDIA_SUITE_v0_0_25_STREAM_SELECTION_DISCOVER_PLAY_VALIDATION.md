# Nougat Media Suite v0.0.25 Stream Selection and Discover Play Validation

## Same-version owner-test repair

- The redundant `STREAM` and `Online video: <provider>` text must not be rendered above the Stream provider row.
- The cream/white provider container and border must not be rendered behind the service selector row.
- The selected provider notch remains the persistent provider-state indicator.
- Provider theming/quilt tinting and all other v0.0.25 behavior remain unchanged.

## Required base

- Git branch: `main`.
- Accepted base commit: `f66d35b671c9bceee6151dc63003dc3ec24578e8`.
- Installed same-version repair base executable: existing `Nougat_Media_Suite_v25` reporting `Nougat Media Suite v0.0.25`, with Git HEAD still at accepted v0.0.24 until owner acceptance.
- Protected licensing files and decentralized Search engine/bridge files must match the accepted hashes exactly.
- The accepted literal concept-sheet N and exact quilt source must remain byte-identical.

## Deterministic v0.0.25 gates

- CMake target and visible version are v0.0.25.
- All five Stream providers have distinct provider-driven quilt tint mappings.
- Common Stream action buttons inherit the selected provider palette.
- Selected Stream provider renders a downward active notch rather than the old underline-only marker.
- The redundant `STREAM` / `Online video: <provider>` label and the old cream provider container/border are absent from `draw_stream_screen`; the five provider selectors sit directly on the provider-tinted quilt.
- Discover has independent persistent mode and target selector states and can expose two active notches simultaneously.
- TMDb/service actions remain momentary buttons.
- Local Discover play resolves a recommendation through the Jellyfin catalog to an actual Movie or Episode node before native playback.
- Series-level local play sorts real episodes and uses most-recent matching local viewing history when available, otherwise the first real episode.
- Fake-Jellyfin behavior gate proves first-episode fallback and watched-episode resume without using owner media or credentials.
- Search engine/bridge and protected licensing hashes remain unchanged.
- Warnings-as-errors stub build, Discover AI self-test, v0.0.25 UI-state self-test, and X11 identity smoke pass.
- Installer contract/rollback test proves exact-base preflight, v24 executable restoration, v25 cleanup on rollback, user-shell launcher restoration, and protected boundaries.
- On the owner workstation, the installer also performs the full native libtorrent + real llama.cpp build, relative `$ORIGIN` AI RPATH verification, native self-tests, final root executable installation, and raw executable icon metadata readback after the final write.

## Preserved visual assets

- Concept-sheet N master SHA-256 remains `5d0239c7999a091bb4b60384b2953444a8e40a7644ca6e18dddac1cb69b00e66`.
- Concept-sheet quilt source SHA-256 remains `eea284cc42f48ea2184ff3ccf8c717c9b43bad10727efe4bbdeaa8c2c025ba21`.
- No v0.0.25 package payload replaces those accepted assets.

## Owner gates after install

1. In Stream, the redundant `STREAM / Online video: <provider>` line and cream/white provider box are gone; the five provider buttons sit directly on the tinted quilt, and selecting YouTube/Rumble/RuTube/VK/OK still changes the page quilt/common controls and leaves a downward notch on the selected provider.
2. In Discover, selecting for example `Random` plus `Local Movie` shows both active notches at the same time.
3. `Play in Nougat...` successfully starts a real local movie or episode in Nougat's native player, including the previously reported local Discover path.
4. Existing concept-sheet N identity and other accepted v0.0.24 behavior remain correct.
