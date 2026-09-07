# Nougat Media Plus v0.0.66 UI Authority + PS3 Candidate

Status: candidate only. Owner acceptance required.

- Base authority: accepted v0.0.65 commit `0b87645b6d7d79c7f2e8570d4e5fb69881a8f552`.
- Owner-supplied screenshots are reference-only and are not packaged or rendered as runtime UI art.
- Existing `src/nougat_ui_sheet_texture_data.hpp` is the button-art authority used by this candidate.
- Duplicate top navigation buttons are removed.
- Side navigation buttons use the approved sheet button surface.
- The side-rail N destination is square and placed at the bottom of the rail.
- Studio film strip is removed and Tools/Drone are separate controls.
- Search/Crawler/P2P/Archive use the approved sheet surface; SEARCH/RAW stay on the same approved surface.
- Dock/executable icon authority is `assets/branding/nougat-media-plus-dock-N.png`.
- RPCS3 remains integrated under Games. No standalone PS3 graphics application.
- Xbox/Xenia source and runtime are frozen.
- No GUI auto-open and no GitHub writes.

## REPAIR2 validation-path correction
The temporary build-directory CLI self-test now receives a byte-identical copy of the existing project UI authority marker at the path the executable resolves from `/proc/self/exe`. The promoted root executable is self-tested again against the real project asset. This changes validation only, not UI rendering or approved scope.
