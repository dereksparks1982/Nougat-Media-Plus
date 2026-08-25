# Nougat Media Suite v0.0.41 Repair v7 Validation

Status: **REJECTED-BUILD REPAIR CANDIDATE. Owner acceptance is still required.**

## Exact repair scope

Repair v7 is a surgical delta on the exact rejected repair-v5 source state. It changes only:

1. Movies/TV Library grid geometry: restore the proven portrait row-fill calculation so shared Movies/TV cards are not height-squeezed and the row width is distributed instead of leaving a dead strip at the right.
2. Library media-card hover popup: repaint on card entry, card exit, and card-to-card movement so a stale description cannot linger until an unrelated repaint.
3. Live TV inner action row: count all eight controls after `Stop Live` so horizontal wheel scrolling can reach `Record`. Global top navigation is untouched.
4. Video-player activity overlay: pointer and media identity share one authoritative 3000 ms timer; the identity is redrawn while the timer is alive so libVLC frame repainting cannot erase it immediately after the mouse stops.

The v0.0.41 IMDb implementation on Movie, TV, and Home cards is preserved unchanged. No unrelated UI/layout/navigation/player/Search/Live-TV behavior is intentionally changed.

## Repair-v6 failure addressed

Repair v6 failed before compilation because its patcher looked for a nonexistent invented source anchor:

`ViewMode::LiveTV,liveTvHeaderScroll,notches,7,std::max(0,W-40)`

The actual accepted-base/repair-v5 code path is:

`scroll_button_row(liveTvButtonsScrollX,7,delta)`

Repair v7 patches that exact source expression to an eight-button extent and uses no `liveTvHeaderScroll` anchor.

## Validation performed before packaging

- Python syntax: PASS for repair-v7 delta and v0.0.41 contract test.
- Bash syntax: PASS for repair-v7 installer.
- Delta-anchor dry run: PASS. Every repair-v7 anchor was applied exactly once against an exact repair-v5 anchor fixture, including the Live TV seven-to-eight control change.
- Focused C++ compile gate: PASS with `g++ -std=c++17 -Wall -Wextra -Werror`. The harness compiled and exercised the row-fill grid calculation, card-hover transition logic, and exact 3000 ms activity boundary.
- Source provenance: the real Live TV wheel expression was independently verified in accepted v0.0.40 source, and the restored row-fill Library formula was independently verified in historical v0.0.29 source.
- Package manifest/hash verification: required before installer execution.
- ZIP integrity: required before handoff.

## Native project gate on the owner machine

This environment does not contain Derek's complete native Nougat source tree plus installed AI/runtime dependencies, so a truthful full Nougat native link cannot be claimed here. The installer therefore reconstructs the exact repair-v5 candidate in a temporary real source tree, applies only the repair-v7 delta, and runs the complete CMake warnings-as-errors native build **before any active-project file is changed**.

Before promotion it also runs:

- `tools/test_nougat_media_suite_v41.py`
- `tools/test_license_protection_v22.py`
- v0.0.35 retained cleanup self-test
- v0.0.36 retained Library/UI/player self-test
- v0.0.37 retained Live TV/System self-test
- v0.0.38 retained Library/Live TV/player self-test
- v0.0.39 retained diagnostic self-test
- v0.0.41 Library/IMDb/grid/hover/activity self-test

If any source anchor, compilation, warning, link, runtime self-test, license gate, root-cleanliness gate, app-closed gate, launcher gate, or icon proof fails, active repair promotion does not occur (or is rolled back after the snapshot boundary).

Nothing in this candidate marks v0.0.41 accepted, committed, tagged, or pushed.
