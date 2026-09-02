# Nougat Media Suite v0.0.53 Candidate Scope

Base authority: accepted/published v0.0.52 commit
`8e346237928d4d358136b926f70e27729b6bd731`.

This is a candidate only. It is not committed, tagged, or pushed. Owner testing is the acceptance gate.

## Implemented candidate work

- Preserve the accepted geometry-derived top-navigation scrolling and validate that the final System tab remains the authority for maximum scroll.
- Make World TV reuse the Live TV Guide outer panel geometry, 166 px channel column, 34 px timeline header, 46 px rows, visible-row calculation, program grid spacing, current-time marker, and vertical scrollbar behavior. World TV keeps its orange palette/data.
- Add explicit World TV failure classes for dependency, resolver, provider, stream, and startup-timeout failures while retaining bounded direct-source probing, User-Agent/referrer support, and playable-frame/audio verification.
- Replace the Studio File Splitter's normal popup workflow with an embedded Studio surface containing source/manifest, output, output name, pieces, maximum-piece-size fields, Split, Reassemble and Verify controls. The existing Python splitter remains a worker and no completion popup is used by the embedded path.
- Rework HDHomeRun ATSC scanning around explicit physical RF channels 2 through 51 and correct 6 MHz center frequencies. Each RF is tuned, status/lock is probed before `/streaminfo`, and scan evidence records RF attempts, locked multiplexes, raw service rows, parsed services, rejected rows, and unique imported channels.
- Scale HDHomeRun RF traversal to the first 65 percent of the full operation instead of treating RF completion as 100 percent. Parsing/resolution and Nougat import/finalization report later phases; unavailable provider guide data is reported truthfully rather than fabricated.
- Preserve the accepted Search button sheet-hover path and assert it in the v53 static lane.
- Add practical Linux emulator discovery/launch mappings for Dolphin, DuckStation, PCSX2, PPSSPP, RPCS3, Cemu, MAME and installed Linux Switch backends without removing Mesen/MesenCE, RMG, Atari800, Stella, BlastEm, DOSBox Staging or Xenia Canary.
- Expand Games artwork matching with new emulator-system collections plus release/revision/dump-noise normalization while retaining ZIP-contained ROM-name matching and the prepared persistent artwork cache.
- Add LAN Viewer v1 backend contracts for private-LAN discovery, read-only catalog enumeration, Unknown / Verified Clean / Blocked trust, and direct streaming only when the selected peer reports Verified Clean media. No WAN relay or automatic port forwarding is introduced.
- Add Child Safe Controls password-state/config protection with a salted iterative local digest and mode-0600 configuration. No third-party child-content repository is silently selected.
- Add runtime component inventory for advisory/CVE mapping, using OSV as the configured public advisory source without starting a resident updater.
- Add NOAA/NWS public-safety alert v1 support with user-configured two-letter area, active-event severity, sent/expiry times, source links, refresh deduplication and a protected persistent local history. Official child-abduction/civil-warning event types are preserved when present in the NWS feed; no unverified social/radio source is converted into an alert.
- Repair process identity so Linux process tools see `NougatMediaSuite` rather than a generic helper identity.
- Preserve the already-implemented rounded/translucent transient-overlay path and reject removal of its shape/opacity functions in the static lane.
- Repair the launcher icon identity and generate v53 icon variants from the approved v51 icon pixels by clearing only alpha outside the mathematical rounded badge. Regenerate embedded 16/32/64 `_NET_WM_ICON` arrays from the same cleaned source and install the same identity into the user icon theme.
- Build tooling uses `$HOME/DKLab/Archive/` singular for the pre-apply rollback snapshot, promotes only `Nougat_Media_Suite_v53` into the project root after a successful version smoke test, archives/removes the old v52 root executable only after v53 succeeds, and performs no commit/tag/push.

## Truthful candidate limitations

- World TV still requires owner-machine/current-network validation of every exposed broadcaster before acceptance. The worker rejects sources that fail verification, but this packaging environment cannot certify current broadcaster availability.
- HDHomeRun RF/service behavior requires owner-machine validation against the FLEX DUO and the previously observed ~66-channel result. The candidate adds the evidence needed to distinguish reception loss from parser/import loss rather than asserting a channel count in advance.
- The current HDHomeRun provider exposes channel lineup and RF/service data but not a separate guide feed. v53 reports that guide phase as unavailable instead of inventing guide success.
- No dedicated national AMBER-provider endpoint is claimed without an owner-approved legitimate source. NOAA/NWS official CAP events are accepted when that source publishes them.
- App-wide controller navigation and Aerial Production remain roadmap work unless separately approved.
