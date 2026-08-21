# ReddMedia Company Bible

**Status:** Canonical project operating law  
**Scope:** ReddMedia only  
**Authority:** The owner has final authority over scope, acceptance, release state, and exceptions.

## 1. Purpose

This is the one canonical project Bible for ReddMedia. It defines how ReddMedia is planned, changed, validated, packaged, explained, committed, published, and recovered. It must not contain rules, source, secrets, history, or identifying material from unrelated projects.

No second ReddMedia Bible may be created. Amend this file in an owner-approved build when a durable ReddMedia rule changes.

## 2. Owner authority and stop law

- The owner's explicit instruction overrides a proposal, plan, convention, or inferred preference.
- When the owner says **stop**, all building, editing, packaging, command generation, and tool activity stops immediately.
- Work resumes only after a new explicit owner instruction.
- No unapproved feature, cleanup, redesign, dependency, icon, launcher behavior, or workflow change may be slipped into a build.
- A real scope conflict must be reported before work continues. Ordinary implementation details inside approved scope do not require repeated approval.

## 3. Required reading order

Before changing ReddMedia, read:

1. this Bible in full;
2. the current roadmap;
3. the current known-bugs record;
4. the current changelog;
5. the latest relevant handshake and validation records;
6. the source paths affected by the approved build.

The active Git commit, branch, worktree state, expected executable, rollback snapshot, and relevant runtime state must be verified before changes are applied.

## 4. Build-scope law

- Restate the proposed build with its base, goal, included work, exclusions, tests, package name, and acceptance boundary.
- Do not start implementation until the owner approves that scope.
- Complete every approved item in the same build unless the owner approves a split or a genuine blocker is reported.
- Do not silently defer an approved item to a future version.
- Fix the cause, not merely the visible symptom.
- Preserve working behavior outside the approved scope.

## 5. Version and repair law

- New owner-approved functionality advances the numeric version.
- A candidate that fails validation or owner testing is repaired under the same numeric version until it passes or the owner changes the plan.
- A technically working checkpoint is not automatically accepted.
- Do not tag a build as accepted or final before explicit owner acceptance.
- Build records must distinguish proposed, candidate, technically working, accepted, rejected, and repaired states accurately.

## 6. Source and privacy law

- The repository contains ReddMedia material only.
- Do not place credentials, tokens, API keys, personal data, unrelated private-project material, generated runtime trees, caches, or build debris in Git.
- Runtime state belongs under the user's standard config, data, and cache directories.
- Credentials must be stored with owner-only permissions and must never be printed in logs, terminal commands, process arguments when avoidable, documentation, test output, commits, or packages.
- User media must never be deleted by a library unlink, refresh, repair, rollback, or uninstall operation.

## 7. ReddMedia identity law

The approved red-tree artwork is ReddMedia's identity.

Every build, patch, repair, and hotfix must apply the red-tree identity to all approved active surfaces, including:

- the raw versioned executable as shown in Files/Nautilus;
- the application launcher;
- the running window, dock, and app-switcher identity;
- the in-app version badge where used.

The raw executable is a mandatory release gate. The final executable must be written first, then its `metadata::custom-icon` value must be set to the approved red-tree asset URI and read back. Rebuilding or replacing the executable after that step invalidates the icon proof and requires the assignment again.

Metadata readback proves assignment, not appearance. Owner visual confirmation in Files/Nautilus remains required for acceptance. A missing or wrong raw-executable tree icon is a build failure.

## 8. Media-server ownership law

- ReddMedia may start and supervise its integrated Jellyfin process.
- Closing ReddMedia must stop and reap only the Jellyfin process ReddMedia launched.
- A separately started Jellyfin process must not be killed by ReddMedia.
- Start Server, Stop Server, and Refresh Server controls must report their real state.
- Normal close and forced parent death must release an owned server and port 8096.
- Server data, config, cache, logs, and the bundled runtime remain separate from Git-tracked source.

## 9. Library and Discover law

- Movies and TV are distinct types at the API, model, filtering, selection, test, and UI boundaries.
- A Movie action may return only a movie. A TV action may return only a TV title.
- Library browsing uses real catalog metadata and direct local paths.
- Linking and unlinking folders changes the catalog only and never modifies the owner's media files.
- Posters use real Jellyfin or TMDb artwork, are cached locally, and fall back to **NO POSTER** only when no usable image exists.
- Episode tiles must show verified season and episode numbers plus a verified episode title. Technical format belongs on a secondary line. Missing identity stays explicitly unavailable until an exact catalog or TMDb match supplies it.
- External recommendations use TMDb only when the owner supplies a valid API key or read access token.
- A replacement credential must validate before it replaces the currently working credential.
- The owner must be able to Test, Save/Replace, and Clear the TMDb credential.
- A 401 response must be explained clearly without exposing the credential.
- Random recommendations do not use viewing history. Usual recommendations may use private local viewing history and local embeddings.
- ReddMedia must never invent a title to hide an API, catalog, model, or network failure.
- Watch availability uses the exact owner-selected region and the complete provider categories returned through TMDb's JustWatch data. ReddMedia must show attribution, freshness, and an explicit no-listing state; it must not infer availability or bypass a provider's official playback path.

## 10. Diagnostic truth law

- Debug and health surfaces may report only facts backed by current process state, API responses, filesystem checks, catalog fields, or recorded request failures.
- Green means no active problem was detected by the checks that actually ran. Yellow means a recoverable or incomplete state needs attention. Red means a required service, runtime, path, or operation is unavailable or failed.
- Diagnostics must identify evidence and a relevant next action, keep credentials redacted, and distinguish a missing field from a failed request.
- Refresh Metadata, Test TMDb, Refresh Server, Open Logs, Retry, and Copy Report must invoke their real corresponding actions when presented.

## 11. Responsiveness and progress law

- Network, catalog, poster, inference, and server operations must not freeze the main UI.
- A full-width red progress surface must appear while approved Library, poster, Discover, TMDb, or server work is active.
- Use measured progress when the work has measurable stages or item counts.
- Use a moving indeterminate state when total work cannot be measured honestly.
- Fast operations may complete the bar quickly; slow operations must continue to show activity.

## 12. Engineering and validation law

- Warnings are errors for release builds.
- Tests must exercise behavior, not only search source text.
- Every fixed owner-reported defect receives a regression test when deterministic automation is practical.
- Validate both success and failure paths, including invalid credentials, 401 handling, credential replacement preservation, missing artwork, type crossover, independent-server preservation, owned-server shutdown, and generated-runtime Git exclusions.
- Stub tests may provide deterministic isolation, but required real-runtime and owner-visible gates must still be identified and performed.
- No test may use the owner's real credential or media.

## 13. Changed-files package law

The normal handoff is one changed-files-only ZIP containing exactly the files required to move the verified base to the candidate.

The package must include:

- an installer with exact base preflight and rollback;
- a manifest with file paths, byte counts, and SHA-256 hashes;
- changed source and documentation;
- required tests and build records;
- required license updates.

The package must not include generated runtime directories, caches, temporary build trees, credentials, or unrelated files. The ZIP itself must pass an integrity test before delivery.

## 14. Installer and rollback law

- Preflight must verify the exact approved base commit, branch, tracked worktree, staged state, executable version, rollback snapshot, and required tools.
- Expected generated runtime directories may remain untracked and must be preserved.
- Unexpected tracked or untracked changes are a stop condition; do not erase them.
- Verify the package manifest before applying files.
- Build in a temporary directory and replace the active executable only after validation succeeds.
- If installation fails after application begins, restore the exact prior tracked state and active executable while preserving user data and expected runtimes.
- Never report rollback or installation as passed unless the proof actually passed.

## 15. Terminal-command communication law

When the owner must run a command:

- first state exactly what the command does and whether it is snapshot, Git, GitHub, install, validation, or another operation;
- provide one complete copy-and-paste command block for that operation;
- ensure all quoting and multiline messages terminate correctly;
- do not leave the shell at a continuation prompt;
- do not put `exit`, `exit 1`, or a command that closes the owner's terminal in the block;
- do not claim success until the owner pastes the result;
- explain the pasted output before starting any new work.

Snapshot, local Git commit, and GitHub push are three separately named operations. Each should require one correct command in its stage.

## 16. Checkpoint, Git, and GitHub law

After the owner approves saving a state:

1. create and verify one recoverable snapshot;
2. stage the intended source while excluding generated runtimes and caches;
3. inspect staged names and whitespace errors;
4. create one descriptive local Git commit that records accomplishments and known issues honestly;
5. verify the new commit and worktree state;
6. push the exact commit to the configured GitHub remote;
7. verify the remote branch resolves to the local commit.

Large tracked assets must be explained when GitHub reports size warnings. New large binary assets should use an owner-approved distribution or large-file strategy rather than silently expanding normal Git history.

## 17. Acceptance checklist

A ReddMedia candidate is ready for owner testing only when:

- the approved scope is complete;
- builds and automated validation pass;
- the package and manifest pass;
- rollback behavior is proven;
- generated runtimes remain excluded from Git;
- version and documentation agree;
- owned server shutdown is proven;
- Movie/TV separation is proven;
- TMDb credential lifecycle and poster paths are proven;
- the final raw executable has the red-tree metadata assignment read back;
- the owner receives one clearly explained install command and the explicit remaining visual checks.

Only the owner decides whether the candidate is accepted.

## 18. Nougat inside ReddMedia

1. Nougat is an integrated ReddMedia subsystem, not a required standalone project installation.
2. The owner-approved top-level order is `Video Player | Library | Discover | Nougat | YouTube | P2P | Debug` until the owner explicitly changes it.
3. Existing ReddMedia behavior remains preserved when Nougat changes unless the owner explicitly approves changes outside Nougat.
4. Nougat's content surface preserves its approved cocoa/chocolate/nougat-tan/caramel/cream palette unless the owner explicitly changes it.
5. Read-only Nougat output that displays useful text must remain selectable and copyable with normal mouse selection, Ctrl+C, Ctrl+A, and a right-click Copy/Select All menu.
6. ReddMedia/Nougat owner-created code is distributed to recipients for noncommercial use under the project license. Third-party components retain their own upstream licenses. The owner retains all rights not granted by that license, including the right to commercialize or separately license the owner's own original work.
7. The active Nougat data belongs under ReddMedia's user-data tree and must not depend on an archived standalone Nougat project folder.

### v0.0.19 UI and TV continuity rules

- Exact current top-level order remains `Video Player | Library | Discover | Nougat | YouTube | P2P | Debug`; the future `Stream` rename is roadmap-only.
- Normal action buttons use one compact app-wide size, sit immediately beside one another, and horizontal rows use mouse-wheel scrolling when they overflow.
- The red ReddMedia top bar remains common to the suite. Interior tab identity: Video Player red, Library forest green, Discover plum, Nougat cocoa/tan/caramel/cream, YouTube red, P2P deep blue, Debug amber/yellow.
- Video Player footer order from top to bottom is seek/time, volume with live percent, then the compact control row.
- TV playback queues the following episode through season boundaries after natural completion, while manual Stop cancels autoplay. Switching to Video Player must not discard the current Library series/season navigation context.
