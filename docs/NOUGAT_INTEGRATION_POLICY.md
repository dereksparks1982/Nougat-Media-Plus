# Nougat Integration Policy

Nougat is the decentralized-search subsystem inside ReddMedia beginning with v0.0.19. The standalone Nougat v0.0.1 prototype is historical reference only and is not a runtime dependency.

## User-facing identity

- ReddMedia remains the application identity.
- Top-level order: **Video Player | Library | Discover | Nougat | YouTube | P2P | Debug**.
- Nougat uses its approved candy-bar palette inside its own content surface.
- ReddMedia's existing red-tree executable, launcher, and window identity remain unchanged.

## Search behavior

- **Ranked** orders matching records by the local search score.
- **RAW** exposes matching local/peer records in index-return order and does not add a SafeSearch/content-suppression layer.
- Result organization is separate from result membership.
- Current v0.0.19 peer search is direct peer-to-peer querying. Automatic global peer discovery and distributed sharding are future work, not claimed by this candidate.

## Data ownership

Integrated data is stored under `~/.local/share/reddmedia/nougat/`, including the local SQLite index, node identity, and peer list. It does not depend on `$HOME/DKLab/Projects/Nougat`.

## Crawler output

Crawler output is read-only but remains normal selectable information. Mouse selection, Ctrl+C, Ctrl+A, right-click Copy, and right-click Select All are required behavior.

## Licensing

Original ReddMedia/Nougat code is governed by the repository `LICENSE`. Separately licensed dependencies remain governed by their upstream licenses and notices.
