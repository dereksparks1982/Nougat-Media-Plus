# ReddMedia v0.0.19 Nougat/UI/TV Continuity Repair Handshake

## Project and version

- Project: ReddMedia
- Candidate: v0.0.19 replacement candidate
- Required base: exact owner-supplied v0.0.18 touched-file hashes plus root executable reporting `ReddMedia v0.0.18`
- Target root executable: `ReddMedia_v19`
- Candidate status: test candidate only; not owner-accepted, committed, tagged, or pushed

## Owner-approved scope

- Preserve ReddMedia and the existing Video Player, Library, Discover, YouTube, P2P, Debug, Jellyfin, AI, and media behavior.
- Keep the exact current top-level order `Video Player | Library | Discover | Nougat | YouTube | P2P | Debug`. Future `Stream` work remains roadmap-only.
- Keep Nougat integrated natively inside ReddMedia with no dependency on the archived standalone Nougat project.
- Preserve Nougat Ranked/RAW search, crawler, peers, Tor path, candy-bar colors, and selectable/copyable read-only crawler output.
- Standardize normal app action buttons to the compact top-bar footprint and place buttons directly beside one another.
- Mouse-wheel scroll the top navigation and overflowing app-wide action-button rows horizontally.
- Video Player footer: seek/time row above volume row above compact player buttons; keep Fullscreen reachable at half-screen/narrow widths.
- Show live volume percentage and update it from slider, wheel, and keyboard volume changes.
- Give tab interiors distinct identities while preserving the common red top bar: Video Player red, Library forest green, Discover plum, Nougat cocoa/tan/caramel/cream, YouTube red, P2P deep blue, Debug amber/yellow.
- TV episodes autoplay the next episode after natural completion, continuing across season boundaries. Manual Stop cancels autoplay. Returning to Library preserves the current series/season navigation context.
- Same-version installer repair: automatically install Ubuntu `xvfb` and `x11-utils` only when `xvfb-run`/`xwininfo` are missing, then rerun the full validation chain.
- Original ReddMedia/Nougat code is noncommercial for recipients under the project license; third-party code keeps upstream licenses; the copyright owner retains all ungranted rights including commercial use/licensing of the owner's original work.

## Rollback

Before applying touched files, the installer creates an exact pre-v0.0.19 rollback snapshot under `$HOME/DKLab/Archives/ReddMedia Archive/`. Any required failure after application restores the touched v0.0.18 paths and removes candidate-only paths/root executable while preserving generated runtimes and user data.

## Validation

- Nougat Ranked/RAW/crawler/peer/persistence tests.
- Native Nougat C++ bridge test.
- Retained ReddMedia provider/recommendation/metadata/diagnostics and server-lifecycle tests.
- v0.0.19 UI-contract checks for compact buttons, wheel-scroll rows, footer order, volume percentage, tab themes, and TV autoplay wiring.
- C++17 deterministic native build with `-Wall -Wextra -Werror`.
- Bounded Xvfb live-window smoke test.
- Owner-machine full libtorrent + pinned llama.cpp native build.
- Root `ReddMedia_v19` exists, is executable, reports `ReddMedia v0.0.19`, and receives the approved red-tree icon metadata before FINAL PASS.

## Roadmap-only after v0.0.19

- Replace YouTube with the future `Stream` area covering YouTube, Rumble, RUTUBE, VK Video, and OK.ru where supported.
- Plex account/server linking, discovery, library browsing, native playback, multiple servers, and supported Plex Pass paths.

## Package

`ReddMedia_v0_0_19_PINNED_AI_MODEL_RUNTIME_LAYOUT_REPAIR_CHANGED_FILES_ONLY.zip`

## Continuation point

Owner applies candidate, visually/functionally tests the full ReddMedia suite, then explicitly accepts or rejects v0.0.19. Acceptance is required before archive snapshot, commit/tag, or push closeout.

## v0.0.19 Viewing-history completion repair

- Added the missing `RecommendationEngine::record_completed` and `ViewingHistory::record_completed` API to the changed-files payload so the native build receives the same completion support used by TV autoplay.
- Viewing history now persists a `completed` flag with a non-destructive SQLite migration for existing history databases.
- Starting playback records `completed=0`; natural episode completion records `completed=1`. Manual Stop does not call the completion path.
- Deterministic validation now compiles the real recommendation/history sources and verifies an ordinary started movie remains incomplete while a completed TV episode persists as complete.
- Same target remains v0.0.19 because the preceding candidate failed compilation and rolled back to v0.0.18.


## Pinned AI model/runtime-layout repair continuation

- Prior completion-repair candidate compiled and linked the full native `ReddMedia_v19`, then failed only when its real Discover AI self-test could not resolve the pinned Nomic model.
- Actual package/source inspection proved the deployed v0.0.18 changed-file baseline does not carry the model, while the owner-supplied component archive does.
- The repair package carries the exact verified 84,106,624-byte model and Apache-2.0 license.
- The installer verifies SHA-256 before installation, handles absent/correct/non-matching model states without guessing, and rolls back any model it added/replaced if the candidate later fails.
- The temporary full-build executable is given a project-equivalent model path before its real AI self-test.
- The final root `ReddMedia_v19` must also pass `--discover-ai-self-test` from `$HOME/DKLab/Projects/ReddMedia` before FINAL PASS.
