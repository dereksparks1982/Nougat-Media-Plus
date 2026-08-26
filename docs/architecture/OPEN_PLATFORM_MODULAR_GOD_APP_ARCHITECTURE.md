# Nougat Open Platform and Modular "God App" Architecture

Status: long-term architecture and product roadmap. This document does not expand the owner-approved implementation scope of v0.0.50.

## Core product law

Nougat Media Suite must be able to grow into an extremely broad application without turning every installation into an enormous mandatory bundle.

The architectural rule is:

> Nougat is a small useful core surrounded by optional capability.

At the most basic level, Nougat remains a media player. A user who only wants the player should not be forced to install professional post-production, game-authoring, server, office/productivity, AI, media-server, emulator, security, or other optional systems.

A user who wants the complete environment should be able to install those systems into the same coherent application instead of maintaining a drawer full of unrelated programs.

## Freedom to leave

User work belongs to the user, not to Nougat or Elderred Softworks.

Nougat must never rely on project-file captivity as a retention strategy.

Long-term requirements:

- Complete project export wherever technically possible.
- Published specifications for Nougat-created file formats.
- Published schemas and validators for structured project/package formats.
- Prefer established open standards where they can faithfully represent the work.
- Provide multiple export paths when different downstream applications require different interchange formats.
- Preserve source assets and provenance rather than baking everything into an opaque database.
- Clearly report any project feature that cannot be represented by a requested export format.
- Do not require authorization from Elderred Softworks merely to read or write an open Nougat interchange format.
- Do not require a Nougat licensing server to recover a user's project.
- Design archives so a future implementation can recover the work from documentation and data even if the original Nougat binary no longer exists.

The desired retention model is:

> People stay because they prefer Nougat, not because Nougat prevents them from leaving.

## Open Production Manifest direction

The proposed Open Production Manifest should become a public, application-neutral production description capable of representing dependencies, provenance, hashes, versions, approvals, assets, timelines, renders, audio, VFX, mocap, world/server assets, deliverables, and other production state.

Nougat may provide the reference implementation without requiring other applications to use Nougat itself.

NOUGAT_SPLIT_ARCHIVE is an early, much smaller example of this philosophy: data is explicitly described, checksummed, documented, and reconstructable rather than hidden in an application-only container.

## One application, many optional capability families

The long-term Nougat shell may host many capability families while keeping each independently installable, removable, repairable, updatable, and versioned.

Potential optional families include:

- Core Player and media playback.
- Library, discovery, metadata, and media organization.
- Live TV, World TV, recording, guide, and broadcast tools.
- Stream and network playback tools.
- Games and emulation.
- Media Server.
- Security analysis.
- Local AI where desired.
- Workshop file and asset engineering.
- Professional picture editing and finishing.
- Professional audio post-production, Foley, ADR, scoring, sound design, and final mix.
- CGI, compositing, chroma keying, tracking, rendering, simulation, and VFX.
- Animation and motion capture.
- Game-world, map, mod, and persistent-server authoring.
- Collaboration, review, production tracking, and render coordination.
- Document authoring and page layout.
- Spreadsheet and data-table work.
- Presentations and slide authoring.
- Notes, structured notebooks, diagrams, forms, and lightweight databases.
- PDF creation, inspection, annotation, and conversion.
- Image and graphics tooling.
- Developer and diagnostic utilities.

The goal is not to imitate a collection of separate applications that merely share a launcher. Shared project, asset, identity, file, collaboration, provenance, queue, plugin/component, undo/history, search, and export layers should make the capabilities feel like one environment.

## Optionality law

Every non-core capability should declare:

- component ID and version;
- required and optional dependencies;
- download size and installed size;
- license/source/provenance information;
- installation and removal behavior;
- health checks;
- migration requirements;
- data owned by the component;
- whether removal leaves user-created work intact;
- compatible Nougat versions;
- update source/signature/hash information where applicable.

Removing an optional component must not remove unrelated user work.

The application should explain missing capabilities and offer installation instead of crashing or silently degrading.

## Progressive independence from third-party runtimes

Nougat currently depends on proven external systems for some capabilities. Long-term architecture should make every major dependency replaceable behind versioned interfaces.

The goal is not to reject open standards or reimplement mature technology merely for vanity. The goal is to ensure that no external runtime permanently controls Nougat's architecture.

### Native player direction

libVLC is an implementation dependency, not Nougat's permanent identity.

Roadmap direction:

1. Continue isolating playback behind Nougat-owned interfaces.
2. Separate demux, decode, clock/synchronization, audio output, video output, subtitle, track, seeking, chapter, and transport responsibilities.
3. Build Nougat-owned implementations incrementally with regression tests against accepted playback behavior.
4. Continue supporting interoperable media/container/codec standards.
5. Replace libVLC only when the Nougat-native stack is demonstrably mature enough to preserve or improve supported behavior.
6. Keep the transition reversible until the native player has passed broad real-world media testing.

Owning the playback stack must never mean inventing proprietary media formats simply to create lock-in.

### Other systems

Apply the same replaceable-interface rule to media-server integration, emulation hosts, search/privacy transports, security scanners, AI runtimes, render backends, production services, collaboration transport, and other substantial dependencies.

Nougat may integrate an excellent external implementation today while retaining the architectural freedom to replace it tomorrow.

## Basic Sharp long-term migration

Basic Sharp is a future Elderred language target, not a v0.0.50 implementation dependency.

A Nougat rewrite should occur only after Basic Sharp has matured into a production-capable language and toolchain with the features required by Nougat, including reliable native interoperability, filesystem/network/process APIs, concurrency, graphics/windowing access, memory/performance control, testing, debugging, packaging, diagnostics, and stable language/runtime versioning.

Do not perform a destructive big-bang rewrite.

Preferred migration strategy:

1. Define stable language-neutral subsystem interfaces first.
2. Make Basic Sharp capable of consuming and implementing those interfaces.
3. Rewrite isolated low-risk modules first.
4. Run old and new implementations against the same regression suites.
5. Replace modules only after behavior and performance are verified.
6. Progressively move higher-risk systems such as playback, rendering, collaboration, and production-state management after the language/toolchain proves itself.
7. Preserve file-format and project compatibility throughout the migration.

The desired end state is a Nougat codebase primarily or entirely implemented in Basic Sharp without requiring users to sacrifice existing projects or workflows during the transition.

## Product breadth without product bloat

A complete Nougat installation may eventually be enormous. That is acceptable when the user chooses it.

The default architecture must therefore distinguish:

- core application code;
- optional feature modules;
- managed third-party or Nougat-owned runtimes;
- models/data packs;
- templates/assets;
- user-created data;
- rebuildable caches;
- generated renders/output;
- project-local assets;
- system-level dependencies.

This separation is necessary for installation choice, clean upgrades, component repair, source-tree slimming, storage reporting, reproducible builds, and future replacement of individual subsystems.

## Interoperability over exclusivity

Compatibility is a product feature.

Nougat should welcome other applications reading its open formats and should provide adapters to other lawful documented formats when technically feasible. Success is not measured by the number of files that only Nougat can open.

If another editor, renderer, game tool, audio workstation, office suite, archival system, or production tracker can participate in a Nougat-managed project through an open standard, that strengthens the ecosystem.

## Guiding statement

> Build the application people choose when they are free to choose anything else.
