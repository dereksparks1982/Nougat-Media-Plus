# Nougat Media Suite v0.0.40 Rejected-Build Repair v7

Accepted baseline: v0.0.39 commit `e9468cc0a98eefd9efd6f3dfd9ec32851dca8c26`.

This remains an unaccepted v0.0.40 repair candidate.

Repair scope:
- system-wide 3 px caramel loading sliver, no percentage text, no rolling animation;
- Live TV order: Guide, Detect Tuner, Refresh Tuner, Scan Channels, Watch Live, Refresh Guide, Record;
- Detect Tuner opens a dedicated Tuners page and performs detection; Refresh Tuner stays there; Guide returns to Guide;
- approved Nougat N image applied and verified as GNOME Files custom-icon metadata on the actual root executable;
- main Nougat Search repaired with a one-time FTS rebuild, OR-style term matching, and keyless clearnet live-discovery fallback when local/peer search has no result;
- Search row arranged field, SEARCH, RAW;
- Crawler Max Pages shown as a deliberate visible control.

The installer builds from exact accepted v0.0.39 Git files before modifying the active project, runs offline search/parser tests, performs a full native warnings-as-errors build and retained runtime gates, then snapshots and installs. Failure after installation begins triggers rollback.

Revision v6 fixes the v5 repair-script defect that used a C++ brace scanner to replace Python `federated_search()`. v6 uses the top-level `class NougatHandler` boundary and adds an explicit `py_compile` gate before any active project modification.

Revision v7 fixes the confirmed v6 executable-staging defect: an existing rejected root `Nougat_Media_Suite_v40` could be symlinked into the temporary candidate tree, then copied back as a dangling/self-referencing symlink. v7 excludes versioned executables from candidate mirroring, copies the linked binary with symlink dereferencing, and requires regular-file/ELF gates both before and after promotion to the project root.
