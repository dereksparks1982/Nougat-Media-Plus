# Nougat v0.0.69 Games Runtime Containment and Discovery Source Candidate

**Date:** 2026-10-30  
**Scope:** Native Games/emulator runtime safety and detection; no GUI geometry changes  
**Status:** SOURCE COMMITTED WHEN MERGED, runtime testing and owner acceptance outstanding

## Implemented source changes

- Add `src/games/emulator_window_policy.hpp` containing a pure, independently testable eligibility rule for new emulator windows.
- Use process tree ownership **or** isolated process-group membership to identify genuine launcher windows. This helps wrappers that exit or reparent child processes.
- Stop using the previous "any fresh large window" fallback. The host refuses to capture an unrelated new desktop application merely because it appears during emulator startup.
- Use title/class fallback only when no unrelated owner PID is reported; retain Xenia EWMH client-list checks and already-contained child adoption.
- When an embedded game window disappears or containment geometry fails, stop the emulator and clear its process group before reporting exit.
- After SIGTERM and, when necessary, SIGKILL, attempt to reap the direct child with bounded waits instead of a single nonblocking poll. Failed execution reports exit code 127; other early exits and signals are distinguished.
- Bound the window wait requested by launchers between 5 and 90 seconds; keep Nougat's authoritative `NOUGAT_EMBED_XID` as the last environment assignment.
- Compile the previously separate emulator registry source into the existing native application target without changing its target name or version.
- Add `inspect_emulation_support()` to report missing executable, possible missing PS2 firmware, or **detected but unverified** runtime. Discovery never launches an emulator and does not claim that a game works.
- Add the C++17 window-policy assertions, emulator-registry discovery test and offline source-contract audit under `tools/tests/`. These are manually invoked checks, not Actions.

## Review constraints and known limitations

- No application executable, asset, downloaded package, ROM, BIOS, emulator installation, or GitHub Actions is produced.
- No UI geometry, color, button position, game-card art or launch-argument behavior is changed.
- This source-only candidate is not proof of emulator operability on the user's graphics driver, controllers or titles.
- Some backends do not provide `_NET_WM_PID` and may require further careful containment integration rather than reintroducing broad desktop-window capture.
- The `ready_emulation_support()` function remains for existing callers, but its historical name does not guarantee playable games. Use the new probe for honest discovery status.
- The main CMake project and native target still carry an older v0.0.68 label; the existing mismatch is noted, not silently changed. The owner-approved project version stays v0.0.69.

## Follow-up verification

Review [Games Backend Verification Matrix](../GAMES_BACKEND_VERIFICATION_MATRIX_v0_0_69.md) before future emulator/runtime validation. Critical cases: start missing emulator, wrapper-process exit, harmless unrelated desktop window opening during launch, title spoof, emulator window unmap, forced close and process reaping, keyboard/controller input, and title relaunch. Run tests on the internal development checkout only; never use the external drive or GitHub Actions.

No owner testing is requested as part of this update.
