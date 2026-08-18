# ReddMedia v0.0.16 Native Library and Discover AI Handshake

## Accepted base

- Required branch: `main`
- Required commit: `d67cf6e5e0e3ce3036adae5d9695147a7aa771e8`
- Required executable response: `ReddMedia v0.0.15`
- Accepted fallback snapshot: `ReddMedia_v0_0_15_TECHNICALLY_WORKING_20260818_133901`
- Snapshot executable SHA-256: `aab6981418c8fc8da0640a30be74267d4fbe98b4d2d7ecbd42fbe45330706e6d`

## Exact build scope

1. Replace the temporary flat Library file list with separate Movies and TV entry points.
2. Show real catalog titles only; missing artwork uses a text tile, never fake artwork.
3. Movies expose metadata-created box sets and then films.
4. TV exposes series, seasons, and episodes in that order.
5. Movies and TV each support multiple linked folders and non-destructive unlinking.
6. All local playback uses the existing embedded ReddMedia player.
7. Add one top-level Discover tab with inner Usual and Random modes.
8. Both modes expose exactly Local Movie, Local TV, External Movie, and External TV.
9. Return exactly one recommendation per request.
10. Usual uses private SQLite history and local Nomic embeddings; Random does not use history.
11. External uses TMDb, removes owned titles, requests a token on first use, and stores it mode `0600`.
12. Advance the active executable/desktop identity to `ReddMedia_v16` with the red tree present.
13. Build only llama.cpp's required shared-library target.
14. Stop and reap the owned hidden catalog on normal close and use a parent-death safeguard for forced close.

## Explicit exclusions

- No Favorites, Trailers, Genres, or Collections tabs.
- No combined Movies and TV choice.
- No combined Local and External choice.
- No Branch Out control in this build.
- No free-text prompt box.
- No sample, placeholder, or invented movie/TV entries.
- No browser, Jellyfin player, or external video player.
- No live TV/DVR work in v0.0.16.

## Pinned AI inputs

- llama.cpp commit: `9731ad3f29da96f588711a0d1eb08cf210721e16`
- llama.cpp archive SHA-256: `8dc808f9e0166c7fe9f5ec73884392d528c1198fd2ce89f0d60971d7d55ae998`
- Nomic GGUF revision: `0188c9bf409793f810680a5a431e7b899c46104c`
- Nomic Q4_K_M model SHA-256: `d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac`

Any behavior outside this scope requires a later build and explicit owner approval.
