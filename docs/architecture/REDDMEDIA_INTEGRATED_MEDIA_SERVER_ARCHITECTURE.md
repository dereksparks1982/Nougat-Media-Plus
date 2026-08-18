# ReddMedia Integrated Media Server Architecture

ReddMedia v0.0.15 turns the standalone player into the owner of an integrated media-service stack. The user launches ReddMedia; ReddMedia locates, starts, supervises, and privately configures the bundled Jellyfin service. Jellyfin remains a separately licensed process and supplies hidden catalog machinery. ReddMedia remains the only interface and player.

## Runtime ownership

- ReddMedia owns the user-facing desktop application and native libVLC player.
- The stable Jellyfin 10.11.11 server is extracted from the pinned Ubuntu 26.04 amd64 package and runs from `components/jellyfin/runtime/jellyfin/`.
- The matching Jellyfin web resources are extracted from the pinned Ubuntu 26.04 all-architecture package and preserved under `components/jellyfin/runtime/web/`, but are not served.
- Matching source archives and GPL licenses remain in the project; no system Jellyfin service is installed.
- ReddMedia communicates with the service on `127.0.0.1:8096` and uses Jellyfin's `/health` liveness endpoint.
- ReddMedia launches the service with `--nowebclient`, completes first-run setup through the local API, and disables remote access.
- The native Library registers folders, triggers scans, lists videos, and receives cataloged local paths through the API.
- Selecting a Library item calls the same native `open_media(path)` route as Open File. The local file goes directly to embedded libVLC; Jellyfin is not the playback or transcoding path.
- Persistent server data belongs under `~/.local/share/reddmedia/server/`, configuration under `~/.config/reddmedia/server/`, and cache under `~/.cache/reddmedia/server/`.
- Closing the player window does not terminate the media service. Library scanning, recording, and other server duties must be able to continue.

## Component policy

ReddMedia may combine proven open-source components when they provide working machinery. A component remains while useful. Integration must preserve its license, provenance, source, and clear process boundary.

Future TVHeadend integration belongs behind the same ReddMedia-owned product surface, with ReddMedia remaining the launcher, status authority, native player, and unified user experience.
