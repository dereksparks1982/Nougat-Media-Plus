# Nougat Security Analysis runtime boundary

Tracked files in this directory are Nougat's one-shot scanner worker and conservative built-in rules.

`runtime/` is generated on the owner's machine by the v0.0.32 installer and must remain untracked. It contains the private Python environment and the matching capa rule release. The scanner has no daemon mode.

User secrets and extensions live outside the project:

- `~/.config/nougat-media-suite/security/abusech.key` — optional free abuse.ch Community Auth-Key, mode 0600.
- `~/.config/nougat-media-suite/security/rules/` — optional owner-added YARA rules.

Nougat's policy is **WARN ME FIRST**. The scanner never automatically deletes, moves, renames, quarantines, or opens scanned files.
