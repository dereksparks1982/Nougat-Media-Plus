# Nougat Media Suite v0.0.37 Validation

## Scope
Native Live TV Watch + Classic Guide + System/Visual Repair.

## Automated candidate gates
- C++17 warnings-as-errors stub compile.
- Retained v0.0.29-v0.0.36 executable self-tests with intentional v0.0.37 geometry expectations.
- v0.0.37 System/Library/Home/player/Live TV self-test.
- Licensing/Search/P2P/diagnostic retained source gates.
- Changed-files manifest and rollback installer contract.

## Owner-machine native gate
The installer performs a non-stub CMake build against the installed libtorrent and llama.cpp runtime, verifies the relative $ORIGIN AI RPATH, installs the root executable, and reruns the regression/self-test suite.

## Owner hardware/UI checks required
1. Library Search shows `Search`, has a green Search button on the right, and actually filters titles.
2. Continue Watching cards use the same size family as the remaining Home cards.
3. Wide seek leaves timestamps at both sides and seek/volume sprites show no pale rectangular halo.
4. Server status uses the stitched circular whole-face state button.
5. Top-level `Debug` is now `System`; Start/Stop/Refresh Server are in System rather than Library.
6. Live TV shows one logical DVB tuner for the physical tuner instead of sibling video/VBI pseudo-tuners.
7. Single-click selects a stored channel; double-click and Watch Live use the same native ATSC playback path.
8. Guide/Refresh Guide/Now show the first classic channels-by-time grid from cached PSIP EIT data.
9. Accepted 66-channel ATSC scan remains functional.

Hardware Watch Live and broadcast EPG quality cannot be proven without the owner's actual tuner/stations; those remain explicit owner acceptance checks.
