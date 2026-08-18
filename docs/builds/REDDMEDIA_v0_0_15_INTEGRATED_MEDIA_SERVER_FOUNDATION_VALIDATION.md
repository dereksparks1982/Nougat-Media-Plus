# ReddMedia v0.0.15 Validation Record

## Builder-side completed checks

- Exact ReddMedia base commit/tag identity: PASS.
- Stable Jellyfin Server 10.11.11 Ubuntu 26.04 package identity and SHA-256: PASS.
- Stable Jellyfin Web 10.11.11 Ubuntu 26.04 package identity and SHA-256: PASS.
- Matching server/web source archives and release commits: PASS.
- Source ZIP structural tests and GPL license preservation: PASS.
- Extracted `Jellyfin.Server 10.11.11.0` version proof: PASS.
- Extracted server startup and `/health` loopback response: PASS.
- Complete startup plus API proof: blocked in the packaging sandbox by its prohibition on Jellyfin's network-change socket; required and enforced by the owner-machine installer.
- Media-server manager standalone `-Wall -Wextra -Werror` compilation: PASS.
- Server communication limited to the documented separate-process HTTP boundary: PASS by source inspection.
- Node/npm/.NET installer dependency removal: PASS.
- Package manifest and changed-path audit: PASS after final package seal.

## Required owner-side native checks

The superseding repair installer must show `PHASE START` and `PHASE PASS` for exact base/candidate preflight, package integrity, repaired-file application, stable Jellyfin package validation/extraction, native ReddMedia stub/full builds, hidden catalog/native API proof, desktop identity, and final source preservation. Acceptance requires the final `FINAL PASS`, visual `Server: Ready` confirmation, native Library listing, and playback inside the existing ReddMedia video surface. A visible Jellyfin setup/player page is a failure.
