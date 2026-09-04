# Nougat Media Suite
## Complete Media Suite / Nero Capability Program

**Roadmap status:** Approved for long-term planning during v0.0.44.  
**Implementation status:** Roadmap only. This document does not authorize a feature build by itself.  
**Version:** v0.0.44 documentation update. No version bump.

## Program Goal

Nougat Media Suite should ultimately cover the practical feature territory of:

1. **Classic Nero**, including the broad Nero 9-era multimedia toolbox.
2. **Later and current Nero**, including the useful capabilities found in modern Nero Platinum and related Nero products.
3. **Future useful Nero additions**, evaluated as Nero evolves.
4. **Nougat-native capabilities beyond Nero**, including the universal native player, Live TV, World TV, media-server functions, decentralized Search, P2P, Discover, Stream, and integrated Games/emulation.

The goal is **functional capability coverage**, not copying Nero proprietary code, artwork, templates, branding, or protected implementation details. Nougat should use its own interface and implementation plus properly licensed third-party/open-source components where appropriate.

---

## Permanent Architecture Rules

### One Nougat Suite
Do not recreate Nero's historical collection of many disconnected applications. Capabilities should live inside the Nougat application and share common media, metadata, settings, diagnostics, and workflow infrastructure.

### One Visible Playback Surface
Anything whose primary action is **Play / Watch / Run** routes through the existing **Nougat native Video Player**.

This includes:
- local movies and TV
- Home and Library playback
- Live TV
- World TV
- Stream Direct Watch
- P2P playback
- Games/emulators

Individual pages should browse, organize, configure, edit, or launch content. They should not grow duplicate black playback surfaces or independent player windows.

For Games, emulator software may provide the emulation engine, but the visible game session should be hosted inside the Nougat native player experience rather than appearing as an unrelated external desktop application.

### Legal Component Boundary
Nougat may integrate properly licensed open-source or redistributable components. Preserve upstream license, source, attribution, and redistribution requirements. Do not copy proprietary Nero code/assets.

### Capability Parity Is Not a Ceiling
Nero capability coverage is a target, not a limit. Nougat-specific systems remain first-class capabilities.

### Ongoing Nero Tracking
Maintain a living capability tracker. When Nero adds a genuinely useful feature in later releases, evaluate it for the Nougat roadmap instead of freezing parity at Nero 9 or a single modern Nero release.

---

# Capability Tracks

## 1. Universal Playback

Long-term target:
- one native Nougat playback surface
- local video/audio playback
- Live TV
- World TV
- Stream Direct Watch
- P2P playback
- game/emulator sessions
- fullscreen through the same player
- unified stop/back/origin return behavior
- shared volume, seek, subtitle, audio-track, chapter, and playback controls where applicable
- no orphan external playback windows

## 2. Media Library / MediaHome-Class Organization

Long-term target:
- Movies
- TV
- Music
- Photos
- Games
- recordings
- playlists
- collections
- metadata and artwork
- ratings
- viewing/listening history
- Continue Watching
- smart filtering and search
- duplicate/similar media awareness
- import/export of lists and metadata
- local and media-server-aware organization
- file/source context actions on every card

## 3. Video Studio

Target full nonlinear editing capability:
- multitrack timeline
- trim, split, ripple, cut, copy, paste
- transitions
- titles and captions
- effects
- picture-in-picture
- chroma key
- speed controls
- crop, rotate, scale
- stabilization
- color correction
- audio mixing
- chapters
- subtitle tracks
- slideshows
- commercial/ad detection and removal tools
- motion/object tracking
- smart rendering where possible
- hardware-accelerated render/export
- project save/load
- export presets

## 4. Audio Studio

Target WaveEditor/SoundTrax-class capability:
- waveform editing
- recording
- trim/cut/fade
- normalization
- EQ
- compression
- noise cleanup
- restoration
- filters/effects
- channel conversion
- multitrack mixing
- automation
- audio extraction from video
- format conversion
- vinyl/tape digitization
- surround/multichannel project support where practical
- project save/load

## 5. Photo Studio

Target:
- photo viewing and organization
- crop
- resize
- rotate
- color/exposure correction
- sharpening
- denoise
- filters/effects
- metadata editing
- batch processing
- format conversion
- restoration
- face/portrait tools
- slideshow creation
- AI-assisted enhancement where appropriate

## 6. Convert / Recode

Target:
- video transcoding
- audio transcoding
- image conversion
- remuxing without unnecessary re-encoding
- batch queues
- codec/container selection
- bitrate/quality controls
- resolution and framerate conversion
- subtitle handling
- audio-track selection
- device/export presets
- disc-to-file workflows
- hardware acceleration
- reusable conversion profiles

## 7. Disc Burning / Nero Burning ROM-Class Tools

Target:
- CD
- DVD
- Blu-ray where supported
- data discs
- audio CDs
- video-disc structures
- ISO/image creation
- image burning
- disc copying
- multisession
- bootable media
- filesystem options
- spanning across media
- verification after write
- erase/rewrite operations
- drive/media information
- safe write-speed controls

## 8. Simplified Disc Workflow / Express-Class Mode

Target:
- guided simple burning
- data disc
- audio disc
- copy disc
- burn image
- erase rewritable media
- clear beginner-friendly workflow using the same underlying disc engine

## 9. DVD / Blu-ray Authoring

Target:
- menu creation
- chapters
- backgrounds
- thumbnails
- buttons
- navigation
- subtitles
- multiple audio tracks
- preview
- menu templates
- final mastering
- integration with Video Studio projects

## 10. Cover & Label Designer

Target:
- disc labels
- jewel-case covers
- DVD/Blu-ray covers
- printable layouts
- text/image layout
- reusable templates
- print preview
- custom dimensions

## 11. Capture & Screen Recording

Target:
- full-screen capture
- window capture
- region capture
- system audio
- microphone
- webcam
- TV/tuner capture
- scheduled capture
- high-resolution/high-framerate recording
- cursor options
- automatic subtitle/transcription integration
- direct handoff into Video Studio

## 12. Live TV / DVR

Target:
- tuner discovery
- channel scan
- channel artwork
- guide/EPG
- **automatic guide collection on Nougat startup**
- **automatic guide update every hour while Nougat is running**
- cached guide retained on temporary failure
- manual Refresh Guide remains available
- playback through native Video Player
- scheduled recording
- series recording
- conflict handling
- timeshift
- recordings library
- recording metadata/artwork
- post-recording editing/export

## 13. World TV

Target:
- actual foreign television channels
- legitimate broadcaster-provided/direct streams
- HLS/DASH/MPEG-TS or other appropriate direct formats
- no YouTube simulcast catalog as the normal World TV source
- language and country agnostic
- no user-location filtering
- no DRM/geoblock bypass
- native Video Player only
- no duplicate black playback surface on the World TV page
- stream-health checks
- bounded reconnect/recovery
- broadcaster/channel metadata and artwork
- practical quality ceiling controls such as 1080p where required

## 14. Backup / Restore

Target BackItUp-class capability:
- file/folder backup
- library-aware backup
- full backup
- incremental backup
- differential backup
- scheduling
- verification
- encryption where appropriate
- removable storage
- network destinations
- optical destinations where useful
- restore browser
- restore verification
- backup history

## 15. Recovery / RescueAgent-Class Tools

Target:
- read-only recovery workflows
- damaged optical-disc recovery
- recoverable removable-storage workflows
- file reconstruction where feasible
- sector/error reporting
- copy-good-data-first strategy
- never risk source media unnecessarily

## 16. Optical Drive / Media Diagnostics

Target DiscSpeed/InfoTool-style capability:
- drive model/capabilities
- supported media
- firmware information
- read/write capability reporting
- transfer-rate tests
- media readability/error tests
- disc/media information
- verification tools
- useful charts/results
- safe diagnostic defaults

## 17. USB / Removable-Media Duplication

Target USBxCopy-style capability:
- copy one source to multiple removable devices
- verification
- progress per device
- safe failure isolation
- formatting options where appropriate
- batch operations
- clear source/destination protection

## 18. Duplicate / Similar Media Manager

Target:
- exact duplicate detection
- perceptual photo similarity
- similar audio detection
- similar video detection
- review before removal
- safe cleanup
- library reconciliation
- no automatic destructive deletion
- reclaim-space reporting

## 19. AI Media Tools

Target useful modern Nero-class and Nougat-native local AI:
- photo upscaling
- video upscaling
- restoration
- denoise
- deblur
- artifact reduction
- frame enhancement
- photo tagging/classification
- speech-to-text subtitles
- subtitle translation
- face/portrait tools
- object detection
- motion tracking
- privacy-conscious local processing where practical
- batch processing
- before/after preview

## 20. Performance / System Media Tools

Target:
- media-workstation diagnostics
- storage throughput tests
- encode/decode benchmarks
- GPU capability reporting
- relevant cache/storage cleanup
- media-specific optimization
- avoid generic junk-cleaner behavior and unnecessary system modification

## 21. Games

Nougat-specific track:
- integrated game library
- Grid/List
- same proven multi-row card layout model as Media Library
- artwork and metadata
- persistent ROM folders
- ZIP ROM discovery
- automatic system recognition
- bundled properly licensed emulator runtimes
- automatic emulator selection
- controllers/gamepads
- save handling
- legal homebrew/public-domain/licensed bundled test titles
- native Nougat player hosting for visible emulator output
- no independent external emulator window as normal play experience
- fullscreen through Nougat
- clean stop/back behavior
- right-click context actions

## 22. Search / Internet Media

Nougat-specific track:
- decentralized Search
- crawler
- P2P
- security analysis
- privacy architecture
- Stream services
- Discover
- direct media playback through native player
- provenance/trust information
- modular long-term privacy/search architecture

## 23. Media Server

Nougat-specific track:
- integrated Jellyfin foundation
- local library server
- remote-device access
- users/devices
- streaming/transcoding
- artwork/metadata
- watch state
- server diagnostics
- eventual Nougat-native server capabilities where justified

---

# Current v0.0.44 Corrections Already Identified

These are not distant Nero-parity ideas. They are immediate behavior corrections for the current v0.0.44 line:

- Games must open inside Nougat's native Video Player rather than as a separate emulator application window.
- World TV must not display a duplicate black playback rectangle over the World TV page.
- World TV playback must switch to and use the native Video Player.
- The same universal-player rule applies to anything else playable in Nougat.
- Live TV guide collection should begin automatically at application startup when channels/tuner configuration are available.
- Live TV guide data should refresh automatically every hour while Nougat is running.
- Automatic guide updates must not interrupt active playback or recording.
- Manual Refresh Guide remains available.

---

# Suggested Dependency Order

## Foundation
- finish universal-player routing
- finish media/card context behavior
- strengthen shared metadata/artwork/cache systems
- strengthen codec/transcode foundation
- establish reusable capture/export/job-queue infrastructure

## Creation Core
- Convert/Recode
- Video Studio
- Audio Studio
- Photo Studio
- Capture/Screen Recorder

## Disc & Physical Media
- optical-drive abstraction
- Burning ROM-class tools
- Express-style simplified workflow
- authoring
- cover/label designer
- drive/media diagnostics
- recovery

## Library Intelligence
- music/photo expansion
- duplicate manager
- AI tagging
- AI restoration/upscaling
- richer playlists/collections

## Preservation & Administration
- backup/restore
- USB duplication
- advanced recovery
- media-workstation diagnostics

## Continuous Parity Program
- periodically compare Nougat capability coverage with current Nero releases
- add useful new Nero features to this roadmap
- do not remove Nougat-native innovation merely to match Nero

---

# Capability Tracking Rule

Each future implementation should be marked with one of:

- **ROADMAP**: approved idea, not scheduled
- **PLANNED**: assigned to a future build
- **IN PROGRESS**
- **CANDIDATE**
- **ACCEPTED**
- **DEFERRED**
- **REJECTED**

No roadmap item becomes build scope solely because it appears in this document. Build scope still requires explicit owner approval under the Company Bible.


---

# Plex-Class Media Server / Library Capability Program

**Added:** v0.0.57 candidate planning and implementation pass.  
**Authority:** Owner-approved permanent roadmap track.  
**Rule:** Capability parity means implementing useful media-server/library functions in Nougat's own architecture and visual language, not copying Plex code, branding, assets, or proprietary implementation.

## v0.0.57 foundation — IN PROGRESS / CANDIDATE
- Live TV multi-tuner Watch recovery without destructive rescans
- World TV official-page/direct-candidate recovery with bounded alternate retries
- Radio consolidated into one professional receiver interface
- Games Library grouped into persistent collapsible console sections
- one-executable GNOME/Resources identity and accepted-icon gate
- persistent pre-rendered Home and Library surfaces; no owner-visible card-by-card rebuild on ordinary tab changes
- Movies as the default Library root
- shared tolerant media identity cleanup for periods, underscores, release tags, years, codecs, resolution tags, and nonstandard catalog/disc prefixes
- Library identity reused by Home / Continue Watching artwork
- owner-controlled **Fix Match** and **Clear Manual Match** with a persistent manual identity lock
- manual title/year/type correction using the existing metadata provider
- manual match survives ordinary metadata refreshes
- Deep Diagnostic private history path repair and truthful completion status
- server status based on the integrated health probe instead of a stale transition flag
- remove Child Safe material and access gate from the System page

## Plex-parity backlog — ROADMAP
1. Full manual metadata editor: title, sort title, year/date, summary, genres, studio, cast, directors, writers, ratings, tags/labels and artwork locks.
2. Multiple versions and editions: 4K/1080p variants, theatrical/director/extended/remaster labels, merge and split controls.
3. Smart Collections and rule-based collections; actor/director/genre/decade/franchise/recently-added collections.
4. Playlists and Smart Playlists, mixed media queues, shuffle/repeat and saved play queues.
5. Automatic filesystem monitoring, partial scans, scheduled scans, changed-file detection and background metadata refresh.
6. Unavailable-media management, retain metadata across disconnected drives, relink moved files, explicit cleanup rules.
7. Trailers and local extras: deleted scenes, featurettes, interviews and behind-the-scenes content.
8. Intro detection and Skip Intro.
9. Credit detection, Skip Credits and credits-aware Up Next timing.
10. Full chapter UI, named chapter selection and chapter thumbnails.
11. Timeline preview thumbnails during scrubbing.
12. Preferred audio language, remembered track choices and commentary-track controls.
13. Preferred/forced subtitle rules, external subtitle management, search/download and appearance controls.
14. Playback profiles for LAN/remote quality, bitrate, Direct Play and transcoding preferences.
15. Direct Play, Direct Stream, audio/video transcoding, hardware acceleration, HDR tone mapping and transcode throttling.
16. Optimized media copies for lower bandwidth/mobile use.
17. Offline downloads with later watch-state synchronization.
18. Authenticated secure remote access, HTTPS and remote bandwidth controls.
19. Library sharing with per-library permissions and read-only access.
20. Multiple users/profiles with separate watch history, Continue Watching, recommendations, watchlists and preferences.
21. Universal Watchlist and acquired-item integration.
22. Watched/unwatched controls, reset progress, season/series watched state and history browser.
23. Personal ratings feeding recommendations and sorting.
24. Advanced filtering by resolution, codec, HDR, year/decade, genre, studio, actor, director, collection, watched state, duration, audio and subtitles.
25. Advanced sorting by title/sort title, release date, added date, last played, play count, runtime, rating and resolution.
26. Cast/crew person pages, library filmographies and director/writer navigation.
27. Richer Discover with streaming availability, related media, cast discovery, upcoming titles and trailers.
28. Server activity dashboard: active sessions, client device, Direct Play/transcode status, resolution, bandwidth and playback position.
29. Historical server statistics, plays over time, most-watched titles, bandwidth/transcoding and user activity.
30. Database maintenance: integrity checks, optimization, orphan cleanup, metadata/artwork cleanup, backups and scheduling.
31. Live TV DVR: current-program recording, scheduled recordings, series rules, new-episode rules, priorities, conflicts and recording library.
32. Live TV time shifting: pause/rewind/fast-forward-to-live rolling buffer.
33. Full Guide/DVR integration: searchable grid guide, program details, favorites, filters and recording actions.
34. Commercial detection/markers and optional skip controls for recordings.
35. Advanced tuner administration: per-tuner status/priority/reservations, assignment and simultaneous activity.
36. Multi-device Nougat clients with synchronized watch state.
37. Casting / remote playback control between Nougat devices and supported targets.
38. Music library: artists, albums, tracks, artwork, genres, lyrics, playlists, smart mixes, ReplayGain and gapless playback.
39. Media analysis: codecs, resolution, audio channels/formats, HDR/Dolby Vision, loudness, intro/credits and thumbnail generation.
40. Webhooks/API automation for playback, library, server and DVR events.
41. Photos expansion and photo metadata organization integrated with the existing Complete Media Suite roadmap.
42. Recordings as a first-class Library media source shared by DVR, Studio and server clients.
43. Strong server health model: API reachability, wrong-process detection, startup failure, authentication failure, Ready/Starting/Stopping/Failed truth states.
44. Diagnostic history comparison and deeper exercised checks rather than merely reporting installed components.
45. Continuous parity review of useful Plex-class features while preserving Nougat-native Radio, Search/Crawler/P2P, World TV, Studio, Games and privacy architecture.

## v0.0.58 candidate scope
- Reliability repair pass: Home/Continue Watching/artwork, native Fix Match, seek edges, Server light, Live TV persisted device preference + HDHomeRun XMLTV guide, Radio layout, File Splitter 450 MiB enforced target, server-aware diagnostics, GNOME app scope.
- Drone and custom-flight-firmware work remains deferred and is not part of this build.


## Radio Universal Receiver + Cellular Lab Program

### Universal Radio service surface
Nougat Radio intentionally exposes a broad service matrix instead of hiding modes for the sake of a small button count. The target is: if a compatible receiver can legally receive a clear signal or supported broadcast/data service, Nougat should have a place for it.

Visible service families include Local, Internet, AM, FM, HD Radio, DAB/DAB+, DRM, Longwave, Mediumwave, Shortwave, Weather, Emergency, Public Safety, Police, Fire, EMS, Government, Military, Airband, Marine, Railroad, CB, FRS/GMRS, MURS, Amateur/Ham, Business, Utilities, Trunked, P25, DMR, NXDN, TETRA, Paging, Numbers, Time/Beacon, ADS-B, ACARS, AIS, Weather Satellite, Satellite, Amateur Satellite, Cellular Lab, Favorites, Recordings, TV Antenna Scan, and ISS/Satellite. ISS/Satellite remains last.

The buttons remain visible even when the current hardware cannot support a mode. Unsupported modes must say which receiver/decoder is required instead of disappearing. Digital/data services may use decoded-data views rather than audio playback. Encrypted traffic is identified as encrypted/locked and is not decrypted or cracked.

### Private 2G GSM Cellular Lab
Roadmap a lab-oriented private GSM network controller inside Radio > Cellular Lab. The first implementation target is a closed network for owned/enrolled test devices using authorized spectrum or RF isolation.

Planned components and views:
- SDR/BTS hardware abstraction with simulation before RF activation.
- Osmocom-compatible BTS/BSC/MSC/HLR integration layer with versioned interfaces.
- 2G GSM tab with network identity, BTS state, channel allocation, attached test devices, signal metrics, and health.
- Subscribers / SIMs tab for private test subscriber identities and SIM inventory.
- Calls / SMS tab for local phone-to-phone calling and SMS between enrolled lab devices.
- Network / RF tab for spectrum, protocol/debug logs, BTS/BSC/MSC/HLR status, alarms, and isolation checks.
- Optional GPRS data after voice/SMS stability.
- Explicit transmit interlock: no RF transmission until hardware, band plan, authorization/isolation, and failsafe checks are satisfied.

### 1G Analog Cellular Lab
After the 2G stack is stable, investigate a historical 1G analog cellular lab using SDR/emulation. Keep it modular and isolated from the 2G implementation. This is a later research target, not an authorization to transmit or to interact with third-party carrier subscribers.
