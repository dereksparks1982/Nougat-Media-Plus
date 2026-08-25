# Nougat Media Suite v0.0.41 Repair v10 Validation

Status: **candidate package validation complete; owner visual/functional acceptance still required**.

Repair v10 stays on v0.0.41 and is limited to the final owner-reported stabilization defects after repair v8/v9. It does not begin Games work, alter Home card geometry, redesign Search, or change the accepted v0.0.41 feature set.

## Repair v10 scope

- Video Player three-second media description: move the identity label into a real X11 child overlay above the libVLC video drawable. Mouse motion resets the existing 3000 ms activity timer without repainting the description every ~16 ms. This targets the reported behavior where the description appeared for the correct three seconds but flashed during those three seconds.
- Search -> Network narrow-window layout: shorten the peer/address field, anchor the action strip immediately after it, give the action strip its own clipped viewport, and make `Add Peer / Remove / START NODE / Search peers` mouse-wheel scroll horizontally when needed.
- Search top sub-tab row: count all six controls (`Search / Crawler / P2P / Virus Scan / Network / Archive`) in horizontal mouse-wheel overflow calculations so Archive can be reached in narrow/half-screen layouts.
- Live TV channel artwork: replace the blank placeholder BMPs for Charge! and Grit with the owner-supplied artwork, add the owner-supplied TCN artwork, and map the exact local channels 36.3 -> Charge!, 36.4 -> TCN, and 46.2 -> Grit.
- Preserve all repair-v8/v9 behavior: Library-only multi-row grid/hover, exact IMDb links, Live TV eight-button inner scrolling, full Video Player stitched tab shell, Discover footer clearance, Search Network footer cleanup, Archive Tor-before-Open-Site behavior, safe maintenance shutdown documentation/installer policy, and global top-tab behavior.

## Validation completed in the build environment

PASS:

- v0.0.41 source contract and retained v0.0.40/v35-v39 behavior.
- Search Archive Tor action, Library grid/hover, exact IMDb, Live TV inner scrolling, Discover footer, Search Network layout, roadmap ordering, and safe-shutdown documentation gate.
- New narrow Network overflow contract: six Search sub-tabs, four Network action buttons, clipped action viewport, shortened input field, and narrow-width runtime geometry self-test.
- New player-description contract: child overlay window is raised above libVLC, the old 16 ms repaint loop is absent, the authoritative 3000 ms timeout remains, and parent-video LeaveNotify ignores child-window transitions.
- Owner-supplied Charge!, TCN, and Grit BMP assets are present, nonblank, hash-verified, and tied to the exact local channel IDs.
- PolyForm Noncommercial / owner-rights / third-party boundary / contributor inbound license gate.
- Clean CMake configure + `-Wall -Wextra -Werror` compile/link in the repository's P2P + AI stub validation lane.
- Runtime self-tests retained from v0.0.35, v0.0.36, v0.0.37, v0.0.38, v0.0.39, and v0.0.41.
- Root executable identity in the validation lane: `Nougat Media Suite v0.0.41`.

The build environment does not provide `libtorrent-rasterbar`, so the full non-stub native lane cannot be proven here. The repair-v10 installer therefore requires a full native warnings-as-errors build and retained runtime suite on the owner's Ubuntu machine **before** it promotes any active project files.

## Owner visual/functional acceptance still required

After install, verify:

1. Move the pointer over playing video: the media description appears steadily, does not flash, remains for the same three-second activity window, then disappears.
2. Search -> Network at half-screen/narrow width: the peer input is shorter, Add Peer begins closer to the left, `Search peers` is not permanently clipped, and mouse wheel over the second row reaches all four actions.
3. Search top sub-tab row at half-screen/narrow width: mouse wheel reaches all six controls through Archive.
4. Live TV Guide: 36.3 Charge!, 36.4 TCN, and 46.2 Grit display the three real owner-supplied pictures rather than blank boxes/text fallbacks.
5. Previously repaired Library, Home, Discover, Archive, Live TV, player controls, IMDb, and global tabs remain unchanged outside this scope.

Only the owner decides whether v0.0.41 is accepted and ready for commit/tag/push/release.
