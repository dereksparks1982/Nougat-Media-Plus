# NOUGAT_SPLIT_ARCHIVE Format v1

Status: v0.0.50 Workshop transport format.

`NOUGAT_SPLIT_ARCHIVE` is a publicly documented, application-independent split/reassembly format used by Nougat Media Suite Workshop. It is designed for moving a file or complete directory tree through services with per-file upload limits while retaining integrity and enough filesystem metadata to reconstruct the original safely.

## Design goals

- Ordinary `.zip` part files that can be inspected with standard ZIP software.
- A small JSON manifest that is authoritative for ordering and integrity.
- SHA-256 verification at source-file, chunk, and completed-part levels.
- No oversized-file exception: a single huge source file may span any number of parts.
- Explicit detection of missing, wrong-size, corrupted, or unreadable parts before reconstruction.
- Preserve directory structure and empty directories.
- Preserve file/directory mode and nanosecond modification time where the host filesystem permits it.
- Preserve only safe relative symbolic links. Absolute or escaping links are rejected on reassembly.
- Never overwrite an existing destination tree silently.
- No Nougat installation is required to understand the format.

## File set

For archive name `MyProject`:

```text
MyProject.manifest.json
MyProject.part001.zip
MyProject.part002.zip
MyProject.part003.zip
...
```

The user may select the manifest or any part in Nougat. A part name locates the sibling manifest by removing `.partNNN.zip` and appending `.manifest.json`.

## ZIP part contents

Parts use ordinary ZIP64-capable ZIP containers. v1 stores chunk entries without compression (`ZIP_STORED`) so a requested maximum part ceiling is predictable.

Chunk names are deterministic:

```text
chunks/000000001.bin
chunks/000000002.bin
...
```

Chunks are opaque byte ranges. Their original path, offset, size, ordering, and SHA-256 are held by the external manifest.

The ZIP is a transport container, not the authoritative directory tree. Extracting the part ZIPs manually yields chunks rather than the original source tree.

## Manifest identity

Required top-level identity fields:

```json
{
  "format": "NOUGAT_SPLIT_ARCHIVE",
  "format_version": 1,
  "archive_name": "MyProject",
  "source_type": "directory",
  "root_name": "MyProject"
}
```

A reader must reject an unsupported format/version rather than guessing.

## Manifest summary fields

v1 writers also record:

- `created_unix`
- `total_source_bytes`
- `file_count`
- `directory_count`
- `max_part_bytes`
- `entries`
- `parts`
- `chunks`

Unknown fields must be ignored by compatible readers unless a future format version says otherwise.

## Entries

Every preserved filesystem object appears in `entries`.

Regular file example:

```json
{
  "path": "MyProject/video/source.mov",
  "type": "file",
  "mode": 420,
  "mtime_ns": 1780000000000000000,
  "size": 912345678,
  "sha256": "...",
  "chunks": [1, 2, 3]
}
```

Directory example:

```json
{
  "path": "MyProject/empty-folder",
  "type": "directory",
  "mode": 493,
  "mtime_ns": 1780000000000000000
}
```

Symbolic-link example:

```json
{
  "path": "MyProject/current",
  "type": "symlink",
  "mode": 511,
  "mtime_ns": 1780000000000000000,
  "target": "versions/current"
}
```

All manifest paths are POSIX-style relative paths. Absolute paths, empty path components, `.` components, and `..` components are invalid.

## Chunks

A chunk record contains:

```json
{
  "index": 1,
  "path": "MyProject/video/source.mov",
  "offset": 0,
  "size": 470000000,
  "sha256": "...",
  "part": 1,
  "entry": "chunks/000000001.bin"
}
```

`index` is globally unique within a set. `part` references a part's numeric `index`. A regular file's `chunks` array gives reconstruction order.

A reader must verify each extracted chunk's byte count and SHA-256 before treating it as valid source data.

## Parts

A part record contains:

```json
{
  "index": 1,
  "file": "MyProject.part001.zip",
  "size": 471048576,
  "sha256": "..."
}
```

The completed ZIP's exact size and SHA-256 are checked before reconstruction. A valid chunk inside a part does not excuse a mismatching part hash.

## Part-size semantics

The v0.0.50 reference writer defaults to a ceiling of **450 MiB** (`471859200` bytes), deliberately below a 500 MB upload ceiling.

When `--max-part-bytes N` is supplied, every completed `.partNNN.zip` must be `<= N`. The writer reserves ZIP metadata headroom and refuses settings too small to be safe.

When a requested number of parts is supplied, the writer calculates a safe target from total source bytes. If filesystem/ZIP overhead means the requested count cannot be achieved safely, the operation fails explicitly rather than generating an oversized or silently different set.

## Reassembly procedure

A conforming reader should:

1. Locate and parse the manifest.
2. Confirm `format` and `format_version`.
3. Reject unsafe manifest paths.
4. Confirm every expected part exists.
5. Confirm every part's exact size and SHA-256.
6. Confirm expected chunk entries exist in their part ZIPs.
7. Confirm every chunk's exact size and SHA-256.
8. Reconstruct into a temporary staging directory.
9. Reconstruct regular files in manifest chunk order.
10. Verify each reconstructed file's final size and SHA-256.
11. Recreate safe relative symbolic links only after regular objects exist.
12. Restore modes/timestamps where supported.
13. Atomically move the staged root into the requested destination where the platform permits it.
14. Refuse to overwrite an existing destination root.

Failure at any verification stage must leave the requested final destination untouched or removed if it was created during the failed operation.

## Symbolic-link safety

v1 never follows source directory symlinks while scanning.

During reconstruction, a link target is accepted only when it is relative and resolves within the reconstructed archive root. Absolute links and relative links that escape the archive root are rejected. This prevents a transport package from creating links into arbitrary host paths.

## Reference implementation

The v0.0.50 stdlib-only reference implementation is:

```text
components/workshop/nougat_split_archive.py
```

Its CLI provides:

```text
inspect SOURCE
split SOURCE --output DIR [--max-part-bytes N | --parts N]
verify MANIFEST_OR_PART
reassemble MANIFEST_OR_PART --output DIR
```

`--json` emits machine-readable status for the native Workshop frontend.

The native C++ adapter is:

```text
src/workshop/split_archive_service.hpp
src/workshop/split_archive_service.cpp
```

## Compatibility rule

Nougat may add fields compatibly within format version 1, but must not change the meaning of existing fields. A breaking layout or security-semantics change requires a new `format_version`. Future Nougat releases should retain readers for older supported versions so split archives are not tied to the exact application build that created them.
