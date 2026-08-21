# ReddMedia v0.0.17 Handshake

## State

Owner-approved single-build scope over technically working checkpoint commit `05eba123462aecb13119bcfcf7fa41bc18f1048e`. This candidate is not accepted or tagged.

## Included work

- Replace the unrelated-project-contaminated root Bible with one ReddMedia-only canonical `COMPANY_BIBLE.md`.
- Enforce Movie-only and TV-only results at request, candidate, and final selection boundaries, including Random mode.
- Accept either a TMDb API key or read access token; provide Test, Save/Replace, and Clear controls; persist valid credentials with owner-only permissions; preserve a working credential when a replacement fails validation; report 401 without exposing the credential.
- Load and cache real local Jellyfin posters and external TMDb posters.
- Show a full-width red loading indicator for Library, poster, Discover, TMDb, and server work.
- Provide Start Server, Stop Server, and Refresh Server controls.
- Stop and reap only the Jellyfin process owned by ReddMedia; preserve an independently started server.
- Exclude generated AI and Jellyfin runtime trees from Git.
- Apply the approved red-tree identity to the final raw `ReddMedia_v17` executable after its last write, verify metadata readback, and refresh Files.

## Exclusions

- No unrelated desktop, GNOME, launcher, visual redesign, media deletion, sample titles, remote AI service, runtime-tree commit, accepted tag, local Git commit, or GitHub push.
- The installer does not store a TMDb credential. Credential entry remains an owner action inside ReddMedia.

## Candidate package

`ReddMedia_v0_0_17_LIBRARY_DISCOVER_SERVER_RELIABILITY_AND_PROJECT_COMPANY_BIBLE_CHANGED_FILES_ONLY.zip`

## Acceptance boundary

Automated validation can prove source behavior, build integrity, rollback, credential permissions, type separation, poster paths, and server ownership. The owner must still run the candidate, visually confirm the red-tree icon on the raw executable in Files, exercise real media and real TMDb responses, and explicitly accept or reject the candidate.
