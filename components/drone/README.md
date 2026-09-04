# Nougat Drone Production Foundation

This directory contains the reproducible integration foundation for Studio -> Drone.

v0.0.60 is intentionally simulation-only. The native UI, Director Shot model, integration-status surface,
telemetry/camera/gimbal lanes, dependency lock, and source-fetch workflow are present, but live aircraft
arming and command transmission are not enabled in this foundation release.

The intended production workflow is:

Director description -> flight/camera path -> preview -> simulator validation -> saved repeatable shot ->
authorized physical-flight execution in a later hardware-validated release.

Third-party source trees are downloaded into `components/drone/vendor/` and are intentionally excluded from
Nougat's own Git staging. Each upstream checkout retains its own license and history. Exact pins are in
`DRONE_STACK_LOCK.json`.
