# ReddMedia v0.0.8 Direct yt-dlp Last Chance Build Handshake

Project: ReddMedia
Version: v0.0.8
Required base: v0.0.7 commit 73181534d8542d8a8ad22ac34314e7526ffbf368
Package: ReddMedia_v0_0_8_DIRECT_YTDLP_LAST_CHANCE.zip

Completed changes:
- Versioned top-level executable `ReddMedia_v8`.
- Direct yt-dlp screen in the app.
- Real bundled yt-dlp Linux engine included at `tools/yt-dlp/yt-dlp`.
- URL field supports direct typing, Ctrl+V paste, and right-click paste.
- URL field no longer uses a file picker.
- Output folder picker remains separate.
- v0.0.7 video player behavior preserved.

Excluded:
- No mod system.
- No p2p.
- No system yt-dlp fallback.
- No self-download.
- No auto-launch.
- No file explorer close command.

Known risk:
- GNOME Files may still show generic icons for raw ELF binaries on some systems. Apply script sets metadata custom icon and installs a trusted desktop launcher to maximize red triangle behavior without MIME hacks.
