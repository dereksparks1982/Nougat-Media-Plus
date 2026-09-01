# Nougat Media Suite v0.0.50 Accepted Known Issues

Elderred Softworks LLC / DKLab

## Release status

v0.0.50 is accepted as the historical release state so development can continue
on v0.0.51.

The owner explicitly accepts this release with the issues below still present.

No item in this document is considered repaired merely because v0.0.50 is
committed.

## File Splitter

The v0.0.50 File Splitter workflow is not the final approved workflow.

Required v0.0.51 repair:

- An ordinary folder can be selected directly.
- Nougat creates the ZIP internally.
- An ordinary file can be selected directly.
- An existing ZIP can be selected and split directly without unnecessary
  recompression.
- The owner chooses the output base name.
- Source name may only be a suggested default.
- The user chooses the desired number of pieces.
- Nougat calculates the resulting approximate piece size before splitting.
- When a configured destination/max-piece-size constraint makes the requested
  piece count insufficient, Nougat calculates and recommends the minimum count.
- Split ZIP naming uses the owner-selected name, for example:
  My Movie.zip.001
  My Movie.zip.002
  My Movie.zip.003
- Reassembly verifies integrity and restores the original content/tree.

## HDHomeRun / unified tuner support

The HDHomeRun FLEX DUO support compiled and discovered the physical hardware,
but the owner did not accept the v0.0.50 presentation or complete scan behavior.

Required v0.0.51 repair:

- One physical FLEX DUO must appear as one physical device card.
- Its two internal tuners appear as nested resources/status rows.
- Internal tuner resources remain independently allocatable.
- Live TV status must be device-neutral.
- Do not tell the user to plug in a WinTV when a supported HDHomeRun is already
  detected.
- If multiple tuner devices are present, show the actual detected devices and
  available tuner resources.
- If none are present, use a generic "No compatible TV tuner detected" message.
- WinTV-specific setup wording belongs only in a WinTV-specific setup flow.
- Scan Channels must perform the genuine complete HDHomeRun RF scan.
- A completed RF scan must not be reported as failed merely because a later
  import, refresh, cleanup, or release step fails.
- Scan execution, result import, guide refresh, and tuner release must have
  separate accurate status reporting.
- Successful completion must show the actual channel count.

## Live TV guide provider wording

Guide refresh/status paths must not assume a Linux DVB frontend when the active
provider is HDHomeRun or another supported tuner provider.

Provider/status wording must reflect the actual active hardware/backend.

## Visible version identity

The owner observed v0.0.49 still displayed in the application UI while the
v0.0.50 executable correctly reports:

Nougat Media Suite v0.0.50

v0.0.51 must make command-line, visible UI, launcher, diagnostic, and release
identity agree.

## Nougat N icon release gate

The previous v0.0.50 icon validation was not sufficient.

Reading back metadata::custom-icon does not prove that the visible approved
Nougat N icon is actually being used.

v0.0.51 must verify the approved N visibly in:

- Files/Nautilus executable presentation
- application launcher/menu
- running application window identity
- dock/task switcher/sidebar where applicable
- in-application Nougat identity where required

The final executable must be written before final icon identity is applied and
verified.

## World TV guide layout

The v0.0.50 World TV channel list does not use the approved guide presentation.

Required v0.0.51 repair:

- World TV uses the same guide-panel structure as the accepted Live TV Guide.
- Left channel column with real channel artwork and station identity.
- Horizontal time scale.
- Program blocks across the timeline.
- Same row geometry, borders, spacing, scrolling, and interaction model.
- World TV retains its orange palette.
- Missing schedule information remains inside the guide geometry and does not
  collapse into a different list design.

## World TV playback reliability

Owner observation: most World TV channels currently do not play reliably.

The existing Russia-24 repair was intentionally surgical and was not a broad
World TV reliability rewrite.

v0.0.51 must investigate and repair World TV source resolution rather than
assuming every failure has the same cause.

Failures must be distinguishable where evidence permits, including:

- dead/stale source
- redirect/source change
- HTTP/TLS failure
- referrer/User-Agent requirement
- unsupported or malformed stream
- timeout
- region restriction
- source opens but contains unusable video/audio
- player-side failure

World TV should not silently hide useful resolver/probe failure information from
Nougat diagnostics.

## World TV channel popup

Owner-observed visual defects:

- The channel popup background is too opaque.
- The popup should be genuinely translucent so video remains visible beneath it.
- A square inner/background insert protrudes beyond the rounded right corners.
- Every popup layer must be clipped to the same approved rounded mask.
- No rectangular fill may stick through the top-right or bottom-right curve.

## v0.0.51 carry-forward

The above items are mandatory carry-forward repair work for v0.0.51.

Additional v0.0.51 feature work approved for planning includes the Radio tab and
the existing v0.0.51 roadmap work. Exact implementation remains subject to the
v0.0.51 owner scope.
