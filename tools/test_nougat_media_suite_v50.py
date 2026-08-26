#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import py_compile
import sys

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def contains_all(text: str, tokens: list[str], label: str) -> None:
    missing = [token for token in tokens if token not in text]
    need(not missing, label + " missing: " + ", ".join(missing))


def main() -> int:
    try:
        main_cpp = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        media_server = (ROOT / "src/media_server/media_server_manager.cpp").read_text(encoding="utf-8")
        paths_hpp = (ROOT / "src/platform/nougat_paths.hpp").read_text(encoding="utf-8")
        manifest = json.loads((ROOT / "config/components/components-v1.json").read_text(encoding="utf-8"))

        contains_all(cmake, [
            "VERSION 0.0.50",
            "Nougat_Media_Suite_v50",
            "NOUGAT_AI_MODE",
            "NOUGAT_P2P_MODE",
            "src/platform/nougat_paths.cpp",
            "src/workshop/split_archive_service.cpp",
            "-Wall -Wextra -Werror",
        ], "v50 CMake")

        contains_all(main_cpp, [
            "NOUGAT_V50_WORKSHOP_PATCH",
            "NOUGAT_V50_NEUTRAL_PICKERS",
            '#include "platform/nougat_paths.hpp"',
            '#include "workshop/split_archive_service.hpp"',
            'printf("Nougat Media Suite v0.0.50\\n")',
            'const std::string versionLabel = "v0.0.50";',
            'input.app_version = "Nougat Media Suite v0.0.50";',
            'draw_tab(studioTab,"Workshop",ViewMode::Studio);',
            'section_text(target, 28, 70, "WORKSHOP", palette.text);',
            "NOUGAT_SPLIT_ARCHIVE v1",
            "start_workshop_split()",
            "start_workshop_reassemble(",
            'nougat::paths::component_runtime("mesen2")',
            'nougat::paths::component_runtime("dosbox-staging")',
            'nougat::paths::component_runtime("xenia")',
        ], "v50 main source")

        forbidden = [
            'draw_tab(studioTab,"Studio",ViewMode::Studio);',
            'section_text(target, 28, 70, "GOLD STUDIO", palette.text);',
            'printf("Nougat Media Suite v0.0.49\\n")',
            'static std::string config_dir() { return home_dir() + "/.config/reddmedia"; }',
            'exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"',
            'exe_dir() + "/components/games/runtime/mesen2/Mesen"',
        ]
        present = [token for token in forbidden if token in main_cpp]
        need(not present, "v50 main source retains forbidden seams: " + ", ".join(present))

        contains_all(paths_hpp, [
            "std::filesystem::path config;",
            "std::filesystem::path data;",
            "std::filesystem::path cache;",
            "std::filesystem::path state;",
            "component_runtime",
            "legacy_config_roots",
        ], "v50 XDG path API")

        contains_all(media_server, [
            'component_runtime("jellyfin")',
            "paths.server_data",
            "paths.server_config",
            "paths.server_cache",
            "paths.server_logs",
            "return false;",
        ], "optional Jellyfin manager")

        need(isinstance(manifest.get("components"), list) and manifest["components"],
             "component manifest does not contain components")
        ids = {item.get("id") for item in manifest["components"] if isinstance(item, dict)}
        for required in {"jellyfin", "llama.cpp", "mesen2", "stella", "blastem", "dosbox-staging", "xenia"}:
            need(required in ids, f"component manifest missing {required}")

        for script in [
            ROOT / "components/workshop/nougat_split_archive.py",
            ROOT / "tools/apply_v50_core.py",
            ROOT / "tools/apply_v50_dialog_labels.py",
        ]:
            py_compile.compile(str(script), doraise=True)

        print("PASS: Nougat Media Suite v0.0.50 source contract")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
