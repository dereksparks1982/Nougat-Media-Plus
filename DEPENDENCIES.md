# Nougat Media Suite Dependencies

## v0.0.18 metadata, diagnostics, and watch-availability requirements

v0.0.18 adds no new linked library. It continues to use `curl` for authenticated TMDb HTTPS requests and uses the TMDb watch-provider endpoints, whose provider data is supplied by JustWatch. `xdg-open` opens only the official watch-options URL returned by TMDb and the local Jellyfin log folder; provider authentication and playback remain in the provider-supported application or website.

The existing X11, Threads, `dl`, libVLC, FFmpeg, llama.cpp, Nomic model, libtorrent, Jellyfin runtime, Python 3, CMake, C++17 compiler, `gio`, and Files/Nautilus requirements remain unchanged. My Services preferences are a small owner-only local file under `~/.config/reddmedia/discover/`; diagnostic reports do not include the TMDb credential.

## v0.0.17 Library and Discover reliability requirements

v0.0.17 adds no new system package. FFmpeg, already required for media and YouTube processing, now also normalizes real Jellyfin and TMDb poster responses into the uncompressed BMP representation used by ReddMedia's native X11 renderer. `curl` continues to provide TMDb HTTPS access and now supports either a validated API key or read access token without printing the credential.

Generated `components/ai/runtime/` and `components/jellyfin/runtime/` trees are installation products and are intentionally excluded from Git.

## v0.0.16 integrated Library and Discover requirements

ReddMedia v0.0.16 bundles pinned llama.cpp source and the Nomic Embed Text v1.5 Q4_K_M GGUF model. The patch installer builds a portable CPU-only shared llama.cpp runtime under `components/ai/runtime/`; no remote inference service or Python AI framework is used. SQLite is loaded from the normal Linux `libsqlite3` runtime for private viewing history. External recommendations use the installed `curl` command for HTTPS requests to TMDb.

The installer disables llama.cpp's optional application, tools, examples, tests, server, and common-utility targets and builds the required `llama` shared-library target directly.

The exact AI source/model revisions and SHA-256 hashes are recorded in `config/ai/pinned_ai_runtime.json`.

## v0.0.15 integrated server requirements

ReddMedia bundles the official stable Jellyfin 10.11.11 server and web packages for Ubuntu 26.04 amd64. Installation uses `dpkg-deb` to extract their runtime files into the ReddMedia component tree; it does not install a system Jellyfin service and does not require Node, npm, or the .NET SDK. The web package is retained for exact upstream source/package preservation, but the running catalog uses `--nowebclient` and ReddMedia supplies the only user interface and player.

The matching server and web source archives are preserved under `components/jellyfin/source/`. The original packages are preserved under `components/jellyfin/packages/`. Run `tools/build_integrated_jellyfin_v15.sh` to verify package identities and hashes, extract both runtimes, confirm `Jellyfin.Server 10.11.11.0`, and atomically install them under `components/jellyfin/runtime/`.

ReddMedia is currently developed and validated on Ubuntu Linux.

## Runtime dependencies

A normal user running ReddMedia v0.0.17 needs:

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

`libtorrent-rasterbar-dev` pulls the appropriate libtorrent runtime and its required Boost/OpenSSL development dependencies on Ubuntu. `libglib2.0-bin` supplies `gio`, which the development patch installer uses to assign and verify the approved red-tree custom icon on the raw versioned executable in GNOME Files.

`dpkg` supplies `dpkg-deb`, which extracts the bundled Jellyfin packages without registering a system service. ReddMedia patch installers check these requirements before changing the project tree.

## Distribution direction

The roadmap calls for a future self-contained Linux distribution, such as an AppImage-style single-file release, that carries appropriate runtime components with ReddMedia so end users do not have to assemble the dependency stack manually. Development-only tools and headers will remain build-time requirements rather than user-facing runtime baggage.

## Nougat integrated search - v0.0.19

- **Python 3**: runs the headless integrated Nougat indexing/crawler/peer engine. No Tkinter dependency is used by ReddMedia.
- **SQLite FTS5**: accessed through Python's standard `sqlite3` module for local full-text indexing. The installer verifies FTS5 before applying the candidate.
- **curl**: used only for the Nougat Tor crawler path through `127.0.0.1:9050` when a local Tor service is available.
- **Tor Browser / Tor**: optional external runtime for `.onion` browsing and crawling. No Tor binary is bundled by v0.0.19.
- Nougat's Python engine otherwise uses Python standard-library modules only and requires no pip packages.
- Active Nougat data: `~/.local/share/reddmedia/nougat/`. The archived standalone Nougat prototype is not an installation dependency.

## v0.0.19 installer UI-smoke prerequisites

The v0.0.19 development installer requires `xvfb-run` and `xwininfo` for its bounded native X11 window smoke test. On Ubuntu these are provided by:

```bash
sudo apt install -y xvfb x11-utils
```

The installer checks for the commands first and installs only the missing package(s) through APT. These are validation/development dependencies, not permanent ReddMedia runtime requirements.


### v0.0.19 repair-carried pinned embedding model

The v0.0.19 AI-runtime-layout repair package carries the already-pinned `nomic-embed-text-v1.5-Q4_K_M.gguf` runtime asset because the deployed v0.0.18 changed-file baseline did not contain the model. The installer verifies the exact 84,106,624-byte file and SHA-256 `d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac` before using it. The model remains under its upstream Apache-2.0 license.
## v0.0.20 Stream and UI requirements

v0.0.20 adds no new linked runtime library. Stream continues to use ReddMedia's bundled `yt-dlp` plus the existing FFmpeg/libVLC path for supported public video URLs. **Open Webpage** uses the installed desktop's normal URL opener (`xdg-open` when available); v0.0.20 does not embed a second browser engine. Rumble, RuTube, VK, and OK support therefore follows the extractors and public URLs supported by the pinned yt-dlp runtime.

The full-tab palette system, 0-200% volume control, Grid/List Library views, visible text carets, and centered wide-window bottom controls are implemented in ReddMedia's existing native X11 code and add no new package dependency.


### v0.0.20 small-handoff runtime rule

The v0.0.20 changed-files handoff does not re-ship the unchanged 84,106,624-byte Nomic model. Accepted v0.0.19 already installed that pinned runtime asset. The v0.0.20 installer verifies the existing file is exactly 84,106,624 bytes with SHA-256 `d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac` before modifying the project.


## Nougat Media Suite v0.0.21 identity/palette build

v0.0.21 adds no new runtime or build dependency. The rename, icon replacement, palette expansion, and service-reactive Stream colors are implemented in the existing native X11 application. Existing backward-compatible runtime/config paths remain unchanged in this identity-only release.
