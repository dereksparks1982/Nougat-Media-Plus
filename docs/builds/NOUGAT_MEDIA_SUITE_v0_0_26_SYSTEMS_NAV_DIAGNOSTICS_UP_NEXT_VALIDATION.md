# Nougat Media Suite v0.0.26 Validation

Base: accepted v0.0.25, commit `c4d174466c2bb30c4eda8f04f09105e5d583040c`.
Target: v0.0.26 candidate.

## Deterministic gates
- `test_license_protection_v22.py`: protected license/owner-rights/third-party boundary PASS.
- `test_nougat_v19.py`: Search engine ranked/raw/crawler/peer/persistence PASS.
- `test_nougat_bridge_v19.py`: native Search bridge PASS.
- `test_nougat_media_suite_retained_v22.py`: watch providers, My Services, episode identity, poster fallback, updated Nougat diagnostic identity PASS.
- `test_media_server_lifecycle_v17.py`: owned-server graceful stop/parent-death stop/independent-server preservation/control contract PASS.
- `test_nougat_diagnostics_v26.py`: TXT report, JSON report, support bundle, sensitive-log redaction, and unattempted-VLC truth handling PASS.
- `test_nougat_media_suite_v26.py`: mouse navigation, Library header, N bottom cleanup, diagnostics, centered 0-200% volume, header layering, 10-second Up Next, P2P deferment, roadmap carry-forward, Search/license preservation PASS.
- warnings-as-errors stub CMake build: PASS.
- stub `--version`: `Nougat Media Suite v0.0.26` PASS.
- Discover AI stub self-test: PASS.
- retained provider/Discover UI-state self-test: PASS.
- v0.0.26 X11 identity/window smoke: PASS.

## Native validation boundary
The package-build container does not provide the libtorrent development package, so the full non-stub libtorrent link cannot be proven here. The installer requires `libtorrent-rasterbar` on the owner's workstation and performs the full native build there, with the real bundled llama.cpp runtime/model, relative `$ORIGIN` RPATH verification, version/self-tests, X11 smoke, and final root-executable verification before declaring FINAL PASS.

## Owner-only real-use checks
- Actual mouse side buttons.
- Actual GNOME dock/sidebar and Files icon edge.
- Header top-tab wheel scrolling over fixed version/server state.
- Subjective volume placement at normal application sizes.
- Diagnostic export inspection against the owner's real Jellyfin/library/Search/AI/Stream state.
- A real TV series natural EOF and the 10-second Up Next transition.

No owner acceptance is implied by deterministic validation. v0.0.26 remains a candidate until the owner explicitly accepts it.
