# ReddMedia v0.0.12 Validation Record

## Build identity

- Version: `0.0.12`
- Build: YouTube Seekable Cache Bridge
- Required base: `v0.0.11`
- Required base commit/tag target: `fd461f625e71ae4c7e239d494d801f46b7e60c12`
- Package: `ReddMedia_v0_0_12_YOUTUBE_UI_AND_RED_STAR_IDENTITY_REPAIR_CHANGED_FILES_ONLY.zip`

## Build-side validation

### Accepted-base reconstruction

PASS. The v0.0.11 changed files used as the package base match the final accepted v0.0.11 repair hashes, including:

- `CHANGELOG.md`: `4b6bc1d0d07ec0d6da8da5e325d98e0b573e44fde644bc7f9f00112ea97960bf`
- `CMakeLists.txt`: `5326c207ea0c730e2bb046b962c8e377de66693149bdaa2df9534f2ba968e765`
- `DEPENDENCIES.md`: `e7bb9a0a4eafb05177453cba156bbb7d54b47612897357d1418b4915ca8a0270`
- `README.md`: `fe11cce62ff75aade1e5c7600a4b938b5e82d8f9f3e84844dd49ca8bc5ac1589`
- `ROADMAP.md`: `55900f6f89b8e8cc15504829b56dee93b2d4a158e5b9be03499bfdbef455d62a`
- `ReddMedia.desktop` and `ReddMedia_v11.desktop`: `8a905c53db04634a52a3c27cebf2c3539d7ed6b7f03e7f2f8a4429abb9fdf118`
- `src/main.cpp`: `a72deb2a9fadfc541b1181d27c32dcd1a49a703c5a585def369d045b94b7da82`

The accepted v0.0.11 binary SHA-256 recorded by the owner is `bd689eb07cb681fad0e206d76dc2b13d55c47b5b716235b856092b80bc60b6c1`.

### C++17 warnings-as-errors compile

PASS in isolated P2P-stub mode. The complete v0.0.12 source set compiled with:

- C++17
- `-Wall`
- `-Wextra`
- `-Werror`
- Threads/X11/dl linkage

The resulting test executable returned exactly `ReddMedia v0.0.12`.

### YouTube localhost bridge fixture

PASS.

A deterministic fake yt-dlp feeder was used to validate the native `YtDlpStreamServer` without network dependence:

- server URL resolved to `http://127.0.0.1:<ephemeral-port>/stream`;
- `HEAD` returned `200 OK` and `Accept-Ranges: bytes`;
- progressive full `GET` returned `200 OK` and the complete deterministic stream;
- `Range: bytes=100-199` returned `206 Partial Content`, exactly 100 bytes, and the correct `Content-Range`;
- suffix range `Range: bytes=-64` returned `206 Partial Content` and exactly 64 bytes;
- an unavailable large range returned `416 Range Not Satisfiable`.

### Timestamp restart and cleanup fixture

PASS.

Two consecutive bridge starts at 61.500 seconds and 127.000 seconds produced the exact yt-dlp argument contracts:

- `--download-sections *00:01:01.500-inf --force-keyframes-at-cuts`
- `--download-sections *00:02:07.000-inf --force-keyframes-at-cuts`

The obsolete local server was stopped before replacement, the second bridge used a different ephemeral loopback port, and the number of `/tmp/reddmedia-ytdlp-*.cache` files was unchanged after cleanup.

### Bundled yt-dlp option compatibility

PASS against a deterministic info-JSON fixture.

- `--skip-download --print duration` returned `125.5`.
- `-f bv*[height<=1080]+ba/b[height<=1080]` was accepted.
- `--download-sections *00:00:10.000-inf` was accepted.
- `--force-keyframes-at-cuts` was accepted.

### Source contracts

PASS.

- YouTube source data flows through the temporary cache and `libvlc_media_new_location` localhost playback path.
- The old yt-dlp media-pipe handoff is removed from the active playback implementation.
- The bridge binds with `INADDR_LOOPBACK`.
- Cache filenames are private per-process/per-segment paths under `/tmp`.
- Stop and object destruction remove the cache.
- The default format selector contains `height<=1080` and no higher-quality override.
- Timeline duration is probed with bundled yt-dlp before initial playback.

### P2P terminology

PASS. Candidate project text contains zero occurrences of the retired public protocol branding. Technical `libtorrent` and `.torrent` references remain where required for implementation, file-format, dependency, and license truth.

### Same-version YouTube UI and red-star identity repair

PASS in build-side static/stub validation.

- Creator-facing tab, page heading, activity-log heading, and status strings use **YouTube**.
- The only quoted `yt-dlp` string left in `src/main.cpp` is the internal bundled executable path `/tools/yt-dlp/yt-dlp`.
- The GNOME/X11 UTF-8 window title contract is `★ ReddMedia`.
- The in-app top-right label is exactly `v0.0.12`.
- `_NET_WM_ICON` now generates a five-point red star instead of the former triangle.
- All seven `assets/icons/reddmedia*.png` application/icon-theme assets are red-star PNGs.
- Both desktop launchers use `Name=ReddMedia`, `Icon=reddmedia`, and `StartupWMClass=ReddMedia`.

## Target-machine installer validation

The same-version repair installer repeats the following before it may print FINAL PASS:

1. `main` and tag `v0.0.11` still point to accepted commit `fd461f625e71ae4c7e239d494d801f46b7e60c12`;
4. runtime/build dependency preflight;
5. pre-mutation real libtorrent C++17 warnings-as-errors compile;
6. real `ReddMedia_v12` ELF/version/libtorrent linkage;
8. localhost HTTP bridge fixture including `HEAD`, `200`, `206`, suffix range, and `416`;
9. stale seek/timestamp-restart/cache-cleanup fixture;
10. bundled yt-dlp duration/1080p/timestamp option fixture;
11. local libVLC location-playback symbol availability;
12. exact post-install changed-path set;
13. zero retired terminology after installation;
14. red-star raw-executable custom-icon metadata and installed user icon-theme bytes after the final binary write.

## Owner acceptance gates after installer FINAL PASS

- YouTube fixture starts inside ReddMedia.
- Playback defaults to no more than 1080p.
- The seek timeline has a usable full-video duration.
- Repeated Fast Forward no longer strands the old one-way pipe.
- Seeking backward works after one or more forward seeks.
- A seek beyond current cached playback restarts and resumes near the requested time after buffering.
- Stop terminates playback/feed activity and leaves no active temporary cache for that playback.
- `ReddMedia_v12` visibly shows the ReddMedia red star in Nautilus.
- The desktop/application launcher visibly uses the same red star.
- The GNOME/X11 title reads `★ ReddMedia` without a version number.
- The in-app top-right label reads only `v0.0.12`.
- Creator-facing playback labels read **YouTube**, not `yt-dlp`.

Owner acceptance is required before snapshot, Git commit/tag, or GitHub closeout.
