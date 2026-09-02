# Nougat Media Suite v0.0.54 File Splitter Professional Validation

Base: owner-accepted and closed v0.0.53.
Scope: promote File Splitter from prototype to a professional in-app tool.

## UI contract

- File Splitter lives under Studio > Tools and opens from a File Splitter button.
- File Splitter is not a separate top-level tab.
- Source / Manifest, Download Location, Output Name, and Pieces are in-app fields.
- No Zenity/file-chooser/question/info popup workflow remains in the splitter worker.
- Source analysis automatically recommends a piece count.
- Recommendation may be accepted with Use Suggestion or overridden manually.
- Analyze, Split, Reassemble, Verify, and Stop are in-app controls.
- Live percentage, status, manifest/result path, and SHA-256 verification state are shown in-app.
- Split/reassemble work runs asynchronously so the X11 UI remains responsive.

## Engine contract

- v3 manifest format with v2 manifest compatibility.
- Streaming file I/O; large inputs are not loaded fully into RAM.
- SHA-256 is recorded for each piece and the complete payload.
- Split output is read back and verified.
- Reassembly verifies pieces and complete payload integrity.
- Cancellation removes incomplete temporary output.
- Output replacement is refused rather than silently overwriting existing data.

## Regression gates

- `src/games/emulator_host.cpp` remains the exact closed-v0.0.53 Git blob:
  `22f03639525a37997ef01a049b35641876399afc`.
- v54 static tests preserve the accepted player renderer/geometry anchors.
- Closed-v53 Xenia Edge embed contracts remain present and unchanged.

## Production build

- CMake target: `Nougat_Media_Suite_v54`.
- Build type: Release.
- `REDDMEDIA_P2P_STUB=OFF`.
- `REDDMEDIA_AI_STUB=OFF`.
- Linked against the owner-machine libtorrent-rasterbar 2.0.12 development bundle.
- Linked against the owner-machine pinned llama.cpp runtime.
- ELF dependency inspection proves `libtorrent-rasterbar.so.2.0` and `libllama.so.0` are required by the final binary.
- Runtime AI RPATH remains `$ORIGIN/components/ai/runtime/lib:$ORIGIN/components/ai/runtime/lib64`.

The owner-machine llama runtime requires GLIBC_2.43, newer than the build container's glibc. The final executable therefore cannot be runtime-smoke-tested in this container, but it was fully compiled and linked against the exact dependency bundle taken from the owner machine. Owner testing remains the acceptance gate.
