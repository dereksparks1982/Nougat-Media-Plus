Nougat Media Suite v0.0.49 Stella Installer Repair
===================================================

Repairs the v0.0.49 Stella package metadata validator after dpkg-deb emitted
labeled control fields such as `Package: stella` rather than bare values.

Also validates extracted Stella shared-library dependencies with ldd before
installing the managed runtime, so a genuine missing library is reported by
name rather than surfacing later as an emulator launch failure.

This repair is safe after the main v0.0.49 source patch has already applied.
`tools/build_v49.py` will detect that the v0.0.49 source repair is already
present and resume the build.

No Git action is performed by this repair.
