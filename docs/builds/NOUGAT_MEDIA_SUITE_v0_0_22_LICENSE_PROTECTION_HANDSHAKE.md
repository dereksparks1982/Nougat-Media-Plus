# Nougat Media Suite v0.0.22 License Protection Handshake

## Base and target

- Required accepted base: commit `b89b0fc187bebc05f3390a76c76cbdc980713600` / tag `v0.0.21`.
- Accepted snapshot: `$HOME/DKLab/Archives/ReddMedia Archive/Nougat_Media_Suite_v0.0.21_ACCEPTED_20260821_025505`.
- Active local repository path: `$HOME/DKLab/Projects/Nougat Media Suite`.
- GitHub repository: `dereksparks1982/Nougat-Media-Suite`.
- Target: v0.0.22 candidate.
- Target root executable: `Nougat_Media_Suite_v22`.

## Approved scope

- Keep PolyForm Noncommercial 1.0.0 as the recipient license for owner-controlled Original Materials.
- Correct the project licensor/copyright identity to Elderred Softworks LLC.
- Add explicit copyright/ownership, contribution, licensing-policy, and pull-request terms.
- Clarify third-party license boundaries without changing any upstream license.
- Add deterministic license-boundary and rollback tests.
- Advance release/version plumbing to v0.0.22 only; retain accepted v0.0.21 application behavior.
- Update the launcher path/version as required by the already-completed repository-directory rename.
- Repair the v0.0.21-to-v0.0.22 runtime handoff: use the accepted llama.cpp runtime at the renamed project path for the base proof, then build v0.0.22 with `$ORIGIN`-relative AI-library discovery so the root executable is not tied to an absolute project directory.

## Package

`Nougat_Media_Suite_v0_0_22_LICENSE_PROTECTION_RUNTIME_PATH_REPAIR_CHANGED_FILES_ONLY.zip`

## Rollback

The installer proves the accepted v0.0.21 executable with the relocated runtime before any source change. Failure after application begins restores the accepted v0.0.21 touched files, root executable, and launcher while preserving generated runtimes, pinned AI model, Search data, media, and user configuration.

## Acceptance boundary

Candidate only until the owner installs, reviews the license files, confirms normal application behavior, and explicitly accepts v0.0.22.
