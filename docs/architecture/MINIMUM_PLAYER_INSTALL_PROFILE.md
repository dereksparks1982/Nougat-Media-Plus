# Nougat Minimum Player Installation Profile

Status: architecture law established during the v0.0.50 component/runtime separation work.

## Rule

The minimum Nougat Media Suite installation is the **Nougat Video Player** and only the dependencies that the player itself requires to launch and play supported media.

A minimum installation must not silently imply or install Library/Discover services, Live TV, World TV, Games/emulators, P2P, Workshop, Jellyfin/Media Server, Security Analysis, Local AI, professional production tools, collaboration services, office/productivity tools, authoring systems, managed models, or unrelated runtimes.

The component manifest names this irreducible component `core-player`, and the `minimal` installation profile must contain exactly `core-player`.

## Default is not the same as minimum

Nougat may offer a **Recommended** installation that preselects a broader set of useful optional capabilities. That does not make those capabilities part of the core. Users must be able to choose the minimum Player profile without installing them.

## Current implementation note

v0.0.50 establishes the installation/component boundary while the existing C++ application is still substantially monolithic. The long-term modularization work should progressively move non-player capabilities behind independently installable feature modules without regressing accepted behavior.

Until that physical split is complete, documentation and installers must not claim that optional feature code has already been completely removed from the executable merely because its external runtime is optional.

## Long-term player independence

While libVLC remains the accepted playback backend, the minimum player may require libVLC or equivalent playback dependencies. The roadmap remains to replace that implementation dependency incrementally with Nougat-owned playback systems after they meet or exceed accepted behavior.

The invariant is the product boundary, not the temporary backend:

> Minimum Nougat = video player.
