# ReddMedia v0.0.15 Build Handshake

## Identity

- Project: ReddMedia
- Target: v0.0.15
- Build: Integrated Media Server Foundation repaired candidate
- Required accepted base: v0.0.14
- Required commit/tag: `0b1b27af587c06b15faf8edef70b2328b149761e` / `v0.0.14`
- Required branch/tree: `main`, clean
- Package: `ReddMedia_v0_0_15_INTEGRATED_MEDIA_SERVER_FOUNDATION_REPAIR_CHANGED_FILES_ONLY.zip`
- Rollback: accepted v0.0.14 commit/tag and accepted snapshot

## Completed candidate work

- Pinned the official stable Jellyfin Server and Jellyfin Web 10.11.11 Ubuntu 26.04 packages.
- Preserved the matching server/web source archives, GPL licenses, release commits, and SHA-256 identities.
- Replaced the rejected master-source Node/.NET build path with verified package extraction and atomic runtime installation.
- Added native ReddMedia service startup, loopback health supervision, bounded polling, failure status, and restart handling.
- Added persistent ReddMedia-owned server data/config/cache/log paths.
- Added visible `Server:` status and v0.0.15 identity.

## Validation truth

Package identities, hashes, archive structure, Jellyfin version output, initial `/health` response, shell syntax, and the isolated media-server manager warnings-as-errors compile passed in the packaging environment. That environment blocks the network-change socket Jellyfin needs for complete startup and also lacks CMake plus the complete X11/libVLC/libtorrent development stack. The installer therefore requires full server/web HTTP readiness and both native ReddMedia builds on Derek's Ubuntu 26.04 system before it can finish.

## Continuation point

Apply the repaired candidate only to the exact clean v0.0.14 base. Accept only after every installer phase ends in `FINAL PASS`, the ReddMedia top bar reaches `Server: Ready`, and `http://127.0.0.1:8096` opens the bundled server setup.
