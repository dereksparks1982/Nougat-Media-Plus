# ReddMedia Roadmap

## Current: v0.0.10 P2P Streaming Core

- Permanent built-in P2P screen.
- Magnet-link and `.torrent` loading.
- Torrent metadata and file selection.
- Stream-while-downloading playback through VLC.
- Playback-aware and seek-aware piece prioritization through libtorrent time-critical deadlines.
- Full torrent download continues behind playback and seeds when complete.
- Resume-state persistence.
- Runtime/build dependency documentation and installer preflight.


## Next feature milestone: v0.0.11 yt-dlp Direct Playback

- Add a **Play** path to the yt-dlp screen so a supported web video can be watched directly in ReddMedia without waiting for a normal saved download to finish.
- Keep the existing yt-dlp **Download** path for users who want the media saved locally.
- Feed yt-dlp-resolved media into the existing ReddMedia/VLC player rather than opening an external player.

## P2P client expansion

Build the streaming core into a complete P2P client while keeping streaming as the normal user-facing playback behavior:

- Torrent queue management and ordering.
- Per-torrent and global download/upload limits.
- Connection limits and ratio/seeding controls.
- Tracker list/status controls, reannounce, and scrape controls.
- DHT, PEX, local-peer-discovery status and controls where useful.
- TCP/uTP connection status.
- UPnP, NAT-PMP/PCP port-mapping status and controls.
- IPv4 and IPv6 status.
- Protocol-encryption controls.
- Proxy controls.
- Web-seed status.
- Private-torrent handling/status.
- Force recheck.
- Move storage/downloaded files.
- Multi-file selection and file priorities.
- Torrent creation and seeding of user-created torrents.
- Initial/super-seeding controls.
- Persistent torrent library with completed and active torrents.
- Stronger crash/shutdown recovery and session-state persistence.

## Linux distribution

- Produce a self-contained ReddMedia Linux distribution so the normal end-user path becomes download and run.
- Bundle appropriate runtime components and their license notices while keeping build-only compilers, headers, CMake, and development packages out of the user distribution.
- Replace the external Zenity dependency with a file/folder dialog solution that can ship with ReddMedia.
- Target a single-file AppImage-style release after the runtime layout is stable.
