# Nougat Media Plus

**Media Convert now integrated into Studio's existing Tools page (unreleased source candidate):** Nougat has a new Media Convert tool below File Splitter and File Assembler without moving either existing button. It uses the internal file browser, eight export presets, safe unique output naming, an asynchronous FFprobe preflight, and the persistent job queue with cancel/retry controls. See the [Media Convert Studio UI Source Review](docs/builds/NOUGAT_v0_0_69_STUDIO_MEDIA_CONVERT_UI_SOURCE_2026_10_30.md). Native compilation, real conversion and owner testing are still outstanding.

**Studio media inspection and service (unreleased source candidate):** FFprobe-based stream/duration inspection, advisory remux compatibility checks, track-aware export planning and a Studio service facade now feed the durable FFmpeg queue. The worker also has stalled-process safeguards and cancellation-before-publish protection. The approved Studio UI remains untouched and no native runtime acceptance has been claimed. See [Studio Inspection and Service Review](docs/builds/NOUGAT_v0_0_69_STUDIO_INSPECTION_SERVICE_SOURCE_2026_10_30.md).

**Studio media-processing engine (unreleased source candidate):** Added a native FFmpeg-backed job engine with eight fixed conversion/remux/audio presets, a persistent queue, safe exports, cancellation, retries, and atomic batch submission. The engine is wired into the existing native build source list but is not connected to the Studio UI yet. See [Studio Engine Source Review](docs/builds/NOUGAT_v0_0_69_STUDIO_ENGINE_SOURCE_CANDIDATE_2026_10_30.md). This is not a compiled or owner-tested release.

**Games runtime and emulator containment (unreleased source candidate):** The Games host now uses process ownership rather than arbitrary fresh desktop windows to identify emulator output; exit and cleanup are more explicit. Emulator discovery now distinguishes a detected binary from verified gameplay. See the [Games Runtime Source Review](docs/builds/NOUGAT_v0_0_69_GAMES_RUNTIME_SOURCE_CANDIDATE_2026_10_30.md) and [Backend Test Matrix](docs/GAMES_BACKEND_VERIFICATION_MATRIX_v0_0_69.md). The existing player UI stays in place. **No emulator backend has been newly accepted by this source work.**

**LAN-hosted first-run terms enforcement (source candidate):** The local media server now requires a server-recognized acknowledgement cookie before answering private media API routes, rather than relying solely on the browser overlay. The existing GitHub Pages bridge is separate and is not secured by this LAN-only change. See [LAN Terms Session Candidate](docs/builds/NOUGAT_v0_0_69_LAN_TERMS_SESSION_CANDIDATE_2026_10_30.md). Full browser/runtime verification remains outstanding.

**LAN streaming lifecycle development (unreleased):** Source-only improvements now track HTTP clients during shutdown, bound video stream startup and idle waits, and mark missing media unavailable in catalog responses. An opt-in LAN HTTP regression suite is included for future validation. See [LAN Lifecycle Source Candidate](docs/builds/NOUGAT_v0_0_69_LAN_LIFECYCLE_SOURCE_CANDIDATE_2026_10_30.md). **This is not a tested executable or release.**

**v0.0.69 LAN reliability source work (unreleased):** The local LAN player now has explicit routes for its configuration, legal notice, service worker and approved icon. Both browser interfaces defer library traffic until local notice acknowledgement, retain the last loaded library during brief outages, and use bounded requests and reconnect checks. The native LAN service improves byte-range validation and playback error reporting. This is committed source work, **not an owner-tested binary release**. See [LAN Reliability Source Review](docs/builds/NOUGAT_v0_0_69_LAN_RELIABILITY_SOURCE_REVIEW_2026_10_30.md).

**First-run responsible-use acknowledgement:** Before normal use of the native GUI or web viewer, Nougat asks users to review its [Legal Notice](LEGAL_NOTICE.md), check an initially unchecked acknowledgement, and select **I Understand & Agree**. Acceptance is saved locally per notice version. Rejecting once explains that acceptance is required; rejecting again exits the native app or removes access to the browser viewer. Without acceptance, neither interface can be used normally. The existing project and third-party licenses remain unchanged.

**Nougat Media Plus** is a native Linux media, entertainment, communications, games, production, networking, and system-tool platform developed by **Elderred Softworks LLC**.

It brings local media playback, movies and television libraries, Live TV, radio, internet media, games and emulation, search, P2P, production tools, diagnostics, security tooling, connected hardware, and a browser-based media player into one project.

🌐 **Nougat Web Viewer:** [https://dereksparks1982.github.io/Nougat-Media-Plus/](https://dereksparks1982.github.io/Nougat-Media-Plus/)

Use the web viewer from a phone, tablet, or desktop browser to access the Nougat media interface remotely while the Nougat media host is running.

## Web Player

### **[Open the Nougat Web Player](https://dereksparks1982.github.io/Nougat-Media-Plus/)**

The Nougat Web Player is the phone and browser companion for your Nougat library. The web interface is hosted through GitHub Pages while your actual movies and TV shows remain on your Nougat server.

The remote player uses the same HTTPS media-bridge architecture proven by the Hanafi web viewer. It supports:

- Movies and TV episodes from the Nougat library
- Phone, tablet, and desktop browsers
- Search and library refresh
- Direct browser playback when the source format is compatible
- Automatic browser-compatibility fallback for other formats
- HTTP Range playback for seeking and normal mobile video controls
- A mobile web-app manifest and installable browser experience
- No requirement to upload your personal media library to GitHub

For playback, the Nougat media host must be running and reachable. GitHub Pages supplies the interface only; the media stream comes from Nougat.

## Core Areas

### Media

Nougat provides native local and network video playback, resume state, subtitles, audio tracks, chapters, episode progression, Continue Watching, movie and television libraries, metadata, artwork, collections, and media discovery.

### Live TV & World TV

Live TV supports compatible over-the-air and network tuners, scanning, channels, guide data, station artwork, playback, tuner diagnostics, and related broadcast tooling. World TV provides an international streaming workspace for supported direct broadcaster and CDN sources.

### Radio & Signal Tools

Nougat includes radio, scanner, receiver, SDR, spectrum, and signal-monitoring foundations covering local broadcast, amateur, aviation, marine, public-safety, satellite, digital-radio, and related services where compatible hardware and lawful access are available.

### Games & Emulation

The Games area provides a unified game library, artwork, managed emulator integration, controller support, archive and ROM discovery, runtime management, and embedded or managed gameplay paths where supported by the backend.

Commercial game content, console firmware, BIOS files, and ROMs are not bundled.

### Search, P2P & Security

Nougat includes privacy-focused search and crawler infrastructure, P2P transfer and stream-while-downloading foundations, diagnostics, and on-demand security-analysis tooling. Security findings follow the project's **WARN ME FIRST** policy rather than silently deleting or quarantining user files.

### Studio, Drone, Network & Hardware

The wider project includes file-production tools, drone-production and simulation foundations, satellite and network tooling, cellular-lab foundations, hardware discovery, service health, and diagnostic systems.

## Linux Project

Nougat Media Plus is developed primarily as a native Linux desktop application. The repository contains the project source, assets, tooling, component integrations, documentation, and supporting infrastructure used to build and test Nougat.

Large project files may use **Git LFS** where appropriate.

## Documentation

The README is intentionally kept as an overview rather than a running history of every build.

For the details, use the project documentation:

- [CHANGELOG.md](CHANGELOG.md) — version and build history
- [ROADMAP.md](ROADMAP.md) — planned and ongoing work
- [DEPENDENCIES.md](DEPENDENCIES.md) — dependency information
- [COMPANY_BIBLE.md](COMPANY_BIBLE.md) — project rules and development policy
- [`docs/`](docs/) — architecture, implementation notes, build records, and subsystem documentation
- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) — third-party notices
- [LICENSE](LICENSE) — project license
- [COPYRIGHT.md](COPYRIGHT.md) — copyright information

Git history, commits, branches preserved in commit ancestry, project documentation, and source files provide the deeper development record without turning this front page into a second changelog.

## Repository Policy

`main` is the authoritative development branch. Historical work should be preserved through Git history and documentation rather than maintained as a collection of long-lived development branches.

Contributions should follow [CONTRIBUTING.md](CONTRIBUTING.md) and the project rules in [COMPANY_BIBLE.md](COMPANY_BIBLE.md).

## Use & Responsibility

Nougat Media Plus is intended for lawful educational, research, development, personal-media, technical, and authorized professional use. Some radio, cellular, satellite, drone, network, communications, or hardware capabilities may require licenses, permissions, compatible equipment, or regulatory authorization depending on the activity and jurisdiction.

Users are responsible for using those capabilities only where they are authorized to do so.

---

**Elderred Softworks LLC · Nougat Media Plus**
