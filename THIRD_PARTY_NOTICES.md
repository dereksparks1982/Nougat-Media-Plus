# Third Party Notices

ReddMedia uses VLC/libVLC for media playback when available on the user system.

ReddMedia uses FFmpeg/FFprobe through yt-dlp for stream merging and media post-processing when available on the user system.

ReddMedia includes the yt-dlp Linux standalone executable provided by the yt-dlp project. The included executable is stored at `tools/yt-dlp/yt-dlp`.

ReddMedia uses libtorrent-rasterbar as its P2P engine. Libtorrent is distributed under a BSD-style license. The required libtorrent license notice is reproduced in `licenses/LIBTORRENT_BSD_LICENSE.txt`.

ReddMedia v0.0.15 includes the official stable Jellyfin 10.11.11 Ubuntu 26.04 server and web packages plus matching source. They remain separately licensed GPL components. The server runs as a hidden separate process with its web client disabled and communicates with ReddMedia through Jellyfin's documented local HTTP API. Their exact releases, commits, package/source hashes, archives, and license copies are recorded under `components/jellyfin/`, `config/media_server/`, and `licenses/jellyfin/`.
