# v0.0.50 Source Slimming Ledger

Status: candidate work. `main` / accepted v0.0.49 remains the historical preservation point while v0.0.50 is developed on `candidate/v0.0.50`.

The purpose of this ledger is to make every large-source-tree removal explicit. v0.0.50 does not delete user media or throw away the knowledge required to reproduce a dependency. Downloaded/generated runtime payloads are moved out of source conceptually and must be installed into Nougat-managed runtime locations instead.

## Rules

1. Preserve source code, product assets, tests, configuration/templates, license/notices, component source/version/hash records, build/install recipes, and legally redistributable product content.
2. Do not keep downloaded third-party runtime binaries, downloaded model files, build products, generated caches, logs, or user data in the canonical source package.
3. Accepted v0.0.49 Git history and the pre-v0.0.50 full-project archive remain the preservation point for historical payload bytes.
4. Git blob SHA-1 values below identify the exact historical Git object. They are not substituted for SHA-256 when a future installer needs cryptographic package verification.
5. If a trusted upstream SHA-256 was not preserved, automatic re-download remains disabled until one is deliberately pinned and verified.

## Large candidate removals

| Path | Bytes | Git blob SHA-1 | Classification | v0.0.50 disposition |
| --- | ---: | --- | --- | --- |
| `components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf` | 84,106,624 | `d787b8af3681aac9ee5d5aadc2ca7377a751af79` | downloaded local-AI model | remove from source; future managed model under `~/.local/share/nougat/runtime/models/`; automatic install blocked until upstream URL/license/SHA-256 are pinned |
| `components/ai/source/llama.cpp-pinned-source.zip` | 38,805,576 | `16d86ee62247fa75c31601254d38fbd500376eb4` | downloaded third-party source archive | remove archive bytes from source; preserve runtime/source recipe instead |
| `components/jellyfin/packages/jellyfin-server_10.11.11+ubu2604_amd64.deb` | 57,355,714 | `76291d6084aa1ac6f541973545327e802ee948fc` | downloaded third-party package | remove from source; Jellyfin becomes optional managed Media Server component |
| `components/jellyfin/packages/jellyfin-web_10.11.11+ubu2604_all.deb` | 31,953,930 | `266644c617b9c15ff8b80be763bbc3d5df8ae519` | downloaded third-party package | remove from source; Jellyfin becomes optional managed Media Server component |
| `components/jellyfin/source/jellyfin-10.11.11.zip` | 3,198,040 | `84302a3729c10472fbf692aae3fc14641e8d26ff` | downloaded third-party source archive | remove archive bytes; preserve project/version/license/upstream record |
| `components/jellyfin/source/jellyfin-web-10.11.11.zip` | 4,897,515 | `594325e4af7312c9e9532b4f37c163d95cb15089` | downloaded third-party source archive | remove archive bytes; preserve project/version/license/upstream record |
| `Nougat_Media_Suite_v49` | 4,224,176 | `561cbf795a1cf92e2f54da7274221c8f56d60073` | generated accepted executable | remove from v0.0.50 source branch; accepted v0.0.49/tag/history remains authoritative |

Known reduction from only the rows above: **224,541,575 bytes** (about 214.1 MiB) from the candidate working tree/source package before counting generated `build/`, downloaded emulator runtimes, yt-dlp payloads, caches, repair payloads, or other generated material.

## Material explicitly preserved

The following categories stay in the source project even when they consume meaningful space:

- Nougat C++/Python source and build scripts.
- Approved UI/concept-sheet/component artwork required to reproduce the application.
- `COMPANY_BIBLE.md`, README, CHANGELOG, roadmap, architecture, licensing, dependency, build and validation records.
- Emulator integration code and per-emulator source/license/version records.
- Legally redistributable bundled homebrew/public-domain/otherwise-approved game content and its upstream license/attribution records.
- Tests and deterministic diagnostic assets.
- Component/update manifest schemas and migration logic.

## Generated `build/`

`/build/` is already ignored by `.gitignore` but legacy generated CMake output is present in Git history. It is build output, not canonical source. v0.0.50 packaging excludes it and the candidate branch should remove tracked build artifacts. A fresh temporary build directory must be used for v0.0.50 validation.

## Runtime migration safety

Historical runtimes that exist on Derek's workstation are not to be blindly deleted by the source cleanup. The v0.0.50 local migration sequence is:

1. detect legacy runtime/cache/data location;
2. install/copy/move into the new Nougat-managed destination;
3. verify the managed component launches and passes its health test;
4. prefer the new path;
5. only then make an old duplicate eligible for explicit cleanup.

No source-tree deletion authorizes deletion of the corresponding runtime from a user's machine.
