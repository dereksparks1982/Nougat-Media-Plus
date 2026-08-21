# Nougat Media Suite v0.0.23 — Exact Concept UI and Stream Repair Validation

Status: CANDIDATE. Not accepted.

## Build-environment validation completed before handoff
- v0.0.23 source contract/regression test: PASS
- v0.0.23 installer rollback contract/simulation: PASS
- Protected license-state hashes: PASS
- Warnings-as-errors stub configuration/build: PASS
- Stub CLI `--version`: PASS (`Nougat Media Suite v0.0.23`)
- Stub Discover AI self-test: PASS
- X11 UI window smoke under Xvfb: PASS
- Approved N active-icon hash gate: PASS
- Package manifest reconstruction/integrity: PASS
- ZIP integrity: PASS

## Native workstation validation still required
The owner's Ubuntu workstation must run the package installer because this build container does not provide the required native `libtorrent-rasterbar` development package and cannot reproduce the owner's real llama.cpp/desktop environment. The installer performs:
- exact base/worktree preflight
- port 8096 preflight
- protected license hash gate
- package/base-file SHA-256 gate
- full source/regression lanes
- warnings-as-errors stub build and X11 smoke
- full native v0.0.23 build
- relative `$ORIGIN` RPATH verification
- final root executable version and AI self-test with `LD_LIBRARY_PATH` removed
- final executable raw custom-icon write/readback
- launcher/icon-theme installation
- rollback on failure

## Owner acceptance gates after FINAL PASS
1. Confirm the top-level layout remains `Video Player | Library | Discover | Search | Stream | Debug`.
2. Confirm only the selected top tab carries the integrated downward point/notch.
3. Confirm the wide/fullscreen top navigation and six player controls center correctly.
4. Confirm the six player buttons remain `Open | Rewind 10s | Play/Pause | Stop | Fast Forward 10s | Fullscreen`.
5. Confirm the seek bar is no longer red and uses the approved Nougat caramel/cream/chocolate treatment.
6. Confirm the volume bar is no longer red, is substantially shorter than the seek bar, and retains the live 0–200% behavior and 100% marker.
7. Confirm the quilted page material and restrained approved per-page tints.
8. Confirm the old candy icon is gone from the raw v23 executable, Files/Nautilus, dock/sidebar, launcher, and running-window identity, replaced by the approved N emblem.
9. Confirm Stream has one shared Direct Play URL field and no redundant Stream `Play` button.
10. Confirm Download, Direct Watch, Open Webpage, and Clear Log still work.
11. Retest the previously failing YouTube Direct Watch URL and capture the Stream log if it still returns 403 or a JavaScript-runtime warning.

No acceptance snapshot, Git tag, or v0.0.23 push is authorized until these owner gates pass.
