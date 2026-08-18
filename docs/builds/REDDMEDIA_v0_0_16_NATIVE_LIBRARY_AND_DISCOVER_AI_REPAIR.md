# ReddMedia v0.0.16 Native Library and Discover AI Repair

## Owner-test failures

1. The first installer configured optional llama.cpp targets off except for the newer unified `llama-app` target. Building the default `all` target therefore entered `app/download.cpp` and `app/llama.cpp`, which failed on `arg.h` and `build-info.h` include paths.
2. The accepted v0.0.15 server manager intentionally detached Jellyfin with `setsid()` and did not stop it from `App::shutdown()`, leaving the catalog and port 8096 alive after the ReddMedia window closed.

The failed installation restored committed v0.0.15 base `d67cf6e5e0e3ce3036adae5d9695147a7aa771e8` before this repair was prepared.

## Same-version corrections

- Configure `LLAMA_BUILD_APP`, `LLAMA_BUILD_COMMON`, tests, tools, examples, server, OpenSSL, and subprocess support off for the bundled inference library build.
- Invoke `cmake --build ... --target llama` instead of the upstream default `all` target.
- Add an explicit, bounded `MediaServerManager::stop()` that signals the owned process group, waits, escalates only when necessary, and reaps the child.
- Call the stop path during ReddMedia shutdown before any bounded player-cleanup emergency exit.
- Set Linux `PR_SET_PDEATHSIG` before the Jellyfin `exec` so forced ReddMedia termination also terminates its directly owned server.
- Prevent server restart polling after shutdown begins.

## Validation

- C++ warnings-as-errors syntax validation for the changed manager and application source.
- Compiled fake-server regression proving both graceful stop and forced parent-death stop.
- Installer source gates for `LLAMA_BUILD_APP=OFF`, `LLAMA_BUILD_COMMON=OFF`, and direct `llama` target compilation.
- Target-machine full llama/Native builds, real hidden-server startup, graceful shutdown, forced-parent-death shutdown, disabled web client, and port 8096 release.

## Package

`ReddMedia_v0_0_16_NATIVE_LIBRARY_AND_DISCOVER_AI_REPAIR_CHANGED_FILES_ONLY.zip`

## Rollback

Any installer failure restores the exact committed v0.0.15 baseline and removes v0.0.16 additions and generated AI runtime files.
