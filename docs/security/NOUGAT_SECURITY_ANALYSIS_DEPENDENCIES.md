# Nougat Security Analysis dependencies — v0.0.32

Nougat Security Analysis is an on-demand subsystem. The scanner process is created for a requested scan (or a completed Nougat download), produces a result, and exits. There is no permanent antivirus daemon or filesystem watcher.

Runtime components installed into the generated, untracked `components/security/runtime/` tree:

- **YARA-X 1.19.0** — VirusTotal — BSD-3-Clause — local rule matching.
- **capa 9.4.0** — Mandiant/Google FLARE — Apache-2.0 — executable capability analysis.
- **capa-rules 9.4.0** — matching upstream rules for capa — Apache-2.0.
- **Magika 1.0.3** — Google — Apache-2.0 — content-based file type identification.

Optional external component:

- **ClamAV `clamscan`** — invoked one-shot when already installed. Nougat does not bundle or link libclamav, and does not start `clamd`.

Optional free community reputation sources:

- MalwareBazaar Community API — hash reputation.
- ThreatFox Community API — IOC/hash reputation.

The community APIs require the owner's free abuse.ch Auth-Key. Nougat stores it only in `~/.config/nougat-media-suite/security/abusech.key` with owner-only permissions. It is never passed on a command line, written to Git, or printed in scan reports.

The v0.0.32 package contains no malware samples. Validation uses harmless fixtures including the EICAR test string.
