Nougat Media Suite v0.0.49
Games Final Owner Repair + Russia-24 Audio Repair
Elderred Softworks LLC / DKLab

SCOPE
-----
This repair is for the rejected, uncommitted v0.0.49 owner-test state that already has the v49 Atari owner repair applied. It does not create a Git commit, tag, or GitHub push.

Games repairs:
1. Atari 2600 / Stella embedding
   - preserves the recursive private X11/XWayland emulator host from the owner repair
   - forces SDL3's SDL_VIDEO_DRIVER=x11 in addition to the legacy SDL_VIDEODRIVER=x11
   - refreshes the managed Stella wrapper so the SDL3 X11 force is present even when Stella was already installed
   - keeps the no-taskbar/no-pager private-window behavior and accepted NES/SNES presentation contract

2. Atari artwork
   - preserves persistent background artwork preparation and Libretro directory-index caches
   - splits CamelCase preservation names such as AlligatorPeople
   - adds conservative verified aliases for known Atari naming gaps including AdventuresOnGX12, Action Man, and Angling
   - keeps the verified 2 Pak Special front-cover fallback
   - does not return to viewport-triggered curl/ffmpeg work

3. Sega ZIP libraries
   - recognizes Sega Genesis / Mega Drive, Master System, and Game Gear ROMs inside ZIP archives
   - Sega .bin files are treated as Genesis only when the linked folder/archive context identifies Sega/Genesis/Mega Drive; the existing Atari .bin rule remains otherwise
   - the ZIP stays canonical; only the selected ROM is prepared in Nougat's private cache when needed
   - uses the managed official BlastEm Linux x86_64 nightly 0.6.3-pre-8013468ed981
   - BlastEm machine selection: gen / sms / gg

4. DOS ZIP packages
   - a DOS ZIP is indexed as one game package rather than hundreds of loose files
   - selects a safe DOS EXE/COM/BAT launcher, preferring NOU_LAUNCH and Original_DOS when present
   - extracts the whole required DOS package only at launch into ~/.cache/reddmedia/games/dos-extracted-v49
   - cache identity includes source path, size, mtime, and selected launcher so unchanged packages are reused across Nougat builds
   - rejects unsafe archive paths and extracted symlinks

5. Preserved Games behavior
   - USA > other English > foreign-only fallback
   - newest final revision preference
   - Games wheel-event coalescing and frame-limited scrollbar drag redraw
   - persistent prepared artwork cache
   - Mesen NES/SNES behavior remains the presentation baseline
   - Xbox/PlayStation multi-gigabyte archives are NOT auto-unpacked by this repair; disc images/game trees should remain extracted on disk

World TV repair:
6. Russia-24 audio
   - Russia-24 (channel id Russia24.ru) now requires a candidate to expose both video and an audio stream before Nougat accepts it
   - Russia-24 may inspect up to six current non-blocked direct-source candidates instead of stopping after three
   - Russia-1 and all other stations retain the existing default probe rules
   - this is intentionally a surgical Russia-24 repair, not a broad World TV rewrite

NOT IN THIS BUILD
-----------------
The LAN Web Player / same-network browser player is deferred to the next build by owner direction.

RUNTIME SOURCE NOTE
-------------------
BlastEm source/packaging details are recorded in components/games/emulators/BLASTEM_RUNTIME_SOURCE.md. The installer validates the official nightly size, runtime revision, expected package contents, shared-library resolution, and records the downloaded SHA-256 locally. The complete portable upstream package and license files are preserved.

OWNER ACCEPTANCE TARGETS
------------------------
Games:
- Atari 2600 opens inside the Nougat Video Player, not in a separate Stella desktop window.
- No separate Stella dock/task-switcher icon.
- Top-edge Stella options bridge still works.
- Previously missing Atari cards such as the owner-observed naming cases resolve artwork when the upstream catalog has a verified match.
- A linked Sega collection can remain ZIPped, indexes normally, and a selected Sega game launches.
- A DOS ZIP appears as one game and launches from Nougat's persistent private DOS cache.
- Wheel scrolling stops when input stops; scrollbar dragging follows the pointer.
- NES and SNES still use the already accepted embedded presentation.

World TV:
- Russia-24 has working audio with video.
- Russia-1 still has working audio.

No Git commit, tag, or push is performed by this repair or the v49 build script.
