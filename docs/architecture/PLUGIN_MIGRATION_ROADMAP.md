# Nougat Media Suite Plugin Migration Roadmap

## Core law

Nougat Media Suite core is:

1. the video player,
2. the minimal application shell required to run that player, and
3. the built-in Nougat plugin discovery/validation/hosting/install-remove system.

Nothing else is hardwired into the final player/core download.

If the user selects no optional features during installation, Nougat installs only that player/plugin core and the minimum runtime/dependencies required for playback and plugin management.

Everything else is an optional installable plugin. If a plugin is not installed, its tab, code payload, background services, runtime payloads, resources and active application footprint must not be present.

The normative plugin behavior and package format are defined by `NOUGAT_PLUGIN_STANDARD_V1.md`.

## Installer modes

### Default
Installs the player/plugin core plus the recommended Nougat plugin set.

### Custom
Installs the player/plugin core plus user-selected feature plugins.

### Advanced Custom
Installs the player/plugin core plus individually selected plugins/components and runtimes.

The player/plugin core is always installed in all three modes.

The installer may install selected plugins from downloaded `.nougat-plugin` packages, from a user-selected folder containing those packages, or from an approved download source. Those paths all produce the same standard plugin directories.

## Physical plugin-folder rule

The plugin directory is the runtime source of truth.

Default location:

`~/.local/share/nougat/plugins/`

Removing a valid plugin directory and relaunching Nougat removes that plugin from the application. Restoring the directory and relaunching restores it after validation. Installer bookkeeping must not override the physical scan.

## Plugin contract

Every Nougat plugin must own its feature code boundary, resources, optional runtime requirements, installer metadata, enable/disable state and uninstall behavior.

A plugin must not discover its resources by guessing relative to a development checkout, `~/Downloads`, the current working directory or the Nougat executable. Its own plugin directory and the built-in plugin host are authoritative.

When a plugin is absent:
- its top-level tab or entry point is hidden;
- its feature code is not part of the player/core package;
- its background processes do not start;
- its runtime dependencies are not required for core startup;
- its resources are not required for core startup;
- the player continues to launch and play media.

When a plugin is installed:
- its code and resources are installed under its managed plugin directory;
- its persistent user data remains in separate Nougat user-data locations;
- its external runtimes, when needed, live in managed runtime/data locations rather than the source tree;
- removing the plugin program does not remove personal media or unrelated user data.

## Download deliverables

The migration is complete only when Nougat can be handed off as independent downloads rather than a monolithic application disguised by plugin checkboxes.

Expected release assets include:

```text
Nougat_Media_Suite_v0.0.50_PLAYER_CORE.zip
Nougat_Plugin_Workshop_v0.0.50.nougat-plugin
Nougat_Plugin_Games_v0.0.50.nougat-plugin
Nougat_Plugin_Library_v0.0.50.nougat-plugin
Nougat_Plugin_Discover_v0.0.50.nougat-plugin
Nougat_Plugin_LiveTV_v0.0.50.nougat-plugin
Nougat_Plugin_WorldTV_v0.0.50.nougat-plugin
Nougat_Plugin_Search_v0.0.50.nougat-plugin
Nougat_Plugin_Stream_v0.0.50.nougat-plugin
Nougat_Plugin_P2P_v0.0.50.nougat-plugin
Nougat_Plugin_MediaServer_v0.0.50.nougat-plugin
Nougat_Plugin_Security_v0.0.50.nougat-plugin
Nougat_Plugin_LocalAI_v0.0.50.nougat-plugin
```

A recommended/default installer may also be published as a convenience, but it must consume the same core/plugin packages.

## Migration order

1. **Workshop**
   - First reference plugin.
   - Split/Reassemble is the first plugin feature.
   - Establishes plugin package format, physical code/resource separation, startup discovery, tab registration, install/remove lifecycle and regression tests.

2. **Games**
   - Emulator library/host UI becomes an independent plugin.
   - Emulator runtimes remain independently managed components owned by the Games plugin, not player core.

3. **Library**
   - Library UI/indexing/metadata behavior becomes an independent plugin.

4. **Discover**
   - Recommendation/discovery UI and services become an independent plugin.

5. **Live TV**
   - Tuner, guide and channel playback capability becomes an independent plugin.

6. **World TV**
   - International television capability becomes an independent plugin.

7. **Search / Secure Search / Crawler**
   - Search and crawler capabilities become optional network/privacy plugins.

8. **Stream**
   - URL/direct-watch streaming capability becomes an independent plugin.

9. **P2P**
   - Torrent/P2P engines and services become fully optional and absent unless installed.

10. **Media Server**
    - Jellyfin integration and server-management services become optional.

11. **Security**
    - Security analysis, YARA and scanning runtime become optional.

12. **Local AI / Recommendations**
    - Local models and AI runtimes become optional managed plugin/components.

13. **System/Diagnostics capabilities**
    - Keep only genuinely core diagnostics needed to diagnose the player/plugin loader in core. Feature-specific diagnostics move with their owning plugins.

14. **Future Workshop production families**
    - Film/TV production, editing, VFX/CGI, audio post, motion capture, collaboration, world/server authoring, office/productivity and other future capabilities follow the same standard.

## One-plugin-at-a-time rule

Do not fake the migration by shipping a small manifest that merely reveals feature code still compiled into the player binary.

Each plugin is physically separated, packaged, installed, validated and removed successfully before it is called complete.

Required acceptance loop for each plugin:

`package -> install -> launch -> feature test -> restart -> update/repair test -> remove folder -> relaunch core -> restore folder -> relaunch -> remove through manager -> relaunch core`

The next plugin does not become the active migration target until the current plugin passes that loop.

## v0.0.50 active target: Workshop

v0.0.50 establishes the built-in plugin foundation and Workshop as the reference Plugin Standard v1 implementation.

Workshop requirements for v0.0.50:
- player/plugin core is the only mandatory application download;
- Workshop implementation code is physically absent from the final player/core package;
- Workshop has a standard plugin manifest/identity;
- Workshop ships as its own `.nougat-plugin` download;
- Workshop resources resolve only from its plugin directory;
- Workshop tab is registered only when Workshop is physically present and valid;
- manually removing the Workshop directory and relaunching removes the Workshop tab;
- restoring the valid directory and relaunching restores the tab;
- uninstalling Workshop removes its application files without touching user media/projects;
- player-only install launches without Workshop files or Workshop dependencies present;
- Default, Custom and Advanced Custom installer modes preserve the always-installed player/plugin core;
- installer/package validation proves there are no source-checkout or `~/Downloads` runtime assumptions.

Only after Workshop passes this physical-separation contract should its separate download be handed to the owner as Plugin #1.
