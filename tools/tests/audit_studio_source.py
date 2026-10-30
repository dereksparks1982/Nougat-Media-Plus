#!/usr/bin/env python3
"""Source-only Nougat Studio queue contract audit.

Does not start FFmpeg, open media, create output, or contact a network.
A green source audit cannot replace an executable's compile/runtime tests.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
H = (ROOT / "src/studio/media_job_engine.hpp").read_text(encoding="utf-8")
CPP = (ROOT / "src/studio/media_job_engine.cpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

class StudioContracts(unittest.TestCase):
    def test_cmake_integration_and_no_version_bump(self):
        self.assertIn("src/studio/media_job_engine.cpp", CMAKE)
        self.assertIn("src/games/emulator_registry.cpp", CMAKE)
        self.assertIn("Nougat_Media_Plus_v68", CMAKE)  # existing, not changed
        self.assertNotIn("Nougat_Media_Plus_v70", CMAKE)

    def test_eight_safe_preset_mappings(self):
        presets = ("RemuxMp4", "RemuxMkv", "VideoMp4H264", "VideoWebmVp9",
                   "AudioMp3", "AudioFlac", "AudioWav", "AudioOpus")
        for name in presets:
            with self.subTest(name=name):
                self.assertIn("Preset::" + name, CPP)
                self.assertIn(name, H)
        for engine in ("libx264", "libvpx-vp9", "libmp3lame", "libopus", "pcm_s16le"):
            self.assertIn(engine, CPP)
        self.assertIn('"copy"', CPP)

    def test_safe_outputs_and_rollback(self):
        self.assertIn("mkstemp(temp_name.data())", CPP)
        self.assertIn("link(temporary.c_str(), job.output.c_str())", CPP)
        self.assertIn("unlink(path.c_str())", CPP)
        self.assertIn("lstat(path.c_str(), &info)", CPP)
        self.assertIn("Output already exists", CPP)
        self.assertIn("Output extension must match", CPP)
        self.assertIn("Input must be a readable regular file", CPP)
        self.assertIn("fsync(dir_fd)", CPP)

    def test_atomic_batch_submission(self):
        self.assertIn("BatchResult Engine::enqueue_batch", CPP)
        self.assertIn("target_paths.insert(item.output.lexically_normal())", CPP)
        self.assertIn("jobs_.resize(old_size)", CPP)
        self.assertIn("if (!save_locked())", CPP)
        self.assertIn("BatchResult enqueue_batch", H)

    def test_durable_history_and_diagnostic_contract(self):
        self.assertIn("NOUGAT_STUDIO_QUEUE_V2", CPP)
        self.assertIn("NOUGAT_STUDIO_QUEUE_V1", CPP)  # backwards-compatible migration
        self.assertIn("durable_next_id", CPP)
        self.assertIn("next_id_ = next;", CPP)
        self.assertIn("bool Engine::forget", CPP)
        self.assertIn("std::string Engine::recent_log", CPP)
        self.assertIn("O_NOFOLLOW | O_NONBLOCK", CPP)
        self.assertIn("Studio encoder log is not a regular file", CPP)
        self.assertIn("std::clamp<std::size_t>", CPP)
        self.assertIn("bool forget(std::uint64_t id)", H)
        self.assertIn("recent_log(std::uint64_t id", H)

    def test_worker_lifecycle_and_recovery(self):
        for required in ("JobState::Queued", "JobState::Running",
                         "JobState::Completed", "JobState::Failed",
                         "JobState::Cancelled", "JobState::Interrupted"):
            self.assertIn(required, CPP)
        self.assertIn("cancel_running_.store(true)", CPP)
        self.assertIn("kill(-child, SIGKILL)", CPP)
        self.assertIn("NOUGAT_STUDIO_QUEUE_V1", CPP)
        self.assertIn("Job interrupted by a previous application exit", CPP)
        self.assertIn("Retry explicitly", CPP)
        self.assertIn("cv_.wait(lock", CPP)

    def test_argv_and_log_handling(self):
        self.assertIn("execv(ffmpeg_.c_str(), argv.data())", CPP)
        self.assertNotIn("system(", CPP)
        self.assertNotIn("popen(", CPP)
        self.assertIn("-progress", CPP)
        self.assertIn("job.id", CPP)
        self.assertIn("O_NOFOLLOW", CPP)

    def test_ffprobe_contract(self):
        header = (ROOT / "src/studio/media_inspector.hpp").read_text(encoding="utf-8")
        source = (ROOT / "src/studio/media_inspector.cpp").read_text(encoding="utf-8")
        for name in ("MediaProbe", "StreamInfo", "CopyAssessment", "MediaInspector"):
            self.assertIn(name, header)
        for check in ("-show_entries", "format=duration:stream=codec_type,codec_name,width,height",
                      "65536U", "std::chrono::steady_clock::now()", "execv(executable.c_str()",
                      "waitpid(child", "CopyAssessment::CompatibleCandidate"):
            self.assertIn(check, source)
        self.assertIn("src/studio/media_inspector.cpp", CMAKE)
        self.assertIn("src/studio/studio_service.cpp", CMAKE)

    def test_studio_controller_and_guarded_exports(self):
        head = (ROOT / "src/studio/studio_service.hpp").read_text(encoding="utf-8")
        source = (ROOT / "src/studio/studio_service.cpp").read_text(encoding="utf-8")
        self.assertIn("ExportPlan plan", head)
        self.assertIn("submit_batch", head)
        self.assertIn("inspector_.inspect", source)
        self.assertIn("engine_.enqueue_batch(items, preset)", source)
        self.assertIn("request.output.extension() != extension", source)
        self.assertIn("Source has no audio track", source)
        self.assertIn("Source has no video track", source)
        self.assertIn("seconds(120)", CPP)
        self.assertIn("private Studio job log folder", CPP)
        self.assertIn("Studio export was cancelled before publication", CPP)

    def test_isolated_subprocess_tests_are_present(self):
        for filename in ("test_studio_media_inspector.cpp",
                         "test_studio_fake_ffmpeg.cpp"):
            test = ROOT / "tools/tests" / filename
            self.assertTrue(test.is_file(), filename)
            content = test.read_text(encoding="utf-8")
            self.assertIn("mkdtemp", content)
            self.assertNotIn("curl ", content)

    def test_isolated_studio_core_project(self):
        isolated = (ROOT / "tools/tests/studio/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("project(NougatStudioCoreTests", isolated)
        self.assertIn("add_library(nougat_studio_core STATIC", isolated)
        self.assertIn("media_inspector.cpp", isolated)
        self.assertIn("studio_service.cpp", isolated)
        self.assertIn("test_studio_media_inspector", isolated)
        self.assertIn("test_studio_fake_ffmpeg", isolated)
        self.assertIn("test_studio_media_job_engine", isolated)
        self.assertIn("enable_testing()", isolated)
        self.assertTrue((ROOT / "tools/tests/studio/README.md").is_file())

    def test_test_source_exists(self):
        path = ROOT / "tools/tests/test_studio_media_job_engine.cpp"
        self.assertTrue(path.is_file())
        contents = path.read_text(encoding="utf-8")
        self.assertIn("persistence_ok()", contents)
        self.assertIn("Engine engine(queue", contents)
        self.assertIn("find_job", contents)

if __name__ == "__main__":
    unittest.main(verbosity=2)
