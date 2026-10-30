# Nougat v0.0.69: Web Player Control & Shared-Host Source Build

**Date:** 2026-10-30  
**Delivery:** GitHub repository only. No GitHub Actions, executable, ZIP, version bump, or owner testing requested.

## Problems addressed

The original full player had a hardcoded remote media origin, while the new phone Watch page stored a different user-selected server. This could make one viewer show media and the other appear offline. Several module endpoints also used unbounded requests. The base fallback module offered unconnected buttons, including the crawler's Same Domain switch. The main player could not directly play a media item selected from a module if its card was hidden by the current Library filter.

## Shared media host

- `web-host.js` provides one per-browser HTTPS origin setting for full player and Quick Watch. Existing phone-only saved settings are migrated to the shared setting.
- Reject credentials, paths, HTTP, URL fragments and query-string secrets. The server must support HTTPS and browser access from the Pages origin. No passwords, media, or token values are uploaded to GitHub.
- A top-bar **CONNECT** button in the full player opens a modal to view/change/test/reset the host address. This overlay does not move the approved rail or native geometry.
- When a server changes, the full player stops old playback, clears the previous library, increments the request generation and ignores stale responses. Module cards and catalog state also refresh.
- The phone Watch player reads the same saved host, and changes on one tab can propagate to the other through the browser's storage events. It also supports local same-origin LAN access when served by the native LAN host.

## Buttons and navigation

- The Nougat full player exposes a stable `playById` bridge so Discover/Search buttons can launch library items by ID even when the card is not currently visible or the Library's TV/Movie filter differs.
- The Search Crawler's **Same Domain** toggle is operational rather than permanently highlighted. It has an 8-second timeout, an HTTPS-only link set and a 1 MiB returned-document cap. Browser CORS rules still apply.
- Archive search gains an actual click target instead of being Enter-only.
- Base Network fallback tabs now show real, browser-visible information or explain missing native access; the enhanced Network module remains in place.
- Diagnostics in the base System fallback now have a working refresh action.
- A small, collapsible *Browser capabilities & host requirements* note explains why native-only functions cannot run without compatible Linux services. It has a working Configure media server shortcut.
- Existing approved navigation rail, tabs and home layout positions remain unchanged.

## Host request lifecycle

- `web-api.js` provides bounded, credentials-omitting browser requests with cancellation and explicit HTTP error reporting. The host-backed Live TV, Radio, Network, System, tuner and World TV-guide modules use it.
- Cross-origin HTTPS bridge requests omit cookies. Local same-origin LAN requests preserve Nougat's first-run terms-session cookie, while host switching avoids old responses overwriting the new server's state.
- Network activity that requires a real tuner, recording backend, emulator, video extraction service, codec or other native support is **not** claimed to be executable inside a static web page.

## Testing and verification

- `tools/tests/test_pages_web_host.js`: isolated Node-based validation, saved-server migration, event emission and reset tests. No network access needed.
- `tools/tests/audit_web_player_controls.py`: offline checks for all 13 tabs, newly connected controls, shared-host configuration, request helpers, Pages shell assets and clean README links.
- JavaScript source syntax checks and direct GitHub blob verification are possible in this environment. Node tests and interactive browser/phone testing remain separate acceptance work.
- The service-worker shell asset cache is updated to include the new client scripts and styles.
- README remains a short product description with both web-viewer links prominently displayed. Build history stays in CHANGELOG.md, ROADMAP.md and technical docs.

**Known limitations:** GitHub Pages remains a static interface. Actual media playback requires a running reachable host and compatible browser format/transcode service. This source build does not guarantee every native-only button is operable remotely. Real Pages deployment, CORS, controller, player, tuner and mobile layout testing are still outstanding.
