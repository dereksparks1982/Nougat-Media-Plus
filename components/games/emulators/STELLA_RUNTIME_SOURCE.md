# Stella runtime provenance for Nougat Media Suite v0.0.49

Nougat v0.0.49 adds a managed Atari 2600 runtime based on **Stella 7.0**.

Upstream project: Stella, the multi-platform Atari 2600 VCS emulator.

Pinned release: `7.0`

Managed Linux asset:

`https://github.com/stella-emu/stella/releases/download/7.0/stella_7.0_amd64.deb`

The Stella project identifies this as its 64-bit Ubuntu 22.04 binary package. Nougat does not install the package system-wide and does not use sudo. `tools/install_game_runtimes_v49.py` verifies that `dpkg-deb` reports package `stella`, version beginning with `7.0`, and architecture `amd64`, then extracts it under `components/games/runtime/stella/package/` and creates the project-local `components/games/runtime/stella/stella` launcher.

The upstream GitHub 7.0 release metadata does not publish a checksum for this asset. Nougat therefore does **not** pretend that an upstream digest exists. The installer calculates the downloaded SHA-256 and writes it, along with the verified package metadata and source URL, to `components/games/runtime/stella/UPSTREAM.txt` for the installed copy. DOSBox Staging and Xenia retain their existing pinned upstream SHA-256 checks from v0.0.48.

Stella remains an upstream open-source project. Preserve upstream licensing notices when redistributing its runtime.
