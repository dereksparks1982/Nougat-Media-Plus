# Nougat Media Suite v0.0.53 - Xbox 360 / GTA IV Runtime Repair

Status: candidate for owner testing
Version: v0.0.53 (same-version repair)

## Approved scope
- Preserve the existing `Nougat_Media_Suite_v53` executable.
- Add the missing Xbox 360/Xenia runtime bridge at the path v53 already uses.
- Make GTA IV Title ID 545407F2 use Vulkan.
- Keep Xenia windowed so the existing v53 X11 EmulatorHost can embed/reparent
  it into Nougat's Games viewport instead of leaving a separate permanent
  emulator window.
- Amend the Company Bible to permanently remove installer requirements.
- Preserve the exact approved Nougat N identity rule.
- Do not clean, rebuild, or repopulate any build directory.
- Do not change unrelated emulators or functionality.
- No commit, tag, push, or release.

## Package design
This is a direct changed-files overlay. It contains no installer and no build
tree. The existing v53 executable is not replaced because the current emulator
handoff explicitly says not to disturb it; v53 already contains the Xenia
launch logic and embedded emulator host.

The runtime bridge retrieves only the pinned official Xenia Canary Linux
AppImage on first Xbox 360 launch and verifies its exact SHA-256 before use.

## Validation performed in the artifact environment
- `bash -n` passes for `components/games/runtime/xenia/xenia_canary`.
- The launcher contains the pinned Xenia release URL and SHA-256.
- GTA IV Title ID configuration is present.
- Vulkan is forced on the launcher command line.
- Xenia fullscreen is disabled for Nougat embedding.
- The ZIP contains no `INSTALL_*` file.
- The ZIP contains no `build/` or `build-v53/` path.
- The ZIP contains no Nougat executable replacement.
- The ZIP contains no ROM, ISO, XEX, BIOS, firmware, key, or game data.
- The corrected Company Bible contains the owner-directed `NO INSTALLERS` law.

## Owner-machine acceptance still required
Actual Xenia startup, GTA IV gameplay, controller input, X11 embedding, and the
existing root executable's approved N icon remain owner-machine checks. This
record does not claim those real-machine checks passed before the owner runs
the candidate.
