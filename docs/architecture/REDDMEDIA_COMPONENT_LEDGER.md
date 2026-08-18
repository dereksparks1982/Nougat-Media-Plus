# ReddMedia Component Ledger

| Responsibility | Component | Integration |
|---|---|---|
| Native playback | ReddMedia + libVLC | In-process dynamic libVLC use |
| Native media-library UI | ReddMedia | In-process X11 Library tab; folder, scan, title-selection, and direct-play controls |
| Hidden media catalog | Jellyfin Server 10.11.11 | Stable Ubuntu 26.04 package extracted as a bundled separate GPL process; local API only, `--nowebclient` |
| Preserved upstream web source/runtime | Jellyfin Web 10.11.11 | Stable package and source retained for provenance and GPL compliance; not served to users |
| Media conversion and probing | FFmpeg/FFprobe | External command/runtime dependency |
| Online video | yt-dlp | Bundled executable controlled by ReddMedia |
| P2P transfer and streaming | libtorrent-rasterbar + ReddMedia bridge | Integrated native subsystem |
| Live TV and DVR expansion | TVHeadend planned | Future bundled provider behind ReddMedia |

Components are selected for working behavior. Removal or replacement requires a separately approved build.
