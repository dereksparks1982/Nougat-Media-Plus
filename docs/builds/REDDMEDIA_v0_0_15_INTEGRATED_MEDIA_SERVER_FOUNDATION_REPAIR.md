# ReddMedia v0.0.15 Candidate Repair Record

## Rejected candidate evidence

The first v0.0.15 candidate used development snapshots of Jellyfin Server and Jellyfin Web and rebuilt both during installation. Jellyfin Web completed its type check, all 162 tests, and production webpack build. The server phase then failed after restore when the .NET SDK reported `MSB4242` because its worker node was shut down. The installer correctly rolled the project back to accepted v0.0.14, so that candidate was not accepted.

The web dependency installation also reported upstream audit findings. Those findings and the server build cancellation made the development snapshot unsuitable as ReddMedia's working base.

## Repair decision

The repaired candidate stays at v0.0.15 because no v0.0.15 build has been accepted. It uses official stable Jellyfin 10.11.11 Ubuntu 26.04 packages supplied by the owner. The installer verifies and extracts those packages without registering a system Jellyfin service or rebuilding upstream source. Matching tagged source archives and GPL licenses remain bundled for provenance and license compliance.

## Acceptance remains pending

This record documents the failed attempt; it does not erase or reclassify it. The first stable-package foundation installed successfully but exposed Jellyfin's setup page, so it was also rejected as a product experience. The same-version native-Library/hidden-server repair supersedes that candidate. v0.0.15 may not be committed, tagged, or treated as accepted until the owner validates the native ReddMedia Library and embedded playback route.
