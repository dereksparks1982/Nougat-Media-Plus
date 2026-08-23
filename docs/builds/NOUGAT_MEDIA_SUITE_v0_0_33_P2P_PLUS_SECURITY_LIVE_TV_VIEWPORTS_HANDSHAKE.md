# Nougat Media Suite v0.0.33 Build Handshake

## Base and target
- Required accepted base: v0.0.32 commit `084ee7ccd82be3a578f738b3bcb6ac8570a573dd`.
- Target: v0.0.33 candidate.
- Rollback: exact touched v0.0.32 state plus launchers, while generated runtimes/user data remain outside Git.

## Approved work
- Hard bordered/clipped page viewports on normal pages; Video Player explicitly unchanged.
- Home Continue Watching hard clipping and existing Home scrollbars; Library outer containment, wrapping toolbar and vertical scrollbar.
- Top navigation hard-clipped between fixed Nougat branding and fixed Server/version area. Order: Home | Video Player | Library | Discover | Live TV | Search | Stream | Debug.
- P2P Plus first management layer: speed limits, seed ratio/time rules, file priorities, queue movement, tracker status, Force Reannounce/Recheck.
- Security hardening: strict ANALYSIS INCOMPLETE, pinned YARA-X 1.19.0, capa 9.4.0/rules, Magika 1.0.3, optional one-shot clamscan, free abuse.ch MalwareBazaar/ThreatFox/URLhaus via Threat Intel Key, WARN ME FIRST and no automatic destructive action.
- Nougat-owned Jellyfin remains running after the UI closes once Start Server is used; reopening adopts it and Stop Server explicitly ends it. External Jellyfin is never killed.
- Live TV top-level scaffold with Linux DVB/V4L2 discovery and first-hardware target Hauppauge WinTV-HVR-955Q. Channel tuning/playback is not claimed in this build.

## Changed implementation areas
CMake/version/desktop metadata, `src/main.cpp`, P2P engine, media-server manager, new Live TV backend, Security Analysis worker/runtime helper, release documentation/tests/installer/manifest.

## Validation
Warnings-as-errors stub build, retained v0.0.32 tests, v0.0.33 integration self-test, security fallback/full-runtime tests, X11 identity smoke, native libtorrent build on owner machine, AI RPATH/model checks, root executable/icon check, package manifest/integrity and rollback gates.

## Known validation boundary
This build environment does not provide the owner's native libtorrent/Jellyfin/AI runtime stack or internet package installation. The handoff installer performs those native/runtime gates on the owner machine before declaring FINAL PASS. Live TV v0.0.33 proves discovery/scaffolding only; real HVR-955Q tuning follows owner hardware probing.

## Continuation
Owner visually/functionally tests v0.0.33. Do not commit/tag/push until explicit acceptance.
