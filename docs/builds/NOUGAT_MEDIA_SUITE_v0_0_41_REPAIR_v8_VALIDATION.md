# Nougat Media Suite v0.0.41 Repair v8 Validation

Date: 2026-08-24
Status: **candidate for owner visual/functional acceptance**
Version: **v0.0.41** (no version bump)
GitHub baseline: accepted v0.0.40 commit `5c8d148b995b84cb96628cec472be514873fd399`
Promotion rule: v0.0.41 remains local/unreleased until owner acceptance.

## Frozen repair scope

Repair v8 folds the final v0.0.41 stabilization list into one candidate without redesigning working areas.

- Library Movies/TV only: replace the one-row regression with a height-aware multi-row 2:3 poster grid, center the used columns, preserve shared Movies/TV geometry, vertical scrolling, exact IMDb actions, and deterministic card-hover repainting. **Home card geometry is not changed.**
- Live TV: retain the eight-control inner action-strip scroll extent through Record. Global top navigation is not part of this repair.
- Video Player: use the same stitched full-page shell as the other main tabs; atomically compose Continue Watching / stopped prompt screens to stop pointer-driven flashing and disappearing button labels; retain the single authoritative 3000 ms pointer/media-identity activity timer.
- Discover: move the JustWatch/TMDb attribution upward so it clears the lower stitch.
- Search > Network: remove the redundant floating `NETWORK / ADVANCED` heading and keep the status/footer line inside the stitched peer panel.
- Search > Archive: support an optional official onion target per archive entry and render actions in `Tor` then `Open Site` order. Internet Archive is populated with its current official `Onion-Location`; no onion addresses are guessed for other archives.
- Preserve all unrelated v0.0.41 behavior, including exact Jellyfin `ProviderIds.Imdb` handling.

## Internet Archive Tor target

Verified current official onion target used by this candidate:

`https://archivep75mbjunhxc6x4j5mwjmomyxb573v42baldlqu56ruil2oiad.onion/`

The UI only renders a Tor action when an archive entry has an explicit verified onion target.

## Validation completed in the build environment

- v0.0.41 Python source contract: **PASS**
- Fresh CMake stub configuration: **PASS**
- Full C++ stub compile/link with project `-Wall -Wextra -Werror`: **PASS**
- v0.0.41 contract against rebuilt stub executable: **PASS**
- `--v35-cleanup-self-test`: **PASS**
- `--v36-library-ui-player-self-test`: **PASS**
- `--v37-live-tv-system-self-test`: **PASS**
- `--v38-library-live-tv-player-self-test`: **PASS**
- `--v39-diagnostic-self-test`: **PASS**
- `--v41-library-imdb-repair-self-test`: **PASS**
- PolyForm/owner-rights/third-party/contributor license gate: **PASS**

The uploaded split project archive omitted the hidden top-level `.github` directory. For build-environment license validation only, the accepted v0.0.40 `.github/PULL_REQUEST_TEMPLATE.md` was restored from the accepted GitHub commit. It is not part of this repair payload and is not promoted by the installer.

## Native-build limitation in this environment

A full non-stub configure was attempted and stopped before compilation because this build container does not provide the native `libtorrent-rasterbar` development package. The uploaded llama runtime also targets a newer glibc than this container. These are build-environment limitations, not accepted runtime proof.

**Repair v8 therefore fails closed on the owner machine:** the installer builds a temporary full native v0.0.41 candidate with warnings as errors and runs the retained validation suite before it changes any active project source or executable. If that native gate fails, nothing is promoted. If any post-promotion gate fails, the installer restores its preinstall snapshot.

## Owner acceptance still required

Automated tests cannot prove the visual behavior of X11/libVLC on the owner's desktop. Before v0.0.41 is accepted or pushed, visually verify at minimum:

1. Library Movies and TV show multiple poster rows at the normal window size and scroll vertically.
2. Home cards look unchanged.
3. Library hover descriptions enter/leave/switch without stale content.
4. Live TV inner controls can scroll through Record.
5. Moving the pointer over Continue Watching / player prompt screens no longer flashes or erases labels.
6. Video Player has one stitched frame around the full tab content.
7. Discover attribution clears the bottom stitch.
8. Search > Network has no floating heading and its bottom status stays inside the stitched panel.
9. Search > Archive shows `Tor` then `Open Site` for Internet Archive, while entries without a verified onion show only `Open Site`.
10. IMDb links still open the exact Jellyfin-provided IMDb title ID.

No commit, tag, GitHub push, release, or version bump is performed by this repair package.
