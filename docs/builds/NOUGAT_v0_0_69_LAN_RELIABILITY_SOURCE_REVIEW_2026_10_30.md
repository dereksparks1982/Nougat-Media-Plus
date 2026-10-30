# Nougat Media Plus: LAN Web Reliability Source Review

**Date:** 2026-10-30  
**Version:** v0.0.69 (unchanged)  
**Status:** Source update pending native and browser runtime validation  
**Release:** None (no binary, ZIP, GitHub Actions, tag or deployment)

## Scope and corrections

### Local LAN server

- Serve every document required by the standalone-style LAN web player through an explicit static asset allowlist: `index.html`, `app.js`, `styles.css`, `config.js`, `sw.js`, `legal-notice.js`, `legal-notice.css`, `LEGAL_NOTICE.md`, the web manifest and the icon route.
- Serve the icon route from the existing approved Nougat N asset. No artwork or approved application geometry was redesigned.
- Serve a same-origin `config.js` for LAN-hosted pages, overriding only the local HTTP response; the published GitHub Pages HTTPS bridge configuration stays unchanged. This is required by the local page's `connect-src 'self'` security policy.
- Add correct MIME types for Markdown, PNG and web manifests.
- Parse single HTTP byte ranges strictly. Reject nonnumeric and malformed ranges instead of interpreting partially numeric strings.
- Open and validate the media file before sending HTTP 200 or 206. Stream from the same file descriptor used for validation and report 416 on unsatisfiable ranges.
- Wait for FFmpeg's first output fragment before replying 200; early transcoding failure now returns an HTTP 502 JSON error.
- Validate peer IPs using numeric parsing; do not accept arbitrary domain names or malformed dotted prefixes as private addresses.
- Do not broaden automatic network binding, cloud relays, router port-forwarding or WAN exposure.

### Native LAN viewer

- Parse `/nougat/v1/catalog` as a response envelope containing `items`; the former top-level object scanner could lose all but one apparent item.
- Carry the item `id` from the server, and construct `/nougat/v1/media?id=...` instead of the unsupported `?path=...` query.
- Percent-encode item IDs and build bracketed IPv6 host URLs when applicable.
- Preserve the existing fail-closed `Verified Clean` trust requirement. The LAN API currently does **not** provide a verified scanner attestation, so remote items must remain Unknown/blocked until real trust data is implemented. Do not falsely label items clean or playable.
- Avoid environment-configured outbound curl proxies for the local catalog request.

### GitHub Pages and component web players

- Both JS clients bound JSON requests to nine seconds.
- Coalesce concurrent catalog reloads to prevent overlapping requests.
- Keep last-loaded library cards if the service temporarily fails; show an honest stale/offline message.
- Retry on browser reconnect or tab visibility and check host status periodically while visible.
- Prevent a delayed direct-play failure from switching the wrong newly selected title. Keep keyboard shortcuts disabled until first-run terms have been accepted.
- Start health/catalog loading only after the local first-run terms event, including when a prior receipt is already saved.
- Advance service-worker cache keys and make the local player's optional icon cache nonfatal.

## Verification contract (no owner testing requested now)

The committed `tools/tests/audit_lan_web_contract.py` checks only source wiring and asset names. It **does not** prove a C++ build, UI function, performance, media streaming or correctness against a live network. Before any future release, run the following independently on the internal development tree:

- Compile C++ with available dependencies, including `src/lan/lan_media_service.cpp`, `src/lan/lan_viewer_service.cpp`, `src/lan/web_player_daemon.cpp`.
- Syntax-check both JavaScript web clients and the terms gate.
- Confirm local GET and HEAD for every shell asset; verify that no URL traversal or arbitrary file reads are possible.
- Confirm HTTP full-media, suffix/open-ended/fixed byte ranges (200/206), malformed ranges (416), and zero-byte media behavior.
- Confirm FFmpeg returns 502 on early failure and cleans up its child on disconnect.
- Confirm catalog parsing of zero, one, several, and malformed/nested records, plus safe URL encoding and trust rejection.
- Confirm phone playback, source switching, supported browser formats, seeking and recovery after the host restarts.
- Confirm the legal acknowledgement blocks normal UI until acceptance. The static web terms UI does not enforce access to the media server. Real server-side authentication and verified acceptance are separate future work.

## Known limitations and deliberately excluded changes

- No new pairing, authenticated session protocol, server-enforced acceptance or HTTPS trust configuration. These require separate design and tests.
- No player layout or approved brand geometry modifications.
- No automatic published releases or GitHub Actions.
- No full hardware or browser acceptance claim.
- No user downloads or terminal instructions are required for this source commit.
