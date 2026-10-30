# Nougat Studio: Media Inspection and Processing Service

**Date:** October 30, 2026
**Application line:** v0.0.69, unchanged
**Status:** Source candidate. Neither native compilation nor hardware/media runtime acceptance is claimed.

## What this source change adds

The existing `src/studio/media_job_engine.cpp` now has an optional FFprobe inspection layer and a single Studio backend facade. The native CMake target includes `media_inspector.cpp` and `studio_service.cpp`; no Studio layouts, button geometry, icons, or executable targets were changed.

### FFprobe inspection

- Inspect a readable local regular media file through an absolute FFprobe executable, without shell interpolation or a network service.
- Parse track codec/kind, video dimensions, and optional duration in milliseconds from an intentionally narrow FFprobe output format.
- Bound inspection duration (default 8 seconds, maximum 30), captured output (64 KiB) and track count (128). Return an explicit error for missing FFprobe, invalid input, process failure, timeout or oversized output.
- Spawn FFprobe in a new process group, discard stderr, and reap it. No external media or network requests are initiated by this code itself; media input provenance remains the caller's responsibility.
- Provide advisory MP4/MKV stream-copy eligibility. This cannot guarantee remux success: a real muxing attempt may still reject tracks.

### Studio service

- `StudioService::plan()` prepares a requested export by inspecting tracks, validating an absolute output path/extension, and refusing video exports with no video or audio-only exports with no audio.
- `StudioService::submit()` passes a real inspected duration into the durable queue for FFmpeg percentage progress, when available.
- `StudioService::submit_batch()` preflights a homogeneous-preset batch and calls the engine's existing all-or-nothing durable batch enqueue.
- The facade exposes job snapshots, retry, cancel, and persistence status so existing Studio controls can be connected later without duplicating FFmpeg command logic.
- Probe and plan operations can block briefly: **call them on a worker thread** when connecting the approved native GUI. There are no new clickable UI controls in this source-only update.

### Core processing reliability

- Stop an FFmpeg worker that produces no progress output for 120 seconds, including when a descendant holds the progress pipe open.
- Require private log-directory creation; keep encoder stderr out of the application console when log-file creation fails.
- Reap the child and clean temporary output on failure, cancellation or stream stall.
- Check cancellation again before making a completed export visible at its final destination.
- Preserve the existing atomic no-overwrite output commit, versioned queue state and interrupted-job recovery model.

### Repository-only test sources

- `test_studio_media_inspector.cpp` uses fake FFprobe executables in a temporary directory to check track parsing, duration, codec advice, timeouts and service submission.
- `test_studio_fake_ffmpeg.cpp` uses a fake encoder to exercise success, failure, queue reload and cancellation without real codecs or user media.
- `audit_studio_source.py` checks the additional source contracts. Existing `test_studio_media_job_engine.cpp` remains in place.
- None of these is a GitHub Actions workflow or a released executable; test source is not evidence that the tests have passed.

## Next work after this candidate

Connect `StudioService` to the existing Studio controls without shifting approved UI geometry, provide queue display and log viewing, add preset and trim controls, and perform native compilation and real-media playback/processing tests in a future owner-approved session.

**No ZIP, installer, executable release, GitHub Actions, external-drive project work, user testing request, or application-version increase.**
