# BlastEm runtime source for Nougat Media Suite v0.0.49

Nougat's Sega Genesis / Mega Drive, Sega Master System, and Sega Game Gear backend is BlastEm.

Pinned upstream build:

- Project: BlastEm by Michael Pavone / RetroDev
- Upstream nightly index: https://www.retrodev.com/blastem/nightlies/
- Linux x86_64 asset: `blastem64-0.6.3-pre-8013468ed981.tar.gz`
- Published upstream: 2026-08-21
- Upstream directory-listed size: 6,053,889 bytes
- Revision: `8013468ed981`

The v0.0.49 installer validates the exact asset size, extracts it with path/link checks, runs `blastem -v` and requires the pinned revision, checks shared-library resolution, and records the downloaded SHA-256 in `components/games/runtime/blastem/UPSTREAM.txt`.

Nougat preserves the complete portable upstream package beneath `components/games/runtime/blastem/package/`, including the upstream COPYING/license materials and bundled libraries. The runtime wrapper forces X11 for Nougat's native embedded emulator host and keeps BlastEm's Nougat-specific XDG configuration/data separate from a user's standalone BlastEm configuration.
