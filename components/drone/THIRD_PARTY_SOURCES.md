# Drone third-party source boundaries

Nougat's Drone Production foundation uses separate upstream projects rather than relicensing their code.

- MAVSDK: public upstream C++ MAVLink API.
- MAVLink: public upstream protocol/message definitions.
- PX4 Autopilot: separate upstream autopilot/SITL source tree.
- ArduPilot: separate upstream autopilot/SITL source tree.
- FFmpeg and GStreamer: detected as host media/video tools when installed.

The fetcher preserves each upstream repository and its license files in its own directory. Nougat v0.0.60 does
not compile or link PX4 or ArduPilot source into the Nougat executable. No third-party project is relicensed as
Elderred Softworks LLC Original Material.

The Drone UI remains simulation-only in this foundation build. Future real-aircraft support must remain
provider-gated, hardware-validated, and explicitly separated from simulation.
