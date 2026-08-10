# ReddMedia v0.0.11 Playback & Transfer Controls Build Handshake

## Project and version

- Project: ReddMedia
- Target: v0.0.11
- Build title: Playback & Transfer Controls
- Current repair package: `ReddMedia_v0_0_11_YTDLP_STREAM_PIPE_REPAIR_CHANGED_FILES_ONLY.zip`

## Required accepted base

- Version: v0.0.10
- Commit: `ea0af8192692165e5f04a1d945e6b52d01e5a91d`
- Tag: `v0.0.10`
- Branch: `main`
- Working tree: clean

## Approved product work

- Add P2P **Stop Download / Resume Download**.
- Stop active P2P playback when stopping the transfer.
- Preserve partial P2P files and resume data so Resume continues the same transfer.
- Add yt-dlp **Play** beside Download and stream through bundled yt-dlp/FFmpeg stdout directly into ReddMedia's embedded VLC player, capped at 1080p by default.
- Keep public-facing feature wording as **P2P** across active repository documentation.
- Add future Archive, Online Video, Live TV, and provider-supported streaming-service integration directions to the roadmap.

## Project paths changed

Modified:

- `CHANGELOG.md`
- `CMakeLists.txt`
- `DEPENDENCIES.md`
- `README.md`
- `ROADMAP.md`
- `ReddMedia.desktop`
- `THIRD_PARTY_NOTICES.md`
- `src/main.cpp`
- `src/p2p_engine.cpp`
- `src/p2p_engine.hpp`

Added:

- `ReddMedia_v11.desktop`
- `docs/builds/REDDMEDIA_v0_0_11_PLAYBACK_TRANSFER_CONTROLS_HANDSHAKE.md`
- `docs/builds/REDDMEDIA_v0_0_11_PLAYBACK_TRANSFER_CONTROLS_VALIDATION.md`

Generated during installation:

- `ReddMedia_v11`

Versioned replacements removed after successful installation:

- `ReddMedia_v10`
- `ReddMedia_v10.desktop`



## Risk controls

- Exact v0.0.10 Git commit, tag, branch, clean tree, and sealed base hashes are checked before mutation.
- Complete dependency preflight occurs before mutation.
- The complete real libtorrent v0.0.11 target is compiled in a temporary candidate tree on the target machine before mutation.
- Native compile uses C++17 with `-Wall -Wextra -Werror`.
- Candidate must report exactly `ReddMedia v0.0.11`, be ELF, and link libtorrent-rasterbar.
- yt-dlp Play uses the bundled yt-dlp executable with FFmpeg, selects the best stream up to 1080p, writes the playable media to stdout, and feeds that pipe directly to embedded libVLC through `libvlc_media_new_fd`.
- ReddMedia v0.0.10 project paths are restored from the accepted Git tag on rollback.
- The raw `ReddMedia_v11` executable must pass the red-triangle custom-icon metadata gate.

## Rollback point

Exact accepted ReddMedia v0.0.10 at commit `ea0af8192692165e5f04a1d945e6b52d01e5a91d`, tag `v0.0.10`.

## Validation plan

1. Complete runtime/build/icon dependency preflight.
2. Exact Git base/tag/branch/clean-tree verification.
3. Package inventory, payload hashes, and accepted-base hashes.
4. Pre-mutation real libtorrent native CMake build with warnings as errors.
5. Exact executable version, ELF, and libtorrent-link identity.
6. Static source contract for P2P Stop/Resume and yt-dlp Play.
7. Bundled yt-dlp/FFmpeg stdout option-compatibility check for the 1080p Play pipeline.
8. Public P2P wording audit on active user-facing repository documentation.
11. Apply the red-triangle executable icon after the final binary write, verify metadata, install the user icon-theme copy, refresh Files/Nautilus when available, and require owner-side visual confirmation before acceptance.
12. Exact changed-path and installed-payload validation.
13. Owner test of P2P Stop/Resume and yt-dlp Play.

## Continuation point

After installer FINAL PASS, Derek performs owner-side controls testing. On acceptance: accepted snapshot first, local Git commit/tag second, GitHub push/verification third.

## Same-version repair continuation

Owner testing of the first v0.0.11 candidate established P2P Stop/Resume as PASS and rejected acceptance because yt-dlp Play did not start video and GNOME Files/Nautilus still showed the generic executable icon. Under the company same-version law, these defects are repaired as v0.0.11 rather than advancing the version.

The repaired package also tightens the repo-wide P2P wording gate and corrects the visible top-bar version surface. The accepted rollback authority remains v0.0.10 at `ea0af8192692165e5f04a1d945e6b52d01e5a91d`.

## Same-version yt-dlp stream-pipe repair

Owner testing proved the direct-URL resolver handoff still did not play inside ReddMedia even after carrying HTTP headers. The exact URL succeeded when bundled yt-dlp/FFmpeg streamed to VLC through stdout. v0.0.11 therefore keeps the same approved Play feature but replaces the failed resolver architecture with the proven stdout-pipe architecture. No new feature is introduced and the version remains v0.0.11.
