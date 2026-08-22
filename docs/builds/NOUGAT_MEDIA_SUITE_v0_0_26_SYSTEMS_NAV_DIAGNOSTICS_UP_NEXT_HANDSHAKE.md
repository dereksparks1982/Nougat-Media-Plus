# Nougat Media Suite v0.0.26 Build Handshake

## Identity
- Project: Nougat Media Suite
- Owner: Elderred Softworks LLC
- Target version: v0.0.26
- Required accepted base: v0.0.25
- Required Git base: `c4d174466c2bb30c4eda8f04f09105e5d583040c`
- Required branch: `main`
- Package: `Nougat_Media_Suite_v0_0_26_SYSTEMS_NAV_DIAGNOSTICS_UP_NEXT_CHANGED_FILES_ONLY.zip`
- Root executable after installation: `Nougat_Media_Suite_v26`

## Approved v0.0.26 work
- Mouse side Button 8 navigates Back and Button 9 navigates Forward through Nougat internal navigation history.
- Library removes the redundant root `MEDIA LIBRARY` wording and places List/Grid view controls at the far left.
- The approved N icon perimeter is cleaned app-wide so the tiny light sliver at the bottom edge becomes transparent; all active icon sizes and embedded icon data are regenerated from that cleaned approved N.
- Debug becomes the Nougat Media Suite Diagnostic Center with evidence-backed checks and TXT, JSON, and redacted support-bundle export.
- The Video Player keeps its intentional 0-200% gain range while the Volume label, existing slider, and one correct percentage are centered as a group; the duplicate percentage and rejected square/triangle speaker glyphs are removed.
- Fixed header branding, Server status/dot, and version remain anchored beneath the horizontally scrolling top tabs so tabs roll over them rather than fixed text being painted on top.
- Natural TV episode completion resolves the real next episode and shows an Up Next overlay with a visible 10-second countdown plus Play Next, Back to Series, and Replay. Failure to resolve a next episode is explicit rather than silent.

## Deferred work recorded in roadmap
- v0.0.27: seek-bar hover thumbnail previews and timestamp/chapter-name hover behavior; aesthetic fallback chapter marks remain.
- v0.0.28: focused P2P management expansion toward a BitTorrent Pro-class workflow.

## Protected boundaries
- Protected licensing files remain unchanged.
- Nougat Search engine/bridge behavior remains unchanged.
- Existing P2P engine/stream-server implementation remains unchanged in v0.0.26.
- Existing accepted Stream provider theming and Discover native-play behavior remain retained.
- User media and persistent user data are never deleted by this installer.

## Installer / rollback law
- Preflight requires accepted v0.0.25 Git HEAD and a clean worktree.
- Port 8096 must be free before application.
- Exact package payload and exact changed-file base hashes are verified before edits.
- A rollback snapshot of touched project files plus launcher/icon shell state is created before application.
- Source/regression tests run before build handoff.
- A warnings-as-errors stub build and X11 identity smoke run before the full native build.
- The owner's workstation performs the full native libtorrent + real llama.cpp build.
- The final v26 executable is written and verified before v25 is removed.
- Executable custom-icon metadata is applied only after the final executable write and read back.
- Any post-apply failure restores the exact accepted-v0.0.25 touched state and removes newly added v26 files.

## Validation status before owner installation
- v0.0.26 source contract: PASS.
- Protected licensing boundary: PASS.
- Nougat Search engine/bridge deterministic tests: PASS.
- Retained Library/Discover/diagnostic behavior test: PASS after updating the diagnostic report identity to Nougat Media Suite.
- Media-server lifecycle tests: PASS.
- Diagnostic TXT/JSON/support-bundle/redaction behavioral test: PASS.
- Warnings-as-errors P2P/AI stub build: PASS.
- Discover AI stub self-test: PASS.
- v0.0.25 retained provider/Discover selector self-test: PASS.
- X11 application identity smoke: PASS.
- Full native libtorrent build: required on owner workstation because libtorrent development files are unavailable in the package-build container.

## Owner acceptance checks
- Mouse Back/Forward works through real Nougat navigation.
- Library root has no redundant label and view icons are at far left.
- Dock/window/header N has no light sliver at the lower edge.
- Diagnostic Center exports readable TXT, valid JSON, and a redacted support bundle.
- Volume group is centered, remains 0-200%, has one percentage, and no stray square/triangle glyphs.
- Scrolled top tabs cover the fixed version and Server status/dot instead of those fixed elements painting over the tabs or scrolling away.
- A real TV episode end shows the Up Next overlay, counts from 10 seconds, and successfully starts the resolved next episode unless the owner chooses Back to Series or Replay.

## Continuation / release process
1. Owner runs the installer and pastes terminal output.
2. Owner performs the visual/real-media acceptance checks above.
3. If rejected, repair remains v0.0.26; no acceptance snapshot, commit, tag, or push.
4. If accepted, create the accepted snapshot first.
5. Make the local Git commit only after the accepted snapshot.
6. Create the v0.0.26 tag and push main/tag over the configured SSH remote only after the local commit is verified.
7. Verify remote main and the dereferenced v0.0.26 tag point to the exact accepted commit.

## Current continuation point
v0.0.26 is ready for owner installation/testing once the changed-files package is produced and its final reconstruction validation passes.
