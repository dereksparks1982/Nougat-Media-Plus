# Nougat Media Suite v0.0.41 Repair v4 Candidate Validation

Base commit: `5c8d148b995b84cb96628cec472be514873fd399` (accepted v0.0.40 documentation closeout).

Scope:
- Search Archive directory and external-browser actions.
- Exact IMDb title links from verified Jellyfin `ProviderIds.Imdb` metadata.
- Search result-card bottom-gap geometry repair.
- Live TV `Stop Live`, tuner release, and queued guide continuation.
- Manual idle guide-refresh state repair.
- Shared three-second mouse/video-identity activity behavior in fullscreen, maximized, and ordinary windows.
- Release-integrity documentation and v0.0.42 Home LAN web-viewer roadmap entry.

The installer builds and validates in a temporary candidate tree before replacing the active root executable. Owner acceptance remains required. No commit, tag, or push is performed by the package.

## Repair v2

The first v0.0.41 package stopped safely during temporary-candidate patching before any active-project modification.

Confirmed packaging defect:
- the cache-write patch anchor embedded C++ `\t` and `\n` literals inside a non-raw Python triple-quoted string;
- Python converted those escapes before searching the C++ source, so the literal source text could never match.

Repair v2 uses a raw, single-line semantic anchor on `hex_encode("") << '\\n';` and validates that spelling before packaging.

## Repair v3

Repair v2 stopped safely during the temporary native build before any active-project modification.

Confirmed repair-v2 build defect:
- the temporary candidate mixed real modified files with symlinked source directories;
- a relative `../media_server/jellyfin_api_client.hpp` include traversed the symlinked directory and reached the original v0.0.40 source;
- the compiler therefore saw both the v0.0.41 patched header and the v0.0.40 original header and reported duplicate definitions.

Repair v3 replaces the source-directory symlink strategy entirely. The candidate is exported as one coherent tracked tree from exact base commit `5c8d148b995b84cb96628cec472be514873fd399` using `git archive`. Only the untracked AI runtime is linked into that exported tree afterward.

Repair v3 also removes the version-specific v0.0.40 contract script from the post-build gate and replaces it with a v0.0.41 retained-contract test that checks the retained v0.0.40 Search/loading/Crawler behavior using the v0.0.41 identity and toolbar contract.

## Repair v4

Repair v3 stopped safely during temporary-candidate source validation before compilation and before any active-project modification.

Confirmed repair-v3 defect:
- `CMakeLists.txt`, executable title text, README, launchers, and other v0.0.41 identity surfaces were advanced;
- the visible application top-bar uses a separate `const std::string versionLabel = "v0.0.40";` token;
- that token was not patched, and the new release-integrity gate correctly rejected the incomplete version propagation.

Repair v4 explicitly patches `versionLabel` to `v0.0.41` and retains the strict gate. The gate is not weakened.
