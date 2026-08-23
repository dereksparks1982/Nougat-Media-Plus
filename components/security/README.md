# Nougat Security Analysis

`nougat_security_worker.py` is the one-shot scanner worker. The v0.0.33 installer creates and verifies `runtime/` locally with pinned YARA-X 1.19.0, capa 9.4.0 + capa-rules 9.4.0, and Magika 1.0.3. `runtime/` is generated and untracked.

The worker exits after each file/folder/history request. It never runs a daemon and never quarantines, deletes, moves, renames or opens files automatically.
