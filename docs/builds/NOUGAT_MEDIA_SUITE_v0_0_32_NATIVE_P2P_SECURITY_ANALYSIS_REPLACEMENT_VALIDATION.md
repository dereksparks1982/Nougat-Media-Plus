# Nougat Media Suite v0.0.32 Native P2P + Security Analysis Replacement Validation

Required gates before handoff:

- warnings-as-errors C++ stub build;
- existing v0.0.31 retained behavior and v0.0.32 P2P Range/seek tests;
- security static policy test: one-shot, WARN ME FIRST, no automatic quarantine/destructive behavior;
- harmless file one-shot scan with SHA-256 and 0600 history verification;
- harmless EICAR standard test-string fixture with deterministic YARA-X stub, proving threat reporting without downloading live malware;
- exact Crawler sentence and layout-position-only repair check;
- ordinary Search Node ID removal / Network-advanced retention check;
- P2P local-seed and availability evidence check;
- Home fixed-header clipping and both explicit Home scrollbars;
- seek-style volume track with no oversized outer housing;
- installer/rollback structural test and exact package manifest/hash verification;
- security scaffold syntax/policy verification with one-shot scanner exit behavior; full pinned YARA-X/capa/Magika runtime installation is intentionally deferred;
- owner-machine full native libtorrent + llama.cpp build, `$ORIGIN` runtime-path check, launcher/icon readback, and X11 identity smoke.

No live malware samples are used by the build or validation package.

- Home keeps its fixed top header, clips scrolling cards below it, and adds a right-side vertical page scrollbar plus a sheet-style horizontal Continue Watching scrollbar.
