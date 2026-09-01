# Nougat Media Suite v0.0.53 Carry-Forward

Elderred Softworks LLC / DKLab

Status: mandatory roadmap after the Radio-only v0.0.52 build

v0.0.52 was intentionally narrowed by the owner to professional Radio. Nothing listed below is waived. These items remain mandatory v0.0.53 work unless the owner explicitly changes scope.

## 1. Top navigation scrolling

- Entire final top-level tab must always be reachable.
- Maximum horizontal scroll comes from the actual rendered right edge of the final tab, never a hard-coded tab count.
- Future tabs automatically extend the range.
- Mouse-wheel navigation must reach the complete last tab.
- Automated validation must reject clipping.

## 2. World TV Guide exact geometry

- Reuse the exact Live TV Guide outer dimensions, channel column, timeline, row heights, visible row count, program block geometry, borders, spacing and scrollbar geometry.
- Only the World TV orange palette and data differ.

## 3. World TV playback reliability

- Verify every exposed station against a current playable legitimate source.
- Repair stale/dead URLs where a legitimate source exists.
- Preserve required User-Agent/referrer headers.
- Distinguish dead source, timeout, HTTP/TLS, malformed stream, region restriction and player failure.
- Keep resolver/probe work asynchronous, bounded and cancellable.
- Never expose an unverified station as working.
- Catalog verification is required before handoff.

## 4. Studio File Splitter workflow

Authoritative location is **Studio → Tools tab → File Splitter button**.

File Splitter is a button inside the Tools tab, not another tab. The Tools tab will contain multiple tool buttons over time. Clicking File Splitter loads its complete embedded interface into the Studio/Tools content area.

Required embedded controls:

- Input Source
- Choose Folder
- Choose File
- Choose ZIP
- Output Folder path
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

No configuration wizard and no completion popup. Normal filesystem pickers are allowed only after explicit Browse/Choose actions.

- Ordinary folder → ZIP internally → split.
- Ordinary file → package/split appropriately.
- Existing ZIP → split directly without unnecessary recompression.
- Pieces use `.zip.001`, `.002`, etc.
- Real source size must drive calculation. Unknown folder package size shows `Calculating...`, never fake `0`.
- Output Folder remains permanently visible.
- Maximum Piece Size is optional.
- If a real maximum requires more pieces, recommend the minimum mathematically required count.
- Reassembly verifies integrity and restores the original content/tree, preserving symlinks/modes where practical.
- Completion remains embedded with path, piece count, manifest, progress and integrity.

## 5. HDHomeRun full Scan Channels pipeline

A complete operation has separate truthful phases:

1. RF traversal
2. service/program parsing
3. channel import/update
4. service/channel resolution required by Nougat
5. guide update where available
6. tuner release/finalization

RF success is not total completion. Later failures must identify the exact phase.

## 6. HDHomeRun channel-count regression

Earlier accepted scanning found about 66 channels; rejected v51 observed about 40–44.

Required evidence:

- RF multiplexes attempted
- multiplexes locked
- raw services/subchannels discovered
- services parsed
- channels imported
- duplicate/filter decisions
- rejected/failed services with reasons

If raw tuner results still approach the prior count but imports do not, repair parser/import. If raw reception is lower, investigate timing, skipped RF channels and lost locks against the prior accepted behavior.

## 7. HDHomeRun helper exit evidence

The observed helper exit status `1` must be explained and assigned to the correct operation/phase instead of merely printed.

## 8. HDHomeRun progress truthfulness

- Start at 0.
- RF traversal occupies only its portion of the total job.
- Parsing/import/guide/finalization continue progress afterward.
- 100 only after the whole workflow ends.
- Failure reports the incomplete phase and truthful progress.

## 9. Nougat N icon corner artifact

- Repair the approved N master alpha/mask so everything outside the rounded badge is transparent.
- Regenerate all icon sizes and embedded `_NET_WM_ICON` from the same corrected master.
- Apply the same source to launcher, executable, dock, switcher and Files/Nautilus identity.
- Any visible bottom-corner debris rejects the build.

## 10. Search top-tab hover

Search must use the same hover rendering path, state intensity and transition as neighboring top tabs. Only palette differs.

## 11. Emulator expansion

Inventory the accepted Games architecture and add the remaining owner-approved practical Linux-capable emulator backends with real runtime detection/launch paths. Preserve controller integration, embedding where appropriate, library integration and artwork. No fake system support.

Existing seven remain:

- Mesen / MesenCE
- RMG
- Atari800
- Stella
- BlastEm
- DOSBox Staging
- Xenia Canary

## 12. Games artwork

- Normalize ROM/archive names.
- Strip region/revision/dump/version noise correctly.
- Match contained ZIP ROM names as well as archive names.
- Preserve prepared persistent artwork cache.
- Use accepted artwork sources/aliases.
- Do not regress resolved artwork.
- Known resolvable games showing `NO ART` reject the candidate.

## 13. Root executable and rollback handling

Successful new-version promotion leaves only the current versioned Nougat executable in the project root. Older executables belong in rollback/history. Build tooling must not reject the correct deletion of the prior root executable as an unexpected change.

## 14. Archive path

Canonical path is `$HOME/DKLab/Archive/` singular. Never recreate `$HOME/DKLab/Archives/`. If rejected tooling left material in the plural tree, migrate it safely and remove the bad tree only after successful migration.

## 15. Freeze / process identity

- Network/resolver/probe/player-start work must not block the X11 event loop.
- Workers need bounds, cancellation, timeouts and recovery.
- No special in-app frozen-shutdown button.
- Ubuntu Processes/System Monitor must identify the running product clearly as Nougat Media Suite so ordinary external End/Kill works.
- Do not blindly kill unrelated Jellyfin processes.

## 16. Floating overlays

Transient overlays must be genuinely translucent, rounded/clipped without square backing, and limited to transient overlays rather than whole pages.

## 17. LAN Web Viewer

Continue the Nougat-owned LAN-only service foundation with versioned catalog/artwork/media/resume/history/Live-TV/session/pairing/diagnostic interfaces. No cloud login requirement, external relay or automatic port-forwarding.

## 18. Candidate handoff quality

Do not hand off merely because it compiles. Reject dead controls, known broken artwork, incomplete scan phases, known dead exposed channels, mismatched guide geometry, navigation clipping, wrong embedded/popup workflows, or false status reporting.

## 19. AMBER Alerts and official public warnings

Add a future alerts subsystem using legitimate official warning sources. Roadmap targets include:

- AMBER Alerts
- severe-weather alerts
- civil emergency alerts
- evacuation alerts
- other official public-safety warnings where appropriate

The alert subsystem must remain separate from ordinary scanner audio. It should be location-aware by user configuration/system location permission, identify the source and timestamp, avoid duplicate floods, preserve alert history, and never fabricate an alert from radio noise or an unverified social feed.

## 20. Controller roadmap

App-wide controller navigation remains roadmap work unless the owner explicitly moves it into an active build. Configuration belongs under System, with separate UI, Video, Games and Drone Flight contexts.

## 21. Aerial Production

Aerial Production/drone-control work remains future roadmap only unless explicitly approved into a build.
