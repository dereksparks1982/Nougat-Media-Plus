# Nougat Media Suite v0.0.39 Rejected-Build Repair Validation

Base commit: `9957aa6a4ba439d86cd5b35f580d9cb3a9be1ed1` (pushed v0.0.38).

## Repair contract

- Same version repair: v0.0.39 remains v0.0.39.
- Diagnostics use Passed / Needs Attention / Problem / Not Tested / Information.
- Functional failures alone produce Problem. Optional metadata/poster incompleteness is Information; Search idle is Not Tested.
- Findings carry expected result, observed result, evidence, timestamp and repair guidance.
- Diagnostic history, TXT, JSON and redacted support-bundle output are retained.
- Live TV diagnostics include tuner nodes/state, signal, RF/channel, guide/PSIP state, guide coverage and logo resolution.
- Guide refresh preserves cached EPG events and merges new EIT data instead of clearing first.
- During Live TV playback, guide refresh harvests the currently tuned multiplex without retuning. A full all-frequency refresh remains queued until playback releases the single frontend.
- No channel picture slot may use call-sign text, numbers, truncated text, a generic placeholder or invented artwork.
- Final installation runs `--v39-channel-logo-audit` against the owner's persisted lineup and fails/rolls back if any channel does not resolve to actual artwork.
- VOLUME is a protected boundary and must retain SHA-256 `38197798a97e9ecadf3934daca692446bea586b36e2038c533aa5c92f51077e2`.
- The obsolete v0.0.26 diagnostics regression is updated to validate the new severity model rather than removed.
- ROADMAP records the owner-approved v0.0.40 Games / unified-emulation build after v0.0.39, including legally redistributable `2048-nes` and `NES Waveforms` test-content candidates subject to license re-verification.

## Repair-patcher correction

Function-level Live TV patching replaces the rejected repair ZIP's whitespace-sensitive exact-block substitution. The installer still fails closed if the named functions or required state cannot be identified.

## Automated gates

1. Package hash manifest verification.
2. Git branch `main` and v0.0.38 base commit verification.
3. Protected VOLUME hash preflight.
4. License/Search/P2P protected source regressions.
5. Updated v0.0.26 diagnostics compile/export/redaction regression.
6. v0.0.39 source contract test.
7. Stub CMake build with warnings-as-errors.
8. Retained executable self-tests from v25 through v38, then v39 diagnostic self-test.
9. Retained security-runtime verification.
10. Full native build, relative RPATH/shared-library verification and embedding model probe.
11. Atomic root executable install followed by the full runtime gates again.
12. Real persisted-channel artwork audit.
13. Automatic touched-state rollback on any failure.

The old v0.0.38 *source contract* is intentionally not rerun after patching because it explicitly requires the now-rejected call-sign artwork fallback. The v0.0.38 **compiled behavioral self-test** remains required.

## Corrected repair gate
- Channel-art fallback replacement is function-boundary based; validation is scoped to the channel-logo renderer rather than global source-string matching.
- Tuner patch insertion no longer duplicates the existing `refresh_guide()` signature.
- The exact compiler failure from the rejected repair was an API split: patched `main.cpp` consumed the richer v39 diagnostic fields while the payload installed incompatible diagnostic headers. The v39 diagnostic types/engine now expose the exact interface consumed by `main.cpp`, including Live TV scalar snapshots, `search_status`, `DiagnosticCheck`, `report.checks`, `attention_count`, subsystem `section`, and `write_history_snapshot`.
- `test_v39_diagnostic_api_compat.py` compiles that exact rejected-main interface under `-Wall -Wextra -Werror`.
- Before any live project file is modified, the installer creates a no-write mirror of the current v38/rejected-v39 source tree, applies the entire payload there, runs the v39 contract and retained diagnostics tests, configures CMake with P2P/AI stubs, compiles the ENTIRE Nougat v39 executable with the project warnings-as-errors policy, verifies `--version`, and runs `--v39-diagnostic-self-test`. If any of that fails, the active project has not yet been touched.

## Sole approved UI sheet authority

- `docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png` is replaced by the exact owner-supplied 1536x1024 PNG from 2026-08-23.
- Required SHA-256: `d9e57f0276877cecb69e0d8c23e3a955a78742c135221d9ff4cd902eacb1ad25`.
- This file is the sole approved Nougat UI visual authority. Screenshots supplied during testing are evidence/reference for bugs, geometry, clipping, or behavior only and must never be used as source UI artwork.

## Compile repair evidence

- The latest full-program dry-run stopped safely before touching the active project on `DiagnosticCheck::name`. The v39 compatibility type now exposes `name` as a synchronized alias of the finding title, and the warnings-as-errors API compile fixture explicitly consumes `check.name`.
