# Nougat v0.0.69: LAN Service Lifecycle and Stream Reliability Candidate

**Date:** October 30, 2026  
**Status:** SOURCE CANDIDATE, no release or owner runtime acceptance  
**Repository branch:** main  
**Version:** v0.0.69 unchanged

## What this source increment changes

### Clean LAN server shutdown

- Track sockets for active HTTP clients under a shared mutex instead of letting the listener lose track of all detached handlers.
- When stopping, signal service shutdown and shut down active client sockets so long or stalled clients are not left waiting on a dead UI.
- Let each client handler remove itself from the active set, close its socket once under the same lock, decrement its count, and notify the shutdown wait.
- Bound waiting for detached work to ten seconds, avoiding an indefinite application hang if an external library is stuck. Shared state outlives a slow detached worker, avoiding a dangling service pointer.
- Handle thread startup failures without leaving a counted client or an open descriptor behind.

### FFmpeg and Stream bridge reliability

- Require output readiness before reading initial streaming data; limit startup wait to 15 seconds for local transcodes and 20 seconds for the remote-source bridge.
- Check the stop flag while waiting for output, poll in short intervals, and terminate the child processes when output never arrives.
- Limit repeated periods of no output to sixty seconds before terminating a stalled stream rather than consuming a LAN worker indefinitely.
- Retry interrupted socket sends instead of reporting a normal EINTR as a connection failure.
- Preserve the existing chunked MP4 response and the current browser compatibility model. Do not claim that every codec or service is now supported.

### Honest catalog and artwork state

- Report both indexed media count and the count of files currently accessible as regular files.
- Add a per-item `ready` boolean. Browser clients already understand this field and can disable unavailable items.
- Validate an artwork ID against an indexed media entry before requesting the image from the underlying media library.
- Preserve existing direct-play fallback behavior and the original version field in the LAN contract.

### Validation infrastructure staged for later

- Extend the existing standard-library-only offline source contract audit to catch missing lifecycle and reporting guardrails.
- Add `tools/tests/test_lan_media_http.py` as a deliberately opt-in smoke suite for a privately hosted test server: GET/HEAD assets, correct local config, malformed path rejection, health, catalog shape and optional 206/416 byte-range handling.
- The live suite is disabled unless an explicit numeric local/private test endpoint is supplied. It does not auto-run, add GitHub Actions, install dependencies, or touch an external drive.
- The review does not assert that these checks have passed on the user's hardware. Source presence and protocol contracts alone do not prove playback success.

## Important boundaries

These updates preserve Nougat's approved interface geometry, public GitHub Pages bridge configuration, local-first direction, and existing app version. They do not expose new WAN listeners, add automatic router forwarding, or move any user files.

**Terms acceptance security remains separate:** A browser-side checkbox is a UI gate, not secure authorization against a modified client. LAN media API enforcement, pairing, and session authentication are future work requiring a complete server/client handshake and explicit validation. Do not describe the current implementation as impossible to bypass.

## Verification before any release

Compilation and link verification for the native app and web daemon, JS syntax checks, LAN HTTP integration checks, browser playback, signal-driven shutdown, FFmpeg cancellation, and owner review remain outstanding.

**No executable, ZIP, binary release, GitHub Actions, or version bump is part of this commit.**
