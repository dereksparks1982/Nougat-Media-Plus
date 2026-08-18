# ReddMedia v0.0.16 Native Library and Discover AI Validation

## Automated gates

- Exact committed v0.0.15 base and clean tracked tree.
- Existing untracked Jellyfin runtime is allowed; unrelated untracked files are rejected.
- Disposable Git regression covering simultaneous staged/unstaged tracked changes, an untracked patch manifest, exact addition cleanup, and preservation of the accepted untracked Jellyfin runtime.
- Exact hashes gate automatic recovery of the rejected original candidate's known leftovers; any nonmatching dirty state is rejected without restoration.
- Complete payload SHA-256/byte manifest verification before application.
- Pinned llama.cpp archive integrity and exact Nomic GGUF SHA-256 verification.
- CPU-only llama.cpp shared-runtime build from the bundled source archive, restricted to the required `llama` library target.
- Stub and full native builds with `-Wall -Wextra -Werror`.
- Full-model offline embedding inference through the compiled ReddMedia binary.
- Real SQLite database creation with Movie and TV history rows.
- All eight Discover paths: Usual/Random x Local/External x Movie/TV.
- Random Local and Random External validation with an empty viewing history.
- TMDb mock success, owned-title removal, owner-only token permissions, and explicit failure behavior.
- Source and media-type separation; returned titles must come from the supplied local/TMDb fixture.
- Typed multi-folder link path and non-destructive unlink path.
- Movies root -> box set -> movie hierarchy.
- TV root -> series -> season -> episode hierarchy.
- Hidden Jellyfin runtime starts with `--nowebclient`; the web player remains unavailable.
- Compiled lifecycle regression proves graceful close stops the owned server.
- Forced-parent-death regression proves the owned server exits and port 8096 closes even when ReddMedia is killed.
- Local Library playback still calls the existing embedded `open_media` path.
- `ReddMedia_v16`, `v0.0.16`, and red-tree drawing markers are present.

## Owner validation

1. Open Library. Confirm only Movies and TV are offered as library types.
2. With no folder linked, click Movies and confirm ReddMedia asks for a folder. Repeat for TV.
3. Link two folders to one media type, refresh, and confirm real titles from both appear.
4. Unlink one folder and verify its files remain unchanged on disk.
5. Confirm the TV root contains shows, not individual episodes. Open a show, season, and episode.
6. Confirm a real metadata box set opens to its films and ungrouped films remain individual titles.
7. Play a movie and an episode. Both must open inside ReddMedia's existing player.
8. Open Discover. Confirm Usual displays `DISCOVER USUAL`; Random displays `DISCOVER RANDOM`.
9. Confirm both modes contain exactly Local Movie, Local TV, External Movie, and External TV.
10. Request each option and confirm only one real recommendation is shown.
11. Confirm first External use asks for the TMDb read-access token.
12. Confirm Random can operate before any history exists and its explanation states history was not used.
13. Close ReddMedia and confirm its Jellyfin process disappears and port 8096 is released.
14. Confirm the executable/launcher is `ReddMedia_v16` and the red tree remains beside `v0.0.16`.

## Rollback contract

On installer failure, the complete tracked index and worktree, including `ReddMedia_v15`, are restored atomically from commit
`d67cf6e5e0e3ce3036adae5d9695147a7aa771e8`; v0.0.16 additions and the AI runtime are removed.
The accepted Jellyfin runtime and archived v0.0.15 snapshot are not deleted.
