# Nougat Media Suite v0.0.25 Stream Selection and Discover Play Handshake

- Project: Nougat Media Suite
- Numeric version: v0.0.25
- State: owner-approved same-version candidate repair
- Required accepted Git base: v0.0.24
- Accepted base commit: `f66d35b671c9bceee6151dc63003dc3ec24578e8`
- Required installed base executable: `Nougat_Media_Suite_v24`
- Target executable: `Nougat_Media_Suite_v25`
- Package: `Nougat_Media_Suite_v0_0_25_STREAM_PROVIDER_PANEL_REMOVAL_REPAIR_CHANGED_FILES_ONLY.zip`

## Approved same-version owner-test repair

- Remove the redundant `STREAM` and `Online video: <provider>` text above the Stream provider selector row. The top-level Stream tab identifies the area and the provider downward notch identifies the selected service.
- Remove the now-unnecessary cream/white Stream provider container and its border so the five service selectors sit directly on the active provider-tinted quilt.
- Preserve the provider row position, provider colors, selected-provider notch, provider-driven quilt tint, Stream actions, Discover dual-selection notches, Discover local native-play repair, and every other v0.0.25 behavior unchanged.

## Approved scope

1. Make the selected Stream provider drive the Stream interior controls/accents and tint the exact accepted concept-sheet quilt: YouTube red, Rumble green, RuTube purple, VK blue, and OK orange/caramel.
2. Give the selected Stream provider the concept-sheet downward active notch.
3. Preserve the top-level active-tab notch and add persistent Discover selection notches for two independent groups: `Usual | Random` and `Local Movie | Local TV | External Movie | External TV`. One selection in each group may remain visibly active simultaneously.
4. Keep TMDb and service operations as ordinary action buttons without persistent selected notches.
5. Repair local Discover `Play in Nougat...` so a Jellyfin recommendation resolves to a real playable local movie or episode and then starts the native Nougat player.
6. For a local TV series recommendation, choose the most recently watched matching episode when local history identifies one; otherwise choose the first real episode in season/episode order.
7. Preserve accepted v0.0.24 icon/quilt assets, Search engine/bridge behavior, protected licensing, Library/P2P behavior, Crawler spacing, TV autoplay repair, and pointer-motion performance repair.

## Changed files

The candidate changes version/build plumbing, the main UI/playback orchestration source, the recommendation-engine history accessor, launcher aliases, release documentation, and v0.0.25 validation/install files. No protected license file, decentralized Search engine/bridge file, model, runtime, user data, or media file is changed.

## Rollback

For this same-version repair, installer rollback target is the exact rejected pre-repair v0.0.25 candidate state, including its existing `Nougat_Media_Suite_v25` executable. A failed repair restores that exact v0.0.25 candidate rather than rolling all the way back to v0.0.24.

## Continuation point

After installer FINAL PASS, owner testing must verify the redundant Stream/provider text and cream provider container are gone while the provider row sits directly on the tinted quilt and the provider-reactive colors/notch, simultaneous Discover selector notches, and real local Discover playback remain correct. The candidate remains unaccepted until the owner explicitly accepts it.

## Next planned work

After v0.0.25 acceptance, the next build is planned around mouse Back/Forward navigation plus owner-selected P2P feature expansion toward a BitTorrent Pro-class workflow; exact v0.0.26 scope still requires owner approval before implementation.
