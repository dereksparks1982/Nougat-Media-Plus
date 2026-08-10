# ReddMedia v0.0.11 Playback & Transfer Controls Validation

## Build-side status


PASS


### Isolated native compile

PASS

- CMake configure with `REDDMEDIA_P2P_STUB=ON`.
- C++17 `-Wall -Wextra -Werror` build passed.
- `ReddMedia_v11 --version` returned exactly `ReddMedia v0.0.11`.
- X11 launch smoke under Xvfb remained alive for the bounded smoke interval.

### P2P Stop/Resume contract

PASS

- `P2PEngine` exposes pause, resume, and paused-state operations.
- Stop clears time-critical piece deadlines and pauses the libtorrent handle.
- Stop writes P2P resume state after pausing.
- UI Stop terminates the active P2P stream/player before pausing the transfer.
- Resume reactivates the existing handle without deleting partial files.
- Isolated P2P stub control test proved Stop -> paused -> Resume -> running state transitions.

### yt-dlp Play contract

- yt-dlp screen includes separate Download and Play buttons.
- Owner terminal testing proved bundled yt-dlp + FFmpeg can stream the test YouTube URL successfully to VLC over stdout.
- Play now launches bundled yt-dlp with `--ignore-config --no-playlist --downloader ffmpeg -o -`.
- Default Play quality is capped at 1080p with `bv*[height<=1080]+ba/b[height<=1080]`.
- yt-dlp stdout is connected directly to embedded libVLC through `libvlc_media_new_fd`; ReddMedia no longer reconstructs a direct YouTube media request.
- yt-dlp stderr is kept as the ReddMedia activity log rather than being mixed with binary media bytes.
- Stop and media replacement terminate the active yt-dlp streaming subprocess cleanly.
- Network playback remains excluded from local-file resume persistence.

### Public P2P terminology

PASS

- Active README, ROADMAP, CHANGELOG, DEPENDENCIES, THIRD_PARTY_NOTICES, and UI branding use P2P as the public feature name.
- No retired public protocol branding remains in tracked project text; technical `libtorrent` and `.torrent` identifiers remain where factually required.
- Technical/legal `libtorrent` and `.torrent` references remain only where needed for implementation, dependency, file-format, or license truth.

### Roadmap expansion

PASS

ROADMAP records future:

- Archive: Internet Archive and MiNERVA.
- Online Video: YouTube, Rumble, RUTUBE, VK Video, and OK.ru/Odnoklassniki.
- Live TV: HDHomeRun and direct/local antenna tuner paths, channel scan/list, EPG, live viewing, recording, and DVR.
- Provider-supported streaming-service integration investigation.
- Continued P2P client expansion and later self-contained Linux distribution.


PASS

- Same-version repair-until-PASS law is present.
- Old failed-version-number burn law is absent.
- Public P2P terminology law is present.
- Mandatory ReddMedia red-triangle executable icon law is present.

## Target-machine gates still required

The production installer must compile against the installed libtorrent 2.x development package before mutation and must reach FINAL PASS on Derek's machine. Owner-side P2P Stop/Resume and yt-dlp stdout-pipe playback remain the final live-environment proof.

## Same-version repair after owner testing

The first v0.0.11 candidate reached installer FINAL PASS, but owner testing found two defects before acceptance:

- P2P Stop/Resume: owner-side PASS.
- yt-dlp Play: FAIL; the view switched to Video Player but playback did not start because the resolver handoff discarded yt-dlp HTTP request headers.
- raw executable icon: FAIL; metadata readback passed while GNOME Files/Nautilus still displayed the generic executable icon.

The repaired v0.0.11 candidate:

- resolves a labelled media URL plus yt-dlp `http_headers`;
- applies User-Agent and Referer to libVLC per-media options before Play;
- checks the libVLC Play return value;
- reapplies the red-triangle icon after the final binary write and refreshes Files/Nautilus when available;
- treats owner-side visible red-triangle confirmation as the final icon proof instead of metadata readback alone;
- enforces a case-insensitive repository-wide wording scan across tracked text;
- corrects the visible v0.0.11 top-bar version string.

## yt-dlp same-version repair result to verify

The previous URL/header handoff remained a live FAIL. This repair changes only that implementation path. Production acceptance requires the exact previously failing YouTube URL to play inside the embedded ReddMedia Video Player at no more than 1080p by default.
