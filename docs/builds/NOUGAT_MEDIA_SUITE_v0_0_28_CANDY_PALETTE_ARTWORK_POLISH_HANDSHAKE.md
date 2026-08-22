# Nougat Media Suite v0.0.28 Build Handshake

## Identity
- Project: Nougat Media Suite
- Company: Elderred Softworks LLC
- Required accepted base: v0.0.27
- Accepted base Git commit: `9e3b1820d8a0374b100911b5d2e5146e0688983c`
- Target candidate: v0.0.28
- Target root executable: `Nougat_Media_Suite_v28`
- Package: `Nougat_Media_Suite_v0_0_28_CANDY_PALETTE_ARTWORK_POLISH_CHANGED_FILES_ONLY.zip`
- Candidate remains unaccepted until owner real-machine visual/use testing and explicit approval.

## Approved v0.0.28 work
- Make the main page background the primary visual identity: purple Home, cocoa/chocolate Video Player, green Library, red Discover, cream Search, provider-reactive Stream, charcoal/licorice Debug. Keep roughly 2–3 coordinated colors per page and retain cream selectively for text/trim/panels.
- Replace the remaining pale/white windowed-video halo with a cocoa/chocolate theater surround and restrained caramel trim.
- Preserve Home state when switching tabs. Returning to Home must not rebuild the feed unless watch/library data changed or an explicit refresh is requested.
- Home rests on proper poster artwork. Movies use movie posters; TV Continue Watching resolves matching season poster first, series poster second. The existing silent hover-video preview remains one-at-a-time and restores the poster when the pointer leaves.
- Repair top rounded-corner artwork/hover clipping so no square image pixels protrude through the curved card border.
- Increase Home section/category and metadata readability.
- Make the ordinary LOCAL recommendation wall responsive with at least three cards per row around the owner's ~650px half-screen width. Continue Watching remains its separate horizontal shelf.
- Repair the legacy X11 metadata separator path so intended bullets display cleanly rather than as mojibake/a-cent glyphs.
- Improve Library poster sourcing/presentation using exact catalog TMDb IDs when available, Jellyfin Primary fallback, 480x720 requests, portrait-quality gating, aspect-preserving display, and deliberate `NO POSTER` fallback.
- Remove the redundant standalone `SEARCH` page heading while retaining `Search | Crawler | P2P` and reclaiming its vertical space.

## Explicit split/deferred work
- The owner-observed v0.0.27 TV Up Next/autoplay and `Back to Series` regressions are **not** folded into v0.0.28. They are the focused v0.0.29 TV Playback and Navigation Reliability build.
- The BitTorrent Pro-class P2P expansion moves to v0.0.30.

## Protected/retained behavior
- Preserve accepted v0.0.27 Home Continue Watching/resume/Stop UX, seek previews, title identity, pointer-motion flicker repair, selected-notch layering, Back/Forward, 0–200% volume, Discover, Stream, Search engine/bridge, current P2P implementation, diagnostics, integrated server behavior, licensing boundary, and approved N identity outside this scope.
- Do not claim the known TV Up Next runtime regression is fixed by v0.0.28.

## Changed implementation surfaces
- `src/main.cpp`
- `CMakeLists.txt`
- `README.md`, `ROADMAP.md`, `CHANGELOG.md`, `APPLY_COMMAND.txt`
- launcher aliases advanced to v28
- v28 retained/contract/UI/installer tests
- v28 installer, manifest, handshake, validation record

## Risks
- Stronger page tinting can reduce text/control contrast if area-specific palettes are not paired correctly.
- Home poster-first presentation can look wrong if portrait art is cropped into 16:9; v28 contains it instead of distorting/cropping the poster.
- TMDb poster use must stay exact-ID only to avoid a beautiful but incorrect movie/series match.
- Home persistence must not become stale after actual watch/library changes.
- X11 rounded clipping and child-video matte geometry must avoid repaint artifacts.
- Large poster requests may cost more during first load, so they are cached and quality-gated.

## Rollback
The installer verifies exact clean v0.0.27 Git state and exact accepted-base hashes before changes, creates a timestamped rollback snapshot under `$HOME/DKLab/Archives/ReddMedia Archive/`, and restores touched v0.0.27 project/user-shell state if application, testing, build, launcher, icon metadata, or final validation fails.

## Owner acceptance checks after installer FINAL PASS
- Home, Library, Discover, Search, Video Player, Stream, and Debug show the approved background identity families.
- Home returns instantly after tab switching without showing a reload when data has not changed.
- Home cards show movie posters at rest; TV Continue Watching shows season poster or series fallback; silent hover preview still works and returns to poster.
- Rounded top corners contain artwork/hover pixels cleanly.
- Around half-screen width, LOCAL shows at least three recommendation cards per row and category/metadata lettering is comfortably readable.
- Metadata lines display clean bullet separators without mojibake.
- Library poster quality is visibly improved where exact TMDb/Jellyfin artwork exists; portrait art is not stretched and bad/missing art uses `NO POSTER` rather than mush.
- Search has no redundant `SEARCH` page heading and its real controls use the reclaimed space.
- Video Player has no pale/white halo around the windowed video area.
- Accepted v0.0.27 player/resume/seek/Discover/Search/P2P/diagnostic behavior remains good outside the known TV Up Next issue reserved for v0.0.29.
