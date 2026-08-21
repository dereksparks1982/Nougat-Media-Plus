# ReddMedia v0.0.19 Nougat/UI/TV Continuity Repair Validation

## Candidate status

Test candidate only. Not owner-accepted and not tagged.

## Required deterministic gates

- Nougat engine: Ranked, RAW, crawler validation, peer online/offline, persistence.
- Native C++ Nougat bridge against the integrated engine.
- Retained v0.0.18 provider, My Services, episode identity, poster fallback, diagnostics, recommendation type separation, credential preservation, and server lifecycle behavior.
- UI contract: exact top-level order, compact app-wide action-button sizing, adjacency, horizontal mouse-wheel scrolling, seek/volume/button footer separation, live numeric volume percentage, tab interior themes, TV next-episode autoplay wiring, and Nougat output selection/copy.
- CMake deterministic P2P/AI stub target links under C++17 with `-Wall -Wextra -Werror`.
- `ReddMedia_v19 --version` returns `ReddMedia v0.0.19`.
- Built-in Discover AI self-test passes.
- Xvfb live-window smoke confirms a visible ReddMedia window remains alive for the bounded observation period.

## Owner-machine gates

The installer additionally requires the existing installed libtorrent and pinned llama.cpp runtime, builds the full non-stub executable, verifies its version/self-test, copies it directly to `$HOME/DKLab/Projects/ReddMedia/ReddMedia_v19`, verifies executable/root placement, applies/read-backs the red-tree raw-file icon metadata, and only then updates the launcher/removes the superseded v0.0.18 root executable.

If `xvfb-run` or `xwininfo` is missing, the installer uses Ubuntu APT to install `xvfb` and/or `x11-utils`, then verifies the tools before continuing. A failure never closes the interactive terminal.

## Owner-visible acceptance checks

- Top order exactly: **Video Player | Library | Discover | Nougat | YouTube | P2P | Debug**.
- Top navigation wheel-scrolls horizontally at narrow width.
- Normal action buttons are compact, same footprint, adjacent, and overflowing rows wheel-scroll.
- Video Player at half-screen keeps Fullscreen reachable; volume sits above buttons; seek sits above volume; live volume percent is visible and updates.
- Library green, Discover plum, Nougat candy-bar, YouTube red, P2P blue, Debug amber/yellow interiors.
- Playing a TV episode naturally continues to the next episode and across season boundaries; manual Stop stops the sequence.
- Returning to Library while/after playback returns to the same series/season context rather than the Library root.
- Nougat crawler output can be highlighted, Ctrl+C/Ctrl+A copied/selected, and right-click Copy/Select All.
- Existing ReddMedia playback, Library, Discover, YouTube, P2P, Debug, media server, and data remain normal.

## v0.0.19 Viewing-history completion repair

- Added the missing `RecommendationEngine::record_completed` and `ViewingHistory::record_completed` API to the changed-files payload so the native build receives the same completion support used by TV autoplay.
- Viewing history now persists a `completed` flag with a non-destructive SQLite migration for existing history databases.
- Starting playback records `completed=0`; natural episode completion records `completed=1`. Manual Stop does not call the completion path.
- Deterministic validation now compiles the real recommendation/history sources and verifies an ordinary started movie remains incomplete while a completed TV episode persists as complete.
- Same target remains v0.0.19 because the preceding candidate failed compilation and rolled back to v0.0.18.


## Pinned AI model/runtime-layout repair gate

The previous owner-machine run reached a clean full native link and then failed `--discover-ai-self-test`. Investigation of the actual source/package proved two concrete causes rather than assuming one:

1. The verified component archive contains the required Nomic GGUF, but the deployed v0.0.18 changed-file baseline used by the candidate does not contain that 84 MB model file.
2. The installer ran the full-build executable from its temporary `/tmp/.../full` directory. ReddMedia resolves the model from `exe_dir()/components/ai/models/...`, so that test looked beside the temporary executable rather than beside the installed ReddMedia executable.

The repair therefore bundles and hash-verifies the exact model, installs/preserves it rollback-safely, mirrors the project model directory beside the temporary build executable for the full native AI gate, and reruns the real Discover AI self-test from the final root `ReddMedia_v19` before FINAL PASS.

Pinned model proof:

- File: `components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf`
- Bytes: `84106624`
- SHA-256: `d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac`
- Upstream license: Apache-2.0 (license bundled with the repair package)


### Repair-package validation performed before handoff

- Re-applied only the replacement package payload to a fresh exact v0.0.18 source tree whose touched-file hashes match the manifest.
- Nougat Ranked/RAW/crawler/peer/persistence tests: PASS.
- Native Nougat bridge: PASS.
- Retained ReddMedia behavior and media-server lifecycle lanes: PASS.
- App-wide UI contract, volume percentage, TV autoplay, viewing-history completion, and tab-theme lane: PASS.
- Installer rollback/root/model contract: PASS.
- C++17 `-Wall -Wextra -Werror` deterministic P2P/AI stub build: PASS; version and Discover AI stub self-test: PASS.
- The container cannot complete the real llama.cpp link because the owner-supplied pinned llama runtime requires `GLIBC_2.43` while this build container is older; the linker reports `sqrtf@GLIBC_2.43`. This is an environment boundary, not presented as a PASS.
- The owner machine already proved the same full native source can compile/link through `ReddMedia_v19`; its remaining failure was model resolution. The replacement installer fixes that exact model-presence and executable-relative-layout path and re-runs the real AI test twice on the owner machine.
