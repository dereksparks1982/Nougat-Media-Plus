# Nougat Media Plus v0.0.69: Web Playback Reliability and Host Isolation

**Date:** October 30, 2026  
**Development state:** GitHub source candidate. No application version increase, binary, ZIP, or GitHub Actions.

## Main problems addressed

1. Full Web Player and Quick Watch stored playback positions separately, and the full player used a single resume key for all servers.
2. Three media utilities independently loaded the catalog against an old fixed bridge address, with unbounded duplicate background requests.
3. The legacy connection monitor polled a hardcoded host without the terms gate or request deadline.
4. The full player's catalog-fetch interceptor used one cache for all media servers, serving cached data ahead of actual responses.
5. A missing optional asset caused the entire service-worker install to fail; its generic same-origin caching could intercept unrelated resources.

## Changes

**Cross-viewer Continue Watching:** The new `web-resume.js` normalizes and shares playback history for the full Web Player and Quick Watch in browser-local storage, keyed by media-host origin. Old Quick Watch seconds and the original full-player millisecond history are migrated only into their matching host. The most recent 200 valid entries are retained. Changes in another browser tab can update the current view via a storage notification. Watch adds a Continue filter and preserves the original All/Movies/TV filters.

**Single catalog source:** `player-enhancements.js`, `player-media-session.js` and `modules-media-card-actions.js` now consume the main Web Player's catalog instead of issuing duplicate direct fetches. The core player emits catalog-change notifications and marks its active media with the catalog ID so adjacent episode and metadata matching remain stable even when titles repeat. The existing native LAN artwork fallback is retained.

**Single health source:** `connection-monitor.js` no longer performs independent unbounded requests against `config.js`. It listens to the already-bounded core health checks, with a reconnect cooldown, respecting the first-run terms gate. Core health events carry the media host that was actually checked.

**Media-host-specific offline cache:** `modules-catalog-cache.js` now keys saved catalogs by absolute HTTP(S) host and API pathname, limits catalog age to one day and item count to 3,000, and uses cached entries **only** after a network request genuinely fails. Browser-initiated aborts and an unaccepted legal notice cannot turn into a successful cache hit. The app clearly labels offline cached lists, and a catalog from host A can never masquerade as host B.

**Offline app shell, not offline media:** `sw.js` now installs core static assets even if optional files are missing, uses a Pages-scope allowlist, caches only known static shell files, and never intercepts media, catalog, API, range or cross-origin requests. Offline fallback is limited to already-cached viewer HTML/CSS/JS/assets. The media server must remain reachable to stream movies and TV.

**Remaining module consistency:** Browser radio cellular controls, stream resolver, TMDb maintenance actions, and manual media match updates use the bounded shared API requester. Local manual matches are scoped to the selected host, preserving old matches only for the original default host.

## Regression coverage and acceptance

- `tools/tests/test_web_resume_sync.js`: per-host separation, legacy conversions, 200-item bound and cross-tab update event.
- `tools/tests/test_pages_catalog_cache.js`: same-server offline fallback, no cross-server leak, no terms bypass or disguised request cancellation.
- `tools/tests/test_pages_service_worker.js`: optional-asset installation, static page cache, offline shell, private API/media bypass.
- `tools/tests/audit_pages_web_reliability.py`: source wiring contracts for both players, all three catalog consumers, one health source and safe cache behavior.
- The earlier Watch audit is brought in line with existing LAN same-origin terms-cookie behavior.
- Source-level JavaScript parse and isolated mocks are not substitutes for live browser/device tests.

**Still outstanding:** Running all Node/Python fixtures against a local checkout and checking real desktop/phone playback and resuming, GitHub Pages asset updates, CORS, server availability, codecs, offline display and native LAN terms. No native UI geometry has been changed. Do not describe these features as device-tested until acceptance.

**Repository-only build:** No GitHub Actions, binary, installer or download. Nougat stays v0.0.69.
