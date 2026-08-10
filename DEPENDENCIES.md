# ReddMedia Dependencies

ReddMedia is currently developed and validated on Ubuntu Linux.

## Runtime dependencies

A normal user running ReddMedia v0.0.12 needs:

```bash
sudo apt install -y \
  vlc \
  ffmpeg \
  zenity \
  libx11-6 \
  libtorrent-rasterbar2.0t64
```

### What they provide

- `vlc` provides VLC/libVLC, ReddMedia's playback engine.
- `ffmpeg` provides FFmpeg/FFprobe for yt-dlp merging and media post-processing.
- `zenity` provides the current Linux file and folder dialogs.
- `libx11-6` provides the X11 runtime used by ReddMedia's native desktop interface.
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
  zenity
```

`libtorrent-rasterbar-dev` pulls the appropriate libtorrent runtime and its required Boost/OpenSSL development dependencies on Ubuntu. `libglib2.0-bin` supplies `gio`, which the development patch installer uses to assign and verify the red-star custom icon on the raw versioned executable in GNOME Files.

ReddMedia patch installers check these requirements before changing the project tree.

## Distribution direction

The roadmap calls for a future self-contained Linux distribution, such as an AppImage-style single-file release, that carries appropriate runtime components with ReddMedia so end users do not have to assemble the dependency stack manually. Development-only tools and headers will remain build-time requirements rather than user-facing runtime baggage.
