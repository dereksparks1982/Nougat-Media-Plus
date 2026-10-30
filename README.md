# Nougat Media Plus

**Nougat Media Plus** is a Linux-first media center and entertainment platform developed by **Elderred Softworks LLC**. It brings personal movies and TV, music and radio, live television, games, production tools, and network capabilities together in one project.

## Watch your movies and TV

**[Open Nougat Watch (simplified phone player)](https://dereksparks1982.github.io/Nougat-Media-Plus/watch/)**

**[Open the full Nougat Web Player](https://dereksparks1982.github.io/Nougat-Media-Plus/)**

The browser viewers are hosted on GitHub Pages. Your personal media files **are not uploaded to GitHub**. Playback, catalogs, and artwork come from your running Nougat media server through an HTTPS bridge, including the compatible bridge used for the Hanafi web viewer.

Nougat Watch is the straightforward way to watch from a phone or tablet: connect to your host, search movies or TV, tap a title, play it, and resume later. The full web player has a larger interface with additional modules.

**Connect once for both viewers:** In the full web player, choose **CONNECT** in the top bar to enter or test your HTTPS media server. Quick Watch uses the same saved server setting in that browser.

**To stream remotely:** keep your Nougat media host running and reachable through an HTTPS endpoint that allows connections from the Nougat Pages origin. The web page alone cannot serve files from a computer that is offline. If your host address changes, open **Change media server** in Nougat Watch and enter your own HTTPS bridge address.

## What Nougat includes

| Area | Purpose |
| --- | --- |
| **Media Library & Player** | Organize movies and television, view posters and metadata, resume playback, select audio/subtitle tracks, navigate chapters, and manage local and network media. |
| **Nougat Watch & Web Player** | Use the browser companion to explore and stream your library on phones, tablets, and computers. Compatible direct-play and server-transcoded streams are supported where available. |
| **Live TV & World TV** | Discover compatible broadcast tuners and streams, organize channels, and work with TV guides and international broadcasters. |
| **Music & Radio** | Play and explore audio sources, internet radio, compatible receivers, and radio/signal features. |
| **Games** | Organize games and supported emulators, artwork, controller settings, and game launching in the Nougat experience. User-owned game content and firmware are not included. |
| **Studio** | Media conversion, remuxing, audio extraction, export queues, editing foundations, file processing, and other production tools. |
| **Search, Streaming & P2P** | Explore media and online resources through supported search, stream and transfer facilities. |
| **Network & System** | Diagnose services, connected hardware, streaming connectivity, and system health. |

Nougat is an evolving project. Features vary by platform, compatible hardware, external services, and installed backends. **A feature in the source code or roadmap is not necessarily available in the current tested application.** Some advanced tools are native-only and cannot run in a browser.

## Linux and browser support

The primary application targets **Linux**. The web companions run in modern mobile and desktop browsers; compatibility depends on your device, the source media format, and the server's ability to stream or transcode that format.

The browser interface is a frontend, not a cloud-storage service. To browse and play your own media, the Nougat host must be online. Network access may require your own HTTPS bridge and appropriate security settings.

## Project and documentation

- [Roadmap](ROADMAP.md): planned features and current development priorities.
- [Changelog](CHANGELOG.md): releases and development history.
- [Dependencies](DEPENDENCIES.md): external and build requirements.
- [Contributing](CONTRIBUTING.md): how to help improve Nougat.
- [Company Bible](COMPANY_BIBLE.md): project architecture and development conventions.
- [Documentation](docs/): technical design notes, feature-specific documentation, and source audits.
- [Legal Notice](LEGAL_NOTICE.md), [License](LICENSE), [Copyright](COPYRIGHT.md), and [Third-Party Notices](THIRD_PARTY_NOTICES.md).

**Nougat Media Plus · Elderred Softworks LLC**
