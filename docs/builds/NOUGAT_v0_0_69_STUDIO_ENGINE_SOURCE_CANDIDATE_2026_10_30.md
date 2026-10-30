# Nougat Studio: Shared Processing Engine (v0.0.69 source candidate)

**Date:** 2026-10-30
**Status:** Source code only. No binary build, FFmpeg runtime validation, owner acceptance, or GUI wiring.

## Native foundation

New native modules: `src/studio/media_job_engine.hpp` and `.cpp`. They are included in the existing native CMake source target without changing its name or app version. Studio UI geometry is untouched.

**Eight presets:** MP4 and MKV lossless remux; H.264/AAC MP4; VP9/Opus WebM; and first-audio-track extraction/conversion to MP3, FLAC, WAV, or Opus. FFmpeg and the specified encoders/muxers must exist. Incompatible streams can cause lossless remux to fail, as expected.

## Queue and batch behavior

- One background worker processes jobs sequentially. Queue limit: 10,000 records.
- Job identity, input, output, preset, optional duration, progress, state and error are recorded.
- States: Queued, Running, Completed, Failed, Cancelled, Interrupted.
- Enqueue one job or an **atomic batch**: reject duplicate outputs, missing sources, existing destinations, or unsupported extensions before committing the batch.
- Persist queue in XDG state home under `nougat-media-plus/studio-queue.v1` with hex-encoded fields, same-folder temporary state writes, fsync and rename.
- Previously queued or running jobs are marked Interrupted after restart and require explicit retry. Corrupt queue files stop acceptance of new jobs instead of being overwritten.
- FFmpeg progress uses `-progress pipe:1`; percentage requires a supplied expected duration. Actual completion requires a successfully published output file.
- FFmpeg errors go to a private `studio-logs/job-ID.log` under the queue directory.
- Cancellation or shutdown stops the child process group and reaps the child.

## Output protection

- Absolute source and destination paths are required. Source must be a readable regular file and output directory must exist.
- An existing output, including a symlink, is rejected; originals are never overwritten.
- Export uses a private temporary file in the final output directory; FFmpeg's muxer is specified explicitly.
- On success, `link(temp, output)` provides atomic no-overwrite publication, followed by temporary cleanup. Failed jobs clean up temporary outputs.
- Uses `execv`, not shell interpolation or arbitrary command strings.

## Source tests included for future validation

- `tools/tests/test_studio_media_job_engine.cpp`: lifecycle, encoder unavailable, input/output protection, queue reload, retry, batch rejection and persistence corruption checks.
- `tools/tests/audit_studio_source.py`: source contract and CMake wiring checks.
- Neither suite is claimed as a completed compiler or end-to-end media test. No GitHub Actions, downloads or owner terminal commands.

## Still to develop and verify

Native compile/link, actual transcoding, stream-compatibility checks, cancellation under load, low-disk behavior, FFprobe duration discovery, user-facing Studio queue controls, ETA, advanced presets, hardware acceleration, audio trim, timeline and Library import. The existing Studio UI has not been changed. This is a real backend source candidate, not a finished Studio release.
