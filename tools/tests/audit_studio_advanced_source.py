#!/usr/bin/env python3
"""Source contract audit for advanced Nougat Studio exports.

This checks repository wiring only: it never runs encoders, accesses media,
opens a GUI, moves widgets, or contacts GitHub. It cannot establish runtime
correctness without actual C++ compilation and media test execution.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
def read(name):
    return (ROOT / name).read_text(encoding="utf-8")

class AdvancedStudioContract(unittest.TestCase):
    def test_profile_commands_are_argv_only(self):
        core=read("src/studio/advanced_export.cpp")
        header=read("src/studio/advanced_export.hpp")
        self.assertIn("struct ExportEdits",header)
        for name in ("validate_export_edits", "build_export_command",
                     "serialize_export_edits", "deserialize_export_edits"):
            self.assertIn(name,core)
        for token in ("crop=", "transpose=clock", "scale=", "force_original_aspect_ratio",
                      "loudnorm=", "volume=", "subtitles=filename=", "mov_text",
                      "Preset::SubtitleSrt", "-crf", "-b:v", "-b:a", "-map", "-ss", "-t"):
            self.assertIn(token,core)
        self.assertNotIn("system(",core)
        self.assertNotIn("popen(",core)
        self.assertIn("filter_safe_path",core)
        self.assertIn("Subtitle path exceeds the supported limit.",core)
    def test_queue_settings_are_durable_and_backward_compatible(self):
        source=read("src/studio/media_job_engine.cpp")
        head=read("src/studio/media_job_engine.hpp")
        for version in ("NOUGAT_STUDIO_QUEUE_V1","NOUGAT_STUDIO_QUEUE_V2",
                        "NOUGAT_STUDIO_QUEUE_V3"):
            self.assertIn(version,source)
        self.assertIn("hex_encode(serialize_export_edits(job.edits))",source)
        self.assertIn("deserialize_export_edits(edit_data, edits)",source)
        self.assertIn("build_export_command(",source)
        self.assertIn("job.edits = item.edits",source)
        self.assertIn("job.edits = edits",source)
        self.assertIn("ExportEdits edits;",head)
        self.assertIn("Preset::SubtitleSrt",source)
    def test_named_profile_library_persists_atomically(self):
        source=read("src/studio/profile_library.cpp")
        header=read("src/studio/profile_library.hpp")
        for term in ("NOUGAT_STUDIO_PROFILES_V1", "mkstemp", "fsync", "rename(",
                     "path_symlink", "ProfileLibrary::load", "ProfileLibrary::upsert",
                     "ProfileLibrary::erase", "serialize_export_edits",
                     "deserialize_export_edits"):
            self.assertIn(term,source)
        self.assertIn("std::optional<NamedProfile>",header)
        self.assertIn("std::mutex mutex_",header)
    def test_studio_service_uses_inspection_and_profile_data(self):
        source=read("src/studio/studio_service.cpp")
        header=read("src/studio/studio_service.hpp")
        self.assertIn("validate_export_edits",source)
        self.assertIn("request.edits.start_ms",source)
        self.assertIn("request.edits.duration_ms",source)
        self.assertIn("request.edits.audio_stream",source)
        self.assertIn("request.edits.subtitle_stream",source)
        self.assertIn("engine_.enqueue(",source)
        self.assertIn("items.push_back({request.input, request.output, progress_duration, request.edits})",source)
        self.assertIn("StudioService::save_profile",source)
        self.assertIn("StudioService::apply_profile",source)
        self.assertIn("estimated_output_bytes",header)
        self.assertIn("available_disk_bytes",header)
    def test_builds_and_tests_registered_but_not_automated(self):
        root=read("CMakeLists.txt")
        tests=read("tools/tests/studio/CMakeLists.txt")
        for name in ("advanced_export.cpp","profile_library.cpp"):
            self.assertIn(name,root)
            self.assertIn(name,tests)
        for name in ("test_studio_advanced_export","test_studio_profiles"):
            self.assertIn(name,tests)
            self.assertTrue((ROOT/"tools/tests"/(name+".cpp")).is_file())
        self.assertIn("Nougat_Media_Plus_v68",root)  # Existing unchanged native target
    def test_no_advanced_editor_geometry_was_replaced(self):
        main=read("src/main.cpp")
        self.assertIn("studioConvertQueueBtn",main)
        self.assertIn("studioConvertCancelBtn",main)
        self.assertIn("studioConvertHistoryViewport",main)
        self.assertIn("studioConvertBatchSource",main)

if __name__=="__main__":
    unittest.main(verbosity=2)
