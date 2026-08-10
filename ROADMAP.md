# ReddMedia Roadmap

## Current: v0.0.12 YouTube Seekable Cache Bridge

- Replace the accepted v0.0.11 one-way YouTube playback pipe with a temporary local cache served through a localhost-only HTTP bridge, powered internally by the bundled yt-dlp/FFmpeg engine.
- Keep YouTube Play capped at 1080p by default.
- Resolve the source duration so the ReddMedia seek timeline can address the full video.
- Support HTTP byte ranges for cached media and restart the internal YouTube feeder at the requested timestamp when a seek moves beyond the current cached segment.
- Cancel obsolete seek/feed jobs before starting a replacement segment.
- Remove temporary YouTube cache data on Stop, replacement playback, and clean shutdown.
- Keep playback inside the existing embedded libVLC player.

## Archive

Create one **Archive** area for legitimate public, preservation, and historical sources instead of adding a top-level tab for every archive.

Initial targets:

- **Internet Archive**
  - Search and browse archive items where supported.
  - Open playable media in ReddMedia when the source permits it.
  - Hand P2P sources and downloaded `.torrent` metadata files to ReddMedia's existing P2P system.
- **MiNERVA Archive**
  - Browse/search preserved collections where practical.
  - Hand supported downloads or P2P sources into the appropriate ReddMedia path.

The Archive design should be source-neutral so additional legitimate archives can be added later.

## Online Video

Create one **Online Video** area that can host multiple legitimate video services without turning the top bar into a row of service-specific tabs.

Planned service investigations:

- **YouTube** — first priority; use official search/player integration where available.
- **Rumble**
- **RUTUBE**
- **VK Video**
- **OK.ru / Odnoklassniki**

ReddMedia should search/browse and play inside its own experience where a service officially permits that integration. Authentication, ads, DRM, and service rules remain with the service.

## Live TV

Build a unified **Live TV** area around tuners rather than one hardware brand.

Initial hardware paths:

- **HDHomeRun** network tuner discovery and playback.
- **Local/direct antenna tuner** connected to the computer, subject to Linux driver support for the specific device.

Long-term Live TV features:

- Channel scan.
- Combined channel list across available tuners.
- Electronic program guide (EPG).
- Watch Live.
- Recording.
- DVR recordings/library.
- One combined guide even when channels come from different tuner devices.

## Supported streaming-service integration investigation

Investigate services such as Netflix and Prime Video only through provider-supported integration paths. Where third-party playback is not permitted, ReddMedia may act as a unified library/front door or launcher instead of bypassing provider-controlled playback or DRM.

## P2P expansion

Build the streaming core into a complete P2P client while keeping streaming as the normal user-facing playback behavior:

- Transfer queue management and ordering.
- Per-transfer and global download/upload limits.
- Connection limits and ratio/seeding controls.
- Tracker list/status controls, reannounce, and scrape controls.
- DHT, PEX, and local-peer-discovery status and controls where useful.
- TCP/uTP connection status.
- UPnP, NAT-PMP/PCP port-mapping status and controls.
- IPv4 and IPv6 status.
- Protocol-encryption controls.
- Proxy controls.
- Web-seed status.
- Private-transfer handling/status.
- Force recheck.
- Move storage/downloaded files.
- Multi-file selection and file priorities.
- P2P metadata creation and seeding of user-created files.
- Initial/super-seeding controls where supported.
- Persistent transfer library with completed and active transfers.
- Stronger crash/shutdown recovery and session-state persistence.

## Linux distribution

- Produce a self-contained ReddMedia Linux distribution so the normal end-user path becomes download and run.
- Bundle appropriate runtime components and their license notices while keeping build-only compilers, headers, CMake, and development packages out of the user distribution.
- Replace the external Zenity dependency with a file/folder dialog solution that can ship with ReddMedia.
- Target a single-file AppImage-style release after the runtime layout is stable.
