# Third Party Notices

ReddMedia uses VLC/libVLC for media playback when available on the user system.

ReddMedia uses FFmpeg/FFprobe through yt-dlp for stream merging and media post-processing when available on the user system.

ReddMedia includes the yt-dlp Linux standalone executable provided by the yt-dlp project. The included executable is stored at `tools/yt-dlp/yt-dlp`.

ReddMedia uses libtorrent-rasterbar as its P2P engine. Libtorrent is distributed under a BSD-style license. The required libtorrent license notice is reproduced in `licenses/LIBTORRENT_BSD_LICENSE.txt`.

ReddMedia v0.0.15 includes the official stable Jellyfin 10.11.11 Ubuntu 26.04 server and web packages plus matching source. They remain separately licensed GPL components. The server runs as a hidden separate process with its web client disabled and communicates with ReddMedia through Jellyfin's documented local HTTP API. Their exact releases, commits, package/source hashes, archives, and license copies are recorded under `components/jellyfin/`, `config/media_server/`, and `licenses/jellyfin/`.

ReddMedia v0.0.16 includes pinned llama.cpp source at commit `9731ad3f29da96f588711a0d1eb08cf210721e16` and builds it locally as the CPU embedding runtime. llama.cpp is MIT licensed; its license is reproduced at `licenses/ai/LLAMA_CPP_MIT_LICENSE.txt`.

ReddMedia v0.0.16 includes the `nomic-embed-text-v1.5` Q4_K_M GGUF model from Nomic AI revision `0188c9bf409793f810680a5a431e7b899c46104c`. The model is identified as Apache-2.0; the license text is reproduced at `licenses/ai/NOMIC_EMBED_TEXT_APACHE_2_LICENSE.txt`.

Optional External recommendations use The Movie Database (TMDb) API with an owner-supplied credential. This product uses the TMDB API but is not endorsed or certified by TMDB. No TMDb images or catalog responses are bundled as sample content.
