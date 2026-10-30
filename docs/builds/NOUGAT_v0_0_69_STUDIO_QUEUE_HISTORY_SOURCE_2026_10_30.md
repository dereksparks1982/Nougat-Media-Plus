# Nougat Media Plus v0.0.69: Studio Queue History and Diagnostics

**Date:** October 30, 2026  
**Status:** Source update, native compilation and owner testing not yet performed  
**Delivery:** GitHub source only; no Actions, binary, download, release, or UI geometry change

## Existing integration confirmed

The repository already contains a Studio > Tools > Media Convert interface wired into the FFprobe-based StudioService and persistent FFmpeg job engine. This increment **extends that working source path** instead of building a competing interface.

## Queue history and navigation

- Keep the existing Media Convert source selection, output folder, preset, Queue Export, Cancel Job, Retry Job, list row, and panel geometry.
- Mouse wheel inside the existing job list moves through earlier entries; Page Up and Page Down navigate an entire visible page. Newly queued jobs return to the newest entries.
- Selecting a job displays its ID, state, preset and relevant error in the **existing** status line.
- The in-app Studio file browser now accepts Escape, Enter and Backspace on the Media Convert panel, not only on File Splitter.
- Ctrl+C with a selected job copies its input/output, state, preset, progress, error and most recent 4 KiB of encoder log to the clipboard. It is a deliberate action; logs are not uploaded.
- Ctrl+Delete with a selected terminal job removes its **queue history record only**. Running and queued jobs cannot be removed. Exported media, input files and encoder logs remain untouched.

## Durable queue format (internal schema V2, app remains v0.0.69)

- Existing V1 queue files are read and migrated to a versioned V2 header on the next state save. Previously queued/running items still become *Interrupted* and require explicit retry.
- V2 persists the monotonically increasing **next job ID** in addition to records. Removing the highest-ID historical record cannot cause its identifier or log filename to be reused after restart.
- A corrupt or inconsistent V2 next-ID header fails closed instead of silently replacing existing state.
- Single-job and atomic-batch enqueues persist updated ID counters **with** their queue records, and roll back both in memory if the durable save fails.
- Removing a historical record persists the change atomically, restoring the record in memory if persistence fails.
- Encoder log reads use `O_NOFOLLOW`, require regular files, cap returned data to 16 KiB, and replace control characters with spaces. Logs remain local to the Studio state directory.
- The full job queue remains subject to its existing 10,000-record limit. Forgetting terminal records is explicit, not automatic.

## Future validation

- Compile the native application and isolated Studio core tests on the internal development tree when authorized.
- Exercise migration from V1, ID persistence after history deletion, failed persistence, record removal while processing, log symlink rejection, clipboard text, mouse-wheel and keyboard navigation.
- Test actual conversion and remuxing with FFmpeg under load.
- Consider a future dedicated job-details panel only after the owner approves geometry changes. No existing controls were moved by this source update.

**Version:** Nougat stays **v0.0.69**. The V2 label belongs solely to the private queue storage format.
