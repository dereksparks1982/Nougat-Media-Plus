# Nougat Media Suite v0.0.52 Scope

Elderred Softworks LLC / DKLab

Status: owner-approved candidate scope
Base: GitHub main commit `bc682de962f19b3c80f4718539467eba2aa139cf`
Version: `v0.0.52`

## Build law

v0.0.52 is a **Radio-only functional build**. It replaces the rejected v0.0.51 Radio shell with a real receiver/scanner architecture. No unrelated rejected-v0.0.51 repair is part of this candidate. All non-Radio carry-forward work is assigned to v0.0.53 in `NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md`.

The candidate remains uncommitted, untagged and unpushed until owner testing and acceptance.

## Goal

Make Radio behave like a real radio while remaining usable by somebody who does not want to learn a professional receiver cockpit before listening to something.

Nougat therefore exposes **two faces over one engine**:

- **RADIO**: simplified use-case presets and sensible defaults.
- **PRO**: direct technical controls and truthful hardware/runtime diagnostics.

## Included v0.0.52 work

### Receiver modes and use cases

- Local broadcast reception.
- Local Emergency / Scanner reception.
- NOAA/weather-band reception when hardware supports the band.
- Satellite / ISS receive preset, including the common 145.800 MHz ISS voice downlink starting point.
- Shortwave/HF reception when the attached receiver supports HF.
- Internet Radio through Nougat's existing native audio/libVLC path.
- Favorites and persistent radio recordings.
- TV Antenna Scan bridge to Nougat's existing Live TV tuner path.
- Professional modulation vocabulary including AM, NFM, WFM, USB, LSB, CW, DAB/DAB+, DRM, P25 and raw-IQ capability paths.

### Professional receiver controls

- Direct frequency entry.
- Frequency step up/down.
- Modulation selection.
- Tuning-step selection.
- Gain control.
- Squelch control.
- Receiver/device cycling.
- Start/stop receive.
- Bounded asynchronous scan.
- Favorite current frequency/mode.
- Recording toggle.
- Hardware/runtime capability status.
- Signal/progress presentation when the active backend provides evidence.

### Hardware and provider architecture

- Linux `/dev/radio*` discovery.
- Linux DVB television-antenna frontend discovery.
- SoapySDR-class provider discovery.
- RTL-SDR receive/scan path when `rtl_fm` / `rtl_power` are available.
- Capability recognition for HackRF, LimeSDR, UHD/USRP, Airspy and Airspy HF tooling when present.
- Optional worker/runtime boundaries for OP25, DAB/DAB+, DRM, satellite and trunked-radio engines.
- No fake radio capabilities. A television tuner remains a television RF frontend unless its Linux driver exposes a real radio/raw-IQ interface.

### Public-safety receiver behavior

- Local Emergency / Scanner is receive-only.
- Conventional analog and supported unencrypted digital/trunked reception may be handled by attached receiver hardware and installed decoder workers.
- Encrypted traffic is reported as unavailable/encrypted. Nougat does not attempt to defeat encryption.
- Location-specific scanner programming remains data-driven rather than baking one city's frequencies into the application.

### TX-ready architecture

- Device model records receive and transmit hardware capability separately.
- RF transmit remains disabled by default in this build.
- v0.0.52 includes a **non-radiating TX-chain self-test** that generates/validates local IQ test data without keying RF hardware.
- The architecture must not require a receiver-only redesign before licensed/qualified testers can validate real supported TX hardware in a later owner-approved step.

### Open-source radio foundations

The owner supplied source archives for:

- SoapySDR
- liquid-dsp
- OP25
- GNU Radio 4 core
- KISS FFT

The v0.0.52 vendor tool uses those archives when found and otherwise can obtain the exact pinned upstream snapshots. Upstream licenses remain attached to those source trees. GPL-family software remains a separate process/worker boundary and is not linked into Elderred Softworks LLC Original Materials.

Optional specialized sources may also be staged for DAB/DAB+, DRM, satellite and trunked reception, using the same separated-worker rule.

## Current TV antenna requirement

The existing TV antenna and tuner must be used for every capability the hardware actually exposes. Nougat must not claim AM, FM, HF, scanner or raw-IQ reception from a TV frontend unless the hardware/driver genuinely exposes it.

The Radio screen includes a direct **TV Antenna Scan** action so the currently connected broadcast-TV hardware is useful immediately through the existing Live TV path.

## Exclusions

The following are explicitly not repaired in v0.0.52: File Splitter, HDHomeRun scan pipeline/count regression, World TV guide/playback, Games emulator expansion/artwork, N-icon artifact, Search hover, unrelated top-navigation repair, LAN Web Viewer completion, overlay/process/freeze work, and the broader AMBER/public-alert system. They are recorded for v0.0.53.

## Validation boundary

A v0.0.52 candidate is not accepted merely because it compiles. Before owner handoff the build runner must prove:

- exact base commit and branch,
- clean tracked/staged state before apply,
- deterministic source patch,
- warnings-as-errors compile,
- exact `Nougat Media Suite v0.0.52` CLI identity,
- v52 static Radio contracts,
- no dead v51 Radio-shell text,
- prior executable rollback preservation before replacement,
- only `Nougat_Media_Suite_v52` remains as the active versioned root executable after successful promotion,
- canonical archive root is `$HOME/DKLab/Archive/`, never `$HOME/DKLab/Archives/`,
- no commit, tag or push occurs before owner acceptance.

Hardware reception itself remains an owner-machine validation because the build environment cannot manufacture the owner's tuner/antenna/SDR devices.
