# Nougat Media Suite v0.0.49 owner runtime tests

After the build prints `=== v0.0.49 NATIVE BUILD + EXECUTABLE PROMOTION PASS ===`, test these owner-facing behaviors.

1. **Atari 2600 launch**: double-click an Atari 2600 `.bin`/`.a26`. It must launch with the managed Stella runtime inside Nougat's Video Player, not report that no emulator exists and not fall back to a separate desktop window.
2. **Atari presentation**: gameplay should occupy the Nougat player cleanly. Move the pointer to the top edge and verify Stella Options opens; this bridge requires real SDL/X11 owner testing.
3. **Atari box art**: leave Games open long enough for background artwork preparation, or revisit after it completes. Atari cards should populate from the persistent cache rather than only when first scrolled into view.
4. **Duplicate preference**: where the same game has USA/Japan/Europe copies, USA should be the visible card. If USA is absent, an English-region copy should win. A Japanese-only title may remain because it is the only available copy. Newer final revisions should replace older revisions inside the same region tier.
5. **Wheel stop**: spin the mouse wheel rapidly in Games. The library should stop when input stops instead of replaying a backlog.
6. **Scrollbar drag**: grab the Games vertical scrollbar thumb and drag quickly. The thumb/list should track current pointer position rather than lagging several motions behind.
7. **Cache persistence**: after artwork has prepared, close/reopen v0.0.49. Cards should reuse `~/.cache/reddmedia/games/artwork-prepared-v49/` without refetching/reconverting the whole visible page.
8. **Regression**: NES and SNES Mesen embedded fullscreen/top-edge-option behavior must remain unchanged.

Do not commit or push this candidate until owner testing is accepted.
