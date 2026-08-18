# ReddMedia Known Bugs

## v0.0.16 owner-test focus

- The first repair stopped at preflight because the original candidate's rollback had silently left tracked v0.0.16 files in the worktree. Repair 2 corrects the manifest classification, restores all tracked paths atomically, removes additions separately, and regression-tests preservation of the accepted untracked Jellyfin runtime.
- The rejected first v0.0.16 candidate attempted to compile llama.cpp's unrelated optional `llama-app` target and rolled back at missing `arg.h`/`build-info.h` includes. The same-version repair disables that target and builds only the required library.
- v0.0.15 intentionally left its hidden catalog running after the window closed. The v0.0.16 repair reverses that behavior: owner validation must confirm that closing ReddMedia removes its Jellyfin process and releases port 8096.
- The first Usual recommendation after launch loads the 80 MiB local embedding model and can take longer than later requests, especially on older CPUs.
- External recommendations require internet access, `curl`, and a valid TMDb read-access token supplied by the owner. ReddMedia reports a service error instead of inventing a title when TMDb is unavailable.
- Poster and hierarchy quality depends on the real metadata returned by the hidden catalog. Files that cannot be identified may have a text-only tile and cannot be grouped into a box set by guesswork.
- Library scans remain asynchronous. A newly linked large folder may need a later Refresh before all titles appear.
- Port 8096 is reserved only while ReddMedia and its hidden local catalog are running.

## v0.0.15 owner-test focus

- The startup-readiness candidate reached the real catalog but rolled back after checking for the generated video before Jellyfin's queued scan completed. The scan-completion repair now waits for an actual indexed path; owner validation must confirm that the installer reaches its final pass.
- The first native-Library candidate rolled back safely after the temporary Jellyfin setup server was mistaken for the ready API. The readiness gate now uses `/Startup/User`.
- The repaired candidate still requires owner confirmation that a real media folder appears in the native Library and that **Play Selected** opens the chosen title in ReddMedia's existing embedded player.
- Large first-time libraries can take time to scan. ReddMedia shows a working status and performs catalog work away from the X11 event loop; the owner should press Refresh if new files are still being indexed.
- Port 8096 must be free during installation and normal integrated-server startup; an existing Jellyfin or other service on that port is a deliberate stop condition.

## v0.0.14 owner-test focus

- v0.0.13 owner testing found that a local video paused for several minutes could freeze the application and prevent normal close. v0.0.14 changes the local pause/poll/shutdown lifecycle and requires owner validation with long pauses before acceptance.

## Carried forward from accepted v0.0.13

## v0.0.13

- Actual YouTube playback resolution is not yet directly reported by ReddMedia and may appear below the intended 1080p target.
- Seeking may take approximately 10 to 15 seconds before playback resumes.
- Playback can enter repeated buffer/play cycles after seeking.

These items move to v0.0.14.
