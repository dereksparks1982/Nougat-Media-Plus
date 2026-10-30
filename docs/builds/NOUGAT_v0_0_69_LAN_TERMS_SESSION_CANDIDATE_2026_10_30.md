# Nougat v0.0.69: LAN Terms Session Enforcement Candidate

**Date:** 2026-10-30  
**Status:** SOURCE CANDIDATE, runtime validation outstanding  
**Application version:** v0.0.69, unchanged

## Implemented in source

The previous browser terms overlay gated ordinary clicks, but a user could navigate directly to a LAN media API URL without encountering the overlay. This change makes the **LAN-hosted** HTTP service check for an acknowledgement session before answering private media routes.

- `POST /nougat/v1/terms/accept` is a narrow, same-origin-only endpoint. It records an affirmative client acknowledgement and issues a random 256-bit process-local token from `/dev/urandom` in an `HttpOnly; SameSite=Strict` cookie scoped to `/nougat/v1/`.
- `GET /nougat/v1/terms/status` confirms the receipt is present and recognized. A missing or expired session returns HTTP 403.
- Unacknowledged requests for `/nougat/v1/catalog`, `/artwork`, `/media`, `/transcode`, `/stream`, and other private versioned API routes return HTTP 403. The basic health endpoint remains available without a session, and static shell/terms assets remain accessible.
- The approved browser notice still starts with an unchecked box and requires explicit selection before using `I Understand & Agree`. First rejection warns, second refusal exits/blocks the viewer. A browser session is not unlocked until the server acknowledgement and status verification complete.
- If a user has already accepted the same notice version locally, the browser silently re-registers that existing acknowledgement with the restarted LAN server before enabling normal use. No unnecessary repeat-checkbox prompt.
- The separate HTTPS GitHub Pages viewer remains on its existing bridge configuration. Server-side enforcement described here applies only to requests handled by this **native LAN HTTP service**.
- A cap on stored process-local tokens prevents unbounded session growth. All sessions expire when the LAN service restarts.

## Explicit limitations

This is **a terms-acknowledgement gate, not a secure media-access account system**. A custom HTTP client can explicitly call the acceptance endpoint; no server can prove someone genuinely read a document. The LAN service currently speaks HTTP, not authenticated TLS, so the cookie is not a confidentiality or identity security boundary on an untrusted network. Media access from the separately hosted HTTPS remote bridge is governed by that bridge's own code and is not secured by this patch. Remote-server acceptance, authenticated pairing, user identity, replay prevention, HTTPS session management, and network-level access-control policies require a separately reviewed design.

## Verification requirements before acceptance

- Compile/link both C++ targets with the real dependencies, in the internal development checkout only.
- Check normal browser acceptance, first/second refusal, persistence, local-cookie blocking, and server restart in an isolated test profile.
- Verify an unauthenticated direct HTTP GET to each media API returns 403; health and static terms assets remain accessible.
- Verify a same-origin POST sets the cookie; a status request succeeds with the cookie and fails without it.
- Ensure a cross-site Origin is refused; malformed cookies and invalid tokens must fail closed.
- Check both web-player JavaScript bundles and service-worker cache updates.
- Test with real LAN devices and media, including playback, range requests and transcode failures.
- Review whether a separate authenticated pairing/session layer is necessary before any wider release.

`tools/tests/audit_lan_web_contract.py` contains source contract assertions, and `tools/tests/test_lan_media_http.py` contains **opt-in**, private-host HTTP checks. These files have been added or extended; a successful end-to-end run has **not** been claimed.

**No release, executable, download, GitHub Actions, version increment, or existing UI geometry change.**
