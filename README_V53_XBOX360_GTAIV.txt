NOUGAT MEDIA SUITE v0.0.53 - XBOX 360 / GTA IV CHANGED FILES

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
