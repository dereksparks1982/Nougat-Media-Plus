# Nougat Media Suite v0.0.39 Validation

Target: v0.0.39
Accepted base: v0.0.38

Required automated gates before handoff:
- exact accepted-v0.0.38 touched-state preflight
- package manifest SHA-256 verification
- protected licensing/Search state tests
- retained diagnostic TXT/JSON/support-bundle/redaction test
- v0.0.39 deep-diagnostic/Live-TV contract test
- warnings-as-errors stub build
- retained executable regression ladder through v0.0.38
- v0.0.39 CLI diagnostic/Live TV self-test
- full native libtorrent + llama.cpp build on owner machine
- root executable/version/RPATH verification
- rollback/fail-closed installer contract

Owner visual/hardware validation remains required after installation. Automated tests cannot prove exact on-screen stitching, broadcaster PSIP completeness at a given moment, RF reception quality, or real tuner behavior on the owner's antenna.
