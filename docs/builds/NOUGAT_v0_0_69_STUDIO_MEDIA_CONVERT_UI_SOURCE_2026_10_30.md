# Nougat v0.0.69: Media Convert Studio Interface Integration

**Date:** October 30, 2026
**Status:** Source candidate committed to GitHub; no native compilation or owner runtime acceptance claimed.
**Scope:** Existing native Studio > Tools page and backend `StudioService` integration.

## New in the Studio Tools page

- **Media Convert** is a third tool launch button below the existing File Splitter and File Assembler buttons. Existing Tool/Drone tabs and the approved positions of those two buttons are unchanged.
- Media Convert opens inside Nougat's existing Studio panel and uses the existing internal file browser. There is no GTK/Qt picker, modal system dialog, new desktop window, or layout replacement.
- Choose a source media file, an output folder, and cycle through eight supported FFmpeg presets. The engine determines export compatibility using FFprobe and refuses invalid track/configuration combinations.
- Automatically suggest an output name such as `movie-nougat.mp4`. If that file exists, propose `movie-nougat-2.mp4`, and so on. Active queued/running output destinations are skipped. The native engine also guarantees no existing output file is replaced during publication.
- **Queue Export** submits asynchronously. FFprobe's bounded inspection runs off the X11 event loop, preventing a slow inspection request from blocking Studio's primary UI.
- The new panel lists recent durable Studio jobs, including numeric job ID, queued/running/completed/failed state, percentage when available, and destination filename.
- Select a row, then **Cancel Job** or **Retry Job**. Engine logic rejects invalid or completed-job retry and protects original media.
- The panel polls queue state periodically and redraws when the signature changes rather than doing continuous full-screen repainting.
- Shutting down Nougat waits for any in-flight media-inspection submission before destroying the underlying Studio service. The persistent worker then owns FFmpeg process termination and state recovery.

## Existing behavior preserved

- Existing File Splitter and File Assembler workflows, fields, browsing, buttons, manifest verification and progress behavior are unchanged.
- Existing Studio Tools and Drone tab positions/geometry are unchanged. All Media Convert controls live within an additive, dedicated Studio panel.
- Existing native application CMake executable target remains as-is. The project/owner version stays v0.0.69; the older CMake v0.0.68 label is not silently altered.
- The suite's external drive is not used as a build/workspace location and no user files are modified by this source commit.

## Technical integration

- `src/main.cpp`: new `StudioPanel::MediaConvert`, tool launcher, dedicated browser paths, preset selection, unique output suggestion, async `StudioService::submit`, event-loop status polling, queue selection and cancel/retry actions.
- `StudioService`, `MediaInspector`, and `Engine` remain separate compiled modules and are reused directly; no shell interpolation or duplicate conversion system was introduced.
- `tools/tests/audit_studio_ui_bridge.py` checks the new panel wiring, browser interactions, queue API use, async lifetime, output naming and retention of old approved tool positions.

## Known limitations

- Native compilation, FFmpeg output playback, cancellation under load, codec compatibility, queue throughput and interface input interaction have NOT been tested by the owner or claimed as passed.
- Output filenames are automatic in this first UI pass; users cannot yet edit a custom output filename inside Media Convert.
- Full advanced codec settings, timeline editing, subtitles/chapters, audio trims, batch selection from the interface, visual progress bar and log viewer remain on the roadmap. The queue backend already supports atomic batches.
- Cancel is available on currently running or queued jobs. A submitted preflight/FFprobe operation must finish before its job can be selected and cancelled.
- FFprobe and FFmpeg executable paths currently default to `/usr/bin/ffprobe` and `/usr/bin/ffmpeg`. Missing tools produce errors rather than silently downloading dependencies.

## Future verification

Before accepting this candidate, compile on a trusted internal Linux checkout; run the Studio core regression tests, inspect the added tool in the existing panel, select multiple files, convert/remux real sample media, test unavailable codecs, repeat exports, cancellation, process exit, restart and recovery, and confirm old splitter/assembler geometry in the approved screenshot.

**No GitHub Actions, executable, ZIP, download, installer, new release, or version increment in this source update.**
