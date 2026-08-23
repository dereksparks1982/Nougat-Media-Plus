# Nougat Media Suite v0.0.35 Validation

## Baseline

- Expected Git branch: `main`
- Expected pre-install HEAD: `64eade89e7b9b88f1696bef18e580e22bace978f`
- This is the published v0.0.34 line plus the exact README application-description restoration.

## Automated gates

The installer must complete all of the following before declaring FINAL PASS:

- changed-files manifest byte/SHA verification;
- protected licensing, Nougat engine/bridge, security, P2P-streaming, and diagnostics regression tests;
- warnings-as-errors C++17 stub build;
- retained executable self-tests through v0.0.34 plus the new `--v35-cleanup-self-test`;
- full native build with libtorrent and the pinned llama.cpp runtime;
- relative `$ORIGIN` AI RPATH verification;
- root executable installation and version check for `Nougat_Media_Suite_v35`;
- launcher update and rollback coverage.

## Builder-environment validation

Validated before packaging in the available build environment:

- warnings-as-errors C++17 build with the project AI and P2P stubs: **PASS**;
- retained executable self-tests for Discover and v0.0.25 through v0.0.35: **PASS** for the directly invokable regression gates;
- v0.0.35 source/contract test: **PASS**;
- P2P HTTP Range/seek scheduler regression test: **PASS**;
- diagnostics TXT/JSON/support-bundle/redaction regression test: **PASS**;
- installer syntax and rollback/terminal-safety contract test: **PASS**;
- changed-files manifest byte/SHA verification: **PASS**.
- canonical VOLUME frame-100 pixel comparison against the approved sheet crop (335x47): **PASS, pixel-identical**.
- full-width player transport group centering and narrow-width final-button reach self-test: **PASS**.
- Gold Studio source/contract identity (yellow/gold + brown stitched palette, `Studio` tab / `GOLD STUDIO` page): **PASS**.

The available builder container cannot reproduce the owner's full Ubuntu 26.04 native environment. A full native link is therefore **not claimed here**: the container lacks the owner's libtorrent development environment, and the retained bundled llama.cpp shared library requires a newer glibc symbol set than the container provides. The split source handoff used for packaging also does not contain every owner-tree licensing/security-runtime support file needed to reproduce those owner-machine gates. The installer deliberately reruns the protected licensing/security/P2P/diagnostics tests and the full native build on the verified owner tree before it installs the v0.0.35 root executable; any failure triggers rollback instead of a FINAL PASS.

## v0.0.35 behavioral contracts

- Studio is between Stream and Debug and uses a distinct yellow/gold page family with brown stitched borders and the internal Gold Studio identity.
- Search outer page frame is square; inner controls remain rounded.
- Search, Debug, Live TV, Discover, and Library top inner action rows use Stream's current accepted vertical baseline.
- The Video Player's eight-action transport row is centered as one group at normal/full widths; at narrow widths it scrolls until the final button is fully visible. Debug's ten-action row likewise reaches its final button.
- Library header tools remain a single horizontal scrolling row at narrow widths; List/Grid stay fixed together at the far right and cannot be covered by offscreen tools.
- The runtime package contains `assets/ui/nougat_volume_sheet_frames.bin`; its 100% frame is pixel-identical to the approved sheet VOLUME housing crop and its runtime dimensions are fixed at 335x47 with the percentage rendered outside.
- Selected top-tab pointers use the enlarged v0.0.35 repair dimensions and are painted after page/loading content.
- Partial seek/volume repaints initialize the entire copied strip before XCopyArea, preventing uninitialized black bands.
- Nougat Search/Crawler workers are not detached across App destruction.
- Live TV can execute a native Linux DVB ATSC 1.0 RF 2-36 scan, report lock/signal/quality progress, parse PSIP VCT services, persist discovered channels, and cancel cleanly during shutdown.

## Owner-machine checks

Automated tests cannot prove real antenna reception or visual pixel fidelity on the owner's desktop. After installation, verify:

1. Player VOLUME control against the approved sheet, especially housing/icons/knob/percentage placement.
2. No black horizontal repaint band when the pointer is over the Video Player or the seek display refreshes.
3. Centered Video Player transport group at normal/full width, plus full rightward scrolling of all eight actions in half-screen mode.
4. Search/Discover/Library/Live TV/Debug top-row alignment to Stream; Library remains one row with List/Grid fixed at far right.
5. Debug action-row full rightward scrolling in half-screen mode.
6. Enlarged selected-tab pointer scale/centering across every top-level tab.
7. Live TV subtitle absence and real channel scan with the connected Hauppauge/WinTV-HVR-955Q and antenna.
8. Studio tab placement plus yellow/gold Gold Studio page identity and brown stitched borders.
9. Up Next remains free of the previously fixed flashing regression.
## Installer rollback-recovery correction

- The replacement repair installer recognizes the exact interrupted state left by the earlier rollback bug, where the first v0.0.35 candidate source was restored but `NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v35.json` and the root `Nougat_Media_Suite_v35` executable were incorrectly removed.
- Recovery is accepted only when every remaining first-candidate file matches the recorded byte count and SHA-256 and no unrelated worktree edits exist.
- The original first-candidate manifest is packaged as a recovery-only asset and must match its recorded SHA-256 before restoration.
- If the verified interrupted state is missing the root executable, the installer rebuilds the pre-repair v0.0.35 executable before taking the new rollback snapshot.
- Rollback now preserves/restores a pre-existing v0.0.35 root executable and lets the normal payload rollback logic restore or remove the v0.0.35 manifest according to its true pre-install state. It no longer unconditionally deletes both files after rollback.

