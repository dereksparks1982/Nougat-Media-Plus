# Nougat Media Suite v0.0.50 Candidate Validation

Status: candidate only. Not accepted. Do not tag, merge to accepted `main`, or describe as accepted until owner testing and the normal acceptance workflow are complete.

## Required automated candidate checks

1. Apply the guarded v0.0.50 `src/main.cpp` migration.
2. Apply neutral shared file/folder picker labels required by Workshop.
3. Validate the v0.0.50 source contract.
4. Enforce the minimum installation invariant: `minimal` contains exactly `core-player`.
5. Run NOUGAT_SPLIT_ARCHIVE split/reassemble regression tests.
6. Configure the native build outside the source tree.
7. Compile with `-Wall -Wextra -Werror`.
8. Verify executable identity is exactly `Nougat Media Suite v0.0.50`.
9. Run retained Games/navigation/fullscreen/window-identity native self-tests.
10. Publish the candidate executable as a temporary CI artifact only after all checks pass.

## Product boundary

The minimum Nougat installation is the video player and only dependencies required by the player itself. All other capability families remain optional by architecture, even while the current C++ executable is still being progressively decomposed into physical modules.

## Acceptance boundary

A green CI run proves only that the candidate passed the automated checks available in that environment. It does not replace owner runtime testing, acceptance snapshot, accepted Git commit/tag workflow, GitHub publication verification, or remote hash verification.
