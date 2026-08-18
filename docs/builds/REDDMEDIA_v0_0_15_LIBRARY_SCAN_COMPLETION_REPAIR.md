# ReddMedia v0.0.15 Library Scan Completion Repair

## Owner-test failure

The startup-readiness candidate passed package integrity, installed Jellyfin 10.11.11, built both native configurations with warnings as errors, completed the private API setup, and entered the hidden catalog proof. It then reported `indexed test video not found` and rolled back to accepted v0.0.14.

## Proven root cause

Jellyfin 10.11.11 documents `POST /Library/Refresh` as starting a scan. Its `LibraryManager.ValidateMediaLibrary` implementation queues `RefreshMediaLibraryTask` and immediately returns a completed task. ReddMedia accepted the resulting HTTP 204 and queried `/Items` once before the asynchronous scan indexed the generated video.

## Repair

- The API client polls real `/Items` results after requesting a scan.
- Success requires an indexed video path located inside the requested folder.
- Folder matching enforces a path boundary, so `/media/movies-other` cannot satisfy a wait for `/media/movies`.
- Native **Add Media Folder** waits in its existing background worker, leaving the X11 event loop responsive.
- The installer proof waits up to 180 seconds for its generated test video.
- Native folder addition waits up to 300 seconds for the first indexed video and returns a precise timeout message if none appears.

The playback route is unchanged: **Play Selected** still passes the direct local path to ReddMedia's existing embedded libVLC player. Jellyfin remains hidden and its web player remains unavailable.

Any installation or validation failure still rolls the project back to the exact accepted v0.0.14 base.
