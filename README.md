# Nougat Media Suite

Nougat Media Suite is an all-in-one native Linux media center built by Elderred Softworks LLC for personal media libraries, internet streaming, television, radio, games, search, downloading, media tools, servers, diagnostics, hardware integration, and private local-network media access through one unified Nougat interface.

## Current Status

**Current development checkpoint:** v0.0.55  
**Current owner-accepted release:** v0.0.54

v0.0.55 is an in-progress checkpoint. It is not yet an accepted release because the Web Player server-state indicator and Safari/iPhone LAN access still require repair.

The complete runnable Nougat project tree is stored in this repository, including the bundled emulator/runtime trees, Xenia material, Jellyfin components, AI components, security components, assets, tools, source, and other project dependencies. Large files are stored through Git LFS so the real runtime material remains part of the project instead of being omitted.

Old transfer/checkpoint packaging archives are not part of the runnable repository.

## v0.0.55 - Background Web Player and Complete Runnable Project Checkpoint

v0.0.55 advances Nougat's private-LAN media architecture and preserves the complete runnable project on GitHub.

### Web Player

- Adds the Nougat-branded LAN Web Player for browser access from computers, phones, tablets, televisions, consoles, and other devices on the same private network.
- Uses **port 8096** for the public/LAN Nougat Web Player.
- Uses **127.0.0.1:8098** for the hidden Jellyfin backend.
- Keeps Jellyfin behind Nougat as backend/catalog infrastructure instead of exposing the stock Jellyfin web interface as the Nougat user experience.
- Runs the Web Player as a Nougat-owned background service so the desktop Nougat application does not need to remain open for local browser access.
- Keeps local media playback and local server access independent of mandatory cloud accounts, cloud relays, or an Internet round trip.
- Adds separate health handling for the Nougat Web Player and hidden Jellyfin backend.
- Advances the browser layout for the owner's **1366x768 display at 100% browser zoom**.
- Repairs stale player overlay/status text that could remain painted over active video.
- Uses the five-control fullscreen transport layout **`<<  <  ^  >  >>`** for rewind 10 seconds, previous item, play/pause, next item, and forward 10 seconds.

### Verified v0.0.55 Service State

During owner testing:

- `http://127.0.0.1:8096/nougat/v1/health` returned **HTTP 200 OK** from the Nougat Web Player.
- `http://127.0.0.1:8098/health` returned **HTTP 200 OK / Healthy** from the hidden Jellyfin backend.
- The Nougat Web Player listened on loopback and the active LAN address.
- Jellyfin remained restricted to the hidden loopback backend port.

### Known Unresolved v0.0.55 Issues

1. **Server indicator remains yellow / transitioning**
   - Both the Nougat Web Player and hidden Jellyfin backend have independently returned healthy responses.
   - The desktop readiness/state logic is still failing to promote the visible server state to Ready/green.

2. **Safari/iPhone LAN access remains unresolved**
   - Desktop-to-iPhone LAN reachability was verified.
   - Nougat is listening on the desktop LAN address on port 8096.
   - A packet capture during an iPhone Safari attempt captured no TCP traffic from the phone to port 8096.
   - The root cause remains unresolved.

Because these defects remain open, **v0.0.55 is a checkpoint, not an accepted release**.

## Complete Runnable Repository

Nougat is intentionally stored as a complete runnable project rather than a source-only skeleton.

The repository includes the project material needed for the current Nougat environment, including:

- Nougat application source and assets
- current versioned Nougat executable
- desktop launcher files and approved Nougat N identity assets
- Jellyfin backend components
- Web Player components
- Xenia/Xbox 360 runtime and source material used by Nougat
- emulator runtime material for supported Games systems
- AI runtime material
- security-analysis runtime material
- media-server and LAN components
- Live TV, World TV, Radio, Search, P2P, Studio, Games, Library, Player, Home, Discover, and System source
- project tools, tests, validation records, licensing records, and documentation

Large files are represented through Git LFS because GitHub does not accept very large ordinary Git objects. Git LFS is a repository storage mechanism and is not a runtime dependency of Nougat itself.

## Running the Current Development Checkpoint

From the Nougat Media Suite project folder:

```bash
./Nougat_Media_Suite_v55
```

Version check:

```bash
./Nougat_Media_Suite_v55 --version
```

Expected version identity:

```text
Nougat Media Suite v0.0.55
```

v0.0.55 remains an in-progress checkpoint. The current owner-accepted release remains v0.0.54 until v0.0.55 passes owner testing.

## Core Nougat Areas

### Home

Home is the default media landing surface and includes Continue Watching, local media shelves, recommendation groups, persistent viewing history, resume state, artwork, and hover-preview behavior.

### Video Player

Nougat's native player uses libVLC for local and supported network playback. It includes play/pause, stop, rewind and fast-forward, timeline seeking, volume, fullscreen, resume support, keyboard and mouse controls, subtitle and audio-track handling, episode progression, Up Next behavior, and Live TV/World TV identity handling.

### Library

Library manages Movies and TV with persistent folders, Jellyfin-backed catalog data, artwork, metadata, collections, series/seasons/episodes, search, grid/list presentation, playback history, and native playback through Nougat.

### Discover

Discover provides local and external media discovery, recommendation-style browsing, local viewing-history integration, TMDb-backed external metadata where configured, and provider availability information where supported.

### Live TV

Live TV supports over-the-air tuner discovery, scanning, channel storage, guide data, native playback, tuner ownership/state handling, diagnostics, channel artwork, WinTV devices, and HDHomeRun network tuners including multi-device and multi-tuner enumeration.

### World TV

World TV provides internet-accessible television sources with source verification, guide integration, playback-health diagnostics, bounded resolution/startup behavior, station identity, and truthful failure reporting.

### Radio

Radio provides the Nougat receiver environment for AM, FM, shortwave, weather radio, DAB/DAB+, DRM, internet radio, SDR, favorites, recordings, direct frequency entry, band plans, modulation controls, gain/AGC, squelch, scanning, signal monitoring, presets, and expandable receiver backends.

### Search

Nougat Search combines privacy-first search work, crawler infrastructure, decentralized search architecture, Secure Search protections, replaceable privacy transports, query isolation, private-retrieval research, and integrated P2P workflows.

### P2P

Nougat supports magnet links and local torrent metadata, playable-file selection, stream-while-downloading playback, seek-aware piece priority, persistent resume data, transfer controls, and security-analysis integration.

### Games

Games provides a unified game library and emulator front end with persistent ROM folders, archive-contained game discovery, system recognition, artwork, controller support, automatic emulator selection, and embedded emulator hosting where supported.

Nougat does not bundle copyrighted commercial ROMs or game images. Users provide their own legally obtained game dumps and any required firmware or BIOS.

### Xbox 360 / Xenia

The accepted Xbox 360 path uses the native Linux Xenia Edge runtime. Owner testing verified GTA IV launching through Nougat with game audio and rendered video inside Nougat's existing native player viewport without opening a separate emulator window.

v0.0.55 preserves this accepted behavior and does not intentionally modify the protected Xenia runtime work while the Web Player is being repaired.

### Studio

Studio is Nougat's media-production area. The current professional File Splitter supports source analysis, automatic piece recommendations, manual piece counts, asynchronous splitting, reassembly, verification, SHA-256 integrity checking, progress reporting, cancellation, and large-file streaming rather than loading an entire source into memory.

### System and Diagnostics

Nougat includes server controls, tuner controls, evidence-based diagnostics, hardware reporting, security-analysis status, media-server status, Live TV state, Library health, and report/export infrastructure.

## v0.0.54 - Professional File Splitter and Silver Screen Studio

Nougat Media Suite v0.0.54 is the current owner-accepted release.

- Promotes File Splitter from a prototype into a professional Studio tool under **Studio -> Tools -> File Splitter**.
- Adds in-page **Add File**, **Add Folder**, **Add ZIP / Manifest**, and **Choose Location** controls.
- Analyzes selected sources and recommends balanced piece counts while still allowing manual selection.
- Adds asynchronous Split/Reassemble/Verify operations, live progress, Stop, SHA-256 verification, cancellation cleanup, and compatibility with earlier Nougat split manifests.
- Gives Studio its Silver Screen identity with silver Studio/File Splitter treatment and film-strip presentation.
- Preserves the accepted v0.0.53 Xbox 360 embedded-video path, native player geometry, real libtorrent P2P support, local llama.cpp AI runtime, and the rest of the accepted Nougat feature set.

## v0.0.53 - System Expansion and Xbox 360 Embedded Video

- Expanded World TV and HDHomeRun handling.
- Expanded Games emulator discovery and mappings.
- Added LAN Viewer backend contracts.
- Added Child Safe Controls configuration/password protection, OSV-backed advisory inventory, and NOAA/NWS alert foundations.
- Repaired Linux process and application identity.
- Completed the owner-tested Xbox 360 embedded-video path through Xenia Edge.

## v0.0.52 - Radio Receiver Expansion and Hardware Foundation

v0.0.52 expanded the native Radio backend, hardware capability handling, receiver controls, SDR foundations, receiver discovery, multiple device/tuner support, scanning, favorites, recordings, signal monitoring, and experimental WinTV-HVR-955Q / Silicon Labs Si2157 FM support work.

## v0.0.51 - Radio, Live TV, World TV, Games, File Splitter, and Nougat Identity

v0.0.51 expanded Radio, HDHomeRun support, World TV, Games, Studio/File Splitter work, top-level navigation, Nougat identity, controller architecture, and LAN Web Viewer foundations.

## v0.0.50 - File Splitter, Unified Tuners, and Studio Foundation

v0.0.50 added the initial File Splitter implementation, HDHomeRun discovery/tuner-provider support, FLEX DUO multi-tuner handling, and the Studio foundation.

## v0.0.49 - Games Runtime, Artwork, ZIP Library, and World TV Repair

v0.0.49 expanded Games runtime management, Atari support, embedded emulator handling, artwork matching, Sega ZIP discovery, DOS package handling, and World TV playback/source repair.

## v0.0.48 - Embedded Emulation, DOS, Xbox 360, and Atari Repair

v0.0.48 established the shared embedded emulator host, DOSBox Staging support, Xbox 360 Xenia discovery/runtime handling, emulator-window containment, and Atari recognition repair.

## Dependencies

See [`DEPENDENCIES.md`](DEPENDENCIES.md) for exact build/runtime requirements and dependency notes.

Nougat also bundles or carries project-local components where required by its current architecture. Third-party components retain their own licenses and notices.

## Licensing

Nougat Media Suite Original Materials are made available under the **PolyForm Noncommercial License 1.0.0**. Elderred Softworks LLC retains all rights not granted by that license, including commercial use and separate commercial licensing of its own Original Materials.

See:

- [`LICENSE`](LICENSE)
- [`COPYRIGHT.md`](COPYRIGHT.md)
- [`CONTRIBUTING.md`](CONTRIBUTING.md)
- [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)
- [`docs/LICENSING_POLICY.md`](docs/LICENSING_POLICY.md)
- [`licenses/`](licenses/)

## Roadmap

See [`ROADMAP.md`](ROADMAP.md) for planned Nougat Media Suite work.
