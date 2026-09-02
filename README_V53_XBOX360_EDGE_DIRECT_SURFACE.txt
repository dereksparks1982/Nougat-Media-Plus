Nougat Media Suite v0.0.53 Xbox 360 Edge Direct-Surface Repair

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
