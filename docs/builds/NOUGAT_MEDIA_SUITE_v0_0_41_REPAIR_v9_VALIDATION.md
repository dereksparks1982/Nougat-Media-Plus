# Nougat Media Suite v0.0.41 Repair v9 Validation

Status: **candidate package validation complete; owner visual/functional acceptance still required**.

Repair v9 is the consolidated final v0.0.41 stabilization package built on the exact repair-v8 application source already installed by the owner. It deliberately does not introduce v0.0.42/Games work or alter approved Home card geometry.

## Application repair scope retained from repair v8

- Library Movies + TV use the shared Library-only adaptive multi-row portrait grid; Home card geometry remains separate and unchanged.
- Library card hover transitions repaint deterministically on enter, leave, and card-to-card motion.
- Live TV's inner eight-control action strip can scroll through Record without changing the global top tabs.
- Video Player uses the common stitched full-tab frame and atomic resume/stopped prompt composition to prevent pointer-driven blank/partial prompt buttons.
- The authoritative player activity timeout remains 3000 ms.
- Discover's JustWatch/TMDb attribution is raised clear of the bottom stitched border.
- Search -> Network removes the stray floating heading and keeps status/helper text inside its stitched panel.
- Search -> Archive renders `Tor` before `Open Site` for entries with a curated official onion target; Internet Archive is wired through the existing Nougat Tor open path.
- Exact Jellyfin `ProviderIds.Imdb` behavior remains preserved for eligible Library/Home cards.

## Repair v9 workflow consolidation

- `COMPANY_BIBLE.md` permanently requires install/repair/build/validation command blocks to start with safe Nougat + verified Nougat-owned Jellyfin shutdown.
- The repair-v9 installer itself performs that safe shutdown after package integrity verification and before preflight/build work.
- Jellyfin is never killed by process name alone. The installer requires Nougat ownership evidence: owner token, exact integrated runtime/signature, or the recorded owned PID tied to the exact runtime path.
- Independently started Jellyfin processes are left alone.
- The installer preserves the persistent-server preference; it stops the owned runtime for maintenance but does not disable future automatic restart when Nougat is reopened.
- No shell prompt/output text is embedded in the owner copy/paste install block.
- No commit, tag, GitHub push, release, or version bump is performed.

## Validation performed in the build environment

PASS:

- v0.0.41 source contract, retained v0.0.40 behavior, Archive Tor action, Library grid/hover, IMDb, Live TV inner scrolling, player frame/repaint/activity, Discover footer, Search Network layout, roadmap ordering, and safe-shutdown documentation gate.
- PolyForm Noncommercial / owner-rights / third-party boundary / contributor inbound license gate.
- Clean CMake configure + warnings-as-errors compile/link with the repository's isolated P2P and AI stub lanes.
- Runtime self-tests retained from v0.0.35, v0.0.36, v0.0.37, v0.0.38, v0.0.39, and v0.0.41.
- Root executable identity in the build lane: `Nougat Media Suite v0.0.41`.

The build environment does not provide `libtorrent-rasterbar`, so a full native non-stub configure cannot be completed here. That limitation is explicit rather than hidden. The repair-v9 installer requires a full native warnings-as-errors build on the owner's Ubuntu project before it promotes any active files. The owner's immediately preceding repair-v8 install already proved that exact application source builds natively with libtorrent 2.0.12 on the target machine.

## Owner visual/functional acceptance still required

After install, visually/functionally verify:

1. Library Movies and TV render multiple rows and vertical scrolling at the normal window size; Home cards remain unchanged.
2. Library hover descriptions clear/update immediately.
3. Live TV inner controls can wheel-scroll through Record.
4. Video Player pointer movement no longer causes resume/stopped prompt flashing or disappearing button labels, and the stitched frame surrounds the whole player tab.
5. Discover attribution clears the bottom stitching.
6. Search -> Network has no stray upper-left heading and its bottom status remains inside the stitched panel.
7. Search -> Archive shows `Tor` then `Open Site` for Internet Archive and opens each action through the intended path.

Only the owner decides whether v0.0.41 is accepted and ready for commit/tag/push/release.
