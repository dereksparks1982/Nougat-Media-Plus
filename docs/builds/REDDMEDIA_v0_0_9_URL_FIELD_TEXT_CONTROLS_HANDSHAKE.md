# ReddMedia v0.0.9 URL Field Text Controls Build Handshake

Project: ReddMedia
Version: v0.0.9
Build title: URL Field Text Controls
Required base version: v0.0.8
Required base commit: 21b6219fd57deb14d48815a47df4f2840ed04875
Required base tag: v0.0.8
Required branch: main
Required tree: clean
Package: ReddMedia_v0_0_9_URL_FIELD_TEXT_CONTROLS_CHANGED_FILES_ONLY.zip

Completed changes:
- Ctrl+A selects the full yt-dlp URL value.
- Right-click inside the yt-dlp URL field opens Cut / Copy / Paste.
- Full-field selection is visibly highlighted.
- Cut and Copy place the complete selected URL on the X11 clipboard.
- Paste replaces a full-field selection before inserting clipboard text.
- Active version surfaces advance to v0.0.9 and `ReddMedia_v9`.

Changed project paths:
- CMakeLists.txt
- README.md
- CHANGELOG.md
- ROADMAP.md
- ReddMedia.desktop
- src/main.cpp
- ReddMedia_v8 (deleted)
- ReddMedia_v8.desktop (deleted)
- ReddMedia_v9 (added)
- ReddMedia_v9.desktop (added)
- docs/builds/REDDMEDIA_v0_0_9_URL_FIELD_TEXT_CONTROLS_HANDSHAKE.md (added)
- docs/builds/REDDMEDIA_v0_0_9_URL_FIELD_TEXT_CONTROLS_VALIDATION.md (added)

Validation state:
- Build-side warnings-as-errors compile PASS.
- Build-side native `--version` and ELF checks PASS.
- Build-side Xvfb interaction proof PASS for Ctrl+A plus Cut / Copy / Paste roundtrip.
- Changed-files-only installer rehearsal PASS on a disposable clean v0.0.8 clone.
- Forced post-mutation rollback rehearsal PASS; exact v0.0.8 commit/tree restored.
- Owner-side live X11 UI confirmation remains required after installer final PASS.

Rollback point:
- Exact accepted v0.0.8 tag and commit `21b6219fd57deb14d48815a47df4f2840ed04875`.
- Installer rollback resets to that accepted point and removes candidate untracked files after any post-mutation validation failure.

Continuation point:
- Apply the package to Derek's clean accepted v0.0.8 repository.
- Require final PASS from the installer.
- Confirm Ctrl+A full selection and the Cut / Copy / Paste menu in the live desktop UI.
- Only after owner acceptance proceed to accepted snapshot, local Git commit/tag, then remote closeout.
