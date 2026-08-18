# ReddMedia v0.0.15 Native Library and Hidden Server Repair

## Reason for the repair

The stable Jellyfin foundation installed and built correctly, but exposing Jellyfin's setup wizard and browser player violated the approved product behavior. ReddMedia must remain the only interface and player. That foundation was therefore rejected without advancing the version.

## Implemented behavior

- Jellyfin Server 10.11.11 launches as hidden catalog machinery with `--nowebclient`.
- ReddMedia completes the first-run configuration over the local API and disables remote access.
- The local access token is stored with owner-only permissions under ReddMedia's configuration tree.
- The native ReddMedia top bar includes a Library tab.
- The Library adds media folders, starts scans, lists cataloged videos, supports selection and scrolling, and reports background work.
- Play Selected retrieves the cataloged local path and calls the existing native `open_media(path)` function.
- Local Library playback therefore uses the same embedded libVLC surface and controls as Open File. It does not invoke the Jellyfin web player, an external player, or Jellyfin transcoding.

## Installer proof

The installer accepts either the untouched accepted v0.0.14 tree or the exact rejected v0.0.15 foundation currently on the owner machine. It refuses unknown edits, stops only the exact bundled catalog executable, rebuilds both ReddMedia configurations with warnings as errors, launches a temporary hidden catalog, asserts the web client is unavailable, catalogs a generated video through the compiled native client, and verifies that the returned item contains its direct local path.

Any failure rolls the project back to accepted v0.0.14. Success remains a candidate until the owner confirms the real native Library and embedded playback behavior.
