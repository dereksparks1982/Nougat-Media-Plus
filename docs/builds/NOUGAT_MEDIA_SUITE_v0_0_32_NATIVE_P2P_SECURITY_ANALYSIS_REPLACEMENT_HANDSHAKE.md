# Nougat Media Suite v0.0.32 Native P2P + Security Analysis Replacement Handshake

## Owner-approved scope

This same-version v0.0.32 replacement keeps the installed native P2P candidate and adds Nougat Security Analysis rather than advancing the version.

- Search internal tabs: `Search | Crawler | P2P | Virus Scan`.
- Manual Scan File / Scan Folder / Scan Again / Recent History.
- One-shot security worker only. No Nougat-installed daemon or filesystem watcher.
- Security scaffolding first: SHA-256/type checks and one-shot worker are active now; pinned YARA-X 1.19.0, capa 9.4.0 + capa-rules 9.4.0, and Magika 1.0.3 integration hooks are present but their full runtime installation is intentionally deferred.
- Optional external one-shot `clamscan`; never link/bundle ClamAV.
- Optional free/community MalwareBazaar and ThreatFox lookups using an owner-supplied free abuse.ch Auth-Key stored outside Git.
- WARN ME FIRST: no automatic quarantine/delete/move/rename/open action.
- Completed selected P2P media gets the same one-shot analysis.
- Add explicit local seed/seeding availability evidence.
- Remove ordinary-page Node ID; retain it in Network/Advanced.
- Preserve Crawler geometry and wording; move only its existing status sentence upward.
- Preserve the already-built v32 autoplay, Search stitch, Stream border and P2P streaming work; replace only the rejected oversized volume housing with the shorter Seek-style track.
- v0.0.33 is P2P Plus; advanced torrent-management features are deferred there.

## Rollback and base

Accepted Git base remains v0.0.31 commit `b22d11d4989784dc6df56abbde08344720064790`. The replacement installer is hard-gated to the clean accepted v0.0.31 Git base, snapshots the exact touched state before applying v0.0.32, and restores it on failure.

- Home keeps its fixed top header, clips scrolling cards below it, and adds a right-side vertical page scrollbar plus a sheet-style horizontal Continue Watching scrollbar.
