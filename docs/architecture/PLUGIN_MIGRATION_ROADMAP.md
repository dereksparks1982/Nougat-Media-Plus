# Nougat Media Suite Plugin Migration Roadmap

## Core law

Nougat Media Suite core is the video player.

If the user selects no optional features during installation, Nougat installs only the video player and the minimum runtime/dependencies required for that player to launch and play supported media.

Everything else is an optional installable plugin/component. If a plugin is not installed, its tab, background services, runtime payloads, resources and storage footprint must not be present as active application features.

## Installer modes

### Default
Installs the video player core plus the recommended Nougat plugin set.

### Custom
Installs the video player core plus user-selected feature groups.

### Advanced Custom
Installs the video player core plus individually selected plugins/components and runtimes.

The video player core is always installed in all three modes.

## Plugin contract

Every Nougat plugin must own its feature code boundary, resources, optional runtime requirements, installer metadata, enable/disable state and uninstall behavior.

A plugin must not discover its resources by guessing relative to a development checkout or arbitrary executable location. The plugin manager/installation layout is authoritative.

When a plugin is absent:
- its top-level tab or entry point is hidden;
- its background processes do not start;
- its runtime dependencies are not required for Nougat core startup;
- its resources are not required for core startup;
- the player continues to launch and play media.

When a plugin is installed:
- its resources are installed under the managed Nougat application/plugin layout;
- its user data remains in Nougat user-data locations;
- its external runtimes, when needed, live in managed user-space runtime locations rather than the source tree;
- removing the plugin does not remove personal media or unrelated plugin data.

## Migration order

1. **Workshop**
   - First reference plugin.
   - Split/Reassemble is the first plugin feature.
   - Establishes installed resource lookup, tab visibility, plugin manifest, install/remove lifecycle and regression tests.

2. **Library / Discover**
   - Extract library/discovery UI and metadata services from mandatory core behavior.

3. **Live TV / World TV**
   - Tuner, guide and international TV capabilities become optional media plugins.

4. **Games**
   - Emulator host and emulator runtimes become a plugin family with managed per-emulator components.

5. **Search / Secure Search / Crawler**
   - Search and crawler capabilities become optional network/privacy plugins.

6. **P2P**
   - Torrent/P2P engines and services become fully optional and absent unless installed.

7. **Media Server**
   - Jellyfin integration and server-management services become optional.

8. **Security**
   - Security analysis, YARA and scanning runtime become optional.

9. **Local AI / Recommendations**
   - Local models and AI runtimes remain optional managed components.

10. **Future Workshop families**
    - Film/TV production, VFX/CGI, audio post, motion capture, collaboration, world/server authoring, office/productivity and other future capabilities follow the same plugin contract.

## One-plugin-at-a-time rule

Do not migrate all features at once. Each plugin is migrated, installed, validated and removed successfully before the next plugin becomes the migration target.

Required acceptance loop for each plugin:

`install -> launch -> feature test -> restart -> update/repair test -> remove -> launch core again`

The next plugin does not become the active migration target until the current plugin passes that loop.

## v0.0.50 active target: Workshop

v0.0.50 establishes the installer/plugin foundation and Workshop as Plugin #1.

Workshop requirements for v0.0.50:
- the player remains the only mandatory application feature;
- Workshop has an explicit plugin manifest/identity;
- Workshop resources install to a stable managed plugin resource location;
- Split/Reassemble resolves its worker from that installed plugin resource location;
- Workshop tab is visible only when Workshop is installed/enabled;
- uninstalling Workshop removes its application resources without touching user media;
- a minimum/player-only install launches without Workshop files present;
- Default, Custom and Advanced Custom installer modes all preserve the always-installed player core;
- installer/package validation proves there are no development-checkout or `~/Downloads` resource assumptions.

Only after Workshop passes this contract should v0.0.50 be considered ready for owner acceptance.
