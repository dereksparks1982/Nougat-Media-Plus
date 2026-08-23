# Nougat Media Suite v0.0.31 - Exact Approved UI Sheet Components - Validation

## Container validation

The candidate source was configured and built with `-Wall -Wextra -Werror` through the project's CMake configuration using the P2P and AI stubs available in the build environment.

Validated locally before packaging:

- `Nougat_Media_Suite_v31 --version` reports `Nougat Media Suite v0.0.31`;
- retained v0.0.30 UI/Library/Player self-test passes;
- v0.0.31 exact UI-sheet component self-test passes;
- the runtime includes the sheet-derived button surface texture generated from the canonical approved sheet;
- source contract validates retained page palettes, protected files and unchanged Home-card functions;
- X11 application identity smoke validates canonical WM class/application identity under Xvfb;
- package manifest hashes and ZIP integrity are verified.

## Owner-machine gates performed by installer

The installer additionally performs the real non-stub native build using the owner's installed libtorrent/AI runtime, checks the relative `$ORIGIN` runtime path, runs retained runtime self-tests, writes the final root `Nougat_Media_Suite_v31`, applies/read-backs the approved N custom-icon metadata, installs launchers, and removes the superseded v0.0.30 root executable only after v0.0.31 passes.

## Required visual acceptance

The owner should compare the running application directly against the approved UI sheet. Components that still do not visually match the sheet are not considered accepted merely because automated tests pass. Existing page color palettes must remain the accepted v0.0.30 palettes.
