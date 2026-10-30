# Nougat Studio v0.0.69: Advanced Export and Edit Engine

**Source date:** October 30, 2026  
**Version:** v0.0.69 unchanged  
**Status:** Source implementation, not a tested executable or an accepted GUI release  
**Target:** Existing native Linux Studio processing engine

## Delivered in source

The existing Studio job engine now carries an `ExportEdits` value with each job, including folder-batch jobs. These values are validated before enqueue and before FFmpeg execution. They are persisted in **Studio queue storage V3** and survive shutdown/retry. V1 and V2 queue files remain readable and automatically migrate when state is saved. Storage schema V3 is **not an application version bump**.

### Video export and editing

- Frame-accurate-intent trim start/duration (milliseconds) for edited video/audio exports. On stream-copy remuxes, exact cut boundaries still depend on the source's keyframes.
- Bounded output size overrides (even dimensions, 160x120 to 7680x4320), preserving aspect ratio with padding; frame-rate override 1-120 fps.
- Crop rectangle with bounded coordinates, plus clockwise rotation by 90°, 180°, or 270°.
- H.264/MP4 and VP9/WebM encoding via trusted argv-based FFmpeg invocation, quality (CRF), optional target video bitrate and audio bitrate.
- Existing eight presets remain at their original numeric IDs; a **ninth** `SubtitleSrt` preset is appended for extracting source text-subtitle tracks to SRT. Bitmap subtitles that FFmpeg cannot convert to text may fail cleanly.

### Audio and subtitles

- Select an audio stream by ordinal index, drop all audio, set gain from -24 to +24 dB, or normalize using FFmpeg `loudnorm`.
- Remux tracks without re-encoding where allowed. Removing/choosing subtitles or audio tracks uses FFmpeg stream maps instead of deleting anything from the source.
- Add external SRT as a soft subtitle track to MP4/MKV exports where compatible; MP4 uses `mov_text`.
- Burn an external local SRT into video re-encodes. For filtergraph safety, the subtitle **burn-in path** currently permits only letters/numbers and `/`, `_`, `-`, `.`; other path styles are explicitly rejected rather than inserted raw into a filtergraph.
- Select a source subtitle track for remux or SRT extraction. No claims of full language auto-detection or compatibility with every subtitle codec.
- Reject contradictory settings such as crop on audio-only output, filters on stream-copy remux, or subtitle burning while dropping subtitles.

### Saved profiles, queue reliability and estimates

- New local `ProfileLibrary` allows creating, replacing, listing, retrieving and removing up to 100 named profiles. Names are validated; malformed profile files fail closed. Saved files use same-directory atomic replacement, `fsync`, no symlink destinations and owner-private permissions.
- StudioService can apply a saved profile to an export request, but does **not** submit until requested.
- The service preflights selected audio/subtitle tracks against the probed source, validates edits and adjusts progress-duration estimates for trims before enqueuing.
- When duration and explicit bitrates support a meaningful estimate, the export plan reports an **advisory output size** and destination available disk space. CRF-only/stream-copy sizes are intentionally unknown; disk-space warnings are advisory, not guarantees.
- The existing folder-preview-to-batch workflow continues to work. Each item carries its own immutable saved options, and the entire batch is atomically queued or rejected.

### Linux and repository boundaries

- FFmpeg is invoked with an explicit argv vector, never a user-constructed shell script or `system()` call.
- The existing source validation, temporary outputs, no-overwrite publishing, queue restart recovery, cancel/retry, status history and application UI geometry are preserved.
- New C++ tests target argument construction, serialization, invalid edit combinations, subtitle extraction and sidecar handling, profile migration/storage, and queue V1/V2/V3 compatibility.
- Both existing native and isolated CMake source lists are updated, but **no compilation or real transcoding run is claimed**.
- No ROMs, codecs, firmware, downloads, executable, ZIP, GitHub Actions, deployment or external-drive work.

## What is not yet complete

This is substantial engine work, but **advanced editing controls have not been added to the Media Convert GUI**. It is important not to mistake backend capability for a clickable UI feature. Future builds should connect profile selection and editing fields without moving existing geometry, add timeline multi-cut and video joining, audio replacement/mixing, subtitle language tagging, richer advanced presets, and real-media compatibility tests. Always validate with sample files on the internal development tree before owner acceptance.

**Test status:** source contracts and fixture definitions prepared; native compilation, full isolated C++ regression execution, and real FFmpeg/FFprobe device testing are outstanding.
