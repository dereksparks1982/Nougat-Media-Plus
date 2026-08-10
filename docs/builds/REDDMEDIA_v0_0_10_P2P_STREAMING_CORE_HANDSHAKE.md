# ReddMedia v0.0.10 P2P Streaming Core Build Handshake

## Project and version

- Project: ReddMedia
- Version: v0.0.10
- Build title: P2P Streaming Core
- Package: `ReddMedia_v0_0_10_P2P_STREAMING_CORE_STABILITY_REPAIR_CHANGED_FILES_ONLY.zip`

## Git anchor and current repair base

- Accepted Git anchor remains ReddMedia v0.0.9.
- Accepted commit: `c2e3ebf56f0078292047c5feec25a1dd39eb50f2`
- Accepted tag: `v0.0.9`
- Branch: `main`
- The repair installer may be applied over the exact previously-installed v0.0.10 candidate while Git HEAD still points at the accepted v0.0.9 anchor.
- The previous v0.0.10 candidate package SHA-256 used as the repair-base authority is `7210525aacd3a48127354a71b47f150963d33366fcc2a192bfddb8ca187b6f0c`.

## Completed v0.0.10 product work

- Permanent built-in P2P screen.
- Magnet-link loading.
- Local `.torrent` loading.
- Torrent metadata and file list.
- Automatic obvious-video selection and manual file selection.
- Download folder selection.
- Torrent progress, download/upload rates, peers, seeds, state and name.
- One stream-first playback path through VLC.
- Localhost-only HTTP Range bridge.
- Playback-aware libtorrent time-critical piece priorities.
- Full torrent download continues behind playback and can seed after completion.
- P2P resume-state persistence under `~/.config/reddmedia/p2p/`.
- Ctrl+A and Cut / Copy / Paste in the P2P source field.
- Dependency documentation and libtorrent licensing records.
- Self-contained Linux distribution target recorded in the roadmap.

## Same-version stabilization repairs

- Restored buffered offscreen repainting for seek/time partial updates so the row is copied onscreen atomically instead of visibly erased and redrawn.
- Restored the same buffered protection for volume partial updates to preserve the earlier no-flash behavior.
- Added stream-request generations so a newer VLC HTTP range request supersedes obsolete P2P stream workers after a seek.
- New stream requests clear obsolete libtorrent piece deadlines before prioritizing the newly requested playback region.
- Added legal HTTP suffix-byte-range support for requests such as `Range: bytes=-5000000`.
- Added bounded stream-socket send/receive waits so abandoned seek connections cannot remain blocked indefinitely.
- Rebuilt the README release history from the actual repository records so v0.0.1 through v0.0.10 each explain their purpose and user-visible changes.
- Reapplied the red-triangle custom icon metadata to `ReddMedia_v10` and made successful icon verification an installer gate.

## Project files changed by this repair package

- `CHANGELOG.md`
- `DEPENDENCIES.md`
- `README.md`
- `ROADMAP.md`
- `docs/builds/REDDMEDIA_v0_0_10_P2P_STREAMING_CORE_HANDSHAKE.md`
- `docs/builds/REDDMEDIA_v0_0_10_P2P_STREAMING_CORE_VALIDATION.md`
- `src/main.cpp`
- `src/p2p_stream_server.cpp`
- `src/p2p_stream_server.hpp`

Generated/rebuilt during installation:

- `ReddMedia_v10`

## Risk controls

- Installer verifies the exact v0.0.9 Git anchor and either the exact previous v0.0.10 candidate repair-base hashes or the clean accepted v0.0.9 source state.
- Complete build/runtime dependency preflight happens before mutation.
- Real libtorrent target compiles in a temporary candidate tree before mutation.
- Compile policy remains C++17 with `-Wall -Wextra -Werror`.
- Candidate must report exactly `ReddMedia v0.0.10`, be an ELF executable, and link to libtorrent-rasterbar.
- HTTP stream server remains bound only to `127.0.0.1`.
- Package inventory and payload SHA-256 hashes are sealed.
- Post-install source/payload hashes, generated binary identity, changed-path scope, and red-triangle executable icon metadata are validated.
- Failure after mutation restores the exact pre-repair v0.0.10 candidate when repairing in place; a fresh install failure restores accepted v0.0.9.

## Rollback point

- Accepted authority: ReddMedia v0.0.9 at `c2e3ebf56f0078292047c5feec25a1dd39eb50f2`.
- For an in-place same-version repair, the installer additionally snapshots the exact current v0.0.10 candidate paths before mutation and restores them on failure.

## Validation plan

1. Complete dependency preflight including `gio` for executable icon assignment.
2. Git anchor/tag/branch verification.
3. Exact previous-candidate or accepted-base source/hash verification.
4. Package inventory and payload-hash verification.
5. Real libtorrent native pre-mutation compile.
6. Exact executable version, ELF, and libtorrent-link identity.
7. Isolated deterministic HTTP Range tests including ordinary range, suffix range, invalid range, and seek-request supersession.
8. Static source checks for buffered seek repaint, stale-request cancellation, deadline clearing, and loopback-only binding.
9. Install repaired payload and rebuilt `ReddMedia_v10`.
10. Apply and verify red-triangle executable custom icon metadata.
11. Exact installed payload, generated binary, and changed-path verification.
12. Owner test of local playback seek-row stability and P2P forward/backward seeking while the torrent is incomplete.

## Owner validation already proven before this repair

- Magnet-link intake and torrent downloading: PASS.
- Local `.torrent` intake and torrent downloading: PASS.
- Torrent metadata/file display and automatic video selection: PASS.
- Playback while the torrent was still incomplete: PASS.

## Continuation point

After this repair installer reaches FINAL PASS, Derek performs the visual/seek stability test. If accepted, create the accepted v0.0.10 snapshot first, then local Git commit/tag, then GitHub closeout.
