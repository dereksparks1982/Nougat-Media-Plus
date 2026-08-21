# ReddMedia v0.0.17 Validation

## Candidate status

Validation candidate only. Not owner-accepted and not tagged.

## Deterministic validation completed in the build workspace

- Compiled changed C++ modules with C++17, `-Wall -Wextra -Werror`.
- Syntax-validated the full native application translation unit through the isolated X11 test headers.
- Exercised both TMDb API-key and read-access-token authorization against a local mock service.
- Verified successful credential persistence uses mode `0600`.
- Verified an invalid replacement receives a clear 401 and does not overwrite the working credential.
- Verified Test and Clear behavior.
- Repeated Random and Usual selection for both Movie and TV sources and proved no type crossover.
- Retrieved, normalized, and cached Jellyfin and TMDb poster data.
- Exercised normal owned-server stop, forced parent-death stop, and independent-server preservation on port 8096.
- Verified Start Server, Stop Server, and Refresh Server controls are present.
- Verified generated AI/Jellyfin runtime paths are ignored and excluded from the changed-files package.

## Installer gates

The owner-machine installer must still:

1. verify exact commit `05eba123462aecb13119bcfcf7fa41bc18f1048e`, clean tracked/index state, v0.0.16 executable, rollback snapshot, runtimes, and free port 8096;
2. verify every packaged payload hash;
3. configure and build native and isolated-stub targets with warnings as errors;
4. run v0.0.17 Discover/poster/credential and server-lifecycle tests;
5. install the final binary only after all tests pass;
6. apply and read back the raw executable red-tree metadata after the final binary write;
7. preserve both generated runtime trees and restore the exact v0.0.16 tracked state if a post-application step fails.

## Remaining owner-visible validation

- Confirm `ReddMedia_v17 --version` reports v0.0.17.
- Confirm the raw `ReddMedia_v17` file shows the approved red-tree icon in Files.
- Confirm real Movies and TV Library entries show available artwork.
- Confirm Local and External Movie buttons never return TV, and Local and External TV buttons never return movies.
- Confirm real TMDb Test, Save/Replace, Clear, persistence after restart, and 401 correction behavior.
- Confirm the red loading bar moves during slow work and completes quickly during fast work.
- Confirm Start, Stop, and Refresh report real server state; closing ReddMedia releases its owned server and port 8096; a separately started server remains running.
