# Nougat Media Suite Company Bible

**Status:** Canonical project operating law  
**Scope:** Nougat Media Suite only  
**Authority:** The owner has final authority over scope, acceptance, release state, and exceptions.

## 1. Purpose

This is the one canonical project Bible for Nougat Media Suite. It defines how Nougat Media Suite is planned, changed, validated, packaged, explained, committed, published, and recovered. It must not contain rules, source, secrets, history, or identifying material from unrelated projects.

No second Nougat Media Suite Bible may be created. Amend this file in an owner-approved build when a durable Nougat Media Suite rule changes.

## 2. Owner authority and stop law

- The owner's explicit instruction overrides a proposal, plan, convention, or inferred preference.
- When the owner says **stop**, all building, editing, packaging, command generation, and tool activity stops immediately.
- Work resumes only after a new explicit owner instruction.
- No unapproved feature, cleanup, redesign, dependency, icon, launcher behavior, or workflow change may be slipped into a build.
- A real scope conflict must be reported before work continues. Ordinary implementation details inside approved scope do not require repeated approval.

## 3. Required reading order

Before changing Nougat Media Suite, read:

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

- The repository contains Nougat Media Suite material and its own historical ReddMedia lineage only.
- Do not place credentials, tokens, API keys, personal data, unrelated private-project material, generated runtime trees, caches, or build debris in Git.
- Runtime state belongs under the user's standard config, data, and cache directories.
- Credentials must be stored with owner-only permissions and must never be printed in logs, terminal commands, process arguments when avoidable, documentation, test output, commits, or packages.
- User media must never be deleted by a library unlink, refresh, repair, rollback, or uninstall operation.

## 6A. Licensing and contribution law

- The project licensor/copyright identity for owner-controlled Original Materials is **Elderred Softworks LLC**.
- Original Materials are licensed to recipients under the **PolyForm Noncommercial License 1.0.0** unless the owner explicitly approves a different license for a defined file or release.
- The recipient license does not restrict Elderred Softworks LLC from commercially using, distributing, sublicensing, dual-licensing, or relicensing its own Original Materials.
- A project-level license never relicenses third-party software, models, artwork, services, APIs, or data. Upstream notices and license copies must remain intact.
- Outside contributions may enter the project only under the owner-approved contributor terms in `CONTRIBUTING.md`, including an inbound grant broad enough to preserve the owner's ability to maintain, sublicense, relicense, and commercially license the combined project.
- No build may change `LICENSE`, `COPYRIGHT.md`, `CONTRIBUTING.md`, `THIRD_PARTY_NOTICES.md`, or `docs/LICENSING_POLICY.md` without explicit owner approval.
- A release must run the current license-boundary regression test before handoff.

## 7. Nougat Media Suite identity law

The owner-approved rounded-square chocolate/nougat **N + play triangle** artwork is Nougat Media Suite's active identity beginning with v0.0.21.

Every build, patch, repair, and hotfix must apply the approved active identity to the raw versioned executable in Files/Nautilus, the application launcher, the running window/dock/app-switcher identity, and the in-app version badge where used.

The raw executable is a mandatory release gate. The final executable must be written first, then its `metadata::custom-icon` value must be set to the approved Nougat Media Suite asset URI and read back. Rebuilding or replacing the executable after that step invalidates the icon proof. Owner visual confirmation in Files/Nautilus remains required for acceptance.

The accepted red-tree artwork remains historical ReddMedia identity through v0.0.20 and must not remain as the active v0.0.21 identity.

## 8. Media-server ownership law

- Nougat Media Suite may start and supervise its integrated Jellyfin process.
- Closing Nougat Media Suite must stop and reap only the Jellyfin process Nougat Media Suite launched.
- A separately started Jellyfin process must not be killed by Nougat Media Suite.
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
- Nougat Media Suite must never invent a title to hide an API, catalog, model, or network failure.
- Watch availability uses the exact owner-selected region and the complete provider categories returned through TMDb's JustWatch data. ReddMedia must show attribution, freshness, and an explicit no-listing state; it must not infer availability or bypass a provider's official playback path.

## 10. Diagnostic truth law

- Debug and health surfaces may report only facts backed by current process state, API responses, filesystem checks, catalog fields, or recorded request failures.
- Green means no active problem was detected by the checks that actually ran. Yellow means a recoverable or incomplete state needs attention. Red means a required service, runtime, path, or operation is unavailable or failed.
- Diagnostics must identify evidence and a relevant next action, keep credentials redacted, and distinguish a missing field from a failed request.
- Refresh Metadata, Test TMDb, Refresh Server, Open Logs, Retry, and Copy Report must invoke their real corresponding actions when presented.

## 11. Responsiveness and progress law

- Network, catalog, poster, inference, and server operations must not freeze the main UI.
- A full-width palette-aware progress surface must appear while approved Library, poster, Discover, TMDb, or server work is active.
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

A Nougat Media Suite candidate is ready for owner testing only when:

- the approved scope is complete;
- builds and automated validation pass;
- the package and manifest pass;
- rollback behavior is proven;
- generated runtimes remain excluded from Git;
- version and documentation agree;
- owned server shutdown is proven;
- Movie/TV separation is proven;
- TMDb credential lifecycle and poster paths are proven;
- the final raw executable has the approved Nougat Media Suite icon metadata assignment read back;
- the owner receives one clearly explained install command and the explicit remaining visual checks.

Only the owner decides whether the candidate is accepted.

## 18. Search inside Nougat Media Suite

1. The decentralized Nougat engine is the integrated **Search** subsystem inside Nougat Media Suite.
2. The owner-approved v0.0.21 top-level order is `Video Player | Library | Discover | Search | Stream | Debug`. Media/torrent P2P lives under Search, not as a top-level area.
3. Accepted behavior remains preserved when identity/palette work changes unless functionality is explicitly approved.
4. Search uses the approved cocoa/chocolate/nougat-cream/caramel palette. Its ordinary internal sections are `Search | Crawler | P2P`; decentralized peer/node administration is available only through a smaller Network/Advanced surface.
5. Read-only Search output remains selectable/copyable with normal mouse selection, Ctrl+C, Ctrl+A, and right-click Copy/Select All.
6. Nougat Media Suite owner-created code is distributed to recipients for noncommercial use under the project license; third-party components retain upstream licenses; the owner retains all rights not granted.
7. Active Search data remains under the backward-compatible existing user-data tree.

### v0.0.21 identity and palette rules

- Official visible application identity is **Nougat Media Suite**; historical ReddMedia identity ended with accepted v0.0.20.
- Root executable: `Nougat_Media_Suite_v21`.
- Former top-level Nougat tab label: **Search**.
- Common suite chrome: chocolate/cocoa/cream/caramel with the approved N + play icon.
- Video Player: chocolate/cocoa/caramel; Library: forest/sage; Discover: plum/lavender; Search: cocoa/nougat/caramel; Debug: graphite/amber. P2P may retain a navy/ice sub-surface inside Search.
- Stream retains only YouTube, Rumble, RuTube, VK, and OK in v0.0.21; selecting one changes Stream to that service's recognizable palette.
- Additional Stream services and other post-rebrand UI/feature work are deferred to v0.0.23 after the v0.0.22 licensing release. The approved v0.0.21 UI repairs are the Search/P2P navigation consolidation and working Library view-icon controls.
- v0.0.21 does not rename the Git working directory or migrate existing user data/config paths.

### v0.0.21 Library view-control rule

- The root Library heading is followed by two compact icon controls: three horizontal lines for List and a 2x2 four-square symbol for Grid.
- These controls must change the actual Library presentation when clicked; active state is indicated by the button treatment, never by `Grid [x]` or `List [x]` text.
- Movies and TV retain independent persisted view choices.

### retained v0.0.20 behavior rules

- Video Player volume defaults to 100%, ranges through 200%, and shows live value plus 100% marker.
- Compact bottom controls center when width permits and remain wheel-scrollable when narrow.
- Movies and TV independently persist Grid/List Library preferences.
- Custom editable text fields show focus/caret state.
- Default Play remains the native Nougat Media Suite player.

### v0.0.23 exact concept UI rules

- The owner-uploaded Nougat Media Suite concept sheet is the canonical visual authority for the v0.0.23 UI treatment. Approximate alternate themes are not substitutes.
- The established top-level layout remains `Video Player | Library | Discover | Search | Stream | Debug`; styling may not reorder or replace those tabs without explicit owner approval.
- The selected top-level tab uses the integrated downward point/notch shown in the concept sheet.
- The established Video Player control order remains `Open | Rewind 10s | Play/Pause | Stop | Fast Forward 10s | Fullscreen`.
- The common page material is the approved cream quilted surface, with only subtle owner-approved area tinting.
- Seek and volume controls use the concept-sheet caramel/cream/chocolate palette; the old red fill is not part of the Nougat v0.0.23 theme.
- Volume remains 0-200% with a visible 100% marker and uses the compact concept-sheet geometry.
- The active icon is the exact owner-approved square chocolate/caramel N emblem extracted from the concept sheet; the former candy-filled N icon is no longer active identity.
- Stream retains YouTube, Rumble, RuTube, VK, and OK. Stream uses one shared Direct Play URL field and one native playback action named Direct Watch; the redundant Stream Play button is removed.
- Additional Stream services, Web Player, Plex integration, or other feature expansion require separate owner approval.
## 19. v0.0.24 Search-page polish law

- Search-page polish is scoped to the existing Search area and must not change decentralized-search engine behavior without separate owner approval.
- The suite header uses the approved N emblem at the far left with `NOUGAT MEDIA SUITE` immediately beside it; the version/server area does not carry a duplicate N.
- Search's internal `Search | Crawler | P2P` tabs use the approved concept-sheet control family and selected-tab point/notch.
- `Network...` and `SEARCH` share the same right-side alignment and width.
- Search fields, results, peer lists, crawl logs, and embedded P2P surfaces use the approved cream/caramel/chocolate concept family rather than the legacy flat dark slab.
- The GNOME launcher/running-window identity must resolve to the approved Nougat N, not a generic gear fallback.

