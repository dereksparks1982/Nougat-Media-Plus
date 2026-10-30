#!/usr/bin/env python3
"""Static contract for the new Studio Media Convert panel.

This does not build, launch, or access user media. Runtime and real conversion
validation remain separate. Existing approved Studio control coordinates must
not be moved by the converter integration.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
MAIN = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
SERVICE = (ROOT / "src/studio/studio_service.cpp").read_text(encoding="utf-8")

class MediaConvertUiContract(unittest.TestCase):
    def test_approved_tools_and_drone_geometry_is_unchanged(self):
        expected = (
            "studioToolsTab={kPageLeft,kTopBarH*2+56,126,34};",
            "studioDroneTab={studioToolsTab.x+studioToolsTab.w+10,studioToolsTab.y,126,34};",
            "studioFileSplitterToolBtn={panel.x+18,panel.y+78,studioToolW,38};",
            "studioFileAssemblerToolBtn={panel.x+18+studioToolW+studioToolGap,panel.y+78,studioToolW,38};",
        )
        for position in expected:
            self.assertIn(position, MAIN)

    def test_media_convert_is_a_separate_tools_panel(self):
        self.assertIn("enum class StudioPanel { Tools, Drone, FileSplitter, MediaConvert }", MAIN)
        self.assertIn('button_on(target,studioMediaConvertToolBtn,"Media Convert")', MAIN)
        self.assertIn("studioMediaConvertToolBtn={panel.x+18,panel.y+202,studioToolW,38};", MAIN)
        self.assertIn("if (studioPanel == StudioPanel::MediaConvert)", MAIN)
        self.assertIn('text(target,panel.x+18,panel.y+28,"TOOLS",palette.text)', MAIN)
        self.assertIn("studioPanel=StudioPanel::FileSplitter;", MAIN)
        self.assertIn("studioPanel=StudioPanel::Drone;", MAIN)

    def test_source_and_output_browse_inside_nougat(self):
        self.assertIn("open_studio_browser(StudioBrowserPurpose::SourceFile)", MAIN)
        self.assertIn("open_studio_browser(StudioBrowserPurpose::OutputFolder)", MAIN)
        self.assertIn("studioConvertSource = studioBrowserSelectedPath;", MAIN)
        self.assertIn("studioConvertFolder = studioBrowserPath;", MAIN)
        self.assertIn('"Browse inside Nougat; no external file chooser."', MAIN)

    def test_eight_presets_and_safe_output_names(self):
        self.assertIn("static_cast<::nougat::studio::Preset>((index + 1) % 8)", MAIN)
        self.assertIn('source.stem().string() + "-nougat"', MAIN)
        self.assertIn("std::filesystem::exists(proposed,ec)", MAIN)
        self.assertIn("std::filesystem::is_symlink(proposed,ec)", MAIN)
        self.assertIn("job.state == ::nougat::studio::JobState::Queued", MAIN)
        self.assertIn("job.state == ::nougat::studio::JobState::Running", MAIN)
        self.assertIn("if (!reserved) return proposed;", MAIN)

    def test_async_export_and_explicit_lifetime(self):
        self.assertIn("std::async(std::launch::async, [service, request]", MAIN)
        self.assertIn("service->submit(request)", MAIN)
        self.assertIn("studioConvertSubmission.wait_for(std::chrono::milliseconds(0))", MAIN)
        self.assertIn("studioConvertSubmission.get()", MAIN)
        self.assertIn("if (studioConvertSubmission.valid()) studioConvertSubmission.wait();", MAIN)
        self.assertIn("studioConvertService.reset();", MAIN)
        self.assertIn("poll_studio_convert();", MAIN)

    def test_queue_job_selection_and_controls(self):
        self.assertIn("studioConvertService->cancel(studioConvertSelectedJob)", MAIN)
        self.assertIn("studioConvertService->retry(studioConvertSelectedJob)", MAIN)
        self.assertIn("studioConvertRows.push_back({row,job.id})", MAIN)
        self.assertIn("studioConvertSelectedJob = row.second", MAIN)
        self.assertIn("studioConvertService->jobs()", MAIN)
        self.assertIn("inspector_.inspect(request.input)", SERVICE)
        self.assertIn("engine_.enqueue_batch(items, preset)", SERVICE)

if __name__ == "__main__":
    unittest.main(verbosity=2)
