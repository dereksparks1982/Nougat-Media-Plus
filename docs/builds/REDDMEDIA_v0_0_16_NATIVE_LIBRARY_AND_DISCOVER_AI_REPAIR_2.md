# ReddMedia v0.0.16 Native Library and Discover AI Repair 2

## Owner-test failure

The first repair correctly rejected a dirty v0.0.15 worktree, but those files were leftovers from the original candidate's failed rollback. The original rollback put `REDDMEDIA_PATCH_MANIFEST.json` in its modified-path list even though that file does not exist in committed v0.0.15. Passing that untracked path to `git restore` made the entire restore command fail before it restored any tracked file. `|| true` then hid the Git failure and printed a false rollback-pass message.

## Same-version correction

- Classify `REDDMEDIA_PATCH_MANIFEST.json` as a v0.0.16 addition.
- Restore the complete tracked index and worktree atomically from commit `d67cf6e5e0e3ce3036adae5d9695147a7aa771e8` with one pathspec rooted at `.`.
- Remove each known v0.0.16 addition separately after the tracked restore.
- Preserve `components/jellyfin/runtime/`, which is the accepted untracked v0.0.15 server runtime.
- Never mask a tracked-restore failure.
- Print the exact tracked or staged paths that cause preflight rejection.
- Recognize the exact, hash-verified leftovers produced by the rejected original candidate, save their patch and manifest under ReddMedia's local install backups, recover the committed baseline automatically, and continue installation. Any other dirty state remains a deliberate stop condition.

## Regression proof

`tools/test_installer_rollback_v16.py` creates a disposable Git repository containing simultaneous staged and unstaged tracked changes, an untracked patch manifest, another v0.0.16 addition, and an accepted untracked Jellyfin runtime fixture. It runs the corrected restore semantics and verifies:

- staged and unstaged tracked changes return to the committed baseline;
- exact v0.0.16 additions are removed;
- the accepted untracked Jellyfin runtime remains unchanged;
- both tracked and staged Git diffs are clean.

## Package

`ReddMedia_v0_0_16_NATIVE_LIBRARY_AND_DISCOVER_AI_REPAIR_2_CHANGED_FILES_ONLY.zip`
