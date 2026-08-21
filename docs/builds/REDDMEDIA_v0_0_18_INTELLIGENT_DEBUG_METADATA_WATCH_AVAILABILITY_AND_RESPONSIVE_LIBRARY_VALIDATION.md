# ReddMedia v0.0.18 Validation

## Candidate status

Validation candidate only. Not owner-accepted and not tagged.

## Deterministic build-workspace validation

- Compiled every changed non-UI C++ module with C++17, `-Wall -Wextra -Werror`.
- Syntax-validated the complete native application translation unit with C++17, warnings as errors, and isolated X11 headers.
- Linked the complete stub-lane native application against the system X11 runtime, verified `ReddMedia v0.0.18`, and passed the built-in Discover AI self-test.
- Repeated Usual and Random Local/External Movie and TV recommendations and proved type separation.
- Proved a rejected 401 credential replacement preserves the validated owner-only credential.
- Parsed exact United States subscription, free, ad-supported, rent, and buy availability from mock TMDb watch-provider responses.
- Proved duplicate movie/TV catalog providers are de-duplicated, and My Services survives reload in a `0600` local settings file.
- Parsed season/episode numbers and technical stream details from mock Jellyfin metadata, inherited series artwork, and rejected a technical filename as an episode title.
- Retrieved exact TMDb episode identity, season poster, and movie poster fallback metadata.
- Produced yellow diagnostics for verified incomplete episode identity, green when corrected, and a credential-redacted report.
- Retained v0.0.17 server-ownership, poster-cache, loading-progress, and installer-rollback regression lanes.

## Owner-machine installer gates

The installer must still:

1. verify exact commit `05eba123462aecb13119bcfcf7fa41bc18f1048e`, branch `main`, unchanged index, exact installed v0.0.17 payload, expected generated runtimes, executable version, and free port 8096;
2. preserve the current v0.0.17 source/executable in a pre-v0.0.18 rollback snapshot before applying files;
3. verify every packaged payload hash and reject unexpected worktree material;
4. configure and build isolated-stub and full native targets with warnings as errors;
5. run v0.0.18 behavior tests plus retained server-lifecycle and rollback tests;
6. install the final binary only after all validation passes;
7. remove only superseded versioned launcher/binary files, preserving user data and generated runtimes;
8. apply and read back the raw executable red-tree metadata after the final binary write and then refresh Files/Nautilus;
9. restore the exact v0.0.17 candidate source and executable if any post-application step fails.

## Remaining owner-visible validation

- Confirm `ReddMedia_v18 --version` reports `ReddMedia v0.0.18`.
- Confirm the raw `ReddMedia_v18` file visibly shows the approved red-tree icon in Files.
- Confirm a real episode displays `SxxExx - title` and technical format on separate lines.
- Confirm two Library rows are visible at the normal non-fullscreen window size and wheel/arrow navigation remains consistent.
- Confirm real posters resolve through item, series/parent, or exact TMDb fallback; genuinely unidentified items alone show **NO POSTER**.
- Confirm the top bar shows one Server label with green/yellow/red behavior and no duplicate under the Library controls.
- Confirm Debug actions report real current issues, open the local log folder, and copy a useful credential-redacted report.
- Confirm a long Discover synopsis starts at its beginning, wraps, and scrolls.
- Confirm External Movie and TV show every current United States provider/category returned through TMDb, mark selected My Services, open the official watch-options link, and clearly report no listing when appropriate.
