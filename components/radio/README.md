# Nougat Radio Components

Nougat Radio is a Nougat-owned receiver UI and orchestration layer. Hardware/decoder projects remain replaceable upstream components behind explicit provider or worker boundaries.

## v0.0.52 architecture

The main executable owns the simple **RADIO** and advanced **PRO** interfaces, presets, persistent favorites/recordings, capability reporting, device selection, receive/scan lifecycle, and the non-radiating TX-chain test.

Optional radio engines are staged under `components/radio/upstream/` by `tools/vendor_radio_sources_v52.py`. The main Nougat executable does not require every optional engine in order to start. If a receiver/decoder is unavailable, the UI reports that capability as unavailable instead of faking success.

### Core upstream foundations

- **SoapySDR**: vendor-neutral SDR device abstraction.
- **liquid-dsp**: permissively licensed DSP building blocks for future in-process filters/demodulation.
- **KISS FFT**: permissively licensed FFT foundation for spectrum/waterfall work.
- **GNU Radio 4 core**: modern permissively licensed GNU Radio core source staged for future modular flowgraph work. v0.0.52 does not require it to build because its compiler/CMake requirements are newer than Nougat's current base toolchain.
- **OP25**: separate GPL-family worker/runtime for supported unencrypted P25 reception. It is not linked into Elderred Softworks LLC Original Materials.

### Specialized worker candidates

- **welle.io** for DAB/DAB+.
- **Dream** for DRM.
- **SatDump** for satellite data reception/decoding where applicable.
- **Trunk Recorder** for supported unencrypted trunked/conventional systems.

GPL-family components remain separate executables/workers and retain their own licenses. Their presence does not relicense Nougat's Original Materials.

## Current hardware truthfulness

A TV antenna connected to a Linux DVB television tuner is useful to Nougat immediately through **TV Antenna Scan**, but the tuner is not treated as a general-purpose SDR unless the driver really exposes radio/raw-IQ capability. Attaching a broader-band SDR/communications receiver expands the Radio modes automatically.

## Transmit boundary

The v0.0.52 data model is RX/TX-capable, but RF transmission is disabled by default. The included TX-chain test is non-radiating and writes generated IQ test material locally. Real RF transmission requires a later owner-approved hardware/provider implementation and an attached TX-capable device.
