# Nougat Workshop Professional Production Roadmap

Status: long-term roadmap only. This document does not expand the implementation scope of v0.0.50 beyond the owner-approved Workshop splitter/reassembler and v0.0.50 architecture work.

## Product identity

**Workshop** is a top-level tab inside Nougat Media Suite. It is not a separate application and it is not branded "Studio" or "Gold Studio".

The long-term objective is a native production environment broad enough to support serious feature-film, television, animation, VFX, audio-post, game-world, custom-server, preservation, and file-engineering workflows without forcing a production to maintain a drawer full of disconnected utilities.

Workshop should combine specialist-grade tools through shared project, asset, provenance, collaboration, and processing layers. Individual tools may overlap capabilities found in other products; the differentiator is that the complete production chain lives in one coherent Nougat environment and the tools understand one another's data.

## Production reference and visual north star

Use the grounded live-action/VFX integration associated with high-end late-2000s feature work, especially the first *Transformers* film and the broader Michael Bay / ILM-style production discipline, as an important visual and workflow reference.

The goal is not to reproduce proprietary ILM software or copy another application's interface. The goal is to support the same classes of work with modern, replaceable components while preserving the visual restraint that made those shots feel photographed rather than synthetically polished.

Core rendering principle:

> More technically advanced is better only when the finished shot looks more physically convincing.

Workshop CGI/VFX defaults should therefore favor photographed reality over sterile perfection: physically grounded light, environment interaction, believable weight and inertia, imperfect surfaces, restrained specularity, contact shadows, atmospheric integration, real optical behavior, coherent motion blur, grain/noise matching, and compositing that inherits the photography instead of overpowering it.

## Professional picture editing and post-production

Roadmap Workshop toward a complete post-production pipeline after principal photography:

- Professional nonlinear editing with multiple video, audio, subtitle, data, and adjustment tracks.
- Source/program monitors, bins, smart bins, selects, markers, subclips, nested timelines, compound clips, versioned sequences, and non-destructive editing.
- Frame-accurate trim, ripple, roll, slip, slide, lift, extract, replace, overwrite, insert, snapping, multicam, synchronized cameras, and proxy/optimized-media workflows.
- Timecode-aware ingest and conform, camera/reel/scene/take metadata, edit decision interchange, and relinking/consolidation.
- Picture lock workflows that hand a stable edit to VFX, sound, scoring, color, finishing, and mastering while retaining controlled revision history.
- Background rendering, render cache, distributed rendering, render-farm coordination, and dependency-aware invalidation.
- Professional deliverables and mastering, including production codecs and interchange formats where technically and legally supportable.
- Dailies, review exports, annotation passes, side-by-side and A/B comparison, version comparison, and final QC.

## Color, finishing, and mastering

- Color-managed pipelines suitable for modern SDR/HDR production, with future ACES-oriented workflows where appropriate.
- Lift/gamma/gain, curves, hue/saturation tools, qualifiers, tracked masks/windows, shot matching, still/reference galleries, and node-graph grading.
- Waveform, RGB parade, vectorscope, histogram, false-color, clipping/gamut warnings, and other production scopes.
- LUT management, camera transforms, display transforms, output transforms, and reproducible project color settings.
- Grain/noise matching, texture preservation, highlight rolloff, black-level consistency, and lens/sensor character matching.
- Final conform, titles, captions, legal-range checks, loudness/QC coordination, mastering, and archive masters.

## CGI, compositing, and VFX

Workshop should eventually include a production-grade compositing and CGI environment rather than merely consumer effects.

### Chroma key / green and blue screen

- Screen-color sampling from points and regions rather than assuming a fixed green value.
- Chroma-space and luminance-aware matte extraction.
- Core matte and edge matte separation.
- Fractional alpha for hair, motion blur, translucent material, smoke, glass, and fine detail.
- Garbage mattes and holdout mattes with animation/tracking.
- Edge-aware matte refinement, morphological cleanup, guided/bilateral-style refinement, and subpixel edges.
- Advanced despill with skin, hair, reflective-object, and edge protection.
- Foreground edge-color reconstruction.
- Temporal matte stabilization with motion-compensated processing where appropriate.
- Light wrap, background color interaction, and compositing controls.
- No generative-image dependency is required for the keyer. The mathematical/optical keyer must stand on its own.

### Tracking and matchmove

- Point, planar, object, and mask tracking.
- Camera solve / matchmove from live-action plates.
- Lens distortion calibration and undistort/redistort workflows.
- Screen replacement and corner/perspective tracking.
- Set extensions and tracked 2D/3D insertions.
- Virtual camera tools and production metadata exchange.

### Rotoscope, paint, and cleanup

- Bezier/spline masks with feathering, motion blur, tracking, and per-point animation.
- Rotoscope layers, holdouts, mattes, garbage masks, and reusable mask groups.
- Wire, rig, blemish, marker, and unwanted-object cleanup.
- Frame paint, clone, patch, reference-frame propagation, and restoration operations.

### 2D/3D compositing

- Node-based compositing graph with explicit image/data flow.
- Multichannel image support, image sequences, EXR-oriented workflows, premultiplication controls, masks, mattes, and passes.
- Transform, perspective, merge, grade, blur, sharpen, distort, key, matte, depth, vector, and utility nodes.
- Deep/advanced compositing research lane as the architecture matures.
- 2.5D/3D compositing space for cards, cameras, lights, geometry, projections, and tracked scenes.

### Rendering and material realism

- Physically based materials with layerable paint, metal, clear coat, dielectric, translucent, subsurface, fabric, glass, dust, grime, scratches, oxidation, fingerprints, damage, and non-uniform roughness.
- HDR/environment-light capture and plate-derived lighting/reference workflows.
- Reflection and shadow catchers for live-action integration.
- Contact shadow and indirect-bounce discipline.
- Atmospheric haze, fog, volumetrics, dust, debris, smoke, fire, sparks, water, destruction, particles, and environmental interaction.
- Realistic depth of field, lens behavior, chromatic characteristics, rolling-shutter behavior where applicable, sensor/film grain, exposure, and motion blur.
- A future material-weathering system should make pristine CGI the exception rather than the default.

### Mass, scale, and physical interaction

Workshop should treat perceived mass as a production problem, not merely an animation slider:

- Center-of-mass visualization.
- Acceleration/deceleration and inertia tools.
- Secondary motion, flex, lag, rattling, suspension, and mechanical follow-through.
- Footfall/contact response, terrain deformation, debris impulse, camera response, and environmental interaction.
- Scale cues, atmospheric depth, motion cadence, lens perspective, and physically coherent camera movement.

## Animation, character, and motion capture

Motion capture is a first-class future Workshop subsystem.

- Body motion capture from professional marker-based systems and support for markerless sources where technically appropriate.
- Facial performance capture.
- Hand and finger capture.
- Multi-actor takes.
- Timecode/audio synchronization with picture and production sound.
- Skeleton definition, calibration, solve, retargeting, scale compensation, coordinate-system conversion, and reusable character profiles.
- Mocap cleanup: jitter removal, gap filling, foot locking, contact preservation, drift correction, trajectory editing, pose repair, filtering, and layered manual animation.
- Preserve the original capture as immutable source data and apply cleanup/retargeting non-destructively.
- Blend motion capture with hand-keyed animation without destroying either source.
- Performance takes, take comparison, metadata, notes, favorites, selects, and version history.
- Virtual production camera workflows in which a director or cinematographer can operate a tracked virtual camera through a CG scene/performance.
- Camera motion capture and tracked practical-camera integration.

## Professional audio post-production

Workshop should support the complete movie sound path, not simply waveform trimming.

### Dialogue / ADR

- Production-dialogue organization by scene, character, take, channel, microphone, and timecode.
- ADR cueing, streamers/countdowns, take recording, take comparison, comping, alignment, and synchronization.
- Dialogue editing, clip gain, fades, crossfades, room-tone management, noise/problem inspection, and repair-oriented processing.

### Foley and sound effects

- Foley cue sheets and recording sessions synchronized to picture.
- Footsteps, cloth, props, impacts, movement, environment, machines, vehicles, weapons, creatures, and production-specific effect organization.
- Searchable sound-effects library with tags, categories, metadata, favorites, audition-to-picture, and drag-to-timeline workflows.
- Layered sound construction and reusable effect stacks.

### Sound design

- Multi-layer sound design with buses, routing, sends, automation, time stretching, pitch processing, spatial positioning, convolution/reverb, modulation, filtering, dynamics, distortion, and other production effects.
- Sound-object and environment organization tied to scenes/shots/assets when useful.

### Music / score

- Temp music, score spotting, cue markers, cue sheets, stems, revisions, and synchronization to picture.
- Tempo maps, bar/beat grids, MIDI and instrument-host research lanes, and score delivery organization where appropriate.
- Separate music stems for final mix and alternate deliverables.

### Final mix

- Professional mixer with tracks, buses, VCAs/groups, sends, automation, gain, pan, EQ, dynamics, metering, loudness, phase, delay, and surround/immersive layouts as supported.
- Dialogue, music, effects, Foley, ambience, and print-master stems.
- Loudness/peak standards, downmix checking, alternate language/version support, and delivery/QC reports.

## Production asset and project core

All major Workshop tools should share a common project/asset model instead of behaving as unrelated applications.

An asset may include footage, audio, an image sequence, subtitle file, 3D model, texture, material, animation, mocap take, map, script, server package, ROM/homebrew asset, VFX plate, render, proxy, cache, or generated deliverable.

The shared layer should track:

- Stable asset identity independent of the current filename/location.
- Source path and managed/generated derivatives.
- Hashes and integrity state.
- Provenance, source/license records where relevant, creation/import history, and responsible author.
- Dependencies and reverse dependencies.
- Versions and approved versions.
- Proxy/render/cache relationships.
- Project usage: sequences, shots, scenes, maps, servers, packages, or other consumers.
- Missing/offline/relinked state.
- Storage locations and controlled migration.

A production should be able to answer questions such as:

- Which shots depend on this robot model?
- Which sequences use this camera original?
- Which renders became stale after this material changed?
- Which package contains this exact asset revision?
- Does the archived camera original still match its ingest checksum?

## Collaborative Production

Team collaboration is a first-class Workshop architecture requirement.

Not every asset should behave like a shared text document. Mergeable structured state can synchronize live; huge or non-mergeable binary assets need ownership/locking and explicit versions.

### Live project presence

- Show who is currently in a project.
- Show which sequence, shot, scene, map, asset, mix, animation, or task each collaborator is working on when permissions allow.
- Near-real-time propagation for appropriate structured edits, annotations, status changes, metadata, and project operations.
- Clear online/offline/synchronizing state rather than silent divergence.

### Version history and accountability

- Every meaningful project change records author, timestamp, revision, affected objects, and parent revision.
- Compare revisions and restore prior versions.
- Named checkpoints and production milestones.
- Reversible operations where technically possible.
- History must remain understandable to artists and directors rather than exposing only low-level source-control mechanics.

### Asset locking and merge policy

- Merge collaborative state where an operation has deterministic conflict handling.
- Lock or reserve non-mergeable assets when simultaneous editing could destroy work.
- Explicit lock owner, lock reason/status, stale-lock recovery, and administrator override with audit history.
- Never silently accept last-writer-wins for production-critical binary assets.

### Shot / task workflow

Support configurable production states such as:

`Needs Animation -> Animation Review -> Lighting -> Render -> Composite -> Director Review -> Approved -> Final`

- Assign people/departments.
- Due dates/priorities where desired.
- Notes and threaded discussion attached to the actual shot/asset/task.
- Dependencies and blockers.
- Department handoffs.
- Approval/rejection history.

### Live review and approval

- Synchronized review sessions with shared playhead/timecode.
- Frame-accurate comments.
- Draw/paint annotations over frames without modifying source media.
- Compare versions A/B, wipe, difference, and side-by-side.
- Director/VFX supervisor approval states.
- Timecoded notes delivered directly back to the responsible task/shot.

### Branches, experiments, and controlled merge

- Allow experimental versions without damaging the production-approved state.
- Merge approved work back through explicit review.
- Keep project checkpoints and reversible merge history.
- Provide artist-friendly terminology while retaining rigorous revision semantics underneath.

### Permissions

Potential roles include director, producer, picture editor, assistant editor, VFX supervisor, animator, mocap artist, modeler, texture/material artist, FX artist, compositor, colorist, dialogue editor, Foley artist, sound designer, re-recording mixer, music department, world designer, server administrator, reviewer, and project administrator.

Permissions should be granular and project-defined rather than hard-coding one studio hierarchy.

### Shared rendering and jobs

- Shared render/job queue.
- Visible queued/running/completed/failed state.
- Worker capability discovery.
- Ownership, priority, retries, cancellation, logs, outputs, and dependency tracking.
- Render invalidation when upstream assets change.
- Local workstation rendering plus future LAN render nodes/farm coordination.

### Network model

- LAN-first collaboration must be possible without cloud dependence.
- Remote collaboration should use secure authenticated transport and explicit exposure/configuration.
- A studio should be able to self-host its complete project collaboration environment.
- Offline/local work should reconcile safely after reconnection where the data model permits it.
- Large files should use chunking, hashing, resumable transfer, deduplication, and integrity verification rather than naïve whole-file retransmission.

## Game world, map, mod, and custom-server authoring

Workshop should eventually absorb the broad categories of community tools historically required to create and operate custom persistent worlds such as Ultima Online shards, while remaining engine/game-format neutral.

- 2D tile-map and large-world map editing.
- Terrain sculpting, heightmaps, procedural terrain, biome painting, roads, paths, water, and region tools.
- Static-object/tile/prefab placement.
- Collision, trigger, region, zone, portal, teleport, gate, and instance editing.
- Spawn, NPC, resource node, item, loot, vendor, economy, crafting, housing, faction/reputation, quest, dialogue, weather, calendar, day/night, and event editors.
- Navigation/pathfinding visualization and navmesh tools.
- AI-behavior graph and scripting tools.
- Script editor with validation, autocomplete, references, and debugger hooks where supported.
- Character/item/NPC/database browsers.
- Sprite, tile, texture, animation, sound, music-zone, and localization asset tools.
- Server-rule configuration, accounts, roles, permissions, GM/admin tooling, command tools, console, logs, event/packet diagnostics where lawful and appropriate.
- Server health, world-state inspection, spawn-density/traffic/economy visualization, backups, snapshots, rollback, migrations, staging/test/live environments, and controlled deployment.
- Local simulated clients/bots for load and regression testing where protocol/game licensing permits it.
- Deterministic test scenarios for quests, maps, scripts, events, and server behavior.
- Map/world/server diffs, patch generation, dependency manifests, mod packaging, conflict detection, signatures, shard/world export/import, server cloning, templates, and asset provenance.
- Import/export adapters for established lawful community formats rather than forcing creators into proprietary Nougat-only data.

## File and asset engineering

Workshop also serves as Nougat's deep file-engineering toolbox.

Planned families include:

- Split and Reassemble.
- Smart transport/package builder with manifests and hashes.
- File-tree snapshot and compare.
- Universal file/media inspector.
- Hash and verification center.
- Exact and near-duplicate detection.
- Archive create/test/browse/extract/split tooling.
- Recovery/parity data generation.
- Batch rename and organizer.
- Metadata inspection/editing/sanitization.
- Media conversion/remux/lossless cutting/joining.
- Track/subtitle/audio/image workshops.
- Contact sheets/thumbnails.
- Media health checks and recoverable-container repair.
- Compression laboratory with repeatable quality/size/encode-time comparison.
- Disc/ISO utilities for lawful non-DRM media.
- Folder catalogs/manifests.
- Verified copy/move.
- Nougat media-collection auditor.
- Game/ROM package inspector for user-provided lawful content.
- Live-TV recording processing.
- Privacy/sanitization reports.

## Workshop Recipes

Workshop tools should ultimately compose into reusable non-code recipes.

Examples:

`Verify -> Extract -> Rename -> Convert -> Strip Metadata -> Hash -> Package -> Split`

`Camera Card -> Verified Offload -> Production Manifest -> Proxies -> Dailies -> Backup -> Import`

`World Source -> Validate Maps/Scripts/Assets -> Automated Tests -> Compare Release -> Patch -> Sign -> Staging`

Recipes must expose every destructive step before execution and support dry-run/preview where practical.

## v0.0.50 boundary

v0.0.50 does **not** attempt to build the professional production environment described above.

The v0.0.50 Workshop implementation is deliberately narrow:

- Rename the user-facing Studio tab to **Workshop** and remove obsolete Gold Studio branding.
- Add the first real Workshop tool: **File Splitter / Reassembler**.
- Split files, folders, or project trees using either a requested part count or a maximum part size.
- Generate `NOUGAT_SPLIT_ARCHIVE` manifests with ordered part/chunk metadata and SHA-256 integrity records.
- Reassemble by selecting a manifest or any part in the set, detect missing/corrupt pieces, verify hashes, and reconstruct the original tree exactly as safely supported.
- Preserve filenames, directory structure, empty directories, timestamps/permissions where supported, and safe symlink semantics.
- Default to an upload-safe target around 450 MB when the user wants a package under a 500 MB ceiling.
- Keep the format publicly documented and independent of the Nougat application so future versions can remain compatible.

All professional editing, VFX, CGI, motion-capture, collaborative-production, world/server-authoring, and advanced Workshop systems in this document remain roadmap work requiring later owner-approved builds.
