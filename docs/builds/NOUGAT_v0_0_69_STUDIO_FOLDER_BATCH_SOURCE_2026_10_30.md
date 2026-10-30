# Nougat Media Plus v0.0.69: Folder Batches and Queue Control

**Source change:** October 30, 2026  
**Release status:** Unreleased source candidate. Native compile and application testing are not claimed.  
**Version:** v0.0.69, unchanged.

## Studio Media Convert workflow

- In the existing Media Convert panel, press **Ctrl+B** to choose a source folder using Nougat's built-in browser. A selected folder triggers a non-destructive background preview.
- By default only files directly in the selected folder are scanned. **Ctrl+Shift+B** switches to recursive scanning (maximum four levels in the UI) and refreshes the preview.
- Choose the already-existing **Output Folder** and **Format** controls. Changing these options invalidates any previously accepted preview and requests another scan.
- The preview shows eligible and skipped counts. **Ctrl+Shift+C** copies file-level plans and skip reasons to the clipboard.
- Only pressing the existing **Queue Export** button after a successful preview will submit the batch. The UI queues at most 200 sources in this workflow, with FFprobe validation happening asynchronously.
- A batch is one durable enqueue transaction. Any failing input inspection, output collision or state-persistence failure rejects the entire proposed batch, rather than silently queuing only some jobs.
- The existing single-file conversion path, approved button rectangles and panel sizes remain unchanged.

## Folder scan safety

- Source and destination must be existing, separate absolute directories that cannot contain each other.
- Root directory symbolic links are rejected. Hidden files/directories and symbolic-link entries are not followed or imported.
- File-extension allowlist covers commonly used video and audio containers, without claiming they are all decodable.
- Input scanning is bounded (at most 20,000 traversed entries, 1,000 planned files in the reusable backend, 200 in the current UI, recursion depth at most eight in backend).
- Output filenames use the source stem plus `-nougat` and the preset extension. Duplicate stems and existing outputs are **skipped with reasons**, never overwritten or renamed behind the user's back.
- Proposed outputs reserve currently queued/running destinations. The preflight stage does not open/create output files, transcode, or modify sources.
- The final submit path still runs StudioService stream inspection and the existing engine's no-overwrite checks. Changing filesystem state during the preview can cause a safe rejection at submission time.

## Job queue controls

- **Ctrl+P** toggles a session-local pause of **future** jobs. A currently running FFmpeg process is allowed to finish, and pause never kills or suspends its PID.
- **Ctrl+Up** / **Ctrl+Down** move the selected *queued* job earlier/later among other queued jobs. Active/completed/cancelled jobs cannot be reordered.
- Ordering changes persist in the durable queue. If writing state fails, ordering is rolled back.
- Pause is intentionally session-local; interrupted/queued work is not silently restarted on application relaunch.
- Existing Cancel, Retry, history scrolling, Ctrl+C diagnostic copying and Ctrl+Delete history removal remain intact.

## Future testing and known limitations

The added C++ tests exercise scanning, collisions, symlinks, source/output overlap, recursion limits, queue pause/order and restart safety using disposable local fixtures. The Python source audit checks integration and geometry anchors. Both CMake source lists include the new folder planner, and the isolated core CMake project lists the two new tests.

No test suite is being automatically executed, and no compiler/runtime or user-device acceptance result is claimed here. Future validation must cover huge folders, permission failures, case-sensitive collision behavior, real media stream probing, full 200-file export batches, and cancellation/relaunch. The approved interface must not be moved without explicit owner permission.

**No executable, download, installer, GitHub Actions, network deployment or application version change.**
