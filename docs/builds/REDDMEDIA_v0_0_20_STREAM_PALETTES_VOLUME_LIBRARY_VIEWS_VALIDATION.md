# ReddMedia v0.0.20 Validation Record

## Pre-handoff validation

- Deterministic ReddMedia v0.0.20 behavior suite: PASS.
- Nougat Ranked/RAW/crawler/peer/persistence retained behavior: PASS.
- Native Nougat bridge: PASS.
- Watch-provider / My Services / episode identity / poster fallback / diagnostics retained harness: PASS.
- Viewing-history completion and TV-autoplay contract: PASS.
- v0.0.20 UI contract: palettes, Stream selectors/actions, 0-200% volume, 100% marker, centered wide bottom controls, Grid/List Library mode, visible caret markers: PASS.
- C++17 deterministic stub build with `-Wall -Wextra -Werror`: PASS.
- `ReddMedia_v20 --version`: PASS.
- Stub Discover AI self-test: PASS.
- Bounded Xvfb native-window smoke: PASS.

## Environment-limited gate

The build container uses glibc 2.41 while the already-pinned owner runtime contains llama.cpp binaries requiring glibc 2.43, so the final full linked runtime lane cannot be truthfully completed in this container. The installer therefore repeats the full real libtorrent + pinned llama.cpp build and real model self-test on the owner's Ubuntu environment before it can install the root executable or print FINAL PASS.

## Owner-visible checks after FINAL PASS

- Exact top order is `Video Player | Library | Discover | Nougat | Stream | P2P | Debug`.
- Full interior palette is visibly applied in every tab.
- Volume starts at 100%, reaches 200%, and shows the 100% marker.
- Bottom player buttons center when wide and remain wheel-scrollable when narrow.
- Movies and TV independently retain Grid/List selection.
- Custom fields show a visible caret when focused.
- Stream platform selectors and Download / Play / Direct Watch / Open Webpage work with supported public URLs.
- Existing local playback, Library, Discover, Nougat, P2P, Debug, TV autoplay, and Library navigation continuity remain intact.
- Raw `ReddMedia_v20` shows the approved red-tree icon in Files/Nautilus.

## Accepted-base package-gate repair

- Final accepted v0.0.19 manifest hash verified: `26106d40b8878d08c7fb759217c6a56ad8c5221df1a732028261450c272f8a47`.
- Replacement package rejects the stale intermediate manifest hash before project modification.
- Feature payload is unchanged from the v0.0.20 test candidate.

- Packaging repair also ensures `tools/test_installer_rollback_v20.py` is actually applied before the installer invokes it.
