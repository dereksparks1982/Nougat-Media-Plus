# Nougat Media Suite v0.0.55 First-Party LAN Web Player Handshake

Status: owner-approved same-version repair candidate. The accepted v0.0.54 release remains the rollback base until owner acceptance.

## Approved scope

- Advance/repair candidate identity at v0.0.55 only.
- Make the first-party Nougat Web Player available from phones/tablets/laptops on the same private LAN without requiring the Nougat desktop window to remain open.
- Make port **8096** the user-facing Nougat Web Player address.
- Keep Jellyfin hidden with `--nowebclient`, move its internal backend to **127.0.0.1:8098**, and use it only as Nougat's catalog/API engine.
- Supervise the background Web Player helper with the Nougat-owned persistent media-server stack. Start Server starts/adopts the stack; Stop Server stops the owned stack.
- Migrate an already-running Nougat-owned rejected-v55 Jellyfin process off port 8096 safely. Never claim or kill separately started Jellyfin.
- Bind the Web Player only to loopback/private IPv4 addresses discovered on the host. Do not open router ports, use UPnP, require a cloud login, or add an external relay.
- Keep catalog selection ID-based, direct HTTP byte-range delivery, and FFmpeg fragmented-MP4 compatibility streaming.
- Clear the empty-player overlay when playback begins and clear transient status text after successful playback.
- Fit a 1366×768-class desktop display at 100% browser zoom via short-viewport responsive rules.

## Explicit exclusions

- No PS2/PCSX2 implementation.
- No Xbox/Xenia source, runtime, dependency, launcher, or embedding changes.
- No Games dependency cleanup or movement.
- No File Splitter repair.
- No stock Jellyfin Web exposure or Jellyfin setup wizard.
- No Internet-facing remote access, cloud account, automatic port forwarding, or WAN relay.
- No unrelated Library, Home, Live TV, Radio, Studio, Search, or icon redesign.

## Acceptance boundary

The candidate may be handed to the owner only after warning-as-error compilation, static/regression validation, background service lifecycle proof, migration proof from the rejected 8096 Jellyfin layout, direct HTTP proof on port 8096, hidden-backend loopback proof on 8098, production dependency validation, changed-files manifest validation, ZIP integrity validation, and confirmation that the accepted Xbox emulator-host source remains byte-identical.

Owner testing must still prove access from the owner's phone on the same LAN using `http://<Nougat-PC-IP>:8096`, with the Nougat desktop window closed, plus real-media playback and visual sizing at 100% zoom. v0.0.54 remains accepted until explicit owner acceptance.
