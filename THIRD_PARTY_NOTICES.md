# Nougat Media Suite Third-Party Notices

## License boundary

Nougat Media Suite Original Materials are licensed to recipients under the PolyForm Noncommercial License 1.0.0 as described in `LICENSE`. That license does not relicense any component listed below. Third-party software, models, APIs, services, and data retain their own upstream licenses and terms.

## Jellyfin

Nougat Media Suite includes the pinned Jellyfin server/web runtime and matching source used as its local catalog backend. The bundled Jellyfin server project is GPL-2.0 licensed; the preserved license copies are under `licenses/jellyfin/`. Nougat Media Suite communicates with the separately running local Jellyfin process through its local HTTP API.

## FFmpeg / FFprobe

Nougat Media Suite uses the system FFmpeg/FFprobe installation for media processing and poster normalization. FFmpeg states that most of its code is LGPL-2.1-or-later, with optional GPL components that change the obligations of a particular build. The license of the exact FFmpeg build installed/distributed controls. Nougat Media Suite does not relicense FFmpeg.

## VLC / libVLC

Nougat Media Suite uses the system VLC/libVLC playback engine. VideoLAN licenses the VLC engine/libVLC under LGPL terms; the exact system package remains governed by its own upstream/package license notices. Nougat Media Suite does not relicense VLC/libVLC.

## libtorrent-rasterbar

Nougat Media Suite uses libtorrent-rasterbar as its P2P engine. Upstream identifies libtorrent as BSD-3-Clause. The preserved notice is `licenses/LIBTORRENT_BSD_LICENSE.txt`.

## yt-dlp

Nougat Media Suite uses yt-dlp for supported public video URLs. The yt-dlp source repository is released under the Unlicense, while yt-dlp documents that some packaged standalone binaries can contain third-party code under additional licenses. Any public Nougat Media Suite distribution that bundles a yt-dlp executable must preserve the license and third-party notices applicable to the exact yt-dlp artifact being distributed.

## llama.cpp

Nougat Media Suite includes pinned llama.cpp source/runtime for local CPU embedding inference. llama.cpp is MIT licensed; the preserved notice is `licenses/ai/LLAMA_CPP_MIT_LICENSE.txt`.

## Nomic Embed Text v1.5

Nougat Media Suite uses the pinned `nomic-embed-text-v1.5` model. The upstream model is identified as Apache-2.0; the preserved license is `licenses/ai/NOMIC_EMBED_TEXT_APACHE_2_LICENSE.txt`.

## TMDb and JustWatch data

Optional external recommendations and watch-provider information use The Movie Database (TMDb) API with an owner-supplied credential. This product uses the TMDB API but is not endorsed or certified by TMDB. Watch-provider data returned through TMDb carries the applicable JustWatch attribution. Nougat Media Suite does not claim ownership of TMDb/JustWatch data, provider names, trademarks, or artwork.

## System Python, SQLite, curl, Tor, and desktop tools

Search may use the system Python standard library, SQLite FTS5, curl, and a separately installed local Tor service. These system components are not relicensed by Nougat Media Suite.

## Distribution rule

When a release bundles a third-party binary, source tree, model, or artwork asset, the release package must carry the corresponding notices/license material required by that exact artifact. If a component is merely a system dependency, its installed package continues under the terms supplied by its distributor/upstream project.


## Nougat Radio v0.0.52 upstream components

Nougat Radio may use separately licensed upstream components including SoapySDR (Boost Software License 1.0), liquid-dsp (MIT), KISS FFT (BSD-style), GNU Radio 4 core (MIT), and OP25 (GPL family), plus specialized DAB/DAB+, DRM, satellite and trunked-radio workers when installed. Their original source trees and license files are preserved under `components/radio/upstream/`; those licenses govern those upstream components. Nothing in this notice relicenses upstream code under Nougat's PolyForm license.
