# Nougat Media Suite v0.0.49 Games owner handshake

## Scope

v0.0.49 is the Games/Atari/performance candidate built on the accepted v0.0.48 GitHub baseline.

It adds:

- managed Stella 7.0 Atari 2600 runtime;
- Atari 2600 launch through Nougat's native embedded emulator host;
- clean Stella game presentation with top-edge Options gesture routed to Stella's integrated Tab menu;
- Atari/Libretro artwork matching repair;
- persistent prepared artwork cache that survives Nougat rebuilds;
- background whole-library artwork preparation instead of viewport-triggered curl/ffmpeg work;
- Games wheel-event coalescing and frame-limited scrollbar redraw;
- duplicate/version filtering with owner policy: USA first, then another English release, then foreign-only fallback, with newest final revision preferred inside the chosen region tier.

## Retained emulator presentation authority

The owner-tested NES and SNES Mesen behavior remains the visual/interaction authority: emulator gameplay belongs inside Nougat's Video Player, preserves proper aspect presentation, avoids a separate emulator desktop window, and normally keeps emulator chrome out of the picture while options remain reachable at the top edge.

Stella's top-edge bridge uses its documented in-game Tab Options action. Runtime acceptance still requires owner testing because SDL/X11 handling of synthetic key events is backend/runtime dependent.

## Artwork cache behavior

v0.0.49 keeps the existing v0.0.48 download cache under `~/.cache/reddmedia/games/artwork/` so already-downloaded art is not discarded. It adds `~/.cache/reddmedia/games/artwork-prepared-v49/` for preconverted render-ready BMP cards. New builds reuse those files. The UI draw path no longer starts network requests or ffmpeg conversion.

## Release gate

This package is a candidate. It performs no Git commit, tag, or GitHub push. Owner runtime testing and explicit acceptance remain required before release closeout.
