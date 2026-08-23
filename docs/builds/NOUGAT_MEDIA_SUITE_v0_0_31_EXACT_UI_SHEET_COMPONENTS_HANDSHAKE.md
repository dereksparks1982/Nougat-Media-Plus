# Nougat Media Suite v0.0.31 - Exact Approved UI Sheet Components - Build Handshake

**Base:** accepted/published v0.0.30 at `86cba5361ab67dac834e34709c1cc3d532e8da93`

## Owner-approved scope

The approved Nougat UI concept sheet is the literal component authority for v0.0.31. The existing accepted v0.0.30 page palettes and page identity remain unchanged. This build replaces component construction that only approximately resembled the sheet.

Canonical reference stored in-tree as `docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png` (SHA-256 `cd57f3840bf113f293d5fcfe9b34652098f629d40ffb7587da00e5c938bf2889`).

Included component work:

- raised primary/hover/pressed/disabled button surfaces with sheet-style rim, bevel, inset seam and lower depth;
- a sheet-derived runtime surface texture sampled from the approved PRIMARY component, with the reference hue replaced at runtime by each already-accepted page palette;
- selected top tabs with the approved downward pointer treatment;
- panel frames with the sheet's inset/depth construction;
- input-field bevel/seam treatment;
- icon-button surfaces;
- seek/progress/volume tracks and sheet-style round knobs;
- checkbox treatment;
- shared component renderer reused by top-level tabs, Stream tabs, Search tabs/controls, Discover selectors and Library view controls;
- exact v0.0.30 page palette values retained;
- Home card artwork/geometry retained byte-for-byte at the function level.

## Explicit boundary

This is a UI-component-only release. It does not add P2P behavior, tuner support, radio/SDR behavior, metadata matching changes, Home card redesign, playback behavior, server behavior, licensing changes, or Search-engine behavior.

The previously planned focused P2P expansion moves to a later build. Live TV/NextGen TV and radio/SDR are roadmap items only.

## Release gates

- warnings-as-errors C++ build;
- retained v0.0.30 behavioral contract;
- v0.0.31 component contract;
- exact accepted-base manifest verification;
- protected licensing/Search/P2P hashes unchanged;
- X11 identity/UI smoke;
- final native root executable and `$ORIGIN` AI runtime-path verification on the owner machine;
- approved N raw-executable metadata applied only after the final executable write;
- rollback proof.
