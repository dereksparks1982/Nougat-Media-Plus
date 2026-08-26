# Nougat Plugin Standard v1

Status: v0.0.50 candidate architecture

## Core rule

Nougat Media Suite core consists of:

1. the video player,
2. the application shell required to host that player, and
3. the built-in plugin discovery, validation, install/remove, and hosting system.

Every user-facing feature beyond the player is an optional plugin. A player-only installation remains a complete, launchable Nougat installation.

## The plugin folder is the source of truth

Nougat MUST discover installed plugins by scanning the managed plugin directory at application startup.

Default per-user location:

`$XDG_DATA_HOME/nougat/plugins/`

or, when `XDG_DATA_HOME` is unset:

`~/.local/share/nougat/plugins/`

Each immediate child directory is one candidate plugin. A plugin is considered installed only when the directory exists and contains a valid `plugin.json` manifest that matches the directory ID.

Examples:

```text
~/.local/share/nougat/plugins/
├── workshop/
│   ├── plugin.json
│   ├── bin/
│   └── resources/
├── games/
│   ├── plugin.json
│   ├── bin/
│   └── resources/
└── live-tv/
    ├── plugin.json
    ├── bin/
    └── resources/
```

Removing `games/` and relaunching Nougat MUST make Games disappear from the application. Restoring the same valid directory and relaunching MUST make Games available again. The plugin manager MUST NOT require a hidden database entry in order to discover a plugin that is physically present.

Installer state may record what was selected for maintenance and upgrades, but it MUST NOT override physical plugin discovery.

## Package format

A distributable plugin uses the `.nougat-plugin` extension. Version 1 packages are ZIP-compatible archives containing exactly one top-level plugin directory.

Example:

`Nougat_Plugin_Workshop_v0.0.50.nougat-plugin`

```text
workshop/
├── plugin.json
├── bin/
│   └── nougat-workshop-plugin
├── resources/
│   └── nougat_split_archive.py
├── LICENSES/
└── checksums.sha256
```

A user MAY also install a plugin by copying or moving an already-unpacked valid plugin directory directly into Nougat's managed plugin folder. Nougat validates the directory on the next launch before loading it.

The normal user interfaces MAY provide:

- installer selection from downloaded plugin packages,
- `Install Plugin From File`,
- drag and drop of a `.nougat-plugin` package,
- remove/disable controls,
- automatic download from an approved release source.

All of those mechanisms ultimately create or remove the same standard plugin directory.

## Required manifest fields

`plugin.json` MUST contain at least:

```json
{
  "format": "NOUGAT_PLUGIN",
  "format_version": 1,
  "id": "workshop",
  "display_name": "Workshop",
  "version": "0.0.50",
  "nougat_plugin_api": 1,
  "description": "File-engineering and production workspace.",
  "top_level_tab": "Workshop",
  "required_for_application_start": false,
  "recommended_by_default": true,
  "runtime": {
    "kind": "x11-process",
    "entrypoint": "bin/nougat-workshop-plugin"
  },
  "dependencies": [],
  "features": ["split-reassemble"]
}
```

`id` MUST be a safe directory name and MUST exactly match the plugin directory name. Paths in the manifest MUST be relative paths contained by the plugin directory. Absolute paths and `..` traversal are invalid.

## Runtime model

Nougat Plugin API v1 uses an out-of-process plugin model for user-interface plugins.

The plugin executable is launched by the built-in Nougat plugin host and its X11 window is embedded inside the Nougat shell. This keeps plugin code and dependencies physically outside the player core and avoids requiring third-party plugins to share Nougat's C++ ABI.

The host supplies plugin runtime context through environment variables and/or command-line arguments. Version 1 reserves these names:

- `NOUGAT_PLUGIN_API=1`
- `NOUGAT_PLUGIN_ID=<id>`
- `NOUGAT_PLUGIN_ROOT=<absolute plugin directory>`
- `NOUGAT_PLUGIN_DATA_DIR=<persistent plugin data>`
- `NOUGAT_PLUGIN_CONFIG_DIR=<persistent plugin configuration>`
- `NOUGAT_PLUGIN_CACHE_DIR=<discardable plugin cache>`
- `NOUGAT_PLUGIN_STATE_DIR=<plugin logs/state>`
- `NOUGAT_PLUGIN_PARENT_XID=<Nougat host window id when supplied>`

A plugin MUST resolve its own bundled resources relative to `NOUGAT_PLUGIN_ROOT`, not relative to `~/Downloads`, the source tree, the current working directory, or the Nougat executable.

The process model is intentionally language-neutral. A conforming plugin may be implemented in C++, Rust, Basic Sharp, Python with an appropriate native UI runtime, or another language, provided the packaged runtime satisfies the contract.

## Lifecycle

At startup Nougat MUST:

1. scan the plugin directory,
2. reject unsafe or malformed plugin directories,
3. validate manifest compatibility,
4. validate the declared runtime entrypoint and package integrity information when present,
5. register only valid plugins with the shell,
6. create tabs/menus/capabilities only for plugins that successfully registered.

When a plugin is absent, its tab and feature entry points MUST be absent. Nougat MUST NOT display a dead placeholder page for an uninstalled plugin.

A plugin failure MUST NOT prevent the video player core from starting. A broken plugin is skipped and reported by the plugin manager.

Nougat MAY require a restart after installing or removing a plugin. Hot loading is optional and must never be required for conformance.

## Removal and user data

Removing a plugin directory removes the installed plugin program/resources. Persistent user-created data is separate and is not automatically deleted.

Plugin program files:

`~/.local/share/nougat/plugins/<id>/`

Persistent plugin data:

`~/.local/share/nougat/plugin-data/<id>/`

Plugin configuration:

`~/.config/nougat/plugins/<id>/`

Plugin cache:

`~/.cache/nougat/plugins/<id>/`

Plugin state/logs:

`~/.local/state/nougat/plugins/<id>/`

This separation allows users to remove and reinstall a plugin without losing projects, saves, libraries, or settings unless they explicitly request data removal.

## Dependencies

Plugins declare dependencies in their manifest. Dependencies may name other Nougat plugins, managed runtime components, or system capabilities.

The player core MUST NOT acquire a dependency merely because one optional plugin needs it.

For example, installing a future P2P plugin may install libtorrent-related runtime support. A player-only installation does not.

## Distribution

A Nougat GitHub Release may publish the player/core and every plugin as independent assets on the same release page.

Example:

```text
Nougat_Media_Suite_v0.0.50_PLAYER_CORE.zip
Nougat_Plugin_Workshop_v0.0.50.nougat-plugin
Nougat_Plugin_Games_v0.0.50.nougat-plugin
Nougat_Plugin_Library_v0.0.50.nougat-plugin
Nougat_Plugin_LiveTV_v0.0.50.nougat-plugin
...
```

A recommended/default installer may also be provided, but it is a convenience front end over the same independent core and plugin packages.

## Open implementation rule

The plugin specification is intended to be public. A third party must be able to create, inspect, install, remove, and distribute a conforming plugin without authorization from Elderred Softworks.

Nougat may cryptographically verify package integrity and publisher identity, but the format itself must not depend on a private Elderred service in order to be readable or implementable.

## v0.0.50 migration rule

Workshop is the reference Plugin Standard v1 implementation. It must become physically independent of the player core before it is handed off as a separate plugin download.

After Workshop proves the complete lifecycle, existing Nougat features are migrated one at a time. A feature is not called a completed plugin merely because a manifest hides or reveals code that is still compiled into the player binary.
