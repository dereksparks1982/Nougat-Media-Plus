// Standard-library-only queue integration test with a local fake FFmpeg.
// Does not need codecs, network, video files, or an actual Nougat GUI.
#include "studio/media_job_engine.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace nougat::studio;

struct Scratch {
    fs::path dir;
    Scratch() {
        std::string pattern = (fs::temp_directory_path() / "nougat-studio-pipeline-XXXXXX").string();
        std::vector<char> data(pattern.begin(), pattern.end());
        data.push_back('\0');
        char* result = mkdtemp(data.data());
        assert(result);
        dir = result;
    }
    ~Scratch() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};
void write(const fs::path& file, const std::string& text, bool executable = false) {
    std::ofstream stream(file, std::ios::binary);
    assert(stream.good());
    stream << text;
    stream.close();
    if (executable) assert(chmod(file.c_str(), 0700) == 0);
}
Job wait_for(const Engine& engine, std::uint64_t id, JobState desired, int limit_ms = 4000) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(limit_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        for (const Job& job : engine.snapshot())
            if (job.id == id && job.state == desired) return job;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(false && "Studio job never reached the expected state");
    return {};
}
int main() {
    Scratch scratch;
    const auto source = scratch.dir / "input.wav";
    const auto ffmpeg = scratch.dir / "mock-ffmpeg";
    const auto result = scratch.dir / "output.mp3";
    const auto queue = scratch.dir / "state/queue";
    write(source, "test source is not a real WAV file");
    write(ffmpeg,
          "#!/bin/sh\n"
          "for last; do :; done\n"
          "printf 'fake-export' > \"$last\" || exit 4\n"
          "printf 'out_time_us=2000000\\nprogress=end\\n'\n", true);

    {
        Engine engine(queue, ffmpeg.string());
        const auto job = engine.enqueue(source, result, Preset::AudioMp3, 2000);
        assert(job.ok && job.id > 0);
        const auto finished = wait_for(engine, job.id, JobState::Completed);
        assert(finished.progress_percent == 100);
        assert(fs::file_size(result) == 11U);
        assert(!engine.retry(job.id));
        assert(!engine.cancel(job.id));
        // An earlier output is preserved; no overwrite or truncation.
        assert(!engine.enqueue(source, result, Preset::AudioMp3).ok);
    }
    {
        Engine restored(queue, ffmpeg.string());
        assert(restored.persistence_ok());
        const auto jobs = restored.snapshot();
        assert(jobs.size() == 1);
        assert(jobs[0].state == JobState::Completed);
        assert(fs::file_size(result) == 11U);
    }

    // A cancelling process must not publish an export.
    const auto slow = scratch.dir / "slow-ffmpeg";
    write(slow, "#!/bin/sh\n"
                "for last; do :; done\n"
                "sleep 10\n"
                "printf 'not-published' > \"$last\"\n", true);
    const auto cancelled_output = scratch.dir / "cancel.mp3";
    {
        Engine engine(scratch.dir / "cancel-state/queue", slow.string());
        const auto created = engine.enqueue(source, cancelled_output, Preset::AudioMp3);
        assert(created.ok);
        wait_for(engine, created.id, JobState::Running);
        assert(engine.cancel(created.id));
        (void)wait_for(engine, created.id, JobState::Cancelled, 4500);
        assert(!fs::exists(cancelled_output));
    }

    // A failing encoder may write a partial temporary file but must not
    // publish it under the requested output name.
    const auto broken = scratch.dir / "broken-ffmpeg";
    write(broken, "#!/bin/sh\n"
                  "for last; do :; done\n"
                  "printf 'partial-output' > \"$last\"\n"
                  "exit 8\n", true);
    const auto failed_output = scratch.dir / "failed.mp3";
    {
        Engine engine(scratch.dir / "failed-state/queue", broken.string());
        const auto created = engine.enqueue(source, failed_output, Preset::AudioMp3);
        assert(created.ok);
        const Job failed = wait_for(engine, created.id, JobState::Failed);
        assert(!failed.error.empty());
        assert(!fs::exists(failed_output));
    }
    return 0;
}
