# ReddMedia v0.0.14 Validation Record

## Build-side validation

This record distinguishes build-side evidence from owner-side desktop proof.

Required build-side gates:

- exact accepted v0.0.13 Git base, tag, branch, clean tree and base-file hashes;
- exact accepted v0.0.13 binary identity/hash before mutation;
- package manifest and payload SHA-256 verification;
- pipefail-safe libVLC dependency detection using a fully captured `ldconfig -p` cache before matching `libvlc.so`;
- C++17 `-Wall -Wextra -Werror` target compile with real X11/libtorrent dependencies on the installation machine;
- v0.0.14 runtime/version identity;
- explicit `libvlc_media_player_set_pause(mp, 1/0)` contract;
- paused playback-time and playback-length functions return cached values before calling libVLC;
- chapter discovery returns while paused and becomes media-scoped after one successful metadata scan;
- paused session save uses cached position;
- final player teardown has a bounded close path;
- approved tree source is shared by the project PNG icon family and embedded X11 icon data;
- desktop launcher points at `ReddMedia_v14` and uses `Icon=reddmedia`;
- user-level application and MIME icon copies are refreshed from the v0.0.14 tree assets;
- raw executable receives custom red-tree icon metadata after final binary write;
- accepted P2P linkage remains present;
- exact changed-path validation.

## Build-environment result

A P2P-stub build of the complete v0.0.14 C++ target passed `-Wall -Wextra -Werror` during package preparation. Real libtorrent linkage and target desktop integration are intentionally revalidated by the installer on Derek's Ubuntu workstation.

## Owner-side validation required

The build is not accepted solely because the installer reports PASS. Derek must perform the real desktop test:

1. Open a normal local TV episode/video file.
2. Play normally.
3. Pause for approximately one minute and resume.
4. Pause for at least five minutes and resume.
5. While paused, confirm ReddMedia remains responsive.
6. Close ReddMedia while paused and confirm it closes normally.
7. Reopen and confirm the saved local position is sensible.
8. Repeat pause/resume several times.
9. Confirm the approved red-tree icon visually on the dock/app switcher, launcher and versioned raw executable.
10. Confirm the top-right in-app label shows the small red tree beside `v0.0.14`.

Only owner-side evidence can convert those desktop items from candidate behavior to accepted behavior.
