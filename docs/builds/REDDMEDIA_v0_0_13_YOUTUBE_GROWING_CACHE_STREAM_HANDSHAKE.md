# ReddMedia v0.0.13 Build Handshake

## Identity

- Project: ReddMedia
- Target version: v0.0.13
- Build: YouTube Growing Cache Stream Repair
- Required base version: accepted v0.0.12
- Required base commit: `5e9531a69cd5a60c4268caa006c37f6fe04c303e`
- Required base tag: `v0.0.12`
- Required branch: `main`
- Package: `ReddMedia_v0_0_13_YOUTUBE_GROWING_CACHE_STREAM_REPAIR_CHANGED_FILES_ONLY.zip`

## Owner-observed failure that opened v0.0.13

A long-form YouTube episode began playing through the v0.0.12 localhost cache bridge but froze after about four seconds. A second attempt froze after about five seconds. The activity log showed the yt-dlp/FFmpeg feeder had started correctly and was producing a 1920x1080 WebM stream. The failure therefore occurred in the bridge between the growing cache and embedded VLC.

## Root cause

v0.0.12 answered an open-ended VLC byte-range request using the number of bytes that happened to exist in the cache at request time as both the response end and the apparent total representation size. The feeder continued writing afterward, but VLC had already been told that the earlier cache frontier was the end of the resource. Playback stopped when VLC consumed that finite response.

## v0.0.13 implementation

- Preserve the v0.0.12 yt-dlp/FFmpeg temporary-cache architecture.
- For open-ended range requests such as `Range: bytes=0-`, return an HTTP 206 indeterminate-length live range with `Content-Range` total `*` and chunked transfer coding.
- Keep the client connection alive at the cache frontier and wait for newly appended bytes while the feeder remains active.
- Keep ordinary non-range GET as a growing chunked response.
- Preserve finite byte-range and suffix-range responses for cached probes; while the feeder remains active, report unknown total length instead of claiming the current cache size is final.
- Increase the initial YouTube cache target from 256 KiB to 512 KiB.
- Increase embedded VLC network caching from 2500 ms to 5000 ms.
- Preserve the YouTube 1080p maximum, duration probe, timestamp-restart seeking, stale-stream replacement, Stop/shutdown cleanup, P2P behavior, YouTube creator-facing terminology, and red-star identity.

## Changed source/project paths

Modified:

- `CHANGELOG.md`
- `CMakeLists.txt`
- `README.md`
- `ROADMAP.md`
- `ReddMedia.desktop`
- `src/main.cpp`
- `src/ytdlp_stream_server.cpp`

Added:

- `ReddMedia_v13.desktop`
- `docs/builds/REDDMEDIA_v0_0_13_YOUTUBE_GROWING_CACHE_STREAM_HANDSHAKE.md`
- `docs/builds/REDDMEDIA_v0_0_13_YOUTUBE_GROWING_CACHE_STREAM_VALIDATION.md`

Version transition/removal:

- `ReddMedia_v12` -> `ReddMedia_v13`
- `ReddMedia_v12.desktop` -> `ReddMedia_v13.desktop`



## Validation contract

- Exact clean accepted v0.0.12 Git base, tag, branch, and commit.
- Changed-files-only manifest and payload hashes.
- C++17 warnings-as-errors compile.
- Real libtorrent linkage on the target Ubuntu system.
- Version truth at v0.0.13.
- 1080p yt-dlp selector preserved.
- Open-ended range response uses unknown complete length and chunked transfer coding.
- Deterministic slow-growing feeder test must prove one VLC-style open-ended request receives bytes appended after the request began.
- Finite range, suffix range, invalid range, timestamp restart, stale-server replacement, and cache cleanup regression checks.
- Localhost-only binding.
- YouTube creator-facing terminology and red-star app/executable identity retained.
- Final owner test on long-form YouTube playback before acceptance.

## Risks

The bridge is intentionally serving an aggregating local resource whose complete byte length is not known while yt-dlp/FFmpeg is still writing. v0.0.13 therefore uses indeterminate-length HTTP range semantics and chunked delivery for open-ended requests. Network stalls can still cause buffering, but they must not be mistaken for end-of-file merely because VLC catches the current cache frontier.

## Rollback

Accepted rollback point: ReddMedia v0.0.12 at commit `5e9531a69cd5a60c4268caa006c37f6fe04c303e`, tag `v0.0.12`.

## Continuation point

After installer PASS, owner-test the same long-form YouTube episode that froze at four to five seconds. Do not accept/commit/tag v0.0.13 until playback continues beyond the former freeze point and normal Stop/replay behavior is confirmed.
