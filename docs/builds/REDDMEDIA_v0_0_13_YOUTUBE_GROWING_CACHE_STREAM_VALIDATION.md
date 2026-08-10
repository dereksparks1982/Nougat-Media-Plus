# ReddMedia v0.0.13 Validation Record

## Target

ReddMedia v0.0.13, YouTube Growing Cache Stream Repair.

Required accepted base: v0.0.12, commit `5e9531a69cd5a60c4268caa006c37f6fe04c303e`, tag `v0.0.12`.

## Failure reproduced from owner testing

v0.0.12 could start a 1080p long-form YouTube stream but playback stopped after approximately four to five seconds. The variable freeze point matched the amount of data cached when VLC opened the HTTP range response.

## Repair validation gates

1. Version and source identity
   - CMake project version is 0.0.13.
   - Native target is `ReddMedia_v13`.
   - `--version` prints `ReddMedia v0.0.13`.
   - in-app top-right label is `v0.0.13`.

2. Progressive cache contract
   - open-ended byte range is not finalized to the cache size present at request start.
   - open-ended 206 response reports unknown complete length using `/*`.
   - open-ended 206 response uses chunked transfer coding.
   - bridge waits at the current cache frontier while the feeder is active and resumes sending when bytes arrive.
   - no-range GET remains progressive and chunked.

3. Slow-growth regression
   - deterministic feeder appends 64 KiB blocks over multiple seconds.
   - client captures the cache size near request start.
   - a single `Range: bytes=0-` request must receive substantially more bytes than existed at request start without reconnecting.
   - this gate specifically catches the v0.0.12 four-to-five-second freeze class.

4. Cached range compatibility
   - finite byte range returns 206 with exact requested bytes when available.
   - suffix byte range returns 206.
   - unsatisfiable range returns 416.
   - total length remains unknown while feeder is active.

5. Playback policy
   - yt-dlp/FFmpeg selector remains `bv*[height<=1080]+ba/b[height<=1080]`.
   - timestamp restart retains `--download-sections` and `--force-keyframes-at-cuts`.
   - startup cache target is 512 KiB.
   - libVLC network caching is 5000 ms.

6. Lifecycle
   - replacement seek creates a new localhost bridge and invalidates the old stream.
   - temporary cache is removed on Stop/replacement/shutdown.
   - server binds only to 127.0.0.1.

7. Regression identity
   - real P2P/libtorrent linkage preserved on target system.
   - creator-facing network-video name remains YouTube.
   - `★ ReddMedia` title and red-star app/executable identity remain in place.
   - retired public terminology gate remains zero.

## Build-side result

The standalone YouTube bridge compiles under C++17 with `-Wall -Wextra -Werror`. The full ReddMedia source compiles in isolated P2P-stub mode in the package-construction environment. The target installer requires a real libtorrent build and linkage before mutation.

The deterministic slow-growing bridge test passes when a single open-ended range connection receives cache bytes written well after that HTTP request began.

## Owner acceptance gate

Owner must run long-form YouTube playback in installed v0.0.13. Acceptance requires playback to continue beyond the previous four-to-five-second freeze, followed by Stop/replay and practical seek testing. Owner acceptance remains authoritative.
