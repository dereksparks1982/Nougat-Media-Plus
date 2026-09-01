# Nougat Media Suite v0.0.51 Candidate Scope

Base: accepted/published v0.0.50 commit `45f163752e5f1e0ed00f7d6d851bb6f6a5abf96e`.

## Candidate work

- Repairs the File Splitter around direct folder, normal file, and existing ZIP input; owner-selected output name; owner-selected piece count; mathematically derived minimum recommendation when an explicit maximum-piece setting requires it; `.zip.001` naming; SHA-256 verification; exact packaged reassembly.
- Repairs HDHomeRun physical-device presentation so one FLEX-class unit is one device card with nested tuner resources, makes detection/status provider-neutral, and separates completed RF traversal from helper/cleanup status.
- Rebuilds World TV around the Live-TV-style timeline guide geometry using World TV orange data/palette and improves direct-stream resolution tolerance/evidence; source verification stays off the X11 event thread and teardown does not synchronously wait on the long-running World TV network workers.
- Makes translucent rounded treatment a system-wide policy for transient player overlays rather than a World-TV-only exception.
- Adds the top-level Mulberry Radio foundation with AM, FM, Shortwave, Weather, DAB/DAB+, DRM, Internet Radio, SDR, Favorites, and Recordings divisions. Radio is an independent root view and does not map the Video Player/resume child merely by opening Radio. Hardware-dependent modes stay unavailable without a real supporting provider/device.
- Advances the LAN Web Viewer versioned endpoint foundation for health, catalog, history, artwork, media, HLS, Live TV, devices/session, pairing and browser UI while preserving LAN-only/no-cloud defaults.
- Replaces the old Nougat brand mark with the owner-approved v0.0.51 artwork: the exact N crop is the executable/window/launcher/sidebar icon, and the exact N + cursive `Nougat Media Suite` lockup is scaled into the top application bar.
- Corrects all current executable/diagnostic/header identity to v0.0.51, repairs the N alpha silhouette so both bottom corners outside the rounded badge are transparent, and makes successful promotion replace v50 rather than leaving old and new root executables side by side.
- Repairs the top-level horizontal navigation structurally: the tab list is one source of truth, the maximum scroll is derived from the rendered final System tab, and narrow-width regression checks require the complete last tab to be reachable.

## Controller roadmap locked by owner

Nougat will gain a unified Controller & Remote Input Framework for the entire app. Controller setup belongs in the System tab. D-pad/left-stick navigation, A/Cross Select, B/Circle Back/Cancel, tab/page actions, subtitles/audio/player controls, remapping, dead zones, sensitivity and controller testing feed a common action abstraction. UI, Video Player, Games and Drone Flight are separate contexts. Drone Flight owns flight axes exclusively while armed so those inputs cannot simultaneously navigate Nougat.

No Git commit/tag/GitHub push is part of candidate construction. Owner testing and acceptance come first.
