# ReddMedia v0.0.8 Direct yt-dlp Last Chance Validation

Validation performed in build sandbox:

- `ReddMedia_v8 --version` prints `ReddMedia v0.0.8`.
- Bundled yt-dlp binary exists at `tools/yt-dlp/yt-dlp`.
- Bundled yt-dlp binary prints version `2026.07.04`.
- C++ source compiled with `-Wall -Wextra -Werror`.
- Package includes real top-level executable `ReddMedia_v8`, not a symlink and not a fake desktop-file wrapper.
- Package includes `ReddMedia_v8.desktop` as an additional launcher helper.
- Package does not include a mod system or yt-dlp mod package.

Manual validation required on Derek's machine:

- Double-click `ReddMedia_v8` launches the app.
- `ReddMedia_v8` shows red triangle metadata in Files if GNOME honors custom-icon metadata.
- `ReddMedia_v8.desktop` launcher shows red triangle and launches after trust metadata is applied.
- URL field accepts click focus, typing, Ctrl+V paste, and right-click paste.
- Download runs bundled yt-dlp and writes visible log output.
- App closes cleanly.
