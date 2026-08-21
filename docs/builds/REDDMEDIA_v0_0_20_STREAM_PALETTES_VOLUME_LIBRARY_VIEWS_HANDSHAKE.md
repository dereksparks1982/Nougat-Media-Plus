# ReddMedia v0.0.20 Stream / Palettes / Volume / Library Views Handshake

## Project and version

- Project: ReddMedia
- Required accepted base: commit `e1cf0aa1ca55757c8048e5f142f9c1d92f23cb26` / tag `v0.0.19`
- Target: v0.0.20 candidate
- Target root executable: `ReddMedia_v20`
- Status at handoff: candidate for owner testing only; not accepted, committed, tagged, or pushed

## Owner-approved scope

- Preserve all accepted v0.0.19 ReddMedia behavior unless directly changed below.
- Top-level order: `Video Player | Library | Discover | Nougat | Stream | P2P | Debug`.
- Replace the former YouTube top-level area with Stream, containing YouTube, Rumble, RuTube, VK, and OK selectors.
- Preserve the existing URL Download and Play workflow and activity output. Add Direct Watch through ReddMedia's native player and Open Webpage through the user's default browser.
- Give every major tab a coordinated full interior palette: Video Player red/burgundy, Library forest/sage, Discover plum/lavender, Nougat candy-bar, Stream teal/cyan, P2P navy/steel, Debug charcoal/amber.
- Expand player volume to 0-200%, default 100%, with live percent and a visible 100% marker.
- Center the compact bottom player control group when sufficient width exists; preserve horizontal wheel scrolling when narrow.
- Add independent persistent Grid/List Library view choices for Movies and TV.
- Show a visible caret/focus state in custom editable fields.

## Explicitly deferred

- Poster/artwork quality and aspect-ratio overhaul.
- Rounded-corner visual polish.
- Shorter/balanced volume-bar geometry.
- Wide-window top-navigation centering.
- Local-only optional ReddMedia Web Player button.
- Plex account/server integration.

## Changed-file package

`ReddMedia_v0_0_20_ACCEPTED_V19_BASE_MANIFEST_REPAIR_CHANGED_FILES_ONLY.zip`

## Rollback

The installer snapshots the exact touched v0.0.19 files plus the root v0.0.19 executable/desktop launcher before applying the candidate. Failure after apply restores those paths and the user's prior launcher while preserving generated Jellyfin/AI runtimes, the exact pre-existing pinned model state, Nougat data, media, and user configuration.

## Validation

- Exact v0.0.19 commit/tag/base executable preflight.
- Package manifest byte/hash proof and exact touched-file base hashes.
- Retained Nougat engine/bridge behavior.
- Retained ReddMedia recommendation, watch-provider, metadata, diagnostics, viewing-history, TV-autoplay, and media-server lifecycle behavior.
- v0.0.20 UI contract: Stream order/platforms/actions, full palettes, 0-200% volume, centered bottom controls, Grid/List persistence contract, and caret markers.
- C++17 `-Wall -Wextra -Werror` deterministic stub build.
- Bounded X11/Xvfb live-window smoke.
- Owner-machine full libtorrent + pinned llama.cpp build.
- Final root `ReddMedia_v20 --version` and real Discover AI self-test.
- Approved red-tree raw executable icon assignment/readback after the final executable write.

## Continuation point

Owner applies and visually tests v0.0.20. Only explicit owner acceptance allows the accepted snapshot, local commit/tag, and GitHub push sequence.

## Accepted-v0.0.19 base-manifest repair

- Corrected the v0.0.20 package gate to use the final accepted `REDDMEDIA_PATCH_MANIFEST_v19.json` SHA-256: `26106d40b8878d08c7fb759217c6a56ad8c5221df1a732028261450c272f8a47`.
- No ReddMedia feature code changed in this repair.

- Packaging repair also ensures `tools/test_installer_rollback_v20.py` is actually applied before the installer invokes it.
