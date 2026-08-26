#!/usr/bin/env python3
"""Apply the v0.0.50 guarded main.cpp migration.

The Nougat UI currently lives in one very large main.cpp. This patcher uses
exact base snippets and refuses to write anything if an expected seam moved.
It is intentionally idempotent: once the v50 marker exists, it validates the
result and exits successfully.
"""

from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
MARKER = "NOUGAT_V50_WORKSHOP_PATCH"


def fail(message: str) -> None:
    raise RuntimeError(message)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        fail(f"{label}: expected exactly 1 base match, found {count}")
    return text.replace(old, new, 1)


def replace_count(text: str, old: str, new: str, expected: int, label: str) -> str:
    count = text.count(old)
    if count != expected:
        fail(f"{label}: expected {expected} base matches, found {count}")
    return text.replace(old, new)


def validate_applied(text: str) -> None:
    required = [
        MARKER,
        '#include "platform/nougat_paths.hpp"',
        '#include "workshop/split_archive_service.hpp"',
        'draw_tab(studioTab,"Workshop",ViewMode::Studio);',
        'section_text(target, 28, 70, "WORKSHOP", palette.text);',
        'Nougat Media Suite v0.0.50',
        'const std::string versionLabel = "v0.0.50";',
        'nougat::paths::component_runtime("dosbox-staging")',
        'nougat::paths::component_runtime("xenia")',
        'nougat::paths::component_runtime("mesen2")',
        'workshopSplitBtn',
        'start_workshop_split()',
        'start_workshop_reassemble(',
        'poll_workshop_worker();',
    ]
    missing = [item for item in required if item not in text]
    if missing:
        fail("applied v50 source is missing: " + ", ".join(missing))
    forbidden = [
        'draw_tab(studioTab,"Studio",ViewMode::Studio);',
        'section_text(target, 28, 70, "GOLD STUDIO", palette.text);',
        'static std::string config_dir() { return home_dir() + "/.config/reddmedia"; }',
        'exe_dir() + "/components/games/runtime/mesen2/Mesen"',
        'exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"',
    ]
    present = [item for item in forbidden if item in text]
    if present:
        fail("v50 source still contains forbidden legacy seams: " + ", ".join(present))


def main() -> int:
    try:
        text = MAIN.read_text(encoding="utf-8")
        if MARKER in text:
            validate_applied(text)
            print("PASS: v0.0.50 core source patch already applied and valid")
            return 0

        text = replace_once(
            text,
            '#include "media_server/media_server_manager.hpp"\n',
            '#include "media_server/media_server_manager.hpp"\n'
            '#include "platform/nougat_paths.hpp"\n'
            '#include "workshop/split_archive_service.hpp"\n',
            "v50 includes",
        )

        text = replace_once(
            text,
            'static std::string config_dir() { return home_dir() + "/.config/reddmedia"; }\n'
            'static std::string session_file() { return config_dir() + "/session.json"; }\n'
            'static void ensure_config_dir() {\n'
            '    std::string base = home_dir() + "/.config";\n'
            '    mkdir(base.c_str(), 0755);\n'
            '    mkdir(config_dir().c_str(), 0755);\n'
            '}\n',
            'static std::string config_dir() { return nougat::paths::layout().config.string(); }\n'
            'static std::string session_file() { return config_dir() + "/session.json"; }\n'
            'static void ensure_config_dir() {\n'
            '    std::string error;\n'
            '    (void)nougat::paths::ensure_runtime_layout(&error);\n'
            '}\n',
            "v50 config path",
        )

        text = replace_once(
            text,
            'struct DebugUiState {\n',
            f'''// {MARKER}\nstruct WorkshopUiState {{\n    std::mutex mutex;\n    std::string status = "Choose a file or folder to inspect.";\n    bool busy = false;\n    bool updated = false;\n    int operation = 0; // 1 inspect, 2 split, 3 reassemble\n    long long total_bytes = 0;\n    long long file_count = 0;\n    long long directory_count = 0;\n    long long largest_file_bytes = 0;\n    long long suggested_parts = 1;\n}};\nstruct DebugUiState {{\n''',
            "v50 Workshop UI state",
        )

        text = replace_once(
            text,
            '    std::shared_ptr<ServerUiState> serverState = std::make_shared<ServerUiState>();\n'
            '    std::thread serverWorker;\n',
            '    std::shared_ptr<ServerUiState> serverState = std::make_shared<ServerUiState>();\n'
            '    std::thread serverWorker;\n'
            '    std::shared_ptr<WorkshopUiState> workshopState = std::make_shared<WorkshopUiState>();\n'
            '    std::thread workshopWorker;\n'
            '    std::string workshopSourcePath;\n'
            '    std::string workshopOutputFolder;\n'
            '    bool workshopSourceIsFolder = false;\n'
            '    bool workshopUsePartCount = false;\n'
            '    unsigned workshopPartCount = 2;\n'
            '    unsigned long long workshopMaxPartMiB = 450;\n'
            '    Rect workshopFileBtn, workshopFolderBtn, workshopOutputBtn, workshopModeBtn;\n'
            '    Rect workshopMinusBtn, workshopPlusBtn, workshopSplitBtn, workshopReassembleBtn;\n',
            "v50 Workshop members",
        )

        # Local AI/model payloads are managed components instead of source-tree files.
        text = replace_count(
            text,
            'exe_dir() + "/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"',
            '(nougat::paths::layout().runtime / "models" / "nomic-embed-text-v1.5-Q4_K_M.gguf").string()',
            4,
            "v50 managed AI model",
        )
        text = replace_once(
            text,
            'input.ai_runtime_path = exe_dir() + "/components/ai/runtime";',
            'input.ai_runtime_path = nougat::paths::component_runtime("llama.cpp").string();',
            "v50 managed AI runtime diagnostic",
        )

        # Managed emulator locations remain ahead of PATH fallbacks.
        game_paths = {
            'exe_dir() + "/components/games/runtime/mesen2/Mesen"':
                '(nougat::paths::component_runtime("mesen2") / "Mesen").string()',
            'exe_dir() + "/components/games/runtime/rmg/AppRun"':
                '(nougat::paths::component_runtime("rmg") / "AppRun").string()',
            'exe_dir() + "/components/games/runtime/atari800/AppRun"':
                '(nougat::paths::component_runtime("atari800") / "AppRun").string()',
            'exe_dir() + "/components/games/runtime/stella/stella"':
                '(nougat::paths::component_runtime("stella") / "stella").string()',
            'exe_dir() + "/components/games/runtime/blastem/blastem"':
                '(nougat::paths::component_runtime("blastem") / "blastem").string()',
            'exe_dir() + "/components/games/runtime/dosbox-staging/dosbox"':
                '(nougat::paths::component_runtime("dosbox-staging") / "dosbox").string()',
            'exe_dir() + "/components/games/runtime/xenia/xenia_canary"':
                '(nougat::paths::component_runtime("xenia") / "xenia_canary").string()',
            'exe_dir() + "/components/games/runtime/xenia/xenia"':
                '(nougat::paths::component_runtime("xenia") / "xenia").string()',
        }
        for old, new in game_paths.items():
            count = text.count(old)
            if count < 1:
                fail(f"v50 game runtime seam missing: {old}")
            text = text.replace(old, new)

        # Windows Xenia fallback is a two-line expression in the v49 source.
        text = replace_once(
            text,
            'const std::string canary =\n                    exe_dir() +\n                    "/components/games/runtime/xenia/xenia_canary.exe";',
            'const std::string canary =\n                    (nougat::paths::component_runtime("xenia") / "xenia_canary.exe").string();',
            "v50 Xenia Canary Windows path",
        )
        text = replace_once(
            text,
            'const std::string master =\n                    exe_dir() +\n                    "/components/games/runtime/xenia/xenia.exe";',
            'const std::string master =\n                    (nougat::paths::component_runtime("xenia") / "xenia.exe").string();',
            "v50 Xenia master Windows path",
        )
        text = text.replace(
            '"components/games/runtime/dosbox-staging/, or set NOUGAT_DOSBOX."',
            '"the managed Games component, use a system DOSBox, or set NOUGAT_DOSBOX."',
        )
        text = text.replace(
            '"xenia_canary.exe in components/games/runtime/xenia/, "',
            '"the managed Xenia component, "',
        )

        # Stable, version-independent cache/state locations.
        text = replace_count(
            text,
            'std::filesystem::path(home_dir()) / ".cache" / "reddmedia" / "games" / "artwork"',
            'nougat::paths::layout().artwork_cache / "games" / "remote"',
            1,
            "v50 Games remote artwork cache",
        )
        text = replace_count(
            text,
            'std::filesystem::path(home_dir()) / ".cache" / "reddmedia" / "games" / "artwork-prepared-v49"',
            'nougat::paths::layout().artwork_cache / "games" / "prepared"',
            1,
            "v50 Games prepared artwork cache",
        )
        text = replace_once(
            text,
            'const std::filesystem::path cache_dir = std::filesystem::path(home_dir()) / ".cache" / "reddmedia" / "games";\n'
            '        const std::filesystem::path prepared_dir = cache_dir / "artwork-prepared-v49";',
            'const std::filesystem::path cache_dir = nougat::paths::layout().artwork_cache / "games";\n'
            '        const std::filesystem::path prepared_dir = cache_dir / "prepared";',
            "v50 Games artwork prefetch cache",
        )
        text = replace_once(
            text,
            'const std::filesystem::path dir = std::filesystem::path(home_dir()) /\n'
            '            ".cache" / "reddmedia" / "games" / "logs";',
            'const std::filesystem::path dir = nougat::paths::layout().logs / "games";',
            "v50 Games log path",
        )
        text = replace_once(
            text,
            'const std::string dir=home_dir()+"/.cache/reddmedia/world_tv/artwork";',
            'const std::string dir=(nougat::paths::layout().artwork_cache / "world_tv").string();',
            "v50 World TV artwork cache",
        )

        # Version identity. Retained v49 regression-test flag names deliberately stay v49.
        text = replace_once(
            text,
            'const std::string versionLabel = "v0.0.49";',
            'const std::string versionLabel = "v0.0.50";',
            "v50 header version",
        )
        text = replace_once(
            text,
            'input.app_version = "Nougat Media Suite v0.0.49";',
            'input.app_version = "Nougat Media Suite v0.0.50";',
            "v50 diagnostic identity",
        )
        text = replace_once(
            text,
            'printf("Nougat Media Suite v0.0.49\\n");',
            'printf("Nougat Media Suite v0.0.50\\n");',
            "v50 command-line identity",
        )

        # User-facing Studio identity becomes Workshop; keep the internal enum to
        # minimize risk to navigation/palette state.
        text = replace_once(
            text,
            'draw_tab(studioTab,"Studio",ViewMode::Studio);',
            'draw_tab(studioTab,"Workshop",ViewMode::Studio);',
            "v50 Workshop top tab",
        )
        text = replace_once(
            text,
            'case ViewMode::Studio: return "Studio";',
            'case ViewMode::Studio: return "Workshop";',
            "v50 Workshop current-view name",
        )

        workshop_methods_and_draw = r'''    std::string workshop_worker_script() const {
        const std::filesystem::path installed = std::filesystem::path(exe_dir()) /
            "components" / "workshop" / "nougat_split_archive.py";
        const std::filesystem::path development = std::filesystem::path(exe_dir()).parent_path() /
            "components" / "workshop" / "nougat_split_archive.py";
        std::error_code ec;
        if (std::filesystem::is_regular_file(installed, ec) && !ec) return installed.string();
        ec.clear();
        if (std::filesystem::is_regular_file(development, ec) && !ec) return development.string();
        return installed.string();
    }

    static std::string workshop_bytes_label(long long bytes) {
        if (bytes < 0) bytes = 0;
        const unsigned long long value = static_cast<unsigned long long>(bytes);
        const unsigned long long mib = (value + 1024ULL * 1024ULL - 1ULL) / (1024ULL * 1024ULL);
        return std::to_string(value) + " bytes (" + std::to_string(mib) + " MiB)";
    }

    void set_workshop_status(const std::string& status) {
        std::lock_guard<std::mutex> lock(workshopState->mutex);
        workshopState->status = status;
        workshopState->updated = true;
    }

    bool workshop_busy() const {
        std::lock_guard<std::mutex> lock(workshopState->mutex);
        return workshopState->busy;
    }

    void start_workshop_inspect(const std::string& source, bool is_folder) {
        if (source.empty() || workshop_busy()) return;
        if (workshopWorker.joinable()) workshopWorker.join();
        workshopSourcePath = source;
        workshopSourceIsFolder = is_folder;
        workshopPartCount = 2;
        {
            std::lock_guard<std::mutex> lock(workshopState->mutex);
            workshopState->busy = true;
            workshopState->updated = false;
            workshopState->operation = 1;
            workshopState->status = "Inspecting source tree...";
            workshopState->total_bytes = 0;
            workshopState->file_count = 0;
            workshopState->directory_count = 0;
            workshopState->largest_file_bytes = 0;
            workshopState->suggested_parts = 1;
        }
        redraw();
        const std::shared_ptr<WorkshopUiState> state = workshopState;
        const std::string worker_script = workshop_worker_script();
        workshopWorker = std::thread([state, worker_script, source]() {
            nougat::workshop::SplitArchiveService service(worker_script);
            const nougat::workshop::CommandResult result = service.inspect(source);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (result.ok()) {
                state->total_bytes = json_value_number(result.standard_output, "total_bytes");
                state->file_count = json_value_number(result.standard_output, "file_count");
                state->directory_count = json_value_number(result.standard_output, "directory_count");
                state->largest_file_bytes = json_value_number(result.standard_output, "largest_file_bytes");
                state->suggested_parts = std::max<long long>(1, json_value_number(result.standard_output, "suggested_parts"));
                state->status = "Inspection complete. Choose output and split settings.";
            } else {
                const std::string detail = !result.standard_error.empty() ? result.standard_error : result.standard_output;
                state->status = detail.empty() ? "Workshop inspection failed." : "Inspection failed: " + detail;
            }
            state->busy = false;
            state->updated = true;
        });
    }

    void start_workshop_split() {
        if (workshop_busy()) return;
        if (workshopSourcePath.empty()) { set_workshop_status("Choose a source file or folder first."); redraw(); return; }
        if (workshopOutputFolder.empty()) { set_workshop_status("Choose an output folder first."); redraw(); return; }
        if (workshopWorker.joinable()) workshopWorker.join();
        const std::string source = workshopSourcePath;
        const std::string output = workshopOutputFolder;
        const bool by_count = workshopUsePartCount;
        const unsigned count = std::max(1U, workshopPartCount);
        const unsigned long long max_bytes = std::max<unsigned long long>(25ULL, workshopMaxPartMiB) * 1024ULL * 1024ULL;
        {
            std::lock_guard<std::mutex> lock(workshopState->mutex);
            workshopState->busy = true;
            workshopState->updated = false;
            workshopState->operation = 2;
            workshopState->status = "Splitting, hashing, and verifying parts...";
        }
        redraw();
        const std::shared_ptr<WorkshopUiState> state = workshopState;
        const std::string worker_script = workshop_worker_script();
        workshopWorker = std::thread([state, worker_script, source, output, by_count, count, max_bytes]() {
            nougat::workshop::SplitArchiveService service(worker_script);
            const nougat::workshop::CommandResult result = by_count
                ? service.split_by_part_count(source, output, count)
                : service.split_by_max_size(source, output, max_bytes);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (result.ok()) {
                const std::string manifest = json_value_string(result.standard_output, "manifest");
                state->status = manifest.empty() ? "Split complete and verified." : "Split complete: " + manifest;
            } else {
                const std::string detail = !result.standard_error.empty() ? result.standard_error : result.standard_output;
                state->status = detail.empty() ? "Workshop split failed." : "Split failed: " + detail;
            }
            state->busy = false;
            state->updated = true;
        });
    }

    void start_workshop_reassemble(const std::string& selected) {
        if (workshop_busy() || selected.empty()) return;
        if (workshopOutputFolder.empty()) { set_workshop_status("Choose an output folder before reassembling."); redraw(); return; }
        if (workshopWorker.joinable()) workshopWorker.join();
        const std::string output = workshopOutputFolder;
        {
            std::lock_guard<std::mutex> lock(workshopState->mutex);
            workshopState->busy = true;
            workshopState->updated = false;
            workshopState->operation = 3;
            workshopState->status = "Verifying every part before reassembly...";
        }
        redraw();
        const std::shared_ptr<WorkshopUiState> state = workshopState;
        const std::string worker_script = workshop_worker_script();
        workshopWorker = std::thread([state, worker_script, selected, output]() {
            nougat::workshop::SplitArchiveService service(worker_script);
            const nougat::workshop::CommandResult result = service.reassemble(selected, output);
            std::lock_guard<std::mutex> lock(state->mutex);
            if (result.ok()) {
                const std::string restored = json_value_string(result.standard_output, "output");
                state->status = restored.empty() ? "Reassembly complete and verified." : "Reassembled and verified: " + restored;
            } else {
                const std::string detail = !result.standard_error.empty() ? result.standard_error : result.standard_output;
                state->status = detail.empty() ? "Workshop reassembly failed." : "Reassembly failed: " + detail;
            }
            state->busy = false;
            state->updated = true;
        });
    }

    void poll_workshop_worker() {
        bool updated = false;
        int operation = 0;
        long long suggested = 1;
        {
            std::lock_guard<std::mutex> lock(workshopState->mutex);
            updated = workshopState->updated;
            operation = workshopState->operation;
            suggested = workshopState->suggested_parts;
            if (updated) workshopState->updated = false;
        }
        if (!updated) return;
        if (workshopWorker.joinable()) workshopWorker.join();
        if (operation == 1) workshopPartCount = static_cast<unsigned>(std::max<long long>(1, suggested));
        if (!fullscreen && currentView == ViewMode::Studio) redraw();
    }

    void handle_workshop_click(int x, int y) {
        if (workshop_busy()) return;
        if (workshopFileBtn.contains(x,y)) {
            const std::string selected = choose_file_dialog();
            if (!selected.empty()) start_workshop_inspect(selected, false);
            return;
        }
        if (workshopFolderBtn.contains(x,y)) {
            const std::string selected = choose_folder_dialog();
            if (!selected.empty()) start_workshop_inspect(selected, true);
            return;
        }
        if (workshopOutputBtn.contains(x,y)) {
            const std::string selected = choose_folder_dialog();
            if (!selected.empty()) { workshopOutputFolder = selected; set_workshop_status("Output folder selected."); redraw(); }
            return;
        }
        if (workshopModeBtn.contains(x,y)) {
            workshopUsePartCount = !workshopUsePartCount;
            redraw();
            return;
        }
        if (workshopMinusBtn.contains(x,y)) {
            if (workshopUsePartCount) workshopPartCount = std::max(1U, workshopPartCount - (workshopPartCount > 1U ? 1U : 0U));
            else workshopMaxPartMiB = std::max<unsigned long long>(25ULL, workshopMaxPartMiB > 25ULL ? workshopMaxPartMiB - 25ULL : 25ULL);
            redraw();
            return;
        }
        if (workshopPlusBtn.contains(x,y)) {
            if (workshopUsePartCount) workshopPartCount = std::min(9999U, workshopPartCount + 1U);
            else workshopMaxPartMiB = std::min<unsigned long long>(16384ULL, workshopMaxPartMiB + 25ULL);
            redraw();
            return;
        }
        if (workshopSplitBtn.contains(x,y)) { start_workshop_split(); return; }
        if (workshopReassembleBtn.contains(x,y)) {
            const std::string selected = choose_file_dialog();
            if (!selected.empty()) start_workshop_reassemble(selected);
            return;
        }
    }

    void draw_studio_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Studio);
        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Studio);
        section_text(target, 28, 70, "WORKSHOP", palette.text);
        text(target, 28, 96, "File engineering and production workspace.", palette.muted);
        Rect panel{28, 118, std::max(240, W - 56), std::max(220, H - 148)};
        draw_primary_panel(target, panel, palette);
        text(target, panel.x + 16, panel.y + 28, "Split / Reassemble", palette.text);

        const int buttonY = panel.y + 42;
        workshopFileBtn = {panel.x + 16, buttonY, 108, 28};
        workshopFolderBtn = {workshopFileBtn.x + workshopFileBtn.w + 8, buttonY, 108, 28};
        workshopOutputBtn = {workshopFolderBtn.x + workshopFolderBtn.w + 8, buttonY, 120, 28};
        button_on(target, workshopFileBtn, "Source File");
        button_on(target, workshopFolderBtn, "Source Folder");
        button_on(target, workshopOutputBtn, "Output Folder");

        const std::string sourceLabel = workshopSourcePath.empty() ? "Source: not selected" : "Source: " + workshopSourcePath;
        const std::string outputLabel = workshopOutputFolder.empty() ? "Output: not selected" : "Output: " + workshopOutputFolder;
        text(target, panel.x + 16, buttonY + 48, head_to_width(sourceLabel, panel.w - 32), palette.text);
        text(target, panel.x + 16, buttonY + 68, head_to_width(outputLabel, panel.w - 32), palette.muted);

        const int settingsY = buttonY + 84;
        workshopModeBtn = {panel.x + 16, settingsY, 156, 28};
        workshopMinusBtn = {workshopModeBtn.x + workshopModeBtn.w + 8, settingsY, 36, 28};
        workshopPlusBtn = {workshopMinusBtn.x + workshopMinusBtn.w + 82, settingsY, 36, 28};
        button_on(target, workshopModeBtn, workshopUsePartCount ? "Mode: Part Count" : "Mode: Max Size");
        button_on(target, workshopMinusBtn, "-");
        button_on(target, workshopPlusBtn, "+");
        const std::string settingValue = workshopUsePartCount
            ? std::to_string(workshopPartCount) + " parts"
            : std::to_string(workshopMaxPartMiB) + " MiB";
        text(target, workshopMinusBtn.x + workshopMinusBtn.w + 8, settingsY + 19, settingValue, palette.text);

        WorkshopUiState snapshot;
        {
            std::lock_guard<std::mutex> lock(workshopState->mutex);
            snapshot.status = workshopState->status;
            snapshot.busy = workshopState->busy;
            snapshot.total_bytes = workshopState->total_bytes;
            snapshot.file_count = workshopState->file_count;
            snapshot.directory_count = workshopState->directory_count;
            snapshot.largest_file_bytes = workshopState->largest_file_bytes;
            snapshot.suggested_parts = workshopState->suggested_parts;
        }

        int infoY = settingsY + 52;
        if (snapshot.total_bytes > 0 || snapshot.file_count > 0 || snapshot.directory_count > 0) {
            text(target, panel.x + 16, infoY,
                 "Total: " + workshop_bytes_label(snapshot.total_bytes) +
                 "   Files: " + std::to_string(snapshot.file_count) +
                 "   Folders: " + std::to_string(snapshot.directory_count), palette.text);
            infoY += 20;
            text(target, panel.x + 16, infoY,
                 "Largest file: " + workshop_bytes_label(snapshot.largest_file_bytes) +
                 "   Suggested at 450 MiB: " + std::to_string(std::max<long long>(1, snapshot.suggested_parts)) + " part(s)", palette.muted);
            infoY += 24;

            const unsigned long long oneMiB = 1024ULL * 1024ULL;
            std::string plan;
            if (workshopUsePartCount) {
                const unsigned parts = std::max(1U, workshopPartCount);
                const unsigned long long target = (static_cast<unsigned long long>(std::max<long long>(0, snapshot.total_bytes)) + parts - 1ULL) / parts;
                const unsigned long long targetMiB = (target + oneMiB - 1ULL) / oneMiB + 1ULL;
                plan = "Current plan: " + std::to_string(parts) + " part(s), approximately " + std::to_string(targetMiB) + " MiB payload target each.";
            } else {
                const unsigned long long ceiling = std::max<unsigned long long>(25ULL, workshopMaxPartMiB) * oneMiB;
                const unsigned long long payload = ceiling > oneMiB ? ceiling - oneMiB : ceiling;
                const unsigned long long total = static_cast<unsigned long long>(std::max<long long>(0, snapshot.total_bytes));
                const unsigned long long count = std::max<unsigned long long>(1ULL, (total + payload - 1ULL) / std::max<unsigned long long>(1ULL, payload));
                plan = "Current plan: about " + std::to_string(count) + " part(s), each no larger than " + std::to_string(workshopMaxPartMiB) + " MiB.";
            }
            text(target, panel.x + 16, infoY, head_to_width(plan, panel.w - 32), palette.text);
            infoY += 30;
        }

        workshopSplitBtn = {panel.x + 16, infoY, 118, 30};
        workshopReassembleBtn = {workshopSplitBtn.x + workshopSplitBtn.w + 10, infoY, 148, 30};
        button_on(target, workshopSplitBtn, snapshot.busy ? "Working..." : "Split + Verify");
        button_on(target, workshopReassembleBtn, "Reassemble");
        text(target, panel.x + 16, infoY + 52,
             head_to_width(snapshot.status, panel.w - 32), snapshot.busy ? palette.text : palette.muted);
        text(target, panel.x + 16, infoY + 74,
             "Format: NOUGAT_SPLIT_ARCHIVE v1 | SHA-256 verified parts and reconstructed files", palette.muted);
    }
'''

        old_draw = '''    void draw_studio_screen(Drawable target) {\n        const ViewPalette palette = palette_for(ViewMode::Studio);\n        draw_quilted_background(target, {0,32,W,H-32}, ViewMode::Studio);\n        section_text(target, 28, 70, "GOLD STUDIO", palette.text);\n        text(target, 28, 96, "Nougat media-processing workspace foundation.", palette.muted);\n        Rect panel{28, 118, std::max(240, W - 56), std::max(150, H - 148)};\n        draw_primary_panel(target, panel, palette);\n        text(target, panel.x + 16, panel.y + 30, "Planned processing engine: FFmpeg/libav-backed Convert, Audio Lab, Quick Edit, Batch, and full timeline Studio.", palette.text);\n        text(target, panel.x + 16, panel.y + 56, "v0.0.40 keeps the Gold Studio navigation/palette foundation; processing tools remain roadmap work.", palette.muted);\n    }\n'''
        text = replace_once(text, old_draw, workshop_methods_and_draw, "v50 Workshop page implementation")

        text = replace_once(
            text,
            '            &homeTab,&videoPlayerTab,&libraryTab,&discoverTab,&liveTvTab,&worldTvTab,&nougatTab,&ytdlpTab,&studioTab,&gamesTab,&debugTab,\n',
            '            &homeTab,&videoPlayerTab,&libraryTab,&discoverTab,&liveTvTab,&worldTvTab,&nougatTab,&ytdlpTab,&studioTab,&gamesTab,&debugTab,\n'
            '            &workshopFileBtn,&workshopFolderBtn,&workshopOutputBtn,&workshopModeBtn,&workshopMinusBtn,&workshopPlusBtn,&workshopSplitBtn,&workshopReassembleBtn,\n',
            "v50 Workshop hover targets",
        )

        text = replace_once(
            text,
            '        if (currentView == ViewMode::WorldTV) {\n'
            '            handle_world_tv_click(x,y,eventTime);\n'
            '            return;\n'
            '        }\n'
            '        if (currentView == ViewMode::Games) {\n',
            '        if (currentView == ViewMode::WorldTV) {\n'
            '            handle_world_tv_click(x,y,eventTime);\n'
            '            return;\n'
            '        }\n'
            '        if (currentView == ViewMode::Studio) {\n'
            '            handle_workshop_click(x,y);\n'
            '            return;\n'
            '        }\n'
            '        if (currentView == ViewMode::Games) {\n',
            "v50 Workshop click routing",
        )

        text = replace_count(
            text,
            '            poll_server_worker();\n',
            '            poll_server_worker();\n            poll_workshop_worker();\n',
            1,
            "v50 Workshop worker polling",
        )
        text = replace_once(
            text,
            '        if (serverWorker.joinable()) serverWorker.join();\n'
            '        if (debugWorker.joinable()) {\n',
            '        if (serverWorker.joinable()) serverWorker.join();\n'
            '        if (workshopWorker.joinable()) workshopWorker.join();\n'
            '        if (debugWorker.joinable()) {\n',
            "v50 Workshop shutdown join",
        )

        validate_applied(text)
        MAIN.write_text(text, encoding="utf-8")
        print("PASS: applied guarded v0.0.50 Workshop, version, path, cache, and managed-runtime patch")
        return 0
    except Exception as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        print("No v0.0.50 main.cpp write was performed unless every earlier guard passed and final validation succeeded.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
