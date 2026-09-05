# Nougat Media Plus

**Nougat Media Plus** is a native Linux entertainment and media platform developed by **Elderred Softworks LLC**. It combines television, radio, local media, internet streaming, games, production tools, connected hardware, search, diagnostics, and network playback inside one unified desktop application.

## **Media & Playback**

The built-in **Video Player** handles local and network media with playback controls, seeking, fullscreen playback, volume control, chapters, resume state, episode progression, Up Next, keyboard and mouse controls, and Live TV channel navigation.

**Home** provides Continue Watching and media shelves. **Library** manages movies, television, music, videos, artwork, metadata, persistent media folders, playback history, automatic discovery, list and grid views, collection organization, and owner-controlled metadata correction through Fix Match.

**Discover** provides recommendation-oriented browsing, while **Nougat Search** combines privacy-focused search, crawler infrastructure, decentralized-search foundations, Secure Search protections, P2P downloading, torrent and magnet support, stream-while-downloading playback, metadata retrieval, playable-file selection, transfer progress, security analysis, and on-device recommendation intelligence.

## **Live Television**

**Live TV** supports compatible over-the-air television hardware including Hauppauge WinTV devices and HDHomeRun network tuners.

The television system includes tuner discovery, channel scanning, channel storage, guide data, native playback, channel navigation, tuner diagnostics, artwork, preferred-device handling, physical-device grouping, multiple physical tuners, and independently allocatable tuner resources when supported by the hardware.

**World TV** extends television beyond antenna reception with internet-accessible broadcaster sources, guide integration, source verification, asynchronous stream resolution, alternate-source recovery, playback-health diagnostics, bounded connection and retry handling, station artwork, source diagnostics, and clear failure reporting.

## **Radio & Signal Monitoring**

**Radio** provides a unified receiver, scanner, and signal-monitoring environment for local and internet radio, AM, FM, HD Radio, DAB/DAB+, DRM, longwave, mediumwave, shortwave, weather, emergency services, public safety, aviation, marine, railroad, amateur radio, digital radio systems, beacon services, aircraft and marine data, weather satellites, amateur satellites, and ISS/satellite reception paths.

Professional receiver controls include direct frequency entry, modulation and tuning-step controls, bandwidth and filtering, gain and AGC, squelch, scanning, signal monitoring, favorites, recordings, presets, spectrum and waterfall foundations, automatic hardware discovery, multiple receiver resources, provider-neutral hardware handling, and expandable SDR and decoder backends.

Services unavailable with connected hardware are identified truthfully rather than presented as active. Encrypted traffic is identified as encrypted or locked rather than represented as decrypted.

## **Cellular Lab**

**Cellular Lab** provides a foundation for private and controlled cellular-network experimentation using owned or explicitly authorized equipment.

Its architecture includes private cellular-network concepts, subscriber and SIM management, enrolled test devices, local calls and SMS, RF and spectrum monitoring, protocol diagnostics, network-health tooling, and future historical analog-cellular experimentation.

Actual transmission capabilities depend on compatible hardware, implementation status, authorization, and appropriate spectrum conditions.

## **Games**

**Games** provides a unified game library and emulator front end with persistent ROM folders, archive-contained game discovery, system recognition, artwork, controller support, emulator selection, managed emulator runtimes, and embedded gameplay where supported.

The Games architecture is designed to support multiple console generations while keeping game organization, artwork, controls, emulator management, launch behavior, and future embedded rendering inside the Nougat Media Plus environment.

## **Studio & Production Tools**

**Studio** provides integrated media-production, file-production, and planning tools.

**File Splitter** creates verified split sets using target-based sizing and integrity checks. **File Assembler** discovers numbered parts, validates completeness, ordering, sizes, and hashes, reconstructs the original payload, and verifies final integrity.

## **Drone Production**

**Drone Production** provides a director-oriented aerial-production environment for simulation, telemetry, camera and gimbal coordination, repeatable flight paths, mission planning, and Director Shot workflows.

The system keeps aircraft movement, camera movement, timing, subject tracking, shot metadata, simulated telemetry, payload controls, and production planning together inside the application.

## **Streaming & Network Media**

**Stream** provides direct URL playback and webpage access for supported internet-video sources and services.

Nougat Media Plus also includes a **Jellyfin-backed media-server foundation** and a Nougat-branded **Web Player** for browsers and devices on the local network.

Local media access is designed to operate without a mandatory cloud account, cloud relay, or Internet connection. The network-media architecture supports phones, tablets, laptops, televisions, browsers, and other LAN clients while keeping Nougat Media Plus as the visible user-facing experience.

## **Diagnostics, Privacy & Hardware Integration**

Nougat Media Plus includes diagnostic and system-status tools, hardware reporting, server-health monitoring, media and tuner diagnostics, security-scanning foundations, persistent application state, privacy infrastructure, and expandable service integrations.

Its modular architecture allows new media, tuner, receiver, emulator, networking, search, privacy, production, and hardware capabilities to be added without replacing the rest of the application.

## **Nougat Media Plus Interface**

Beginning with **v0.0.62**, Nougat Media Plus uses the approved black and deep-green tactical interface together with the metallic tactical-green **N** identity.

The active identity is applied across the application, launcher, versioned executable, X11 window identity, GNOME dock and app switcher, and in-app branding.

The tactical visual system coordinates navigation, panels, controls, dialogs, status surfaces, application identity, and UI interaction sounds while preserving the functionality of the individual Nougat systems.

## **Platform & Repository**

Nougat Media Plus is developed primarily as a **native Linux desktop application**.

The GitHub repository preserves the complete runnable project tree including source, assets, tools, bundled runtimes, emulator components, media-server infrastructure, AI components, security components, and required project dependencies.

Oversized project files are stored through **Git LFS** rather than intentionally omitted from project history.

## **Educational and Lawful Use Notice**

Nougat Media Plus is provided for **educational, research, testing, and development purposes only**.

Elderred Softworks LLC does not endorse or authorize unlawful activity, unauthorized access, unauthorized interception or surveillance, circumvention of access controls, harassment, property damage, invasion of privacy, intellectual-property infringement, interference with communications or services, or other harmful or illegal activity.

Users are responsible for ensuring that their use of Nougat Media Plus complies with applicable laws, regulations, licensing requirements, spectrum and communications rules, privacy obligations, intellectual-property rights, contractual restrictions, and third-party terms.

Features involving networks, radio, cellular systems, devices, accounts, media, signals, or data should be used only with resources the user owns or is explicitly authorized to access, operate, test, receive, modify, or analyze.

Nothing in Nougat Media Plus, its source code, or its documentation grants permission to perform an act that would otherwise be unlawful or unauthorized. Availability of a technical capability does not imply legal authorization to use it in a particular jurisdiction or environment.

### **Intended Users**

Nougat Media Plus is designed primarily for **licensed radio operators, communications professionals, broadcast and satellite technicians, researchers, educators, government agencies, military personnel, public-safety professionals, engineers, and other authorized technical users** working with communications, media, RF systems, satellites, drones, networking, and related hardware.

Some capabilities may require licenses, permissions, certifications, spectrum authorization, equipment approval, or organizational authority before lawful use.

Nougat Media Plus does not verify a user's credentials or authorization. Users are solely responsible for ensuring that their use of the software, connected equipment, frequencies, networks, satellites, aircraft, and communications systems is lawful and authorized.

The inclusion of professional, governmental, public-safety, or military use cases does not imply endorsement, affiliation, certification, or authorization by any government agency, military organization, regulator, or equipment manufacturer.

## v0.0.62 - Nougat Media Plus Tactical UI Conversion

Nougat Media Plus v0.0.62 is the accepted tactical-interface and product-identity release built from the accepted v0.0.61 baseline.

### Technical changes

- Renamed the active product from Nougat Media Suite to Nougat Media Plus.
- Advanced the active root executable to `Nougat_Media_Plus_v62`.
- Advanced runtime and build version reporting to v0.0.62.
- Updated `src/main.cpp`, `CMakeLists.txt`, launcher identity, and embedded X11 icon data for Nougat Media Plus.
- Installed the approved metallic tactical green N as the active application identity.
- Applied the approved N to the in-app identity, raw executable, launcher, GNOME dock/app switcher, and X11 window identity.
- Removed the retired legacy-name subtitle from the active application lockup.
- Added the black/deep-green tactical interface across navigation, tabs, buttons, panels, dialogs, fields, status surfaces, and controls.
- Preserved the approved tactical Video Player appearance.
- REPAIR5 fixes the blank-page regression by preventing the final tactical page-frame pass from painting over already-rendered page contents.
- Preserved Home, Library, Discover, Live TV, World TV, Radio, Search, Stream, Studio, Games, Debug/System, and Video Player behavior.
- Added the approved `UI Click Deep` one-shot hover/pointer-entry sound.
- Added the approved `el_boss UI Button Click` one-shot activation sound.
- Hover audio plays once on pointer entry and does not loop while the pointer remains inside a control.
- Activation audio plays once per actual activation.
- Disabled controls remain silent.
- Added the approved tactical UI authority artwork, tactical green N branding assets, icon assets, and UI audio assets.
- Retired `Nougat_Media_Suite_v61` as the active root executable.
- Known NES/SNES and Xbox 360 emulator issues from v0.0.61 remain open and carry forward to the next Games/emulator overhaul.

## v0.0.61 - Drone Mission Control and Games Expansion

Nougat Media Suite v0.0.61 is the owner-approved closeout release built from the accepted v0.0.60 baseline. All Drone Mission Control work, Games Library expansion, emulator-runtime work, and known Games regressions belong to this single v0.0.61 build.

### Drone Mission Control

- Replaces the smaller v0.0.60 Drone foundation panel with the native dark-green **Nougat Drone Lab Mission Control** workspace.
- Advances Director Shot from architecture scaffolding into an editable simulation workspace.
- Allows Mission Planner waypoints to be added and edited as a Director Shot path.
- Adds New Shot, path clearing, shot saving, and simulated path execution.
- Adds animated trajectory visualization for the planned aircraft route.
- Updates simulated altitude, horizontal speed, vertical speed, heading, battery, link state, and flight-instrument information while a shot runs.
- Adds interactive camera and gimbal controls for pitch and yaw.
- Adds Follow and Manual gimbal behavior.
- Adds simulation-only photo and video-record controls.
- Adds simulation-only payload ARM and RELEASE controls.
- Keeps PX4, ArduPilot, MAVLink, MAVSDK, FFmpeg, and GStreamer integration/dependency discovery visible without falsely reporting hardware as connected.
- Keeps real-aircraft arming and command transmission disabled.
- Preserves the approved **Nougat N** identity and the **Nougat Media Suite** product name.
- Keeps **Nougat Media Plus** only as a deferred future identity proposal and does not rename v0.0.61.

### Games Library Expansion

- Reworks the Games Library around persistent collapsible console groups.
- Preserves both **Grid** and **List** presentation modes.
- Restores large game-artwork cards inside expanded console groups instead of reducing games to tiny rows.
- Adds bounded vertical scrolling to the Systems panel so later consoles remain inside the Nougat interface.
- Preserves persistent ROM folders, system recognition, artwork, metadata, controller architecture, ZIP-aware discovery, and Nougat's emulator-host foundation.
- Adds the pinned **MesenCE 2.2.1** runtime foundation for NES and SNES.
- Preserves the existing Xbox 360 / Xenia runtime and Nougat embedding infrastructure for the complete v0.0.62 repair.
- Advances the active executable to **Nougat_Media_Suite_v61**.

### Known Games Issues Carried Into v0.0.62

- **NES:** MesenCE currently opens in its own external fullscreen emulator window instead of rendering inside Nougat's native Video Player viewport.
- **SNES:** MesenCE currently has the same external fullscreen-window regression and does not render inside Nougat's native Video Player viewport.
- **Xbox 360:** the Nougat-contained Xenia window path was restored, but GTA IV currently fails to run successfully through that path.
- These failures are intentionally documented rather than being falsely marked fixed.
- v0.0.62 is assigned a complete Games and emulator overhaul rather than another narrow emulator patch.
- v0.0.62 must audit every system exposed by Nougat, add the required emulator backends, and validate real video, audio, controller input, startup, shutdown, cleanup, and relaunch behavior.
- Emulator discovery alone does not count as working support.
- Systems intended to use Nougat-contained gameplay must render inside Nougat rather than opening unintended detached desktop emulator windows.

## v0.0.60 - Drone Production Foundation

Nougat Media Suite v0.0.60 is the **owner-accepted release** and frozen baseline from which v0.0.61 was developed.

- Adds a dedicated **Drone** interior tab beside Tools inside Studio.
- Establishes the native **Drone Production** workspace for aerial cinematography and production planning.
- Adds the **Director Shot** foundation for repeatable aircraft and camera movement.
- Establishes the production workflow foundation for **Describe -> Path -> Preview -> Simulate -> Save -> Authorized Flight**.
- Adds New Shot, Simulate Shot, and Record Path scaffolding.
- Adds telemetry foundations for GPS, altitude, speed, heading, battery, and vehicle-link state.
- Adds camera and gimbal pipeline foundations.
- Detects FFmpeg and GStreamer for future live drone-camera ingest.
- Adds integration discovery for **MAVLink, MAVSDK, PX4, and ArduPilot**.
- Adds pinned, reproducible third-party Drone source metadata and automatic source-fetch tooling.
- Preserves upstream project repositories and licenses separately from Nougat-owned source.
- Stores downloaded third-party working trees under the ignored `components/drone/vendor/` area.
- Adds a machine-readable Director Shot schema for future cinematic automation.
- Keeps this foundation release **simulation-only**. Real-aircraft arming and command transmission remain disabled.
- Preserves the approved **Nougat N** identity and includes reusable icon-identity installation support.
- Advances the active executable to **Nougat_Media_Suite_v60**.

## v0.0.59 - File Assembler, Live Splitter Recalculation, and Stable Player Seek Control

Nougat Media Suite v0.0.59 is the **owner-accepted release** and frozen baseline for the next build.

- Makes **Target MiB** live in File Splitter: valid numeric edits immediately recalculate Pieces, Recommended count, and estimated size per piece.
- Uses `ceil(payload_bytes / target_bytes)` for the target recommendation.
- A later manual **Pieces** edit becomes the owner override; editing Target MiB again makes the target authoritative.
- Allows targets above the safe upload ceiling to recalculate without silently changing the owner's typed value, while blocking Split above **476 MiB**.
- Repairs the first-open Splitter layout with reserved **Pieces | Target MiB | Recommended** columns so text cannot overlap.
- Adds **File Assembler** as its own **Studio -> Tools** entry beside File Splitter.
- Removes Reassemble and Verify from the File Splitter workflow itself.
- File Assembler accepts either the `.zip.parts.json` manifest or any numbered `.zip.###` split part and automatically discovers the matching set.
- Verifies part completeness, ordering, and recorded hashes before reconstruction and verifies the final reconstructed payload/original afterward.
- Replaces the deforming moving seek sprite with one stable independent round Nougat/Search-style seek thumb.
- Keeps the seek thumb fully formed throughout playback, including **0%, intermediate positions, 100%, windowed playback, and fullscreen**.
- Advances the active root executable to **Nougat_Media_Suite_v59**.

## v0.0.58 - Reliability, Tuner Preference, Native Fix Match, Radio Expansion, and Studio Repair

Nougat Media Suite v0.0.58 is the **owner-accepted release** and frozen baseline for v0.0.59.

- Repairs Home and Library warm-start behavior so usable cached content appears before silent background refresh completes instead of rebuilding the visible interface during ordinary launches.
- Preserves valid recent Continue Watching movies and television episodes across launches and improves migration of older resume records.
- Stops discarding otherwise valid server/catalog Continue Watching entries merely because a server-internal media path is not directly readable by the desktop process.
- Strengthens artwork recovery through Jellyfin and provider identity, poster and backdrop/key-art fallbacks, and generated local video frames before a black last resort.
- Allows older Continue Watching records without complete stored provider identity to recover artwork through metadata identity resolution.
- Advances **Fix Match** into a Nougat-native multi-candidate correction workflow with title, year, and media-type search, direct provider IDs, candidate artwork and metadata, explicit owner selection, persistent manual matches, Edit Metadata, Choose Artwork, and Clear Manual Match support.
- Repairs the shared round seek/thumb rendering so endpoint geometry does not produce the rejected crescent or halo appearance.
- Allows left-clicking the unloaded black Video Player viewport to open media while preserving the existing right-click context menu and normal loaded-player behavior.
- Persists the preferred **physical Live TV tuner device** instead of confusing an individual HDHomeRun tuner resource with the entire device.
- Keeps HDHomeRun tuner resources grouped under their physical-device identity and preserves same-device tuner fallback.
- Advances Live TV guide harvesting, cache preservation, active-multiplex collection, idle-tuner refresh behavior, and the supported HDHomeRun/SiliconDust XMLTV route.
- Expands Radio into the universal receiver and scanner service matrix documented in the introduction and roadmap.
- Keeps **Internet** immediately after Local and keeps **ISS/Satellite** as the final Radio service option.
- Includes **AM, FM, HD Radio, DAB/DAB+, DRM, Longwave, Mediumwave, Shortwave, Weather, Emergency, Public Safety, Police, Fire, EMS, Government, Military, Airband, Marine, Railroad, CB, FRS/GMRS, MURS, Amateur/Ham, Business, Utilities, Trunked, P25, DMR, NXDN, TETRA, Paging, Numbers, Time/Beacon, ADS-B, ACARS, AIS, Weather Satellite, Satellite, Amateur Satellite, and ISS/Satellite** service positions alongside Local and Internet.
- Adds additional vertical separation between Radio's professional receiver controls and the SIGNAL/status/information area so the lower information does not crowd the buttons.
- Adds **Cellular Lab** interface tabs for **2G GSM**, **1G Analog**, **Subscribers / SIMs**, **Calls / SMS**, and **Network / RF**.
- Roadmaps the Cellular Lab around private GSM BTS/BSC/MSC/HLR integration, owned test SIMs/subscribers, attached devices, local calls and SMS, RF/spectrum and protocol diagnostics, network health, and later isolated GPRS data.
- Keeps actual cellular-network transmission implementation separate from the v0.0.58 interface and architecture foundation.
- Restores the header **Server** status indicator to the exact accepted v0.0.57 stitched Nougat-circle renderer. Its size, shape, stitching, bevel, border, depth, and position remain unchanged; only the green, yellow, or red face color changes according to server state.
- Updates File Splitter to the **450 MiB** default target model with an editable Target MiB field and enforced maximum output-part sizing.
- Improves diagnostics so server-internal catalog paths are distinguished from genuinely unavailable local media sources.
- Advances GNOME/Resources application identity through the canonical **com.elderredsoftworks.NougatMediaSuite** identity and direct association with the real Nougat executable.
- Advances the active root executable to **Nougat_Media_Suite_v58**.

### Known work carried forward into v0.0.59

- **File Splitter live recalculation:** editing Target MiB immediately recalculates Pieces, Recommended count, and estimated MiB per piece. A later manual Pieces edit becomes the owner override.
- **File Splitter first-open layout:** reserve fixed columns and spacing for Pieces, Target MiB, and Recommended so the initial text cannot overlap or appear jumbled.
- **Separate File Assembler:** add File Assembler as its own **Studio -> Tools** entry beside File Splitter. Move Reassemble out of the File Splitter workflow. File Assembler accepts a manifest or split part, discovers the remaining pieces, validates completeness, ordering, and hashes, rebuilds the original file, and verifies the final SHA-256.

## v0.0.57 - Media Identity, Live TV Recovery, World TV, Radio, and Games Repair

Nougat Media Suite v0.0.57 is the owner-accepted release and frozen baseline for v0.0.58.

- Strengthens Library filename normalization for real-world media names and adds persistent owner-controlled **Fix Match** metadata corrections.
- Keeps Movies as the authoritative default Library root while preserving the existing movie and television architecture.
- Removes the rejected Child Safe / parent-lock presentation and gate from the System interface.
- Repairs server-health evaluation so valid HTTP **2xx** responses are accepted instead of depending on one exact HTTP status-line spelling.
- Repairs startup, close, worker-state, and UI-loop behavior uncovered during v0.0.57 testing.
- Improves Live TV tuner allocation and retry across usable physical tuners without destructive rescans or changing the stored channel lineup and artwork.
- Expands World TV recovery using official broadcaster pages, direct stream candidates, and bounded alternate-source retries.
- Consolidates Radio into one professional receiver interface with **Local, Internet, Emergency, Weather, Shortwave, and ISS / Sat** sources.
- Rebuilds the Games Library around persistent collapsible console sections instead of one giant flat mixed list.
- Advances GNOME/application identity and the active root executable to **Nougat_Media_Suite_v57**.

Known issues carried forward into v0.0.58:

- The header **Server** status light still needs its approved Nougat stitched-button visual repair.
- File Splitter must recommend output sizes safely below the **500 MB upload limit** and must provide a manually editable target-size control.
- Live TV does not yet persist the owner's preferred tuner device, so a connected WinTV can be selected ahead of HDHomeRun even when HDHomeRun is intended for television.
- The previously carried Player seek-thumb edge / halo visual issue remains scheduled for repair.

## v0.0.56 - Player Identity and Fullscreen Playback Repair

Nougat Media Suite v0.0.56 advances the native Player and fullscreen playback system while preserving the existing Games, Radio, Live TV, Web Player, server, Search, P2P, Studio, and media-library architecture.

- Cleans television episode identity to the format **Series • SxxExx • Episode Title** without duplicate episode codes, redundant year information, raw filenames, or release metadata.
- Applies the same clean identity to **Up Next**.
- Preserves the established fullscreen transport behavior: **<<** previous episode/channel/station, **<** rewind 10 seconds, play/pause, **>** fast-forward 10 seconds, and **>>** next episode/channel/station.
- Adds chapter-aware fullscreen seeking using real chapter timestamps when available and Nougat's automatic fallback chapter marks when real chapter metadata is unavailable.
- Preserves the existing automatic fallback chapter spacing used by the normal Player.
- Extends the fullscreen seek backdrop horizontally to provide dedicated space for elapsed and total playback time without increasing its height or changing the seekbar size.
- Keeps fullscreen playback time updating while the activity controls are visible.
- Preserves the additional Live TV timing information, including program start and end clock values.
- Makes the main Nougat window disappear immediately when closed with the window-manager X while normal shutdown cleanup continues.
- Advances the active root executable from **Nougat_Media_Suite_v55** to **Nougat_Media_Suite_v56**.

Known carried-forward issue: the seekbar round-thumb edge/halo appearance remains visually incorrect and is deferred for repair in the next build.

## v0.0.55 - Background Web Player and Complete Runnable Project Checkpoint

Nougat Media Suite v0.0.55 advances the private-LAN Web Player and complete-project distribution work while v0.0.54 remains the current owner-accepted release.

- Adds the Nougat-branded LAN Web Player for browser access from computers, phones, tablets, televisions, consoles, and other devices on the same private network.
- Uses port **8096** for the public/LAN Nougat Web Player and **127.0.0.1:8098** for the hidden Jellyfin backend.
- Keeps Jellyfin behind Nougat as backend/catalog infrastructure instead of exposing the stock Jellyfin web interface as Nougat's user-facing experience.
- Runs the Web Player as a Nougat-owned background service so the desktop Nougat interface does not need to remain open for local-network browser access.
- Keeps local media playback and local server access independent of mandatory cloud accounts, cloud relays, or an Internet connection.
- Adds separate health handling for the Nougat Web Player and hidden Jellyfin backend.
- Advances the browser layout for **1366x768 at 100% browser zoom**.
- Uses the fullscreen transport layout **``<<  <  ^  >  >>``** for rewind 10 seconds, previous, play/pause, next, and forward 10 seconds.
- Preserves the approved Nougat N identity and the accepted v0.0.54 Games behavior, including the owner-tested Xbox 360/Xenia embedded-video path.
- Stores the complete runnable Nougat project tree on GitHub, including bundled emulator runtimes, Xenia, Jellyfin, AI, security components, source, assets, tools, and other required project dependencies.
- Stores oversized project files through Git LFS instead of removing those runtime files from the project.
- Keeps obsolete transfer/checkpoint package archives out of the runnable repository.

During owner testing, the Nougat Web Player returned HTTP 200 from `127.0.0.1:8096/nougat/v1/health` and the hidden Jellyfin backend returned HTTP 200 / Healthy from `127.0.0.1:8098/health`.

Known unresolved v0.0.55 issues are the desktop Server indicator remaining yellow / transitioning despite healthy Web Player and Jellyfin responses, and unresolved Safari/iPhone LAN access. v0.0.55 remains an in-progress checkpoint and is not yet an accepted release.

## v0.0.54 - Professional File Splitter and Silver Screen Studio

Nougat Media Suite v0.0.54 is the current owner-accepted release.

- Promotes File Splitter from a prototype into a professional Studio tool opened from **Studio -> Tools -> File Splitter**, rather than giving File Splitter its own top-level tab.
- Adds in-page **Add File**, **Add Folder**, **Add ZIP / Manifest**, and **Choose Location** browsing controls. File Splitter routine workflow remains inside Nougat without external chooser or status popups.
- Analyzes the selected source and recommends a balanced number of pieces automatically while still allowing manual piece-count selection and a **Use Suggestion** action.
- Adds calculated piece sizing, asynchronous Split/Reassemble/Verify operations, live in-page progress, and a real Stop control so large operations do not freeze the Nougat interface.
- Streams large files rather than loading them wholly into memory, records SHA-256 hashes for produced pieces, verifies reassembled data, cleans incomplete output after cancellation, and retains compatibility with earlier Nougat v2 split manifests.
- Gives Studio its Silver Screen identity with a silver top-level Studio button, silver Studio/Tools/File Splitter treatment, and a film-strip header across the Studio page.
- Preserves the accepted v0.0.53 Xbox 360 embedded-video path, native player geometry, real libtorrent P2P support, local llama.cpp AI runtime, and the rest of the accepted Nougat feature set.
- Enforces the one-root-executable release gate: **Nougat_Media_Suite_v54** is the sole active versioned executable in the project root.

## v0.0.53 - System Expansion and Xbox 360 Embedded Video

Nougat Media Suite v0.0.53 is the accepted release immediately preceding v0.0.54, built on the accepted v0.0.52 base.

- Aligns World TV with the Live TV Guide geometry while preserving World TV's own orange presentation and adds clearer dependency, resolver, provider, stream, and startup-timeout failure reporting.
- Reworks HDHomeRun ATSC scanning around physical RF channels 2 through 51 with explicit lock/service evidence, phased scan progress, truthful guide availability, and improved import diagnostics.
- Expands Games emulator discovery and launch mappings for Dolphin, DuckStation, PCSX2, PPSSPP, RPCS3, Cemu, MAME, installed Linux Switch backends, and the existing Nougat emulator set, while expanding persistent artwork matching and release-name normalization.
- Adds LAN Viewer v1 backend contracts for private-LAN discovery, read-only catalog access, trust state, and Verified Clean direct streaming without WAN relay or automatic port forwarding.
- Adds Child Safe Controls configuration/password protection, runtime component advisory/CVE inventory using OSV as the configured public advisory source, and NOAA/NWS public-safety alert support with protected local history.
- Repairs Linux process identity, preserves the approved translucent/rounded overlay path, and updates v0.0.53 launcher/executable icon identity from the approved Nougat N artwork.
- Completes the owner-tested Xbox 360 embedded-video path through the native Linux Xenia Edge runtime. GTA IV launches through Nougat with game audio and rendered video inside Nougat's existing native player viewport without opening a separate emulator window.

<!-- NOUGAT_V53_XBOX360_README_CONSOLIDATED_START -->
### Xbox 360 / GTA IV integration notes

The following v0.0.53 integration notes were previously stored as separate root README files and are consolidated here so `README.md` remains Nougat's single canonical README.

#### GTA IV runtime bridge

DIRECT OVERLAY - NO INSTALLER

This package is rooted exactly like the Nougat Media Suite project.
It does not contain or touch a build directory.
It does not replace Nougat_Media_Suite_v53.

Included:
- Corrected COMPANY_BIBLE.md with NO INSTALLERS law.
- Xenia Canary runtime bridge at components/games/runtime/xenia/xenia_canary.
- GTA IV Title ID 545407F2 Vulkan/windowed profile.
- Xenia upstream license/source record.
- Validation/build record and SHA-256 manifest.

The first time Nougat starts an Xbox 360 title through this bridge, it retrieves
the exact pinned official Xenia Canary Linux AppImage and verifies the pinned
SHA-256 before running it. The verified binary is then reused.

No game files or console system files are included.
No commit, tag, push, or release is performed by this package.

#### Xenia Edge direct-surface repair

Changed-files-only package. No installer.

This package replaces the Xbox 360 launcher glue only and adds two precompiled
Linux x86-64 helper binaries plus their source. It does not rebuild Nougat and
does not rebuild Xenia Edge on the owner's computer.

Required existing runtime:
  components/games/runtime/xenia/xenia_edge_linux.AppImage
  pinned upstream SHA-256:
  dce3d41f2d5126d5bdbd91e87f7d2ccded89d87e349306804688a3cb4e477591

Failure behavior is fail-closed: if Nougat's native Games/video X11 viewport
cannot be identified, Xenia Edge is not launched as a detached window.

#### Xenia Edge direct-child repair

Scope:
- Runtime-only Xbox 360 embedding repair.
- Does not rebuild or replace Nougat_Media_Suite_v53.
- Uses the NOUGAT_EMBED_XID already supplied by local v53 EmulatorHost.
- Creates one real X11 InputOutput child directly inside Nougat's Games/video viewport.
- Xenia Edge's XCB/Vulkan surface is redirected to that child before swapchain creation.
- Edge's GTK top-level window is realized but not mapped as a desktop window.

This replaces the rejected parent-surface/proxy/reparent experiments.
The existing xenia_edge_linux.AppImage remains required in the same runtime directory.

<!-- NOUGAT_V53_XBOX360_README_CONSOLIDATED_END -->

## v0.0.52 - Radio Receiver Expansion and Hardware Foundation

Nougat Media Suite v0.0.52 expands the Radio receiver and hardware foundation, including the new Radio backend, hardware capability handling, receiver controls, component sources, provider-neutral tuner work, SDR foundations, receiver discovery, multi-device support, scanning, favorites, recordings, signal monitoring, and experimental Hauppauge WinTV-HVR-955Q / Silicon Labs Si2157 FM support work. Known Radio and UI issues remain carried forward for repair.

## v0.0.51 - Radio, Live TV, World TV, Games, File Splitter, and Nougat Identity

v0.0.51 expands Nougat Media Suite across Radio, Live TV, World TV, Games, Studio tools, desktop identity, navigation, and local-network media foundations.

- Adds the new top-level **Radio** area with AM, FM, Shortwave, Weather, DAB/DAB+, DRM, Internet Radio, SDR, Favorites, and Recordings architecture.
- Advances **HDHomeRun FLEX DUO** support with physical-device grouping, two independent tuner resources, provider-neutral tuner status, expanded scan diagnostics, and continued full channel-scan pipeline work.
- Advances **World TV** with guide-layout work, current-source verification, playback-health diagnostics, asynchronous stream resolution, bounded timeouts, and improved failure reporting.
- Expands the **Games** architecture with additional emulator integration work, ROM/archive recognition, controller handling, artwork matching, and persistent prepared-artwork support.
- Reworks **Studio -> Tools -> File Splitter** toward the full embedded Nougat workflow for folders, files, existing ZIPs, output destination, piece count, calculated piece size, splitting, reassembly, progress, and integrity verification.
- Adds the owner-approved new **Nougat N** application identity and branded top-header artwork across the v0.0.51 desktop application.
- Advances top-level navigation toward geometry-derived scrolling so the complete final tab remains reachable as Nougat grows.
- Continues system-wide translucent floating-overlay and rounded-corner clipping work.
- Carries forward the **LAN Web Viewer** foundation for local browser access to Nougat media, artwork, history, Live TV, sessions, pairing, and diagnostics without requiring a cloud login or automatic port forwarding.
- Records the unified controller framework for Nougat UI navigation, Video Player, Games, and future Drone Flight contexts.
- Carries forward all owner-tested v0.0.51 repair requirements documented in `docs/builds/NOUGAT_MEDIA_SUITE_v0_0_51_REJECTED_BUILD_LOG.md`.

## v0.0.50 - File Splitter, Unified Tuners, and Studio Foundation

v0.0.50 advances Nougat Media Suite with the first File Splitter implementation, unified HDHomeRun/WinTV tuner groundwork, and Studio naming/architecture work.

- Adds the initial Nougat File Splitter implementation and its regression-test foundation.
- Adds HDHomeRun discovery and tuner-provider support alongside the existing Linux DVB/WinTV path.
- Detects the owner's HDHomeRun FLEX DUO and exposes its two independently usable tuner resources.
- Adds HDHomeRun provider tests using deterministic synthetic tuner responses.
- Preserves the existing native Live TV, Games, Library, Search, Stream, World TV, Studio, diagnostics, and media-server foundations outside the approved v0.0.50 work.
- Renames the production area to **Studio** as the foundation for Nougat's future filmmaking and production environment.
- v0.0.50 is accepted with documented known issues carried forward to v0.0.51, including File Splitter workflow repair, HDHomeRun physical-device grouping and full-scan completion, provider-neutral Live TV status, visible version/icon identity, World TV guide presentation, World TV playback reliability, and World TV popup transparency/clipping.

See `docs/builds/NOUGAT_MEDIA_SUITE_v0_0_50_ACCEPTED_KNOWN_ISSUES.md` for the complete accepted-known-issues record.

## v0.0.49 - Games Runtime, Artwork, ZIP Library, and World TV Repair

v0.0.49 repairs and expands the Games runtime path found during owner testing while preserving the accepted v0.0.48 embedded-emulator foundation.

- Adds a Nougat-managed Atari 2600 Stella runtime and keeps Atari gameplay embedded inside Nougat's native Video Player instead of exposing a separate desktop emulator window.
- Strengthens X11/XWayland emulator-window ownership, hides captured emulator windows from the taskbar/pager, and preserves the top-edge emulator options bridge.
- Repairs Atari artwork matching for preservation-set filenames with persistent Libretro directory caches, conservative normalized aliases, and prepared artwork that survives Nougat rebuilds.
- Filters same-game variants so USA is preferred, then another English release, then foreign-only releases; newer final revisions win within the selected region tier.
- Removes queued Games mouse-wheel coasting and limits scrollbar-drag redraw so the list follows current pointer movement instead of stale event backlog.
- Adds Sega Genesis / Mega Drive, Master System, and Game Gear ROM discovery inside ZIP libraries with managed BlastEm runtime support.
- Treats DOS ZIP packages as one game, safely extracts the complete package only when launched, chooses a safe EXE/COM/BAT entry point, and reuses the private extracted cache when the source is unchanged.
- Repairs Russia-24 World TV source acceptance so a candidate must provide both video and audio, while preserving the existing probe behavior for other stations.
- Repairs visible and diagnostic v0.0.49 identity and preserves accepted NES/SNES, DOS, Xbox 360, Atari800, World TV, Secure Search, Library, Live TV, Stream, Studio, P2P, diagnostics, and desktop behavior outside this scope.
- The LAN Web Player remains deferred beyond v0.0.49.

## v0.0.48 - Embedded Emulation, DOS, Xbox 360, and Atari Repair

v0.0.48 expands Games around one Nougat-owned embedded emulator host inside the native Video Player and repairs the emulator integration found during owner testing.

- Adds a shared X11/XWayland emulator host so supported emulator backends run inside Nougat's Video Player instead of falling back to separate desktop windows.
- Adds DOS folder discovery and a pinned DOSBox Staging 0.82.2 runtime, including per-game configuration support and repaired DOS launcher handling.
- Adds Xbox 360 `.iso` and extracted `default.xex` discovery with the pinned Xenia Canary Linux runtime. Large Xbox images are deliberately not unpacked from ZIP archives.
- Hardens foreign X11 window handling against disappearing-window/`BadWindow` races so an emulator closing cannot take the entire Nougat process down.
- Reasserts embedded-child geometry after the final player layout is calculated, repairing the small/offset emulator-window presentation.
- Repairs Mesen presentation so the game occupies the Nougat player correctly while its menu remains hidden during play and becomes available by moving the pointer to the top edge. Owner testing verified this behavior with NES.
- Expands Atari recognition: `.bin` is recognized as Atari 2600 for the current supported backend set, ordinary `.xex` is Atari 8-bit, only `default.xex` is Xbox 360, and `.sta` save-state files remain excluded from the game library.
- Preserves the accepted v0.0.47 World TV, Secure Search, Live TV, Library, Stream, Studio, P2P, diagnostics, desktop identity, and fullscreen-control behavior.
- Nougat does not bundle copyrighted commercial ROMs or game images. Users supply their own legally obtained game dumps.

## v0.0.47 - World TV Channels, Playback Reliability, and Desktop Identity

v0.0.47 expands World TV as a normal channel surface and repairs owner-tested playback and desktop-identity failures.

- Adds Al Quran Al Kareem TV, including its Kaaba live view, as an ordinary World TV channel using the official Aloula live source resolver.
- Adds a larger Russian television group including Russia 24, Russia 1, Russia-K, Channel One Russia, NTV, MIR 24, RBC TV, TV Center, Big Asia, Duma TV, and Vmeste RF, plus additional Turkish and Asian channels.
- Treats World TV like television: real station/network artwork is required before a channel is exposed, and available current-program guide information is shown in the channel list and player identity.
- Adds alternate-source resolution, pre-play visible-video probing, startup verification, stall/buffering detection, and bounded reconnect so dead/black feeds fail truthfully instead of sitting on a black screen.
- Shows the World TV station identity and logo in the native Video Player when the mouse is active, matching the Live TV player behavior.
- Restores true-fullscreen mouse transport controls as three approved-sheet square buttons: [<] rewind, [^] play/pause, [>] fast-forward.
- Gives World TV its own orange page/tab color family instead of sharing Live TV teal.
- Repairs top-tab scrolling so the System tab can be fully exposed.
- Repairs GNOME dock/app-switcher identity by installing and validating the canonical Nougat desktop entry and approved N icon, in addition to the existing executable icon metadata.
- Preserves the v0.0.46 scanner, Secure Search privacy, LAN foundation, native-player, Library, Discover, Stream, Studio, Games, P2P, and diagnostics behavior outside this approved scope.

## v0.0.46 - World TV Repair, Scanner Control, and LAN Media Foundation

v0.0.46 repairs owner-reported release and runtime failures while preserving the v0.0.45 Secure Search privacy contract.

- Repairs X11/libVLC window ownership so the World TV video drawable is unmapped outside the native player and cannot cover the World TV station list.
- Adds a verified World TV startup window: a successful libVLC `play()` request is no longer treated as proof of playback; Playing/Paused verifies startup, while timeout/error states enter bounded reconnect and then report a truthful failure.
- Turns the Virus Scan `Scan Again` control into a real `Stop Scan` control while a scan is running.
- Runs the security worker in its own process group and terminates that complete Nougat-owned group on Stop Scan or app shutdown, preventing orphaned capa/ClamAV children.
- Preserves partial scan output on cancellation and reports `SCAN CANCELLED` instead of pretending the scan completed.
- Separates live **Threats** and **Suspicious** counts instead of combining both into one ambiguous `flagged` number.
- Reworks collection/system scanning around one bulk ClamAV collection pass, broad fast checks, and deep capa/reputation analysis only when an indicator justifies it.
- Adds the versioned LAN media-service foundation for catalog/history, direct media delivery, future HLS/transcoding, pairing, and a local web UI, with `nougat.local` reserved for friendly discovery and direct local-IP access retained as the fallback.
- LAN architecture is WAN-independent by rule: no cloud account, cloud relay, automatic UPnP/port-forwarding, or Internet round trip is required for locally owned media.
- Adds permanent release gates requiring exactly one root Nougat executable, the current `Nougat_Media_Suite_v46`, and the approved Nougat N custom icon on that final binary after the last copy/write.
- Adds a README order gate: product heading and full suite introduction first, current version notes second, older version history afterward.
- Preserves v0.0.45 Secure Search fail-closed behavior and accepted player, Library, Discover, Live TV, Stream, Studio, Games, P2P, diagnostics, and media-server behavior outside this repair scope.

## v0.0.45 - Secure Search Foundation

Nougat Search now has a fail-closed privacy foundation. User queries are sent to a local no-network search worker through private stdin IPC instead of process arguments, automatic live-discovery fallback is removed, crawler networking is separated from user Search, and plaintext remote peer Search is disabled until a versioned Privacy Broker transport can satisfy Nougat's privacy policy. The crawler now uses a truthful Nougat Search identity and reports access restrictions such as robots policy, bot-policy blocks, rate limits, authentication, payment-required responses, feeds, and temporary unavailability.

The v0.0.45 architecture deliberately leaves production OHTTP/multi-relay transport, PIR/homomorphic private retrieval, mix/batching defenses, post-quantum transport, browser containment, and signed relay-directory work behind replaceable interfaces rather than hard-wiring today's mechanism into the application.

Nougat Media Suite is the new official identity of the Linux media application previously released as ReddMedia through accepted v0.0.20. It combines native local playback, a hidden local Jellyfin catalog foundation, local recommendation AI, optional TMDb discovery, decentralized Search, multi-platform Stream URL handling, and built-in P2P transfer/streaming in one desktop application. It also includes native Live TV, World TV, Studio, and an integrated Games library with persistent ROM folders, ZIP ROM discovery, automatic game-system and emulator selection, controller support, game artwork support, and expanded Nintendo and Atari system support. Nougat also includes responsive multi-row media-library browsing, security analysis, diagnostics, and integrated media-server controls. World TV is now a dedicated top-level international television area using direct non-YouTube broadcast feeds with a 1080p ceiling, and Games includes pinned MesenCE, RMG, and Atari800 emulator runtimes, Library-identical multi-row cards, automatic artwork retrieval, and right-click card actions shared across Nougat card surfaces.

## v0.0.44 - World TV, Playable Games, Artwork, and Global Card Actions

v0.0.44 turns the v0.0.43 foundations into directly testable television and game-library paths.

- Moves World TV into its own top-level tab immediately after Live TV; Live TV remains the local OTA/tuner area.
- Removes the YouTube World TV catalog and replaces it with direct international linear HLS broadcaster/CDN feeds.
- Caps adaptive World TV playback at 1080p and uses direct libVLC playback with lower startup caching, HTTP reconnect, and bounded automatic reconnect after an unexpected live-stream end.
- Adds pinned MesenCE 2.2.1 Linux x64 for NES, SNES, Game Boy, Game Boy Color, and Game Boy Advance; RMG 0.9.0 for Nintendo 64; and Atari800 7.1.2 x86_64 for Atari 5200 and Atari 8-bit, while retaining compatible installed backends for Atari 2600/7800/Lynx.
- Makes Games use the same multi-row card geometry and vertical scrollbar behavior as the Media Library.
- Adds automatic visible-card artwork lookup through the Libretro Named_Boxarts catalog while preserving local sidecar artwork as the first choice.
- Packages verified artwork for both legally redistributable bundled NES test titles during installation; Waveforms artwork is generated from the actual MIT-licensed ROM with MesenCE.
- Adds right-click context menus to Home, Library, Discover, Live TV, World TV, and Games cards with actions appropriate to each card type, including Open Source where a local source exists.
- Preserves the exact approved Nougat N icon gate and existing behavior outside this approved scope.

## v0.0.43 - Games, World TV, and Responsive Grid Repair

v0.0.43 builds on v0.0.42 and expands the current Nougat Media Suite feature set.

- Expands Games with Grid and List views.
- Adds system and source badges to game entries.
- Adds double-click game launching.
- Keeps linked ROM folders persistent between Nougat sessions.
- Adds ZIP-contained ROM discovery and safe private-cache extraction.
- Adds automatic recognition for NES, SNES, Game Boy, Game Boy Color, Game Boy Advance, Nintendo 64, Atari 2600, Atari 5200, Atari 7800, Atari 8-bit, and Atari Lynx formats.
- Adds automatic selection among supported emulator backends.
- Adds local game box-art and sidecar artwork support.
- Preserves the legally redistributable bundled NES starter games and their upstream license records.
- Adds recognition of the tested Manta USB gamepad.
- Expands World TV with additional legitimate international broadcaster sources, including Al Jazeera Arabic and DW News.
- Keeps World TV language-agnostic without Nougat filtering countries or languages according to device location.
- Repairs the Movies and TV Library responsive grid so poster browsing can use multiple rows instead of collapsing into one long row.
- Repairs Discover My Services with a wider draggable scrollbar, dedicated scrollbar gutter, and improved provider-row spacing.

## v0.0.42 - Persistent Libraries, Live TV Maintenance, Security, Intelligence, and Games

v0.0.42 builds from the sealed v0.0.41 baseline and preserves the accepted native-player and page behavior outside the owner-approved scope.

- Preserves Movies/TV folder mappings in a Nougat-owned recovery registry while keeping Jellyfin's legacy-compatible persistent state paths.
- Improves metadata identification with filesystem/season structure evidence and exact provider-ID matching, while adding reusable recommendation embedding caches.
- Repairs Discover Live TV locking, My Services header/scrollbar behavior, Library/Home card fill, and the Live TV to local-media playback handoff.
- Keeps Live TV guide data persistent, performs stale guide maintenance when the tuner is idle, and shows a pulsing `Refreshing` state during active guide work.
- Adds `World TV` for verified official internet broadcaster sources without geolocation filtering or DRM/geoblock bypass.
- Expands Virus Scan with Movies, TV, Quick, and System scans while preserving the existing controls.
- Adds the top-level `Games` tab after Studio, persistent user ROM folders, backend discovery, controller visibility, and a playable emulator-launch path.
- Bundles/pins legally redistributable NES starter content with upstream licensing preserved by the installer.
- Repairs the Video Player context-menu subtitle row so the full `Subtitles On / Off` row toggles and the active state word is bold.
- Raises the volume and transport rows slightly while leaving the approved seek bar untouched.

## v0.0.41 - Housekeeping, Archives, IMDb, Live TV and Player Activity Repair

v0.0.41 builds on accepted v0.0.40 without changing Nougat's native-player rule or accepted page identities.

- Search gains one `Archive` tab rather than one top-level button per archive site.
- Archive.org, Minerva Archive, and the curated archive/library directory open in the system browser.
- Library Movies and TV use the repaired shared poster-card geometry without height-driven shrinking.
- Movies, TV series/episodes, and Home cards show `IMDb` only when Nougat has an exact IMDb ProviderId for that title.
- Scrollable Search result cards fill their available result viewport cleanly.
- Live TV gains `Stop Live`; stopping live playback releases the tuner and lets a queued full guide sweep continue.
- The video identity overlay and mouse pointer now share the same three-second activity window in fullscreen, maximized, and normal window sizes.
- The Home LAN phone-browser media viewer/streaming foundation remains roadmap work after v0.0.42.

## v0.0.40 - System Loading, Live TV Tuner Navigation, Search Repair, and Crawler UI

v0.0.40 builds directly on accepted v0.0.39 and repairs system-wide loading presentation, Live TV tuner administration, Nougat Search behavior, executable branding, and Crawler controls.

### v0.0.40 changes

- Replaces the oversized percentage loading surface with a thin 3 px caramel loading sliver directly below the global header.
- Removes percentage text and continuously rolling indeterminate loading animation.
- Live TV controls are ordered Guide, Detect Tuner, Refresh Tuner, Scan Channels, Watch Live, Refresh Guide, Record.
- Detect Tuner opens a dedicated Tuners page; Refresh Tuner refreshes that page; Guide returns to the normal guide.
- Applies the approved Nougat N artwork to the actual root executable through GNOME custom-icon metadata.
- Repairs existing Nougat Search indexes with an FTS rebuild migration.
- Improves multi-word Search matching so useful partial-term results are not discarded.
- Adds keyless clearnet bootstrap discovery when the local and peer index does not provide enough useful results.
- Arranges the Search row as Search field, SEARCH, RAW.
- Makes Crawler Max Pages a deliberate visible minus, value, plus control.

## v0.0.39 - Deep Diagnostics and Live TV Guide Reliability

v0.0.39 builds directly on accepted v0.0.38. It focuses on evidence-based diagnostics and reliable native ATSC guide behavior without relocating approved UI.

### v0.0.39 changes

- Diagnostic Center now reports subsystem health instead of allowing low-severity content-quality statistics to poison the entire suite status.
- The summary separates Passed, Needs Attention, Problems, Not Tested, and Information and exposes observed evidence, expected state, and suggested action per check.
- Library diagnostics use a recursive catalog query for real Movie/Collection/Series/Season/Episode counts; missing descriptions/posters remain informational.
- Search idle is explicitly Not Tested rather than a fault.
- Live TV diagnostics understand tuner nodes, tuner-use state, signal lock/quality, current channel/program, guide coverage/freshness, current-multiplex harvesting, and queued full-guide refresh.
- Cached guide data loads automatically and is merged with longer PSIP EIT/VCT collection passes rather than being discarded when a broadcaster omits a table during one cycle.
- While watching Live TV, Nougat harvests PSIP from the active RF multiplex without retuning; the single WinTV frontend queues other-frequency refresh work until idle.
- Bundled real network-logo coverage is expanded and channel-logo cards are square.
- The accepted player layout and VOLUME component remain untouched; only the explicitly rejected progress-bar bottom strip/stitch clarity and Live TV timing color are repaired.
- Diagnostic JSON/TXT/support bundles are retained, with private diagnostic-history snapshots stored locally for comparison.

## v0.0.38 - Library, Live TV and Player Exact-Sheet Polish

v0.0.38 continues from accepted v0.0.37 and focuses on the explicitly approved owner-visible repairs without changing the proven Live TV playback pipeline or relocating unrelated UI.

### v0.0.38 changes

- Library Search behaves as a true focused text field with visible dark text/caret, immediate typing, select-all/backspace behavior, and empty-query reset, in its existing approved position.
- Continue Watching admits local movies/episodes after 10 seconds of actual playback, filtering out shorter accidental opens.
- Library cards prioritize complete titles, wrap when practical, and move codec/resolution/audio details into a Nougat-style hover/focus popup.
- The loading/progress bar is back in its original system-wide lane directly below the top navigation. Determinate loading uses the literal approved-sheet PROGRESS BAR; its height is only large enough to contain the percentage, and the percentage rides inside the moving caramel fill.
- Player pointer and title/program information share one three-second activity timer in normal, resized, maximized and fullscreen playback.
- The approved-sheet seek and volume artwork remains the authority. Seek retains its stitched artwork and drops only the separate pale underside crop-shadow. Volume keeps its current centered geometry and artwork, drops only its small pale underside crop-shadow, and restores the percentage text to black.
- Existing mouse-wheel scrolling remains unchanged. Visible approved-sheet-style vertical scrollbars are added to vertically scrollable surfaces that lacked one, including Discover, Live TV Guide, Search results/Crawl/Peers, and System diagnostics; Library/Home retain their existing scrollbars.
- Live TV uses Guide as the single guide-view control. The redundant Channels button is removed and the guide grid remains the default Live TV surface.
- Detect Tuner and Refresh Tuner move to System with the other system/hardware controls. Scan Channels, Watch Live, Guide, Refresh Guide, and Record remain on Live TV.
- Live TV channel icon slots use locally packaged real network/broadcaster marks where identified (NBC, PBS, PBS Kids, Telemundo, ABC, CBS, FOX, CW, MeTV, ION, ION Plus, Create, Bounce, Busted, ShopLC). Unknown stations use call-sign text rather than duplicate numbered badges.
- Live TV retains remembered selection, Up/Down + Enter tuning, current-program identity, and aligned broadcast program timing.
- Installer baseline validation is fail-closed so a hash mismatch cannot print FAIL and later report the same preflight as passed.

## v0.0.37 - Native Live TV Watch + Classic Guide + System/Visual Repair

v0.0.37 builds on accepted v0.0.36. It keeps the validated 66-channel ATSC scanner intact, finishes the first owner-testable Watch Live path, adds the first old-school TV-guide grid from broadcast PSIP EIT data, and carries forward the owner-requested Home/player/header/System corrections.

### v0.0.37 changes
- Rename the top-level `Debug` tab to **System**. Move `Start Server`, `Stop Server`, and `Refresh Server` out of Library into System with diagnostics/log/export tools.
- Keep Library focused on media/catalog work; the v0.0.36 sheet-style `Search` row and collection hierarchy remain intact.
- Make Continue Watching use the same physical Home card/poster geometry as the regular recommendation cards while retaining horizontal shelf scrolling, resume progress, and watch-history behavior.
- Extend the approved exact-sheet seek component across most of the player width while keeping elapsed time on its left and total time on its right. Preserve the native end caps and circular knob while stretching only repeatable track spans.
- Remove the opaque white halo/edge artifact around the seek and VOLUME sheet sprites without replacing their approved artwork.
- Replace the generic Server light with the stitched circular sheet-style state button: the whole face changes green/amber/red by server state.
- Collapse Linux DVB/V4L2/VBI exposure into owner-visible logical tuner rows. A usable DVB frontend represents the physical tuner; sibling raw video/VBI nodes are not shown as fake additional tuners.
- Add channel selection plus **double-click to Watch Live**. The `Watch Live` button invokes the same path. Persisted frequency/program metadata is translated into an ATSC libVLC input so the broadcast opens in Nougat's native player.
- Add explicit tuner-use ownership states (`Idle`, `Scanning`, `GuideRefreshing`, `Watching`) so scanning/guide harvesting cannot steal a tuner while live playback owns it.
- Add ATSC PSIP EIT guide harvesting and a persisted guide cache. Existing v0.0.35/v0.0.36 channel files are enriched with RF/program/source metadata when guide refresh sees the VCT.
- Add the first classic TV-guide grid: channels down the left, half-hour time slots across the top, duration-sized program blocks, current-program highlighting, a current-time marker, `Channels`, `Guide`, `Refresh Guide`, and `Now` controls.
- Keep `Record` as an explicit future DVR hook; v0.0.37 does not pretend recording/timeshift is implemented.

## v0.0.36 - Library Hierarchy, Home Artwork, and Exact-Sheet Player/Header Repair

v0.0.36 builds on accepted v0.0.35 and focuses on owner-visible Library/Home/player polish without reworking the already validated ATSC channel scanner.

### v0.0.36 changes
- Add a dedicated Library search field directly below the green action row. It uses the approved sheet INPUT FIELD treatment, displays the placeholder `Search`, and filters the current local Library view by title/series/episode text while preserving the fixed far-right List/Grid pair.
- Enforce movie collection hierarchy client-side as well as through Jellyfin hints: member movies belonging to a BoxSet/collection are removed from the top-level Movies view, the collection remains as the single root card, and opening it shows its films in production-year/name order.
- Repair Home artwork rendering by isolating offscreen card rendering from the page/shelf X11 clip. Continue Watching fills its landscape 16:9 artwork viewport proportionally; Local/Recommended cards fill their portrait poster viewport without the large black gaps caused by the leaked window clip.
- Replace the generic seek drawing at normal owner geometry with a pixel-derived frame family built from the literal approved `SEEKBAR (PROGRESS)` component. The bar is kept at the sheet-derived 378 px width so elapsed time can sit at its left and total time at its right on the same line.
- Make the player control strip one stable repaint unit from seek through transport controls. Seek/time, exact VOLUME housing, percentage, and buttons now redraw together so mouse movement/dragging cannot alternate between clipping the volume top and clipping time/percentage text.
- Preserve the correct approved-sheet VOLUME artwork while clipping away only the rectangular sheet background outside its rounded housing.
- Change the global header material from cream to the tan sampled from the approved VOLUME housing, vertically center the left brand and right Server/version clusters, and replace the generic server dot with the sheet-family circular status indicator.
- Preserve the current lettering/font for a later system-wide typography release rather than changing fonts piecemeal.
- Carry the validated v0.0.35 native ATSC scan forward unchanged as a regression baseline. Native `Watch Live` tuning/playback from persisted channels is assigned to the next build agenda.

## v0.0.35 - Code + Bug Cleanup, UI Alignment, Live TV Scan, and Studio Foundation

v0.0.35 is the stabilization/cleanup release after accepted v0.0.34. It repairs confirmed code and UI defects, strengthens validation that previously allowed false-positive sheet-fidelity PASS results, advances Live TV into a real owner-testable ATSC channel scan, and establishes the new Studio workspace in the top navigation without displacing the cleanup mission.

### v0.0.35 changes
- Repair Nougat Search/Crawler worker lifetime handling so App-owned workers are joined during shutdown instead of being detached across App destruction.
- Repair the player against the approved sheet itself: the seek track keeps timestamps beneath its ends, while Volume now renders from a sheet-pixel-derived 335x47 VOLUME sprite family with the exact cream housing, speaker artwork, caramel/cream track, circular knob treatment, and percentage outside the housing.
- Make Search use the same sharp outer page-frame corners as the other top-level pages while retaining rounded inner controls and panels.
- Use Stream's existing top-inner-control row as the app-wide vertical reference. Search, Debug, Live TV, Discover, and Library now share that baseline; future Studio controls inherit the same ruler.
- Fix narrow-window control reach and post-tab-resize centering: the Video Player's complete 8-button transport row is centered as one group at normal/full widths and scrolls fully to the final action when narrow; Debug's 10-action strip also reaches its final action. Library's header tools stay in one horizontal scrollable row instead of wrapping, while List/Grid remain fixed together at the far right.
- Remove the Live TV hardware-description subtitle and advance the Linux DVB backend from detection into a native ATSC 1.0 over-the-air scan across RF channels 2-36, with lock/signal/quality progress, PSIP VCT service discovery, channel persistence, and honest failure/cancel status.
- Add the top-level Studio tab between Stream and Debug with a true yellow/gold palette with brown stitched borders; the page is branded Gold Studio internally. v0.0.35 provides the page/navigation foundation; the media-processing toolset remains roadmap work.
- Scale the selected top-tab downward pointer to match the enlarged tab bodies and repaint it as final chrome so page backgrounds/loading strips cannot erase it.
- Initialize the complete seek/volume partial-repaint strips before copying them onscreen, preventing the black horizontal band that could appear during player pointer/seek refreshes while preserving the now-stable Up Next overlay behavior.
- Preserve v0.0.34 server persistence, security runtime, P2P, Discover, Library, Stream, licensing, and user-data behavior outside the cleanup scope.

## v0.0.34 - Exact Sheet Tabs/Player Controls + Home/Discover UI Repair

v0.0.34 builds directly on accepted v0.0.33 commit `6763a42bf5c125974e5a2882234fb2ee2e04c512`. It is a focused owner-visible UI/Discover release. The approved Nougat concept sheet is the literal component authority for the global top tabs, player seek bar, and player volume control.

### Owner-approved v0.0.34 changes
- Global top navigation uses the actual concept-sheet tab proportions and construction instead of thin pill buttons: squarer rounded body, double bevel/inset seam, tight inter-tab spacing, and the selected downward pointer. Existing per-page colors remain.
- The entire scrollable top-tab lane is shifted left to sit close to `NOUGAT MEDIA SUITE`; the fixed right-side `Server / dot / version` area is deliberately unchanged.
- Video Player seek bar now follows the actual sheet seek component: slim cream track, caramel progress, round knob, timestamps below.
- Video Player volume now follows the actual sheet volume component: compact cream housing, left/right speaker glyphs, inset track, round knob, and percentage readout. Existing 0-200% Nougat gain behavior is retained.
- Home cards use fixed section geometry rather than media-type-driven heights. Continue Watching is a consistent landscape preview shelf; LOCAL recommendations use a consistent portrait-grid template.
- Home and Library scrollbar dragging consumes only the latest X11 motion and stops when Button1 is released, removing delayed/coasting drag behavior.
- Live TV header text and buttons no longer overlap.
- Discover adds a `Live TV` source selector so `Usual + Live TV` and `Random + Live TV` are valid combinations. Until channel/EPG data exists, the UI reports that truthfully instead of inventing recommendations.
- Discover renames `External Movie` / `External TV` to `TMDb Movie` / `TMDb TV`; backend behavior remains TMDb.
- Page-frame corner repair applies to Home, Library, Discover, Live TV, Stream, and Debug. Search and Video Player retain their accepted frame behavior.
- v0.0.33 persistent server, security runtime, P2P Plus, and tuner-detection foundations are retained unchanged.

## v0.0.33 - P2P Plus, Security Hardening, Persistent Server, Page Viewports, and Live TV Foundation

v0.0.33 builds directly on accepted v0.0.32 commit `084ee7ccd82be3a578f738b3bcb6ac8570a573dd`. Normal pages now use a Nougat-owned bordered clipping viewport so scrolling content cannot bleed under fixed chrome; **Video Player is intentionally unchanged**. Home and Library gain hard containment, Library gains a dedicated vertical scrollbar and wrapping toolbar, and the top navigation is clipped between the fixed Nougat brand and fixed Server/version area.

The top-level order is **Home | Video Player | Library | Discover | Live TV | Search | Stream | Debug**. Live TV is a truthful hardware-discovery foundation with a generic `NougatTunerBackend`, Linux DVB/V4L2 probing, channel database/scan interfaces, and first-hardware targeting for the Hauppauge WinTV-HVR-955Q. It does not claim channel tuning/playback yet.

P2P Plus adds Nougat-owned management interfaces for speed limits, seeding ratio/time rules, per-file priority, queue movement, tracker status, Force Reannounce and Force Recheck while preserving v0.0.32 native Watch Now/seek-aware streaming.

Nougat Security Analysis now requires the pinned one-shot runtime (YARA-X 1.19.0, capa 9.4.0 + matching rules, Magika 1.0.3). Missing required/relevant engines produce **ANALYSIS INCOMPLETE**, never a false clean result. Free abuse.ch MalwareBazaar/ThreatFox/URLhaus reputation remains optional through the owner's free **Threat Intel Key**. ClamAV remains an optional external one-shot second opinion. No resident scanner, filesystem watcher, automatic quarantine, deletion, move, rename, or open action is installed.

The integrated Jellyfin server is now persistent after **Start Server**: closing the Nougat UI leaves a Nougat-owned server running, reopening adopts it, and **Stop Server** is the explicit shutdown path. Independently started Jellyfin is never claimed or killed.

## v0.0.32 - Native P2P Media + Nougat Security Analysis

v0.0.32 keeps the native `Search > P2P` media workflow and adds a fourth Search subsection: `Virus Scan`. P2P accepts magnets and local `.torrent` files, selects playable media, streams through Nougat's localhost HTTP Range bridge into the native player, reprioritizes pieces around playback/seeks, and now reports complete-seed/availability evidence more clearly.

Nougat Security Analysis is deliberately on-demand rather than resident. v0.0.32 establishes the scaffold: manual **Scan File** / **Scan Folder** actions and completed P2P downloads launch a one-shot worker for SHA-256 hashing, file/content identification, extension/type checks, built-in rule plumbing, scan history, optional external one-shot ClamAV, and optional free/community reputation hooks. Pinned YARA-X/capa/Magika integration points are present, while installation of their full runtime is intentionally deferred to the next security-hardening pass. The worker exits after the requested scan. Nougat installs no antivirus daemon and no always-on filesystem watcher.

The owner policy is **WARN ME FIRST**. A finding never causes Nougat to automatically quarantine, delete, move, rename, or open a file. The report distinguishes `NO THREATS DETECTED`, `SUSPICIOUS`, and `THREAT DETECTED`, shows which engine produced the evidence, and keeps private scan history under the user config tree. Generated scanner runtimes and credentials are excluded from Git.

This same-version replacement also removes the stray normal-page Node ID, moves only the existing Crawler status sentence upward without altering the Crawler layout, keeps the autoplay no-flash/Search-seam/Stream-border repairs, replaces the oversized volume housing with the shorter Seek-style track, clips Home content below the fixed header, and adds explicit vertical Home and horizontal Continue Watching scrollbars. Advanced torrent-management work is assigned to **v0.0.33 P2P Plus**.

## v0.0.31 - Exact Approved UI Sheet Components

v0.0.31 is deliberately a UI-component-only release on the accepted/published v0.0.30 baseline. The exact approved sheet is stored as `docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png` so the visual authority travels with the source and build records. The owner-approved Nougat component sheet is the literal authority for control construction. If an ordinary Nougat control still reads as a generic rounded rectangle instead of the sheet's raised stitched/beveled component family, it is replaced in this release.

The component renderer also uses a sheet-derived runtime surface texture sampled from the approved PRIMARY component, then recolors that lighting/depth pattern with each page's existing palette. The accepted page palettes do **not** change. Home remains purple, Video Player cocoa/chocolate, Library green, Discover red, Search cream, Stream provider-reactive, and Debug charcoal. v0.0.31 changes component geometry, bevel/depth, shadows, inset seams, selected tab pointers, fields, panels, icon buttons, checkbox treatment, progress surfaces, seek/volume tracks, and slider knobs while preserving those colors and v0.0.30 behavior.

Home card proportions and artwork remain the accepted v0.0.30 design. Media playback, Library metadata/cache behavior, Discover logic, Search/P2P transport, Stream extraction/playback, TV autoplay, diagnostics, licensing, and user-data paths are outside this release's implementation scope. The focused P2P streaming expansion moves intact to v0.0.32.

The longer roadmap also includes native Live TV/NextGen TV and Radio/SDR work: owner HVR-955Q support, HDHomeRun, ATSC 3.0-capable hardware such as FLEX 4K, and receive-only AM/FM/weather/CB paths through compatible tuner or SDR hardware.

## v0.0.30 - UI Cohesion, Library Performance, and Player Navigation

v0.0.30 builds on the owner-accepted/published v0.0.29 baseline. It makes the native pages feel like one visual system, speeds the Library's perceived metadata load with a persistent cache-first path, makes Library Grid use multiple visible DVD-poster rows, and adds concise `Previous` / `Next` episode navigation to the native player.

Large primary content surfaces on Library, Discover, Stream, and Debug now use the same rounded/inset panel language already established by Search. The active top-tab pointer remains fully visible above page work because the gold busy/progress strip is positioned below the navigation notch rather than painting across it. Home movie/series/season artwork uses portrait 2:3 poster geometry like Library, while episode stills remain landscape.

Library metadata views can display a private persistent cached snapshot immediately, then reconcile against Jellyfin in the background. `Refresh Library` remains the operation that asks Jellyfin to scan the library; its server-side scan phase is honestly indeterminate because the current Jellyfin request does not provide completed/total counts. Once Nougat has an actual item total for metadata/artwork enrichment, the existing gold bar shows a real completed/total-derived percentage. `Refresh Server` refreshes server/process status and does not pretend to be a library metadata scan.

The player control row is `Open | Rewind 10s | Previous | Play/Pause | Next | Fast Forward 10s | Stop | Fullscreen`. Previous/Next use the accepted v0.0.29 episode queue/resolver and disable at boundaries. The compact 0-200% volume slider is retained with a larger, properly proportioned knob.

The focused P2P streaming expansion is intentionally v0.0.32. v0.0.30 does not modify the accepted Search/P2P engine or transport implementation.

## v0.0.29 - TV Playback, Navigation, and Carry-Forward UI Repair

v0.0.29 builds on the owner-accepted/published v0.0.28 baseline. TV playback now resolves the next local episode from the current file's own folder regardless of whether the episode was opened from Home, Library, Open File, resume history, or another local route. Nougat prefers parsed season/episode identity such as `S01E13 -> S01E14`; for catalog-confirmed episodes without those tokens it uses natural filename order. The next-episode queue is prepared when playback starts instead of waiting for end-of-media.

At natural episode completion, the Up Next overlay appears immediately with the resolved next episode and a visible 10-second countdown. `Play Next` starts it immediately, the countdown autoplays it at zero, `Replay` restarts the completed episode, and `Back to Series` resolves the actual Jellyfin series/season context when available instead of dropping to the generic Library root. Manual Stop continues to cancel autoplay. A real executable self-test now exercises same-folder episode ordering, natural filename fallback, and the 10-second Up Next state rather than merely checking that source tokens exist.

This release also carries forward the owner-visible v0.0.28 polish defects intentionally deferred at acceptance. Home no longer swallows top-header wheel events when Home is selected, so the top tab strip scrolls horizontally just as it does on other pages. Vimeo is added immediately after YouTube in Stream using Vimeo's current blue/black/white brand family while retaining Nougat's provider-reactive Stream behavior. The Video Player page background now surrounds the video uniformly without the partial brown rail/matte.

Home artwork is strengthened again: Continue Watching TV episodes prefer an exact episode Primary/still image, then the matching season poster, then series artwork, including resume records created outside the Library hierarchy when the series can be recovered by exact path containment. Movie/poster artwork fills the card artwork region with aspect-preserving cover behavior rather than appearing as a tiny centered stamp. Existing rounded top clipping and one-at-a-time silent hover previews remain intact.

The focused P2P streaming expansion is now v0.0.32.

## v0.0.28 - Candy Palette, Artwork, and UI State Polish

v0.0.28 builds on the owner-accepted/published v0.0.27 baseline. The main quilted page background now carries each native area identity instead of every page reading as cream: Home is purple, Video Player uses cocoa/chocolate/caramel, Library is green, Discover is red, Search remains the only cream-background native page, Stream stays provider-reactive, and Debug is charcoal/licorice. Each area stays restrained to roughly two or three coordinated colors with cream used selectively for readable trim, text, tracks, and panels.

Home now behaves as a persistent desktop surface. Switching to another tab and returning does not rebuild the feed when its data is still current, preserving loaded recommendations, Continue Watching state, artwork, and scrolling. Home cards are poster-first at rest; movie cards use the movie poster, while TV Continue Watching resolves the matching season poster and then the series poster as fallback. The existing silent local hover preview remains and returns to poster art when the pointer leaves. Rounded card artwork and hover frames are clipped through the same top-corner mask so square image pixels cannot protrude through the curved border.

The LOCAL wall uses larger section/category lettering and more readable card metadata. Its responsive layout guarantees at least three recommendation cards across at the owner's roughly 650-pixel half-screen width and fits more as the window widens. The legacy X11 text path translates metadata bullets safely so `2012 • Comedy • Romance` no longer becomes mojibake.

Library poster presentation now prefers proper portrait artwork from an exact catalog TMDb ID when available, falls back to Jellyfin Primary artwork, requests display-useful poster resolution, rejects tiny or landscape results, and preserves portrait aspect instead of stretching arbitrary images into the tile. A deliberate `NO POSTER` state is used when no acceptable art exists.

The Video Player replaces the remaining pale/white windowed-video halo with a dark cocoa/chocolate theater surround and restrained caramel trim. Search removes the redundant standalone `SEARCH` heading and moves its real controls upward while retaining `Search | Crawler | P2P`.

The owner-observed TV Up Next/autoplay and `Back to Series` regressions are intentionally split into v0.0.29 so this release remains a UI/artwork/state build. The focused P2P streaming expansion follows in v0.0.31.

## v0.0.27 - Home, Resume History, Player Polish, and Seek Previews

v0.0.27 builds on the accepted/published v0.0.26 baseline. A new **Home** tab is first in the top navigation and is the default landing page. Home starts with a horizontally scrollable **Continue Watching** shelf driven by persistent per-title resume history. The shelf uses the mouse wheel for left/right movement only while the pointer is over it, shows caramel progress bars, and keeps all unfinished movies/episodes available rather than limiting the list to only a few recent items. Below it, **LOCAL** presents mixed movies and TV in a normal vertically scrolling card wall organized into useful genre/category groups plus a personalized recommendation group that learns from local watch history.

Home uses wide 16:9 cards and prefers higher-resolution Jellyfin backdrop artwork, with primary/TMDb artwork as fallback. Card hover waits for intentional pointer dwell, then shows one muted FFmpeg-extracted moving preview at a time; leaving the card restores its artwork.

Local playback now has durable resume state. Reopening unfinished media can offer `Continue | Start Over | Cancel`, while **Stop** presents an intentional stopped state with `Resume | Restart | Load Different | Back to Library`. Resume positions persist across other files and application restarts and completed media is removed from Continue Watching. Windowed/maximized playback gets rounded video corners; true video fullscreen remains square. The current movie or TV identity is kept visible in the compact strip below windowed video and appears temporarily with mouse activity in true fullscreen.

The seek bar now supports an actual-frame hover preview with timestamp and real chapter name when real chapter metadata applies. Preview extraction runs separately from the active libVLC player and uses a bounded cache. The v0.0.26 pointer-motion flicker path is repaired by repainting only on meaningful hover-state changes instead of scheduling whole-window redraws for raw X11 motion packets. The selected top-tab notch now paints cleanly over the header divider. Redundant `DISCOVER USUAL / DISCOVER RANDOM`, `Direct Play URL`, and `DIAGNOSTIC CENTER` headings are removed; the Stream field placeholder is exactly `Paste URL Then Press Direct Watch / Rumble / RuTube / VK / OK`.

The page-background/artwork/state polish discussed during owner testing is implemented in v0.0.28. The TV Up Next reliability repair is split into v0.0.29 and the focused P2P expansion into v0.0.32.

## v0.0.26 - Systems, Navigation, Diagnostics, and TV Up Next

v0.0.26 builds on the owner-accepted v0.0.25 release. Mouse side buttons now navigate Nougat's internal history (Button 8 Back, Button 9 Forward), the Library root header drops the redundant `MEDIA LIBRARY` label and places the List/Grid view controls at the far left, and the fixed header identity/status layer remains anchored beneath the horizontally scrolling top tabs.

The approved Nougat N artwork receives a perimeter cleanup that removes the tiny lower-edge light sliver while preserving the approved emblem itself across the icon family and embedded window/header icon data. The Video Player keeps its intentional 0-200% volume range, but the Volume label, existing slider, and single correct percentage are centered as one group; the duplicate percentage and rejected square/triangle speaker glyphs are removed.

Debug becomes the **Nougat Media Suite Diagnostic Center**. It gathers evidence from Nougat, the host system, Jellyfin, Library metadata/paths, libVLC playback, Search, the existing P2P core, local AI/TMDb, and Stream. Reports can be copied or exported as human-readable TXT, structured JSON, or a redacted support bundle. Green is reserved for checks that returned healthy evidence; missing or unavailable evidence is kept explicit rather than invented.

At natural TV episode completion, Nougat resolves the real next episode before presenting an **Up Next** overlay. The overlay shows a visible 10-second countdown and `Play Next`, `Back to Series`, and `Replay`; manual choices cancel the countdown, and an unresolved next episode produces an explicit message instead of silently dead-ending.

v0.0.27 added seek-bar hover thumbnail previews while preserving aesthetic fallback chapter marks. During owner testing, v0.0.28 was reassigned to UI/artwork/state polish, v0.0.29 to TV playback/navigation reliability, and the larger P2P-management expansion to v0.0.32.

## v0.0.26 candidate

This candidate adds internal mouse Back/Forward navigation, cleans the Library header and Nougat icon perimeter, upgrades Debug into an evidence-based Diagnostic Center with TXT/JSON/redacted support-bundle exports, cleans and centers the intentional 0-200% volume control, fixes top-header layering during horizontal tab scrolling, and adds a 10-second TV Up Next overlay with Play Next / Back to Series / Replay. P2P feature expansion was later moved to v0.0.32 after the owner split v0.0.28/v0.0.29 into UI/artwork and TV-reliability releases.
- v0.0.33 server-stop repair: persistent Nougat-owned Jellyfin sessions carry a per-session ownership token; Stop Server terminates the complete owned process tree and verifies port 8096 is released without killing Jellyfin by name.

## v0.0.25 - Stream Provider Theme, Persistent Selection, and Discover Native Play

v0.0.25 builds directly on the owner-accepted v0.0.24 release. The selected Stream provider now drives the entire Stream interior: provider-colored controls, accents, and the exact concept-sheet quilt tinted to YouTube red, Rumble green, RuTube purple, VK blue, or OK orange/caramel. The selected provider also receives the concept-sheet downward active notch.

Discover now exposes its persistent state visually. `Usual | Random` is one selector group and `Local Movie | Local TV | External Movie | External TV` is a second independent group, so one active notch from each group may appear at the same time. Action controls such as TMDb tests and credential/service actions remain ordinary buttons.

`Play in Nougat...` for local Discover results now resolves the selected Jellyfin catalog entry to a real playable local movie or episode before starting the native Nougat player. A series-level TV result resumes the most recently watched matching episode when local history identifies one; otherwise it starts the first real episode in season/episode order.

## v0.0.24 - Search Page UI Polish

v0.0.24 is an owner-approved Search-page-only visual polish release on the accepted v0.0.23 base. It moves the approved N emblem and NOUGAT MEDIA SUITE identity to the far-left header position, removes the duplicate right-side N badge, finishes concept-sheet styling for Search/Crawler/P2P controls and panels, aligns Network... and SEARCH on the same right-side column, and strengthens GNOME launcher/window icon association so the approved N replaces the generic gear fallback. Search-engine behavior is intentionally unchanged in this release.

## v0.0.23 - Exact Concept UI and Stream Direct Watch Repair

v0.0.23 is the owner-approved post-license UI and Stream repair build. The uploaded Nougat Media Suite concept sheet is the visual authority for the application theme: rounded stitched/beveled candy-style controls, cream/caramel/chocolate materials, area-specific tab accents, the active-tab point, quilted page surfaces, concept-style seek/volume controls, and the exact square chocolate/caramel N emblem. Existing control order and media behavior are preserved unless explicitly listed below.

- Top-level order remains `Video Player | Library | Discover | Search | Stream | Debug`.
- The active top tab receives the concept-sheet downward point/notch; the navigation group centers when width permits and remains scrollable when narrow.
- The main player row remains `Open | Rewind 10s | Play/Pause | Stop | Fast Forward 10s | Fullscreen` and centers when width permits.
- The seek and volume fills no longer use the old red theme; they use the concept-sheet caramel/cream/chocolate palette.
- Volume remains 0-200% with the 100% marker, but uses the approved compact concept-sheet geometry.
- Video Player, Library, Discover, Search, Stream, and Debug use the quilted material with the approved subtle page tinting.
- Stream keeps only YouTube, Rumble, RuTube, VK, and OK, uses one shared Direct Play URL field, removes the redundant Stream `Play` button, and keeps `Direct Watch` as the native-player action.
- YouTube Direct Watch now detects supported external JavaScript runtimes for current yt-dlp YouTube challenge solving and uses the updated YouTube extraction/playback fallback path.
- The old candy executable/launcher artwork is replaced by the exact N emblem from the approved concept sheet.

## v0.0.22 - License Protection and Contribution Boundary

v0.0.22 is a legal/release-infrastructure build over the owner-accepted v0.0.21 baseline. It does not add or redesign media behavior. It strengthens the project license boundary around Elderred Softworks LLC Original Materials, preserves third-party licensing, adds explicit copyright/ownership notices, establishes inbound contributor terms, and adds automated release checks intended to prevent accidental relicensing.

The versioned root executable advances to `Nougat_Media_Suite_v22` only so the accepted legal release remains versioned consistently with Nougat Media Suite's release rules. The application's media, Search, Stream, Library, Discover, P2P, server, playback, and palette behavior is retained from v0.0.21.

## v0.0.21 - Official Rename, Candy Palette, and Navigation Repair

v0.0.21 builds directly on the owner-accepted ReddMedia v0.0.20 baseline at commit `c3d2c60e5c36407b96a0eba72e2863f884aacd28`. The release establishes the new product identity and candy color system, then folds two owner-approved UI organization repairs into the same still-unaccepted candidate: P2P moves under Search, and the nonworking Library Grid/List text controls become real view-toggle icons. No media engine, recommendation engine, server engine, or additional Stream service is added by this version.

The visible top-level order is **Video Player | Library | Discover | Search | Stream | Debug**. The former top-level **Nougat** label becomes **Search** because Nougat now names the entire suite. Search exposes **Search | Crawler | P2P** as its ordinary internal sections. Decentralized search-node peer controls are tucked behind **Network...** inside Search instead of occupying a normal user-facing tab.

The application/window/launcher identity becomes **Nougat Media Suite**. The versioned root executable is `Nougat_Media_Suite_v21`. The approved rounded-square chocolate/nougat **N + play triangle** artwork replaces the former ReddMedia red-tree icon for the launcher, dock/app switcher, X11 window, raw executable metadata, and the in-app version badge.

The suite-wide visual system is rebuilt around the candy identity. **Video Player is chocolate/cocoa/caramel**, Library is forest/sage, Discover is plum/lavender, Search is chocolate/nougat/caramel, and Debug is graphite/amber. The top navigation itself uses those area identities instead of one inherited ReddMedia-red strip. Stream is service-reactive: selecting **YouTube, Rumble, RuTube, VK, or OK** changes the Stream interior and its tab color to that service's recognizable palette.

In Library, the old `Grid [x]` / `List [x]` text buttons are removed. At the far left of the Library page are two compact view icons: **three horizontal lines** for List and a **2x2 four-square grid** for Grid. Clicking either changes the actual Library layout and the selected icon is highlighted. Movie and TV view preferences remain independent and persistent.

Existing Stream services remain exactly **YouTube | Rumble | RuTube | VK | OK** in v0.0.21. Vimeo, Dailymotion, Twitch, Kick, TikTok, Bilibili, and Niconico are deferred to v0.0.23. The approved nougat-specific UI polish, poster/artwork overhaul, rounded-corner polish, top-navigation centering, volume-bar geometry changes, the optional local Web Player, and Plex integration are also deferred.

The existing Git repository directory and backward-compatible user-data/config paths remain unchanged so the product rename does not silently become a filesystem/data migration. User-visible application identity is Nougat Media Suite.

## v0.0.20

This historical version does not currently have a detailed release entry in the README. Its source and repository history remain the authority for that version.

## v0.0.19

This historical version does not currently have a detailed release entry in the README. Its source and repository history remain the authority for that version.

### Viewing-history completion repair

This replacement v0.0.19 candidate includes the recommendation/viewing-history source changes required by TV natural-end autoplay. Existing SQLite history databases are migrated in place by adding a `completed` column when needed.

## Previous candidate: v0.0.18 Intelligent Debug, Metadata, Watch Availability, and Responsive Library

v0.0.18 makes the native Library and Discover screens explain more and guess less. Episode tiles show verified `SxxExx - title` identity with technical format on a secondary line. Artwork resolves through the item's Jellyfin image, parent/series artwork, and then exact TMDb movie/series/season artwork. Missing titles or posters remain explicitly unavailable when no verified match exists.

The new **Debug** tab runs evidence-based checks against the integrated server, port 8096, generated runtimes, local paths, the current Library level, artwork failures, TMDb configuration, and background work. Results use green, yellow, or red health with a concrete next action. **Run Checks**, **Retry**, **Refresh Metadata**, **Test TMDb**, **Refresh Server**, **Open Logs**, and **Copy Report** invoke real actions; copied reports omit credentials.

Discover now preserves and wraps the beginning of long descriptions. External results show exact United States subscription, free, ad-supported, rental, and purchase listings returned by JustWatch through TMDb, with a refresh time and explicit no-listing state. **My Services** privately marks providers the owner uses; it does not sign into a provider. **Open Watch Options** opens only the official link supplied by TMDb.

The top bar has one `Server:` light: green when ready, yellow during a transition, and red when unavailable. The duplicate Library status was removed. The Library grid now derives drawing, wheel scrolling, and arrow movement from the same responsive layout and shows at least two rows at the normal 1000-by-650 non-fullscreen size.

The final versioned executable is `ReddMedia_v18`. Its installer writes the executable first, assigns the approved red-tree custom-icon metadata directly to that raw executable, reads the assignment back, and refreshes Files/Nautilus before owner visual confirmation. This candidate is not accepted or tagged.

## Previous candidate: v0.0.17 Library, Discover, and Server Reliability

v0.0.17 is one reliability build over the technically working v0.0.16 checkpoint. Movie and TV recommendation requests now pass a final strict type gate, including repeated Random selections. Jellyfin poster responses use a supported source format, are normalized through FFmpeg for the native X11 renderer, and are cached locally; external TMDb results use and cache their real TMDb poster paths. A full-width red loading bar reports Library, poster, Discover, credential, and server work.

Discover now accepts either a TMDb 32-character API key or a TMDb read access token. The screen exposes **Test TMDb**, **Save / Replace**, and **Clear TMDb**. A replacement is validated before it can overwrite a working credential, the saved file remains owner-only, and a rejected credential produces a clear 401 message without exposing the value.

The Library screen now exposes **Start Server**, **Stop Server**, and **Refresh Server**. Closing ReddMedia still stops only the integrated Jellyfin process ReddMedia launched; an independently started Jellyfin process is preserved. Generated Jellyfin and AI runtime trees are excluded from Git. The repository root also contains the one canonical ReddMedia-only `COMPANY_BIBLE.md`.

The final versioned executable was `ReddMedia_v17`. Its installer wrote the executable first, assigned the approved red-tree custom-icon metadata directly to that raw executable, read the assignment back, and refreshed Files/Nautilus before owner visual confirmation.

## Previous build: v0.0.16 Native Library and Discover AI

v0.0.16 replaces the temporary flat Library file list with a native poster grid and two explicit entry points: **Movies** and **TV**. Movies show real cataloged movie titles and metadata-created box sets. TV shows series first, then seasons, then episodes. No sample or invented titles are inserted. Each media type supports more than one linked folder, unlinking removes only the catalog link, and playable items continue through ReddMedia's existing embedded player using their real local file paths.

The new top-level **Discover** tab contains the two approved modes, **Usual** and **Random**. Its heading changes between **DISCOVER USUAL** and **DISCOVER RANDOM**, and each mode exposes exactly **Local Movie**, **Local TV**, **External Movie**, and **External TV**. Every request returns one recommendation. Usual uses private SQLite viewing history and local Nomic embeddings; Random does not read the viewing profile. External results come from TMDb, exclude titles already identified in the local catalog, and require a user-supplied TMDb read-access token stored with mode `0600`.

Embeddings run locally on the CPU with pinned llama.cpp source and the bundled `nomic-embed-text-v1.5` Q4_K_M GGUF model. Metadata sent to TMDb is limited to catalog requests; viewing history and embeddings remain local.

The v0.0.16 owner-test repair builds only llama.cpp's required shared libraries. The hidden catalog server now lives for exactly the ReddMedia process lifetime: closing ReddMedia stops and reaps its owned Jellyfin process, and a parent-death safeguard stops it if ReddMedia is forcibly terminated. Repair 2 also corrects the failed-candidate rollback so tracked files return atomically to the committed v0.0.15 baseline while the accepted untracked Jellyfin runtime is preserved. It can recognize, back up, and recover the exact hash-verified leftovers produced by the rejected original v0.0.16 candidate; it will not overwrite any different dirty state.

## Previous build: v0.0.15 Native Library and Hidden Media Catalog Repair

v0.0.15 adds the stable Jellyfin 10.11.11 service as hidden catalog machinery behind ReddMedia. Its web client is disabled, first-run setup is completed privately through the local API, and remote access is disabled. The native ReddMedia window now has a Library tab for adding media folders, scanning, selecting titles, and playing them. Library selections resolve to their real local file paths and enter the same embedded libVLC player used by Open File, P2P, and YouTube; they do not open a browser or an external player and do not pass local playback through Jellyfin transcoding.

The installer extracts pinned Ubuntu 26.04 packages instead of rebuilding Jellyfin with Node and .NET. Jellyfin remains a separately licensed GPL process with matching source and licenses preserved inside the project.

## Previous build: v0.0.14 Local Pause Stability & Red Tree Identity

v0.0.14 hardens ordinary local-file pause/resume behavior so a long pause cannot strand the X11 event loop behind repeated libVLC polling. Paused playback uses cached time/length state, chapter metadata is discovered once per media item instead of being queried every few seconds, pause/resume uses explicit libVLC pause state, and final player teardown has a bounded close safeguard. The ReddMedia identity now uses the approved red-tree artwork across the launcher, dock/app switcher, raw executable, MIME icons, X11 window icon, and the small tree badge beside the top-right version label.

### Current P2P workflow

- Paste a magnet link or open a local P2P metadata file.
- Choose a download folder.
- ReddMedia retrieves P2P transfer metadata and displays the files in the P2P transfer.
- A single obvious video file is selected automatically; multi-file P2P transfers can be selected manually.
- Press **Play** to begin playback before the P2P transfer finishes downloading.
- ReddMedia serves the selected P2P transfer file to VLC through a localhost-only HTTP Range stream.
- Playback requests drive libtorrent time-critical piece priorities automatically. The user does not choose a technical download strategy.
- The full P2P transfer keeps downloading behind playback and can seed after completion.
- Active P2P transfer resume data is stored under `~/.config/reddmedia/p2p/`.
- The P2P source field supports Ctrl+A and Cut / Copy / Paste.

### v0.0.10 stabilization carried under the same version

The v0.0.10 stabilization pass keeps the same feature version while repairing defects found during owner testing:

- Seek/time and volume partial updates are buffered offscreen before being copied to the X11 window, preventing the direct erase/redraw path that caused visible flashing.
- A new P2P HTTP range request supersedes obsolete stream workers so an old seek cannot continue fighting a newer seek for P2P transfer pieces.
- Old time-critical P2P transfer piece deadlines are cleared when VLC starts a new stream range request.
- HTTP suffix byte ranges such as `Range: bytes=-5000000` are supported for VLC/container probing/seeking.
- Stream sockets have bounded send/receive waits so abandoned seek connections cannot hang indefinitely.
- The installer reapplies and verifies the ReddMedia red-triangle custom icon on the versioned executable.

The v0.0.16 versioned executable was `ReddMedia_v16`.

### v0.0.13 YouTube seek and close stability repair

- Preserves the growing-cache fix that carried long-form YouTube playback beyond the original 4-to-5-second wall.
- Every YouTube rewind, fast-forward, and seek-bar request now restarts the feeder at the requested timestamp with keyframe-aware cuts. ReddMedia no longer uses libVLC `set_time()` inside a growing YouTube cache.
- Seek replacement returns control to the UI immediately and finishes startup from the normal event-loop poll once enough replacement media has buffered.
- A newer seek replaces the previous pending seek by shutting down its localhost bridge and feeder before starting the newer timestamp.
- YouTube shutdown disconnects the localhost bridge/client sockets before stopping and releasing libVLC, preventing the network reader from holding application close hostage.
- YouTube format selection excludes AV1 and prefers H.264/AVC video with AAC audio, with a non-AV1 fallback, at a maximum of 1080p.
- Keeps the 512 KiB startup target, 5000 ms VLC network cache, red-star identity, P2P behavior, and v0.0.13 version identity.

### v0.0.12 YouTube seekable cache bridge

- Same-version identity repair: the application/window/launcher/raw-executable identity now uses the **red ReddMedia star** instead of the former triangle.
- The GNOME/X11 window title is `★ ReddMedia` with no version number in that title, while the in-app top-right version surface shows only `v0.0.12`.
- Same-version UI repair: the creator-facing tab, screen heading, activity-log heading, and status messages now use **YouTube**. The technical `yt-dlp` name remains only where it identifies the bundled engine, executable, command options, or implementation details.
- YouTube **Play** remains capped at 1080p by default.
- ReddMedia asks the bundled yt-dlp engine for the video's duration so the normal seek timeline has a stable full-video time scale.
- The bundled yt-dlp/FFmpeg pipeline writes the active playback segment into a private temporary cache under `/tmp`.
- ReddMedia serves that growing cache only on `127.0.0.1` through an internal HTTP server with byte-range support.
- Seeking within material already reached by the current cached segment uses the local HTTP source.
- Seeking beyond the current cached segment cancels the obsolete feeder/server and restarts the bundled yt-dlp engine at the requested timestamp using `--download-sections` and keyframe-aware cuts.
- Stop, replacement playback, and clean shutdown terminate the active feeder and remove its temporary cache file.
- The embedded libVLC player continues to render inside ReddMedia.

### v0.0.11 transfer and playback controls

- YouTube Play streams directly into ReddMedia through the bundled yt-dlp/FFmpeg pipeline, capped at 1080p by default.

- **Stop Download** pauses the active P2P transfer, stops active P2P playback, and preserves partial data/resume state.
- The same control becomes **Resume Download** while paused and continues the existing transfer.
- YouTube now has **Play** beside **Download**. Play resolves a network media location and hands it to ReddMedia's embedded VLC player without performing the normal saved-file download first.
- YouTube Download remains the normal save-to-disk path.

### v0.0.11 same-version repair

Owner testing proved P2P Stop/Resume but exposed two release defects before acceptance. The v0.0.11 repair keeps the same version number and corrects them:

- YouTube Play was repaired to stream the bundled yt-dlp/FFmpeg output into embedded libVLC, which proved that supported YouTube playback works inside ReddMedia at up to 1080p.
- libVLC Play startup is checked instead of silently treating a failed start as success.
- The red-triangle executable icon is assigned after the final binary write and GNOME Files/Nautilus is refreshed when available; owner-side visual confirmation remains an acceptance gate.
- The visible top-bar version surface is corrected to `ReddMedia v0.0.11`.

## v0.0.13 — YouTube Seek and Close Stability Repair

**Purpose:** keep long-form YouTube playback alive and make rewind/fast-forward/seek replacement and application close stable under the growing-cache architecture.

- Repaired the localhost bridge so an open-ended VLC byte request no longer freezes the cache size at request time.
- Added chunked indeterminate-length range delivery for the growing cache.
- Increased startup and libVLC network buffering for steadier playback.
- Preserved the 1080p ceiling and timestamp-restart seek behavior from v0.0.12.
- Added a slow-growing stream regression specifically designed to catch the 4-to-5-second freeze.
- Same-version stability repair: every YouTube seek now restarts the feeder, seek buffering is non-blocking to the main UI, shutdown disconnects the bridge before libVLC release, and AV1 is excluded in favor of H.264/AVC + AAC preference with a non-AV1 fallback.

## v0.0.12 — YouTube Seekable Cache Bridge

**Purpose:** turn the proven v0.0.11 YouTube playback path into a restartable, seek-aware embedded stream.

What changed:

- Replaced direct libVLC stdin playback for YouTube with a temporary yt-dlp/FFmpeg cache served by a ReddMedia localhost-only HTTP bridge.
- Kept the default YouTube playback ceiling at 1080p.
- Added duration probing so ReddMedia's normal seek timeline represents the full video.
- Added HTTP `HEAD`, full `GET`, byte-range, suffix-range, and invalid-range handling for the local cache source.
- Added timestamp restarts for seeks beyond the current cached playback segment.
- Added stale feeder cancellation and temporary-cache cleanup for seek replacement, Stop, and shutdown.
- Same-version UI/identity repair renames creator-facing network-video labels to **YouTube**, changes the window title to `★ ReddMedia`, shows only `v0.0.12` at the in-app top right, and replaces the application/launcher/raw-executable triangle with the red ReddMedia star.

Validation targets:

- C++17 warnings-as-errors build.
- Localhost-only listener and HTTP range fixture tests.
- yt-dlp 1080p selector and timestamp-restart contract checks.
- Real libtorrent linkage, embedded libVLC location playback, version identity, YouTube labels, title/version split, and red-star application/executable icon gates on the Ubuntu target machine.

## v0.0.11 — Playback & Transfer Controls

**Purpose:** give the two network-media paths the controls needed for everyday use.

What this build adds:

- **Stop Download / Resume Download** on the P2P screen.
- Stopping a P2P transfer also stops active P2P playback and seeding/uploading while preserving partial files and resume state.
- Resume continues the same P2P transfer without discarding completed data.
- A **Play** button on the YouTube screen that streams the bundled yt-dlp/FFmpeg output into ReddMedia's embedded VLC player.
- The existing YouTube **Download** path remains available for saving media normally.
- The roadmap records future Archive, Online Video, Live TV, and supported streaming-service integration work.

Validation target:

- P2P Stop/Resume preserves partial progress.
- YouTube Play starts embedded playback through the bundled yt-dlp/FFmpeg stream path.
- `ReddMedia_v11` retains the red-triangle executable icon.

## v0.0.10 — P2P Streaming Core

**Purpose:** add built-in P2P file transfer to ReddMedia and make watching while downloading the normal P2P behavior.

What this build added:

- Permanent **P2P** application screen.
- libtorrent-rasterbar 2.x integration.
- Magnet-link loading.
- Local P2P metadata-file loading.
- P2P transfer metadata retrieval and file listing.
- Automatic selection of a single obvious video file.
- Manual file selection for multi-file P2P transfers.
- Download folder selection.
- Live P2P transfer name, state, progress, downloaded amount, download/upload speed, peers, and seeds.
- Complete P2P transfer downloading and seeding behind playback.
- Persistent P2P resume data.
- Ctrl+A and Cut / Copy / Paste in the P2P source field.
- A localhost-only HTTP Range bridge between the P2P transfer engine and VLC.
- Time-critical libtorrent piece deadlines driven by what VLC needs for playback.
- Stream-while-downloading playback without exposing separate sequential-download controls.
- `DEPENDENCIES.md` for runtime and developer requirements.
- libtorrent BSD license/third-party notice records.
- Roadmap work for a future self-contained Linux distribution.

Owner-test results that established the milestone:

- Magnet-link intake and P2P transfer downloading worked.
- Local P2P metadata-file intake and transfer downloading worked.
- P2P transfer metadata, file list, peer/seed status, and automatic video selection worked.
- Playback began while a P2P transfer was still downloading.

Stabilization repairs accepted in v0.0.10:

- Restores buffered seek/time and volume partial repainting to remove the flashing regression.
- Cancels obsolete P2P stream requests when VLC seeks to a new range.
- Clears obsolete P2P transfer piece deadlines on a new stream range request.
- Adds legal HTTP suffix-range support used by media probing/seeking.
- Adds bounded stream-socket waits for abandoned requests.
- Reapplies and validates the red-triangle custom icon on `ReddMedia_v10`.
- Expands this README so every numbered ReddMedia release explains what it actually did.

Seek behavior under slow or difficult P2P swarms can still take time because peer availability controls how quickly an undownloaded region arrives.

## v0.0.9 — URL Field Text Controls

**Purpose:** make the YouTube URL box behave like a normal editable text field.

What changed:

- Ctrl+A selects the entire YouTube URL.
- Full-field selection is visibly highlighted.
- Right-click opens **Cut / Copy / Paste**.
- Cut and Copy place the complete selected URL on the X11 clipboard.
- Paste replaces a full-field selection before inserting clipboard text.
- The versioned executable advanced to `ReddMedia_v9`.

Validation highlights:

- Warnings-as-errors native compile passed.
- `ReddMedia_v9 --version` and native ELF checks passed.
- An Xvfb-driven interaction test proved Ctrl+A, Copy, Cut, Paste, and clipboard round-trip behavior.
- Installer and forced rollback rehearsals passed.
- Owner-side live URL-field testing passed before acceptance.

## v0.0.8 — Direct YouTube

**Purpose:** put the downloader inside ReddMedia as a permanent application screen.

What changed:

- Added the direct YouTube screen inside ReddMedia, powered by the bundled yt-dlp engine.
- Bundled the real Linux yt-dlp executable at `tools/yt-dlp/yt-dlp`.
- Added direct URL typing.
- Added Ctrl+V keyboard paste and right-click paste to the URL field.
- Kept output-folder selection separate from URL entry.
- Introduced the versioned executable name `ReddMedia_v8`.
- Preserved the v0.0.7 video-player behavior.
- The apply workflow attempted safe red-triangle custom-icon metadata for the raw executable and installed the matching desktop launcher identity.

Validation highlights:

- Bundled yt-dlp and build/package checks passed.
- Real desktop/download behavior required owner-side testing.

## v0.0.7 — Red Button Polish

**Purpose:** finish the visible red control-button direction.

What changed:

- Bottom player control buttons changed to red.
- Button lettering changed to white.
- The established button order and player behavior were preserved.

Validation highlights:

- Warnings-as-errors compile passed.
- Executable/package and version checks passed.
- Desktop testing verified the visual behavior before acceptance.

## v0.0.6 — Menu, Audio, Subtitle, Chapter, and Close Behavior Repair

**Purpose:** turn the top menus and media-track controls into functional player features.

What changed:

- File became a real menu with **Open File** and **Exit ReddMedia**.
- Audio menu gained embedded audio-track discovery and selection through libVLC.
- Subtitle menu gained subtitles on/off, subtitle-file loading, subtitle-folder loading, delay earlier/later, delay reset, and embedded subtitle-track selection.
- Matching `.srt` files could be detected automatically beside the video or inside common subtitle folders.
- A detected matching subtitle could be enabled automatically.
- Right-click video options gained subtitle control.
- Real embedded chapter information was used when libVLC exposed it.
- Timeline chapter marks used real chapter timestamps when available.
- Right-click chapter controls gained Previous Chapter, Next Chapter, and direct chapter jumps.
- The buffered UI and red-triangle branding from v0.0.5 were preserved.

Validation highlights:

- C++17 warnings-as-errors compile passed.
- Menu/audio/subtitle/chapter package contracts passed.
- Media-specific audio/subtitle/chapter behavior required owner testing with files that actually contain those tracks or metadata.

## v0.0.5 — Branding and Polish Flicker/Icon Repair

**Purpose:** establish ReddMedia's visual identity and move the parent UI to buffered drawing.

What changed:

- Added the red ReddMedia triangle icon assets in multiple sizes.
- Set the X11 `_NET_WM_ICON` window icon.
- Set the X11 window class to `ReddMedia` for launcher/dock matching.
- Updated the desktop launcher to use `Icon=reddmedia` and `StartupWMClass=ReddMedia`.
- Kept the native GNOME/window title bar under system control.
- Kept the in-app header red.
- Replaced direct/partial parent-window repainting with offscreen buffered drawing before onscreen copy.
- Preserved the v0.0.4 red timeline, red volume bar, controls, playback, fullscreen, and resume behavior.
- Removed a rejected custom MIME-icon approach that could interfere with double-click launching.
- Used safe local icon registration/custom-icon metadata attempts for the raw executable.

Validation highlights:

- Native warnings-as-errors compile and link passed.
- Buffered parent-window drawing, icon assets, window class, launcher identity, and safe icon path checks passed.

## v0.0.4 — Time Layout, Skip Buttons, and Red Timeline Repair

**Purpose:** turn the bottom controls into the player layout used by later ReddMedia builds.

What changed:

- Current time moved to the left of the seek bar.
- Total duration moved to the right of the seek bar.
- The time/seek row was lowered to create space below the video.
- The seek/progress fill changed to ReddMedia red.
- Chapter-style tick marks were added to the timeline.
- The volume fill changed to ReddMedia red.
- Visible **Rewind 10s** and **Fast Forward 10s** buttons were added.
- The bottom button order became: Open, Rewind 10s, Play/Pause, Stop, Fast Forward 10s, Fullscreen.
- The new skip buttons used the same 10-second seek behavior as the arrow keys.

Validation highlights:

- Build/package checks passed.
- Owner-side playback testing was required for the final visual positioning and controls.

## v0.0.3 — Mouse, Keyboard, Time Display, and Flicker Repair

**Purpose:** make direct player interaction quicker and repair the bottom playback layout.

What changed:

- Single left-click on video toggled pause/resume.
- Double left-click continued to toggle fullscreen without the single-click action firing first.
- Right-click on video opened a basic player options menu.
- Up/Down Arrow changed volume.
- Left/Right Arrow sought backward/forward by 10 seconds.
- Keyboard seek jumps were standardized to 10 seconds.
- The bottom layout reserved visible space for the time display.
- Playback repaint behavior continued avoiding full-window clearing to reduce flicker.

Validation highlights:

- C++17 warnings-as-errors compile passed.
- Mouse, keyboard, fullscreen, seek, and redraw source contracts passed.

## v0.0.2 — VLC-Style Player Repair 1

**Purpose:** make fullscreen, resume, and repaint behavior act like a real desktop media player.

What changed:

- Fullscreen became video-only, hiding the normal controls.
- Double-clicking the video toggled fullscreen.
- Escape exited fullscreen.
- Resume became a clickable video-area action.
- Resume reopened the saved media file and retried the saved seek after playback began.
- **Load Different File** opened normal file selection.
- Mouse wheel input changed volume only.
- Normal playback refresh stopped clearing the live video surface, reducing flicker.
- Close behavior continued to save session state and release VLC cleanly.

Validation highlights:

- C++17 `-Wall -Wextra -Werror` compile passed.
- X11/dynamic-libVLC source contracts for fullscreen, resume, volume, and repaint behavior passed.

## v0.0.1 — VLC-Style Base Video Player

**Purpose:** establish the first accepted standalone ReddMedia player instead of the earlier rejected prototypes.

What this build added:

- A top-level native ReddMedia executable.
- VLC/libVLC-backed local video playback.
- Open-file support.
- Play/Pause and Stop controls.
- Timeline seeking and current playback time.
- Volume control.
- Fullscreen playback.
- Cursor auto-hide during playback.
- Clean close behavior.
- Basic saved-session resume/load behavior.
- Initial README, changelog, roadmap, license, third-party notices, build handshake, and validation record.

Validation highlights:

- Native C++ build/package checks passed.
- Full graphical playback validation was reserved for the Ubuntu desktop because the build sandbox did not provide the live VLC/X11 environment.

## Main ReddMedia features

- Native X11 desktop interface.
- Native media Library with folder selection, local catalog scanning, title selection, and direct playback in the existing embedded player.
- Native Movies/TV hierarchy with real poster metadata, movie box sets, series, seasons, and episodes.
- Responsive multi-row Library tiles with verified episode numbers/titles and separate technical format.
- Discover Usual/Random recommendations across Local Movie, Local TV, External Movie, and External TV.
- United States JustWatch availability through TMDb, private My Services markings, and official watch-option links.
- Evidence-based Debug and system-health checks with actionable green/yellow/red findings.
- Local SQLite history and offline llama.cpp/Nomic metadata embeddings.
- Hidden local Jellyfin 10.11.11 catalog service with no exposed Jellyfin setup/player web interface.
- VLC/libVLC local video playback.
- Open, Play/Pause, Stop, Rewind 10s, Fast Forward 10s, timeline seeking, volume, fullscreen, and resume support.
- Keyboard and mouse playback controls.
- Embedded audio-track selection.
- External and embedded subtitle controls, automatic matching `.srt` loading, subtitle folder selection, and subtitle delay controls.
- Embedded chapter discovery and chapter navigation when exposed by libVLC.
- YouTube download/playback screen with URL entry and output-folder selection, powered by the bundled yt-dlp engine.
- Built-in P2P magnet and local metadata-file downloading with stream-while-downloading playback.
- Red ReddMedia branding, red controls, red seek/volume bars, and red-tree window/launcher/executable identity.

## Dependencies

See [`DEPENDENCIES.md`](DEPENDENCIES.md) for the exact Ubuntu runtime and build requirements and one-command installation lines.

ReddMedia bundles its yt-dlp executable under `tools/yt-dlp/`. VLC/libVLC, FFmpeg, X11, Zenity, and libtorrent are currently supplied by the Linux system.



## Licensing and third-party software

Nougat Media Suite Original Materials are made available to recipients under the **PolyForm Noncommercial License 1.0.0**. Elderred Softworks LLC retains all rights not granted to recipients, including commercial use and separate commercial licensing of its own Original Materials. See [`LICENSE`](LICENSE), [`COPYRIGHT.md`](COPYRIGHT.md), and [`docs/LICENSING_POLICY.md`](docs/LICENSING_POLICY.md).

Outside contributions are accepted only under [`CONTRIBUTING.md`](CONTRIBUTING.md), which grants the project owner sufficient rights to continue maintaining, sublicensing, relicensing, and commercially licensing the combined project.

Third-party components keep their upstream licenses and terms. See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and the preserved files under `licenses/`.

## Roadmap

See [`ROADMAP.md`](ROADMAP.md) for the next planned ReddMedia milestones.

