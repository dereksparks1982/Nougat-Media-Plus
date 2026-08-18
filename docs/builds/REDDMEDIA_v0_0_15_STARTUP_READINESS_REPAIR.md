# ReddMedia v0.0.15 Startup Readiness Repair

## Owner-test failure

The native Library candidate compiled successfully in both stub and full configurations, installed the pinned Jellyfin 10.11.11 runtime, and kept its web client hidden. Its end-to-end catalog proof then stopped with `The local media catalog returned an unreadable status` and rolled the project back to accepted v0.0.14.

## Root cause

Jellyfin 10.11.11 starts a temporary setup server while the full API is still loading. That server answers `/System/Info/Public` with camel-case JSON. The first ReddMedia client treated this liveness response as full readiness and attempted to read the normal Pascal-case field before the catalog API was available.

## Repair

- ReddMedia no longer uses `/System/Info/Public` as its catalog-readiness gate.
- It waits for `/Startup/User`, which is served by the real API.
- HTTP 200 means the real API is ready for first-time local setup.
- HTTP 401 or 403 means the real API is ready and setup is already complete.
- The installer uses the same readiness gate before running the generated-video catalog proof.

No player path changed. Library selections still pass the direct local media path to ReddMedia's existing embedded libVLC player. Jellyfin remains hidden and its browser player remains unavailable.

Any installation or validation failure still rolls the project back to the exact accepted v0.0.14 base.
