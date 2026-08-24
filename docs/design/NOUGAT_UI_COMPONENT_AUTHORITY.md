# Nougat UI Component Sheet Authority

The file `NOUGAT_UI_COMPONENT_SHEET_APPROVED.png` in this directory is the owner-approved,
literal visual specification for Nougat Media Suite controls.

## Mandatory interpretation

Corresponding application controls are not merely inspired by this sheet. Their visible
shape, bevel, border, fill, knob, track, spacing, and component treatment must reproduce
the approved sheet as exactly as technically possible.

For the Video Player seek control specifically:

- Use the actual **SEEKBAR (PROGRESS)** artwork from the approved sheet.
- The runtime seek asset is derived from the sheet pixels, not a hand-drawn approximation.
- Elapsed time belongs immediately beside the left side of the seek bar.
- Total duration belongs immediately beside the right side of the seek bar.
- Do not push those time readouts to the outer window edges.
- Automated tests are regression aids only. Owner visual acceptance is the final visual gate.

Approved sheet SHA-256: `2aac30780b98606693be192a4cbd4ff0fbe605ac39d561f320022b6fcf0d0589`
Seek-frame asset SHA-256: `35d678ff28564a792b35c7a4fbd796fecc260f9f832935312a5f9dbcba02f7ee`

The seek-frame source crop for this 1536x1024 approved sheet is:
`x=42, y=659, width=378, height=33`.
