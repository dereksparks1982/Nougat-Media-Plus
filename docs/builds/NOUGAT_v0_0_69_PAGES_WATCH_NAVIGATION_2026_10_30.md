# Nougat Media Plus: GitHub Pages Watch and Module Navigation Source Update

**Date:** October 30, 2026  
**Application version:** v0.0.69 (unchanged)  
**Delivery:** GitHub repository source only. No executable, ZIP, terminal patch, GitHub Actions, or release.

## Hanafi remote-viewer reference

Compared the Hanafi Islam Learning Deck's `web-viewer/media/` and the corresponding HTTPS media bridge with Nougat's root GitHub Pages player. Both use the remote Nougat catalog/media/transcode/artwork interface and an HTTPS server separate from the static Pages frontend.

GitHub Pages hosts **HTML, CSS, JavaScript, and an app manifest**, not the user's media files. Playback requires an online Nougat media host accessible through HTTPS that permits CORS from the Nougat Pages origin. A public static page cannot bypass a stopped host, inaccessible network, incompatible codec, or missing CORS configuration.

## New phone-friendly Watch page

- `watch/index.html` is a standalone, simple watch-first UI with search, Movies/TV filters, artwork, playback controls and connection status.
- `watch/watch.js` fetches health and catalog from `/nougat/v1/*` on the configured host and plays `/nougat/v1/media` and `/nougat/v1/transcode` streams.
- `watch/watch.css` uses fluid phone-friendly cards, touch-sized controls, and wrapping layouts with no module/sidebar navigation on the phone page.
- `watch/manifest.webmanifest` makes the Watch surface available for installation where browsers permit.
- Selecting a title starts direct or compatibility playback as appropriate. A direct-play error can fall back to compatibility; users can switch modes manually.
- The player keeps a bounded, per-server local watch-position history and can resume without uploading watch history.
- The connection settings require an HTTPS origin and reject credentials, paths, and invalid protocols. Requests omit cross-origin credentials, have bounded timeouts, and give clear network/offline feedback.
- The existing first-run legal acknowledgement gate applies to this page; media requests are delayed until acknowledgement.
- The Watch page is linked from the root web player's Home view, and both viewer links are near the beginning of `README.md`.

## Existing web player: sidebar and navigation repairs

- The root module bridge formerly waited on an unbounded `/nougat/v1/catalog` fetch before activating a tab. If the media bridge was slow or offline, users could click multiple tabs and see no content.
- Tabs now render immediately, even if catalog fetch is still pending or the host is unreachable.
- Catalog lookup has an 8-second timeout, preserves already-loaded entries during outages, and does not permanently cache a failed promise. It retries after an online event.
- Exceptions and modules that fail to render display an informative panel and Quick Watch link instead of silently leaving an empty page.
- The module catalog fetch no longer begins before the legal notice is acknowledged.
- On short screens the fixed sidebar itself scrolls; buttons retain sufficient height and labels do not collapse into one another.
- The main content remains offset from the existing sidebar. A wrapping Quick Watch row avoids overlapping the Home heading on narrow phones.

**Important:** Some modules depend on Linux-only services, native devices, tuners, specific emulators, or extra media integrations. A functional tab/navigation shell is not a guarantee that those host-backed features operate on GitHub Pages. Unsupported capabilities should report their missing prerequisites instead of pretending to run locally.

## README cleanup

The root README is now a compact product overview: **what Nougat is**, the two web-player links, connection requirements, product feature areas, Linux/browser support and documentation. No individual build notes or release history are duplicated there. The development record remains available in `CHANGELOG.md`, `ROADMAP.md`, `docs/` and Git history.

## Source checks and outstanding acceptance

- Added `tools/tests/audit_pages_watch_navigation.py` for static routing, media API, legal-gate, responsive CSS, manifest, navigation, service-worker asset and README contracts.
- JavaScript source syntax checked for the new Watch page and modified module bridge.
- Real live GitHub Pages deployment, phone layout, browser autoplay policy, HTTPS bridge connectivity, CORS, artwork, subtitle handling and sustained playback are **not yet validated**.
- No GitHub Actions, automatic deployment machinery, executable download, app version change or changes to the approved native UI geometry.

The project owner is not being asked to perform testing during this development stage.
