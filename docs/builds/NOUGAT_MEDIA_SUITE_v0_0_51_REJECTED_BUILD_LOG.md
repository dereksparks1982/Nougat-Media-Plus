# Nougat Media Suite v0.0.51 Rejected Build Log

Elderred Softworks LLC / DKLab

Date: 2026-09-01

## Status

Nougat Media Suite v0.0.51 is REJECTED by the owner.

This commit preserves the rejected v0.0.51 working build as a historical
development checkpoint only.

It is NOT an accepted release and must not be represented as an accepted
v0.0.51 release.

No release tag is authorized for this rejected build.

All failures below remain mandatory repair work.

## 1. Top navigation scrolling

The top navigation became impossible to scroll completely to the right after
additional top-level tabs were added.

Required repair:

- The entire final top-level tab must always be reachable.
- Maximum horizontal scroll must be calculated from the actual rendered right
  edge of the final tab.
- Do not use a hard-coded tab count.
- Adding any future tab must automatically extend the scroll range.
- Mouse-wheel navigation must reach the complete final tab.
- Automated validation must fail if any part of the rightmost tab is clipped.

## 2. World TV Guide geometry

The World TV Guide is not the same size and geometry as the Live TV Guide.

Required repair:

- World TV must reuse the exact Live TV Guide outer dimensions.
- Channel column width must match.
- Timeline dimensions must match.
- Row heights must match.
- Visible row count must match.
- Program block geometry must match.
- Border, spacing and scrollbar geometry must match.
- Only the World TV orange palette and World TV data should differ.

A merely similar layout is rejected.

## 3. World TV playback reliability

Many exposed World TV channels still do not play.

Required repair:

- Verify every exposed channel against a current playable source.
- Repair stale/dead source URLs where a legitimate current source exists.
- Preserve required User-Agent and referrer headers.
- Distinguish dead source, timeout, HTTP/TLS failure, malformed stream,
  region restriction and player failure.
- Resolver and probe work must remain asynchronous and bounded.
- A failed channel attempt must never freeze Nougat.
- Do not expose a channel as working when it cannot be verified.
- Catalog verification is required before owner handoff.

## 4. Radio is a nonfunctional shell

The Radio page currently contains controls without the required working radio
system behind them.

Required Radio areas:

- AM
- FM
- Shortwave
- Weather
- DAB / DAB+
- DRM
- Internet Radio
- SDR
- Favorites
- Recordings

Required repair:

- Radio must have its own functional root view.
- Hardware-backed modes must use actually detected supported hardware.
- Unsupported capabilities must not be falsely presented as working.
- Internet Radio must actually browse/open/play radio streams.
- Favorites and Recordings require real state and actions.
- Playable radio content must use a proper radio/audio playback surface.
- Opening Radio must never inherit Video Player resume state.
- Dead decorative buttons are rejected.

## 5. Emulator expansion incomplete

The rejected build carried forward only the existing emulator backends:

- Mesen / MesenCE
- RMG
- Atari800
- Stella
- BlastEm
- DOSBox Staging
- Xenia Canary

The owner explicitly required the remaining practical emulator support to be
added to v0.0.51.

Required repair:

- Inventory the accepted Games architecture.
- Add the remaining approved Linux-capable emulator backends.
- Integrate real launch paths rather than labels or dead controls.
- Preserve controller integration.
- Preserve embedded-player behavior where appropriate.
- Preserve library and artwork integration.
- Add runtime/backend detection.
- Do not claim a system is supported unless Nougat has a real working backend.

## 6. Games artwork remains incomplete

The Games grid still displays NO ART for titles that should resolve artwork,
including visible Atari 2600 entries.

Required repair:

- Normalize ROM and archive names before matching.
- Strip region, revision, dump and version noise correctly.
- Match ZIP-contained ROM names as well as archive names.
- Preserve the persistent prepared artwork cache.
- Use accepted artwork sources and aliases.
- Never regress artwork that already resolved correctly.
- Audit unresolved visible Games entries before handoff.
- Known resolvable games showing NO ART are a build rejection.

## 7. File Splitter workflow rejected

The File Splitter implementation is rejected.

The correct location is:

Studio -> Tools -> File Splitter

The splitter must be a full embedded Nougat tool page.

It must NOT be implemented as a popup wizard.

### Required embedded controls

- Input Source
- Choose Folder
- Choose File
- Choose ZIP
- Output Folder path field
- Browse / Choose Output Folder
- Output Name
- Number of Pieces
- Calculated Approximate Piece Size
- Optional Maximum Piece Size
- Split
- Reassemble
- Cancel
- Progress
- Status
- Integrity result

### Popup prohibition

Configuration popups are rejected.

Do not use popups for:

- source type
- output name
- number of pieces
- maximum piece size
- completion status
- integrity result

The normal operating-system filesystem picker is acceptable only when the user
presses a Browse/Choose button for a source or destination.

### Output folder defect

The rejected splitter did not provide the required permanent Output Folder box
inside the File Splitter page.

The selected output directory must remain visibly displayed in the embedded
page.

### Zero-size calculation defect

The splitter suggested a maximum/piece size of 0.

That is rejected.

For an ordinary file or existing ZIP, calculate approximate piece size from the
real source size and requested piece count.

For a folder, determine/package enough information to know the resulting ZIP
size before presenting the final piece-size calculation.

If size is not yet known, display Calculating..., never 0 as a fake result.

### Piece workflow

Approved flow:

Choose source -> choose destination/output name -> choose number of pieces ->
show calculated approximate piece size -> confirm -> split.

Maximum piece size is optional and must not be forced as a normal wizard step.

When a real maximum-size limit makes the requested piece count insufficient,
Nougat must calculate and recommend the minimum required piece count.

### Packaging

- Ordinary folder: create ZIP internally, then split.
- Ordinary file: package/split appropriately.
- Existing ZIP: split directly without unnecessary recompression.
- Piece names: .zip.001, .zip.002, .zip.003, etc.
- Reassembly must verify integrity.
- Reassembly must restore original content/tree.

### Completion

No completion popup.

The embedded page must display completion, output path, piece count, manifest,
progress and integrity status.

## 8. HDHomeRun Scan Channels is incomplete

The rejected build treats RF traversal too much like the whole Scan Channels
operation.

A complete Scan Channels operation must contain separate truthful phases:

1. RF traversal
2. service/program parsing
3. channel import/update
4. channel/service resolution required by Nougat
5. guide refresh/update where available
6. tuner release/finalization

RF success alone is not completion.

A later-stage failure must identify the exact failed phase rather than
relabeling successful RF traversal as a generic failure.

## 9. HDHomeRun channel-count regression

Earlier accepted scanning found 66 channels.

The rejected v0.0.51 HDHomeRun scans have reported approximately 40 to 44
channels.

This regression is not accepted without evidence explaining where channels
were lost.

Required diagnostics:

- RF multiplexes attempted
- RF multiplexes locked
- raw services/subchannels discovered
- services successfully parsed
- channels successfully imported
- duplicate/filter decisions
- rejected/failed services with reason

If the raw tuner scan still sees the prior services but Nougat imports fewer,
the parser/importer must be repaired.

If the tuner scan itself sees fewer, scan timing, skipped RFs and lost locks
must be investigated against the prior accepted behavior.

The observed helper exit status must also be accounted for rather than merely
printed as unexplained diagnostic evidence.

## 10. Scan progress bar is false

The Scan Channels progress bar reached the end while the HDHomeRun was still
around RF 11.

This is rejected.

Required repair:

- Start at 0 percent.
- RF progress must track actual RF traversal.
- RF traversal is only one phase of the complete operation.
- Parsing/import/guide/finalization must continue progress afterward.
- 100 percent is allowed only after the complete Scan Channels workflow ends.
- On failure, progress and status must accurately identify the incomplete phase.

## 11. Nougat N icon corner artifact

The approved new N icon still shows a protruding artifact at the bottom-left
corner in the Ubuntu dock.

Earlier candidate inspection also identified bottom-corner alpha contamination.

Required repair:

- Repair the actual master alpha/mask.
- Outside the approved rounded badge must be transparent.
- Regenerate every icon size from that corrected master.
- Regenerate embedded _NET_WM_ICON data from the same master.
- Apply the same corrected source to launcher, executable, dock, switcher and
  Files/Nautilus identity.
- Any visible corner debris on any required identity surface rejects the build.

## 12. Search top-tab hover is inconsistent

The Search top-level tab has a noticeably weaker hover flash/highlight than
the neighboring top-level tabs.

Required repair:

- Search must use the same top-tab hover rendering path as every peer tab.
- Hover intensity/state transition must be equivalent.
- Only the Search cream palette may differ.
- No special weakened Search hover state is allowed.

## 13. Root executable handling

Previous rejected v0.0.51 candidates incorrectly left
Nougat_Media_Suite_v50 beside Nougat_Media_Suite_v51.

The owner manually removed the old executable.

Required permanent rule:

- Successful v0.0.51 promotion leaves Nougat_Media_Suite_v51 as the only
  versioned Nougat executable in the project root.
- Nougat_Media_Suite_v50 belongs in rollback/history, not beside v51.
- Build validation must reject extra versioned root executables.

A later builder also incorrectly rejected the correct deleted-v50 state as an
unexpected working-tree change. That build-workflow bug must not recur.

## 14. Archive path regression

Rejected candidate tooling created:

$HOME/DKLab/Archives

The required canonical location is:

$HOME/DKLab/Archive

Required repair:

- Never create DKLab/Archives again.
- Use the existing singular DKLab/Archive tree.
- Any material created by earlier rejected builders under the plural path must
  be safely migrated into the singular archive tree.
- Do not require the owner to perform that cleanup manually.

## 15. Candidate handoff quality gate

The owner repeatedly rejected v0.0.51 candidates because required scope items
were missing or incomplete.

Permanent handoff requirement:

- Do not hand off a candidate merely because it compiles.
- Do not hand off dead UI shells.
- Do not hand off known broken artwork.
- Do not hand off incomplete scan phases.
- Do not hand off a World TV catalog with known dead exposed channels.
- Do not hand off mismatched guide geometry.
- Do not hand off icon alpha defects.
- Do not hand off navigation that cannot reach the final tab.
- Do not hand off popup workflows where embedded Nougat controls were required.

The candidate must satisfy automated validation before owner-visible testing.

## Commit meaning

The owner explicitly ordered this rejected build to be logged and committed.

This commit therefore preserves the rejected v0.0.51 state for development
history.

It does not convert the build into an accepted release.

No v0.0.51 release tag is authorized by this checkpoint.
