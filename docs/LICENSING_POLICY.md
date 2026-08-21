# Nougat Media Suite Licensing Policy

## 1. Licensing model

Nougat Media Suite uses a **license boundary** rather than pretending every file in the project has one owner or one license.

- **Original Materials controlled by Elderred Softworks LLC**: PolyForm Noncommercial License 1.0.0 for recipients.
- **Third-Party Materials**: retain their upstream licenses and notices.
- **Commercial licensing of Original Materials**: reserved to Elderred Softworks LLC unless separately granted in writing.

The controlling recipient license for Original Materials is the PolyForm Noncommercial License 1.0.0 linked from the root `LICENSE` file.

## 2. What is Original Material

Original Material includes project-specific code, tests, documentation, scripts, UI/branding assets, and historical ReddMedia-lineage material that Elderred Softworks LLC owns or has authority to license. A file containing third-party material does not become owner-created merely because it is stored in this repository.

## 3. What is Third-Party Material

Third-Party Material includes separately licensed libraries, server packages, executable tools, models, upstream source, APIs, service data, and other material whose rights come from another author or provider. Third-party terms take priority for that material.

The main inventory is maintained in `THIRD_PARTY_NOTICES.md` and `licenses/`.

## 4. No accidental relicensing

A project-level license notice never changes the license of Jellyfin, FFmpeg, VLC/libVLC, libtorrent-rasterbar, yt-dlp, llama.cpp, Nomic models, or any other third-party component. Their license notices must remain intact.

Likewise, adding third-party code to Nougat Media Suite does not automatically make that code PolyForm-licensed. Its original license remains attached to it.

## 5. Owner commercial rights

The noncommercial recipient license is a grant from the rights holder. It does not bind the rights holder against using its own work commercially. Elderred Softworks LLC retains the right to offer paid versions, commercial licenses, hosted offerings, dual licensing, or other commercial arrangements for its Original Materials.

## 6. Outside contributions

Outside contributions are accepted only under `CONTRIBUTING.md`. The inbound grant must allow Elderred Softworks LLC to continue maintaining, sublicensing, relicensing, and commercially licensing the combined project.

## 7. Release gate

Before a public release, the build process should verify:

1. the root `LICENSE` and `COPYRIGHT.md` identify the Original Materials and rights holder;
2. `THIRD_PARTY_NOTICES.md` identifies known external components;
3. required upstream license copies/notices remain present;
4. no third-party file is mislabeled as owner-created;
5. contributor terms remain unchanged unless explicitly approved by the owner.

This policy documents project intent and release practice. It does not replace the controlling license texts or third-party terms.
