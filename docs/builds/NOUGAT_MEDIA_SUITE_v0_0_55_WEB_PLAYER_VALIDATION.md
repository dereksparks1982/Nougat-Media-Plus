# Nougat Media Suite v0.0.55 Web Player Validation Record

Status: repaired candidate for owner testing. v0.0.54 remains the accepted rollback base.

## Scope validated

- First-party Nougat LAN Web Player owns private-LAN port 8096.
- Hidden Jellyfin backend remains `--nowebclient` and is moved to loopback-only 127.0.0.1:8098.
- `components/web_player/nougat-web-service` is a separate background executable supervised with the Nougat-owned persistent Jellyfin stack.
- The desktop GUI no longer owns the Web Player listener, so normal UI close cannot stop the background browser endpoint.
- Existing rejected-v55 Nougat-owned Jellyfin on 8096 is migrated to the corrected stack.
- Browser UI assets remain Nougat-owned HTML/CSS/JavaScript.
- Catalog access uses catalog IDs; browser requests do not accept arbitrary filesystem paths.
- Direct media supports HTTP byte ranges.
- FFmpeg fragmented-MP4 compatibility streaming remains available.
- Empty-player text is forcibly hidden during active playback and transient successful-playback status clears.
- Short desktop viewports use an 820px player-shell cap, fitting a 1366×768-class display at 100% browser zoom.
- Five-button true-fullscreen desktop transport remains `<<`, `<`, `^`, `>`, `>>`.
- PS2 remains deferred and Games runtime files are unchanged.

## Validation completed

- Full-suite warning-as-error compile completed with `-Wall -Wextra -Werror`.
- Production executable links the real AI lane (`libllama.so.0`) and real P2P lane (`libtorrent-rasterbar.so.2.0`); no AI/P2P stub build is used for the handoff executable.
- Static/regression test `tools/test_nougat_web_player_v55.py`: PASS.
- Background Web Player helper runtime smoke: `/nougat/v1/health` and `/` served on port 8096 with `Server: Nougat/0.0.55`.
- Persistent-stack harness: Nougat-owned Jellyfin + Web Player started independently of the desktop GUI and Stop Server ownership logic removed both listeners.
- Migration harness: a Nougat-owned legacy/rejected Jellyfin process listening on 0.0.0.0:8096 was terminated by owner-token proof, `network.xml` was rewritten to 8098 + 127.0.0.1, and the Nougat Web Player replaced it on 8096.
- Hidden-backend configuration proof: `InternalHttpPort=8098`, `PublicHttpPort=8098`, `EnableRemoteAccess=false`, and `LocalNetworkAddresses=127.0.0.1`.
- Xbox/Xenia source regression SHA-256: `ccb778246a9677a13dd9c71133b817b8d9138264fe1d8a0cd885edda4f271bef` PASS.

## Build-environment note

The container's glibc is older than the owner-supplied `libllama.so.0` requirement (`GLIBC_2.43`). The production link therefore uses the same linker allowance as the prior v55 candidate for unresolved dependencies inside linked shared libraries. The executable records the real `libllama.so.0` dependency and no AI substitute. The owner workstation supplies the matching runtime.

For build-only P2P headers/linking, the existing temporary libtorrent 2.0 ABI build outside the project tree was used. No temporary libtorrent files are included in the handoff and no Nougat Games/Xenia runtime is modified.

## Owner tests still required

1. Apply/run the v55 executable from the Nougat project root.
2. Confirm the v55 raw executable still shows the same accepted Nougat N identity and v54 is removed only after validation.
3. Confirm existing Games behavior is preserved.
4. With Server running, close the Nougat desktop window and open `http://192.168.1.158:8096` from the phone on the same LAN.
5. Verify the Nougat Web Player, not Swagger/Jellyfin API, appears.
6. Verify real media playback and one compatibility-mode item.
7. Verify the placeholder text disappears during playback and the page fits at 100% zoom on the 1366×768 monitor.

No commit, tag, push, or acceptance snapshot is authorized until owner testing succeeds.
