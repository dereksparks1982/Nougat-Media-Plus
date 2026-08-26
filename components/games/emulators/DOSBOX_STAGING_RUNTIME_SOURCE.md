# DOSBox runtime source for Nougat Media Suite v0.0.48

Nougat v0.0.48 supports DOS games through DOSBox Staging or compatible DOSBox.

Upstream:
https://github.com/dosbox-staging/dosbox-staging

This changed-files package does not redistribute a DOSBox binary. Nougat resolves, in order:

1. `NOUGAT_DOSBOX`
2. `components/games/runtime/dosbox-staging/dosbox`
3. `dosbox-staging` on PATH
4. `dosbox` on PATH

DOS games are discovered as linked directories. Nougat chooses a likely `.EXE`, `.COM`, or `.BAT`
entry point while avoiding common installer/configuration launchers. The first owner acceptance set is:

- Grand Theft Auto / GTA 1
- Prince of Persia
- the owner's Mario DOS game

Nougat launches DOSBox with an X11 video backend and embeds the resulting game window inside the
Nougat Video Player surface. It does not intentionally fall back to a separate emulator window.
Consult upstream LICENSE/COPYING before redistributing any runtime binary.

## v0.0.48 pinned Linux runtime

The v0.0.48 build installer pins DOSBox Staging v0.82.2 Linux x86_64.

SHA-256:
`bc229df72ea103b7865cdca67324772dbffa8e58866477e69a79638b723a0442`
