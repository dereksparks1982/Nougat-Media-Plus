# Nougat Media Suite Search Integration Policy

Beginning with v0.0.21, **Nougat Media Suite** is the application identity and the former top-level **Nougat** tab is labeled **Search**. The underlying decentralized Nougat engine remains an integrated subsystem; the archived standalone Nougat v0.0.1 prototype remains historical reference only and is not a runtime dependency.

## Identity and layout

- Application identity: **Nougat Media Suite**.
- Top-level order: **Video Player | Library | Discover | Search | Stream | Debug**.
- Search uses the suite's cocoa/chocolate/nougat-cream/caramel identity.
- The owner-approved chocolate/nougat **N + play triangle** icon is the active application identity.
- Ordinary Search subnavigation is **Search | Crawler | P2P**. Media/torrent P2P lives here instead of a top-level application tab.
- Ranked/RAW search, crawler, Tor-aware results, selectable/copyable output, and direct URL opening remain unchanged.
- Decentralized search peer/node administration remains available behind **Network...** inside Search instead of occupying a normal tab.

## Data compatibility

Integrated Search data remains under the existing backward-compatible `~/.local/share/reddmedia/nougat/` path in this identity-only version. v0.0.21 does not migrate user data or rename the Git working directory.

## License

Original Nougat Media Suite code is governed by the repository `LICENSE`. Separately licensed dependencies retain their upstream licenses.
