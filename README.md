# ReddMedia

**Version:** v0.0.1  
**Build:** VLC-Style Base Video Player  
**Status:** first corrected executable package candidate

ReddMedia is being restarted as a standalone desktop media app. This v0.0.1 package is only the base video player foundation.

## How to run

1. Unzip the package.
2. Open the `ReddMedia` folder.
3. Double-click the executable named `ReddMedia`.

The executable is included at the top level of the folder:

```text
ReddMedia/
└── ReddMedia
```

No installer command is required for this package.

## What this build does

- Provides a real top-level executable named `ReddMedia`.
- Opens as a standalone Linux desktop window.
- Uses a VLC-style neutral player layout, not the old 1970s terminal theme.
- Uses VLC/libVLC at runtime for playback.
- Opens local media files.
- Provides play/pause, stop, seek, time display, fullscreen, and volume controls.
- Double-clicking the video area toggles fullscreen.
- Escape exits fullscreen.
- The mouse pointer hides after 3 seconds when idle over the video area.
- Moving the mouse shows the pointer again.
- The close button shuts down playback and exits the process.
- Saves the last watched file and playback time locally.
- On the next launch, offers Resume or Load Different File when the last file still exists.

## What this build does not include

- P2P.
- YT-DLP.
- Media library.
- Music library.
- Photos.
- Playlists.
- Module loader.
- LAN/server mode.
- Web browser app.
- Fake media.
- GitHub setup.

## Runtime requirement

ReddMedia uses the system VLC engine. If VLC/libVLC is missing, the app opens and displays a clear message instead of silently failing.

On Ubuntu, installing VLC normally provides the needed runtime pieces.

## Local saved session

ReddMedia stores resume information locally at:

```text
~/.config/reddmedia/session.json
```

It stores the last file path and playback position only. There is no account, cloud sync, or tracking.
