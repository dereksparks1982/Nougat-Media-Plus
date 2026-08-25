# Nougat Media Suite v0.0.41 rejected-build repair v5 validation

Status: **candidate repair only, owner acceptance required**

Base Git commit: `5c8d148b995b84cb96628cec472be514873fd399` (accepted v0.0.40 documentation HEAD)
Repair base: the exact rejected v0.0.41 repair-v4 working state after the owner-approved root cleanup.

## Owner-reported defects repaired

1. Movies and TV Library grid cards were visibly squeezed smaller and out of alignment in the rejected v0.0.41 candidate.
   - The repair is at the shared `library_grid_metrics()` source used by both Movies and TV.
   - The existing 2:3 poster ratio, 50px metadata footer, 8px inter-card gap, drawing/scrolling/keyboard path and responsive window behavior are retained.
   - Height-driven shrinking below the stable 140px card family is removed. A card shrinks below that only when the viewport itself is narrower than one card.

2. IMDb links were incomplete.
   - Exact Jellyfin `ProviderIds.Imdb` data continues to be parsed and cached.
   - `IMDb` is shown for exact `tt` + digits IDs on Movie, TV Series/Episode, Home LOCAL and fully visible Continue Watching cards.
   - The IMDb click target is separate from the card's normal Open/Play/Resume target.
   - No title-text guessing is used.

## Explicit exclusions

No unrelated Library redesign, Home geometry redesign, player redesign, Live TV redesign, Search redesign, Games work, web-viewer implementation, artwork overhaul, licensing change, or version advance is part of repair v5.

## Required validation gates

- Changed-files package SHA/byte manifest verifies before any project modification.
- Exact branch/HEAD and exact known rejected-v41 working-state preflight.
- Existing single-root-executable/root-cleanliness state must be present before install and remain present after install.
- Candidate is reconstructed from the exact v0.0.40 Git archive, then the v0.0.41 patch + repair v5 is applied in a temporary tree.
- CMake Release build uses `-Wall -Wextra -Werror` from the project.
- `tools/test_nougat_media_suite_v41.py` source/behavior contract.
- `--v41-library-imdb-repair-self-test` verifies the shared Movies/TV grid and exact IMDb ID behavior at runtime.
- Retained native self-tests v35 through v39.
- PolyForm/license-boundary regression test.
- Final root contains exactly one `Nougat_Media_Suite_v*` executable: `Nougat_Media_Suite_v41`.
- No versioned `NougatMediaSuite_v*.desktop` files may remain in the project root or user launcher directory.
- Both canonical launchers must point to `Nougat_Media_Suite_v41`.
- Approved Nougat N `metadata::custom-icon` is set and read back after the final executable write.
- No commit, tag, or push occurs in the installer.

## Owner visual checks still required

- Movies Grid: card size, spacing, 2:3 poster proportions and alignment are back to the intended larger card family.
- TV Grid: same geometry as Movies, with no separate drift.
- IMDb text appears where an exact IMDb ID is present on Movies, TV and Home cards.
- Clicking IMDb opens the exact `https://www.imdb.com/title/tt.../` page without triggering the card's normal media action.

Until those checks are accepted by the owner, v0.0.41 remains **REJECTED / candidate repair**.
