# Nougat Minimum Player Installation Profile

Status: architecture law established during the v0.0.50 component/runtime separation work.

## Rule

The minimum Nougat Media Suite installation is the **Nougat Video Player** and only the dependencies that the player itself requires to launch and play supported media.

A minimum installation must not silently imply or install Library/Discover services, Live TV, World TV, Games/emulators, P2P, Workshop, Jellyfin/Media Server, Security Analysis, Local AI, professional production tools, collaboration services, office/productivity tools, authoring systems, managed models, or unrelated runtimes.

The component manifest names this irreducible component `core-player`, and the `minimal` installation profile must contain exactly `core-player`.

## Minimum workspace behavior

A minimum installation should **look and behave like a video player**, not like a disabled copy of the complete suite.

- Launch into the Video Player workspace.
- Do not show top-level feature tabs for optional capability families that are not installed.
- Do not fill the interface with dead buttons or placeholder pages for absent modules.
- Keep only the small shell infrastructure required to operate and maintain the player, such as Settings, Updates, diagnostics needed by the player, and an Add Features / Components entry point.
- Installing an optional capability may add its workspace/tab without reinstalling the player.
- Removing an optional capability removes its workspace from the active UI while preserving user-created work according to that component's removal policy.

The maintenance/component plumbing is infrastructure, not an excuse to redefine the minimum product as a larger suite.

## Default is not the same as minimum

Nougat may offer a **Recommended** installation that preselects a broader set of useful optional capabilities. That does not make those capabilities part of the core. Users must be able to choose the minimum Player profile without installing them.

## Current implementation note

v0.0.50 establishes the installation/component boundary while the existing C++ application is still substantially monolithic. The long-term modularization work should progressively move non-player capabilities behind independently installable feature modules without regressing accepted behavior.

Until that physical split is complete, documentation and installers must not claim that optional feature code has already been completely removed from the executable merely because its external runtime is optional.

Likewise, the current monolithic UI may still expose more workspaces during this transition. That is implementation debt to remove as physical feature modules mature, not the desired minimum-install behavior.

## Long-term player independence

While libVLC remains the accepted playback backend, the minimum player may require libVLC or equivalent playback dependencies. The roadmap remains to replace that implementation dependency incrementally with Nougat-owned playback systems after they meet or exceed accepted behavior.

The invariant is the product boundary, not the temporary backend:

> Minimum Nougat = video player.
