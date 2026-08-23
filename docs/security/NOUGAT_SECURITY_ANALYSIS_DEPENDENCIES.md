# Nougat Security Analysis dependencies — v0.0.33

Nougat Security Analysis is an **on-demand, one-shot** subsystem. No scanner daemon or filesystem watcher is installed.

## Pinned generated runtime

The v0.0.33 installer generates `components/security/runtime/` locally and verifies:

- YARA-X **1.19.0**
- capa **9.4.0**
- capa-rules **9.4.0**
- Magika **1.0.3**

The generated runtime is untracked and replaceable behind Nougat-owned scanner interfaces. The helper stages and verifies a new runtime before replacing an older generated runtime.

## Optional external scanner

`clamscan` may be used as a one-shot second opinion when already installed. Nougat does not run `clamd` and ClamAV availability does not decide analysis completeness.

## Free community threat intelligence

When the owner supplies a free abuse.ch Auth-Key through **Threat Intel Key**, Nougat checks MalwareBazaar, ThreatFox and URLhaus. No paid API or subscription is required or planned for this integration. Credentials are stored locally with mode 0600 and are never passed on the command line or written to scan history.

## Verdict law

`NO THREATS DETECTED` is allowed only when every required/relevant local analysis lane completes. Missing Magika/YARA-X, incomplete applicable capa analysis, or an incomplete requested online reputation pass yields `ANALYSIS INCOMPLETE`. A positive detection still yields `THREAT DETECTED`; structural mismatch evidence yields `SUSPICIOUS`. Nougat never labels a file `Safe`.

## Owner-control law

WARN ME FIRST remains absolute. Nougat never automatically quarantines, deletes, moves, renames or opens a scanned file.
