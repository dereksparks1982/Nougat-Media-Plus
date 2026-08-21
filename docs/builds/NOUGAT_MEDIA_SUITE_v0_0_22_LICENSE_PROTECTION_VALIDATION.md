# Nougat Media Suite v0.0.22 License Protection Validation

Pre-handoff validation must prove:

- exact accepted v0.0.21 Git base/tag and clean tracked tree preflight;
- payload-manifest integrity;
- accepted `Nougat_Media_Suite_v21` is verified using the exact accepted llama.cpp runtime at the renamed Nougat Media Suite project path;
- v0.0.22 embeds only `$ORIGIN/components/ai/runtime/lib[64]` for llama.cpp discovery and contains no absolute ReddMedia or Nougat project runtime path;
- temporary-build and final-root v0.0.22 version/AI self-tests pass with `LD_LIBRARY_PATH` explicitly removed;
- PolyForm Noncommercial 1.0.0 remains the controlling recipient license for Original Materials;
- Elderred Softworks LLC is the project licensor/copyright identity in the new licensing records;
- owner commercial/relicensing rights are explicitly reserved;
- third-party materials are expressly excluded from the project-level license;
- contributor terms grant broad inbound use/sublicense/relicense rights;
- no retained media/UI behavior markers are removed;
- C++17 `-Wall -Wextra -Werror` deterministic stub build;
- `Nougat_Media_Suite_v22 --version` reports `Nougat Media Suite v0.0.22`;
- Discover AI stub self-test and bounded X11 window smoke;
- installer rollback restores the accepted v0.0.21 touched state while preserving runtime/user data;
- owner machine performs the full native libtorrent + pinned llama.cpp/model build before FINAL PASS.

## Container limitation for the runtime repair

The preserved accepted llama.cpp runtime was built against `GLIBC_2.43`, while the packaging container provides `GLIBC_2.41`. The real runtime therefore cannot be linked/launched in this container. The repair is still checked here by source/installer contracts, warnings-as-errors stub build, generated linker-command inspection, and an isolated `$ORIGIN` relocation proof. The owner-machine installer remains the mandatory full native libtorrent + real llama.cpp/model gate and must prove the final root executable with `LD_LIBRARY_PATH` removed before FINAL PASS.
