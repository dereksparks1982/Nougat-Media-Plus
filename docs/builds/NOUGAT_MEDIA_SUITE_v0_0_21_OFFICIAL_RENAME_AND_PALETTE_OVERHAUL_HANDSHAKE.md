# Nougat Media Suite v0.0.21 Identity, Palette, Search/P2P, and Library View Repair Handshake

## Project and version

- Historical project identity through accepted v0.0.20: ReddMedia.
- New official visible identity: **Nougat Media Suite**.
- Required accepted base: commit `c3d2c60e5c36407b96a0eba72e2863f884aacd28` / tag `v0.0.20`.
- Accepted base snapshot: `$HOME/DKLab/Archives/ReddMedia Archive/ReddMedia_v0.0.20_ACCEPTED_20260821_004237`.
- Target: v0.0.21 candidate.
- Target root executable: `Nougat_Media_Suite_v21`.
- Status at handoff: candidate for owner testing only; not accepted, committed, tagged, or pushed.

## Owner-approved scope

- Rename the visible application identity from ReddMedia to **Nougat Media Suite**.
- Rename the top-level **Nougat** tab to **Search** while preserving decentralized search behavior.
- Remove top-level **P2P** and move the media/torrent P2P screen under Search so ordinary Search navigation is **Search | Crawler | P2P**.
- Place decentralized search peer/node administration behind the smaller **Network...** advanced surface instead of a normal Peers tab.
- Replace the old red-tree active identity with the owner-approved rounded-square chocolate/nougat **N + play triangle** icon across launcher, X11/dock/app-switcher identity, raw executable metadata, and in-app version badge.
- Replace common suite chrome with chocolate/cocoa/cream/caramel identity and make each top-level tab carry its own palette identity.
- Use chocolate/cocoa/caramel for Video Player, forest/sage for Library, plum/lavender for Discover, nougat/caramel for Search, graphite/amber for Debug, and service-reactive Stream colors.
- Stream remains limited to YouTube, Rumble, RuTube, VK, and OK in v0.0.21.
- Replace `Grid [x]` / `List [x]` with working List (three lines) and Grid (2x2 squares) icon buttons immediately after the Library heading, preserving independent Movies/TV persistence.
- Preserve accepted v0.0.20 media/server/recommendation/playback behavior outside these approved UI organization changes.


## Deferred work

Additional Stream services, Jelly/Unity candy-button sprites, and every other post-rebrand feature remain for v0.0.22. The v0.0.21 installer does not rename the Git working directory or migrate backward-compatible runtime/config/user-data paths.

## Changed-file package

`Nougat_Media_Suite_v0_0_21_IDENTITY_PALETTE_SEARCH_P2P_LIBRARY_VIEW_REPAIR_CHANGED_FILES_ONLY.zip`

## Rollback

The installer snapshots every touched/deleted tracked v0.0.20 path, the accepted `ReddMedia_v20` root executable, and the current ReddMedia user launcher/icon before applying the candidate. Failure after application begins restores the accepted v0.0.20 state while preserving generated Jellyfin/AI runtimes, the pinned Nomic model, Search data, media, and user configuration.

## Validation

- Exact accepted v0.0.20 commit/tag/root executable and accepted snapshot preflight.
- Exact package manifest payload/base hash proof.
- Retained Nougat Search engine/bridge behavior.
- Retained recommendation, watch-provider, metadata, diagnostics, viewing-history, TV-autoplay, media-server lifecycle, volume, Library Grid/List, and custom-text behavior.
- v0.0.21 identity/palette/navigation contract, exact six-tab top navigation, Search `Search | Crawler | P2P` organization, working Library view-icon contract, and exact Stream service-set gate.
- C++17 `-Wall -Wextra -Werror` deterministic stub build.
- Bounded X11/Xvfb live-window smoke for `Nougat Media Suite`.
- Owner-machine full libtorrent + pinned llama.cpp build and real Discover AI model self-test.
- Final root `Nougat_Media_Suite_v21 --version` gate.
- Approved Nougat Media Suite raw-executable custom-icon assignment/readback after final executable write.

## Continuation point

Owner installs and visually tests v0.0.21. Only explicit owner acceptance allows the accepted snapshot, local commit/tag, and GitHub push sequence.
