# Nougat Media Suite v0.0.55 Web Player candidate icon repair validation

Status: candidate repair, not accepted.

## Rejection being repaired

The first v0.0.55 Web Player candidate showed a generic gear for the raw
`Nougat_Media_Suite_v55` executable in Files/Nautilus. Under the Company Bible,
that alone rejects the build.

## Repair scope

This is an identity-only same-version repair. The v0.0.55 executable bytes and
Web Player implementation are preserved. Games, PS2, Xbox/Xenia, emulator
runtimes, and `src/games/emulator_host.cpp` are not modified.

## Approved icon authority

- `assets/icons/nougat-media-suite-concept-sheet-v24.png`
- SHA-256: `681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804`

## Mandatory apply-time identity gate

The final root executable must be written before icon metadata is assigned.
After extraction into the Nougat project root, the apply procedure must:

1. leave the final `Nougat_Media_Suite_v55` bytes in place;
2. assign `metadata::custom-icon` on that raw executable to the file URI of the
   approved icon master above;
3. immediately read `metadata::custom-icon` back and verify the exact same URI;
4. install/refresh both canonical desktop launchers targeting v55;
5. keep `Icon=nougat-media-suite`, `StartupWMClass=NougatMediaSuite`, and
   `X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite`;
6. refresh the local desktop/icon caches when the host utilities exist.

Moving, replacing, or rebuilding the executable after step 2 invalidates the
identity proof and requires the metadata assignment/readback to be repeated.

## Static validation completed for this handoff

- v55 executable SHA-256 remains
  `a0fdf08597e9026b45b22c507be2e42fec03e33984418572e5628a2ac5f234e7`.
- approved icon master SHA-256 matches the authority above.
- both project desktop launchers target `Nougat_Media_Suite_v55`.
- both project desktop launchers use `Icon=nougat-media-suite`.
- embedded X11 icon-data source remains present in the unchanged v55 source.
- Xbox/Xenia emulator-host source is not part of this repair payload.

Owner visual confirmation in Files/Nautilus and the running dock remains the
final acceptance gate.
