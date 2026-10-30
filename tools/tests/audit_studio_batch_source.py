#!/usr/bin/env python3
"""Source contract check for the Nougat v0.0.69 folder batch and queue controls.

No filesystem traversal, FFmpeg invocation, GUI, ROM, installer or external drive
is accessed by this script. Real behavior still requires owner-authorized tests.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
PLANNER = (ROOT / "src/studio/folder_batch_planner.cpp").read_text(encoding="utf-8")
HEAD = (ROOT / "src/studio/folder_batch_planner.hpp").read_text(encoding="utf-8")
ENGINE = (ROOT / "src/studio/media_job_engine.cpp").read_text(encoding="utf-8")
ENGINE_H = (ROOT / "src/studio/media_job_engine.hpp").read_text(encoding="utf-8")
SERVICE = (ROOT / "src/studio/studio_service.cpp").read_text(encoding="utf-8")
SERVICE_H = (ROOT / "src/studio/studio_service.hpp").read_text(encoding="utf-8")
UI = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
TEST_CMAKE = (ROOT / "tools/tests/studio/CMakeLists.txt").read_text(encoding="utf-8")

class FolderBatchContracts(unittest.TestCase):
    def test_bounds_and_symlinks(self):
        self.assertIn("constexpr std::size_t kMaxEntries = 20000", PLANNER)
        self.assertIn("max_files > 1000", PLANNER)
        self.assertIn("max_depth > 8", PLANNER)
        self.assertIn("source_directory.is_absolute()", PLANNER)
        self.assertIn("output_directory.is_absolute()", PLANNER)
        self.assertIn("entry.is_symlink(check)", PLANNER)
        self.assertIn("fs::canonical(options.source_directory", PLANNER)
        self.assertIn("inside(output, source) || inside(source, output)", PLANNER)
        self.assertIn("fs::symlink_status(file, ec)", PLANNER)
        self.assertIn("destination_exists(proposed)", PLANNER)
        self.assertIn("reserved.insert(proposed.lexically_normal())", PLANNER)
        self.assertIn("std::sort(entries.begin(), entries.end()", PLANNER)
        self.assertIn("FolderBatchPlan plan_folder_batch", PLANNER)
        self.assertIn("bool recursive = false", HEAD)
    def test_batch_submission_is_not_automatic(self):
        self.assertIn("StudioService::preview_folder", SERVICE)
        self.assertIn("StudioService::submit_folder", SERVICE)
        self.assertIn("return submit_batch(preview.exports)", SERVICE)
        self.assertIn("preview_folder(const FolderBatchOptions", SERVICE_H)
        self.assertIn("submit_folder(const FolderBatchPlan", SERVICE_H)
        self.assertIn("studioConvertBatchPreviewReady", UI)
        self.assertIn("start_studio_batch_preview()", UI)
        self.assertIn("studioConvertBatchSubmission=std::async", UI)
        self.assertIn("Queue Export to proceed.", UI)
        self.assertIn("StudioBrowserPurpose::SourceFolder", UI)
        self.assertIn("Ctrl+B", UI)
        self.assertIn("Copied folder preview and skip reasons", UI)
        self.assertIn("studioConvertBatchPlan.skipped", UI)
    def test_pause_and_reorder(self):
        self.assertIn("void Engine::set_queue_paused", ENGINE)
        self.assertIn("bool Engine::queue_paused", ENGINE)
        self.assertIn("bool Engine::move_queued", ENGINE)
        self.assertIn("!queue_paused_", ENGINE)
        self.assertIn("std::iter_swap(found, other)", ENGINE)
        self.assertIn("cv_.notify_all()", ENGINE)
        self.assertIn("bool move_queued(std::uint64_t id, int direction)", ENGINE_H)
        self.assertIn("bool StudioService::move_queued", SERVICE)
        self.assertIn("studioConvertService->set_queue_paused", UI)
        self.assertIn("studioConvertService->move_queued", UI)
        self.assertIn("XK_Up || ks==XK_Down", UI)
        self.assertIn("Queue paused: active encode finishes", UI)
    def test_build_source_wiring(self):
        for content in (CMAKE, TEST_CMAKE):
            self.assertIn("folder_batch_planner.cpp", content)
        for name in ("test_studio_folder_batch", "test_studio_queue_controls"):
            self.assertIn(name, TEST_CMAKE)
            self.assertTrue((ROOT / "tools/tests" / (name+".cpp")).exists())
        self.assertIn("studio_service.cpp", CMAKE)
        self.assertIn("media_job_engine.cpp", CMAKE)
    def test_no_version_ui_geometry_change(self):
        self.assertIn("Nougat_Media_Plus_v68", CMAKE)  # untouched legacy build target
        self.assertIn("panel.y+278+verticalShift", UI)
        self.assertIn("panel.y+238+verticalShift", UI)
        self.assertIn("studioConvertQueueBtn", UI)
        self.assertIn("studioConvertCancelBtn", UI)
        self.assertIn("studioConvertRetryBtn", UI)

if __name__ == "__main__":
    unittest.main(verbosity=2)
