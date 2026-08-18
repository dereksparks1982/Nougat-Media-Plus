# ReddMedia Dependencies

## v0.0.16 integrated Library and Discover requirements

ReddMedia v0.0.16 bundles pinned llama.cpp source and the Nomic Embed Text v1.5 Q4_K_M GGUF model. The patch installer builds a portable CPU-only shared llama.cpp runtime under `components/ai/runtime/`; no remote inference service or Python AI framework is used. SQLite is loaded from the normal Linux `libsqlite3` runtime for private viewing history. External recommendations use the installed `curl` command for HTTPS requests to TMDb.

The installer disables llama.cpp's optional application, tools, examples, tests, server, and common-utility targets and builds the required `llama` shared-library target directly.

The exact AI source/model revisions and SHA-256 hashes are recorded in `config/ai/pinned_ai_runtime.json`.

## v0.0.15 integrated server requirements

ReddMedia bundles the official stable Jellyfin 10.11.11 server and web packages for Ubuntu 26.04 amd64. Installation uses `dpkg-deb` to extract their runtime files into the ReddMedia component tree; it does not install a system Jellyfin service and does not require Node, npm, or the .NET SDK. The web package is retained for exact upstream source/package preservation, but the running catalog uses `--nowebclient` and ReddMedia supplies the only user interface and player.

The matching server and web source archives are preserved under `components/jellyfin/source/`. The original packages are preserved under `components/jellyfin/packages/`. Run `tools/build_integrated_jellyfin_v15.sh` to verify package identities and hashes, extract both runtimes, confirm `Jellyfin.Server 10.11.11.0`, and atomically install them under `components/jellyfin/runtime/`.

ReddMedia is currently developed and validated on Ubuntu Linux.

## Runtime dependencies

A normal user running ReddMedia v0.0.16 needs:

```bash
sudo apt install -y \
  vlc \
  ffmpeg \
  zenity \
  curl \
  libsqlite3-0 \
  libx11-6 \
  libfontconfig1 \
  libicu78 \
  libjemalloc2 \
  libtorrent-rasterbar2.0t64
```

### What they provide

- `vlc` provides VLC/libVLC, ReddMedia's playback engine.
- `ffmpeg` provides FFmpeg/FFprobe for yt-dlp merging and media post-processing.
- `zenity` provides the current Linux file and folder dialogs.
- `curl` provides authenticated HTTPS access to the optional TMDb external catalog.
- `libsqlite3-0` provides the local viewing-history database engine.
- `libx11-6` provides the X11 runtime used by ReddMedia's native desktop interface.
- `libfontconfig1`, `libicu78`, and `libjemalloc2` satisfy the upstream Jellyfin Ubuntu 26.04 runtime package requirements.
- `libtorrent-rasterbar2.0t64` provides the P2P runtime engine.

ReddMedia bundles yt-dlp at `tools/yt-dlp/yt-dlp`; users do not need a separate yt-dlp package.

The exact runtime package name for libtorrent can vary between Ubuntu releases because of ABI transitions. ReddMedia v0.0.12 targets the same libtorrent-rasterbar 2.0.12 package family proven on the accepted v0.0.10 development machine.

## v0.0.12 cache bridge

v0.0.12 adds no new runtime package. Its YouTube cache bridge is implemented in ReddMedia's native C++ code using Linux/POSIX files, processes, threads, and localhost sockets. yt-dlp and FFmpeg remain responsible for obtaining and remuxing supported web video, and libVLC remains the embedded playback engine.

## Build and patch-installer dependencies

Developers compiling ReddMedia, and users applying the current changed-files development patch, need:

```bash
sudo apt install -y \
  build-essential \
  cmake \
  pkgconf \
  libx11-dev \
  libtorrent-rasterbar-dev \
  libglib2.0-bin \
  vlc \
  ffmpeg \
  zenity \
  dpkg \
  unzip
```

`libtorrent-rasterbar-dev` pulls the appropriate libtorrent runtime and its required Boost/OpenSSL development dependencies on Ubuntu. `libglib2.0-bin` supplies `gio`, which the development patch installer uses to assign and verify the red-star custom icon on the raw versioned executable in GNOME Files.

`dpkg` supplies `dpkg-deb`, which extracts the bundled Jellyfin packages without registering a system service. ReddMedia patch installers check these requirements before changing the project tree.

## Distribution direction

The roadmap calls for a future self-contained Linux distribution, such as an AppImage-style single-file release, that carries appropriate runtime components with ReddMedia so end users do not have to assemble the dependency stack manually. Development-only tools and headers will remain build-time requirements rather than user-facing runtime baggage.
