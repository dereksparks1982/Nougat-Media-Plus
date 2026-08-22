# Nougat Media Suite v0.0.27 Build Handshake

## Identity
- Project: Nougat Media Suite
- Company: Elderred Softworks LLC
- Required accepted base: v0.0.26
- Accepted base Git commit: `406c910e606ba58fa8943b1973e45d135ac15eae`
- Target candidate: v0.0.27
- Target root executable: `Nougat_Media_Suite_v27`
- Package: `Nougat_Media_Suite_v0_0_27_HOME_RESUME_PLAYER_POLISH_REPAIRED_CHANGED_FILES_ONLY.zip`
- Candidate remains unaccepted until owner real-machine visual/use testing and explicit approval.


## Same-version installer-validation repair
- The first v0.0.27 package reached the owner machine, applied the approved v27 changed files, then correctly rolled back to accepted v0.0.26 because the post-apply source-test lane invoked the old `tools/test_nougat_media_suite_v26.py` release-identity contract.
- That historical v26 test requires `VERSION 0.0.26` / `Nougat_Media_Suite_v26`, so it is contradictory after the tree has intentionally advanced to v0.0.27.
- The repaired installer removes only that obsolete post-apply identity invocation and keeps `tools/test_nougat_media_suite_retained_v26.py` as the compatibility/regression gate for accepted v26 behavior under v27 identity.
- `tools/test_installer_rollback_v27.py` now explicitly rejects any future post-apply reintroduction of the v26 identity contract.
- No v0.0.27 feature/runtime implementation code changed for this repair.
- Owner rollback from the failed first package completed successfully before this replacement package was produced.

## Approved v0.0.27 work
- Add Home as the first/default top-level tab.
- Continue Watching is the only horizontal Home shelf. It retains all useful unfinished local movies/episodes, shows caramel progress, and uses mouse-wheel left/right scrolling only while the pointer is over that shelf. There are no left/right arrow buttons.
- Under `LOCAL`, show mixed local movies/TV in a normal vertically scrolling card wall, organized by useful genre/category groups plus a watch-history-informed recommendation group.
- Prefer higher-resolution wide Jellyfin backdrop artwork for Home cards, falling back to primary/TMDb artwork when necessary. Do not deliberately stretch tiny UI artwork when a better source is available.
- Add one-at-a-time muted card hover previews from real local video after intentional pointer dwell.
- Add persistent per-title resume history across media changes/application restarts. Reopening unfinished media offers `Continue | Start Over | Cancel`.
- Stop becomes an intentional stopped state with `Resume | Restart | Load Different | Back to Library` while preserving the stopped timestamp.
- Keep movie/TV identity visible under the video in windowed/maximized mode and temporarily overlay it in true fullscreen on mouse activity.
- Round video corners in normal/maximized mode; keep true fullscreen square.
- Add seek-hover actual-frame preview, timestamp, and real chapter name when real chapter metadata applies, without seeking the active libVLC instance.
- Repair raw X11 pointer-motion flicker by redrawing only on meaningful hover transitions.
- Paint the top divider before tabs so it does not cut through the selected-tab notch.
- Remove the redundant `DISCOVER USUAL / DISCOVER RANDOM`, `Direct Play URL`, and `DIAGNOSTIC CENTER` headings.
- Stream URL placeholder is exactly `Paste URL Then Press Direct Watch / Rumble / RuTube / VK / OK`.

## Protected/retained behavior
- v0.0.26 mouse Back/Forward navigation, Library behavior, diagnostics/exports/redaction, centered 0-200% volume, fixed header layering, TV Up Next/countdown, Search/P2P, Stream Direct Watch, Discover behavior, licensing boundary, N icon identity, and server/runtime behavior remain protected.
- v0.0.28 remains the focused P2P expansion. No v28 P2P feature expansion is folded into this candidate.
- No external provider/Xumo/Tubi/native-web-player integration is part of v0.0.27.

## Deferred owner-approved visual proposal
Logged in `ROADMAP.md` for a later version, not applied in v0.0.27:
- Home purple main background.
- Video Player cocoa/chocolate/caramel background family.
- Library green main background.
- Discover red main background.
- Search is the only native page with cream as the main background, with honey/mustard/butterscotch support.
- Stream remains external-provider-reactive, preserving provider identity colors such as YouTube red.
- Debug remains dark charcoal/licorice.
- Use 2-3 coordinated colors per page, with the background doing most of the visual differentiation.
- Replace the bright light video surround with a better-blended dark chocolate/charcoal theater matte, subtle caramel accent if appropriate, and soft shadow; true fullscreen stays edge-to-edge/square.

## Changed implementation surfaces
- `src/main.cpp`
- `src/media_server/jellyfin_api_client.cpp`
- `src/media_server/jellyfin_api_client.hpp`
- `CMakeLists.txt`
- release/readme/roadmap/changelog/apply command
- launcher aliases advanced to v27
- v27 tests, manifest, installer, handshake, validation

## Risks
- X11 sibling/video-window layering and rounded shape transitions.
- Removing raw-motion full repaint without stranding hover visuals.
- FFmpeg frame extraction causing UI stalls if accidentally run synchronously; v27 keeps extraction off the X11 event loop and bounds its cadence/cache.
- Persistent resume records becoming stale or incorrectly marking near-end playback incomplete.
- Series cards needing a real episode path for preview without changing series-card identity.
- Home artwork/catalog calls running before the owned media server becomes ready; Home retries when the server reaches Ready and on revisiting Home.

## Rollback
The installer verifies exact clean v0.0.26 Git state and exact base-file hashes before changes, creates a timestamped rollback snapshot under `$HOME/DKLab/Archives/ReddMedia Archive/`, and restores touched v0.0.26 project/user-shell state if application, testing, build, launcher, icon metadata, or final validation fails.

## Owner acceptance checks after installer FINAL PASS
- Home opens first.
- Continue Watching contains more than three unfinished items when available; caramel progress is correct; wheel over the shelf scrolls sideways; wheel elsewhere scrolls Home vertically; no arrow buttons exist.
- LOCAL card wall scrolls vertically and category/genre organization is useful.
- Card artwork is visibly sharper/wider where better images exist; muted hover preview starts after intentional dwell and only one preview runs.
- Resume survives another file and application restart; Continue/Start Over/Cancel behavior is correct.
- Stop overlay actions and stopped timestamp are correct.
- Movie/TV identity beneath video is correct; true fullscreen title appears only with mouse activity.
- Windowed/maximized video corners are rounded; true fullscreen is square.
- Seek hover shows a real frame/timestamp and real chapter name when applicable without disturbing playback.
- Moving the pointer outside the video no longer causes the reported screen flicker.
- Selected tab notch is not cut by the divider.
- Discover/Stream/Debug redundant headings are gone and Stream placeholder wording is exact.
- Previously accepted v26 Back/Forward, diagnostics, Up Next, volume, Stream/Discover/Search/P2P and server behavior remain good.
