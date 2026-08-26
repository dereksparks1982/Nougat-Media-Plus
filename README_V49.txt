Nougat Media Suite v0.0.49 Games / Atari / Cache / Scroll candidate
====================================================================

BASELINE
  Accepted GitHub v0.0.48 commit:
  f65c320c68cf5451f1151c59fbb2bccc4f5c434e

OWNER-APPROVED SCOPE
  * Add a managed Atari 2600 emulator instead of telling the user to install one.
  * Atari games must use Nougat's embedded Video Player host, following the successful NES/SNES Mesen presentation.
  * Repair Atari box-art matching.
  * Filter same-game variants: USA wins; without USA prefer another English release; only then use a foreign-only release; newest final revision wins within the chosen region tier.
  * Remove Games wheel 'coasting' caused by queued repaint-heavy wheel events.
  * Make scrollbar-thumb dragging track the pointer without stale redraw backlog.
  * Prepare the entire Games artwork library in the background.
  * Keep downloaded/prepared artwork across Nougat builds instead of performing network/ffmpeg work when cards scroll into view.

INSTALL / BUILD
  Extract this changed-files ZIP into the project root, then run:

    cd "$HOME/DKLab/Projects/Nougat Media Suite"
    python3 tools/build_v49.py

  The build script first verifies and applies the repair against the exact accepted
  v0.0.48 source blobs, installs/verifies game runtimes, runs the v49 static contract,
  builds warnings-as-errors, runs native self-tests, and only then promotes the root
  Nougat_Media_Suite_v49 executable.

GIT
  No commit, tag, or GitHub push is performed by this candidate.
