# ReddMedia v0.0.10 P2P Streaming Core Validation

## Historical build status

### Original v0.0.10 dependency/API attempt

SAFE PRE-MUTATION FAIL

- Dependency preflight passed after required packages were installed.
- Exact accepted v0.0.9 Git base passed.
- Package manifest and accepted-base hashes passed.
- Real libtorrent compile exposed an incompatible `.torrent` loader overload for Ubuntu Resolute libtorrent 2.0.12.
- No accepted project files had been mutated.

### Same-number libtorrent API repair

PASS

- `.torrent` loading changed to the libtorrent 2.0.12-supported `load_torrent_file(torrent_path)` overload with exception handling.
- Target-machine native compile then passed.
- Installer reached `FINAL PASS: ReddMedia v0.0.10 P2P Streaming Core installed and validated`.

## Owner P2P validation before stabilization repair

PASS

- Magnet link was accepted.
- Torrent metadata loaded.
- Video file was selected automatically.
- Peers/seeds and transfer status populated.
- Torrent downloaded successfully.
- Local `.torrent` file intake also worked.
- Playback began while a torrent was still incomplete.

DEFECTS FOUND

- Seek/time control redraw visibly flashed again.
- Seeking forward into P2P content could recover, but a later seek back toward the beginning could leave playback stalled during incomplete-download testing.
- Raw `ReddMedia_v10` executable displayed a generic executable/gears icon in GNOME Files instead of the required red triangle.
- README release descriptions were too vague to explain what every build actually accomplished.

## Stabilization repair build-side validation

### C++17 isolated build

PASS

- CMake configure with `REDDMEDIA_P2P_STUB=ON`.
- `-Wall -Wextra -Werror` compile passed.
- `ReddMedia_v10 --version` returned exactly `ReddMedia v0.0.10`.

### Seek/volume repaint source contract

PASS

- Seek/time partial refresh draws to an offscreen Pixmap.
- Only the completed seek/time strip is copied to the live X11 window.
- Volume partial refresh uses the same buffered method.
- The direct `draw_seek_time_row(win)` flashing path is absent.

### P2P seek-stream contract

PASS

- Stream server maintains a monotonic request generation.
- Every new GET range request becomes the active stream generation.
- Obsolete workers stop when their generation is no longer current.
- New GET range requests clear old torrent piece deadlines.
- Socket send/receive waits are bounded.
- Loopback-only bind remains in place.

### HTTP Range protocol tests

PASS

A deterministic fake P2P engine drove the exact repaired `P2PStreamServer` source:

- Explicit byte range returned HTTP 206 with exact `Content-Range` and fixture bytes.
- Suffix byte range returned HTTP 206 with the exact final 1,000-byte range and fixture bytes.
- HEAD returned the correct full-file length and no body.
- Invalid range returned HTTP 416.
- A newer seek request superseded an intentionally stalled older request; the newer range completed while the old response stopped before delivering its full requested body.

### README history audit

PASS

README now includes explicit entries for:

- v0.0.1 VLC-Style Base Video Player
- v0.0.2 VLC-Style Player Repair 1
- v0.0.3 Mouse, Keyboard, Time Display, and Flicker Repair
- v0.0.4 Time Layout, Skip Buttons, and Red Timeline Repair
- v0.0.5 Branding and Polish Flicker/Icon Repair
- v0.0.6 Menu, Audio, Subtitle, Chapter, and Close Behavior Repair
- v0.0.7 Red Button Polish
- v0.0.8 Direct yt-dlp
- v0.0.9 URL Field Text Controls
- v0.0.10 P2P Streaming Core

The entries were derived from the repository build handshakes, validation records, changelog and source history rather than memory.

### Exact package/source sealing

PASS

- Exact ZIP payload compiled in the isolated P2P stub lane with C++17 and warnings as errors.
- Exact ZIP payload launched successfully under Xvfb.
- ZIP inventory matched the manifest exactly and every payload SHA-256 matched.
- README audit confirmed explicit v0.0.1 through v0.0.10 release-history entries.
- Apply script passed shell syntax validation.

### Executable icon gate

Required target-machine validation:

- `gio set` assigns `metadata::custom-icon` on `ReddMedia_v10` to the repository red-triangle PNG.
- `gio info -a metadata::custom-icon ReddMedia_v10` must return the exact red-triangle file URI.
- Installer does not report FINAL PASS if the executable custom icon cannot be assigned and verified.

## Owner validation required after repair

1. Launch `ReddMedia_v10` from Files and confirm the executable itself shows the red triangle rather than gears.
2. Play local media and confirm the seek/time row no longer flashes during normal time updates or clicking/dragging the timeline.
3. Start a P2P video while still incomplete.
4. Seek substantially forward into undownloaded content and allow the swarm time to obtain the required pieces.
5. Seek back toward the beginning before the torrent completes and confirm playback recovers rather than locking permanently.
6. Confirm ordinary playback, magnet intake and local `.torrent` intake remain working.

## Acceptance state

Pending repaired-package install and owner stability confirmation.
