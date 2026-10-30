// Standard-library-only Studio queue regression test.
// Build manually after owner authorizes testing (NO GitHub Actions):
// c++ -std=c++17 -Wall -Wextra -Werror -pthread -Isrc
//   tools/tests/test_studio_media_job_engine.cpp src/studio/media_job_engine.cpp
//   -o /tmp/nougat-studio-queue-test
#include "studio/media_job_engine.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace nougat::studio;

struct TestDirectory {
    fs::path path;
    TestDirectory() {
        std::string pattern = (fs::temp_directory_path() / "nougat-studio-test-XXXXXX").string();
        std::vector<char> bytes(pattern.begin(), pattern.end());
        bytes.push_back('\0');
        char* made = mkdtemp(bytes.data());
        assert(made != nullptr);
        path = made;
    }
    ~TestDirectory() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

void write_file(const fs::path& path, const std::string& contents) {
    std::ofstream out(path, std::ios::binary);
    assert(out.good());
    out << contents;
    out.flush();
    assert(out.good());
}

Job find_job(const Engine& engine, std::uint64_t id) {
    for (const auto& job : engine.snapshot()) if (job.id == id) return job;
    assert(false && "Job missing from snapshot");
    return {};
}

int main() {
    TestDirectory scratch;
    const auto input = scratch.path / "input.wav";
    const auto output = scratch.path / "output.mp3";
    const auto queue = scratch.path / "internal-state" / "queue.txt";
    write_file(input, "not valid audio but sufficient for source validation");

    assert(std::string(preset_name(Preset::AudioMp3)) == "audio-mp3");
    assert(std::string(preset_name(Preset::RemuxMkv)) == "remux-mkv");
    assert(std::string(state_name(JobState::Interrupted)) == "interrupted");

    // Missing input, wrong extension and existing destination fail safely.
    {
        Engine engine(queue, "/no/such/ffmpeg");
        assert(engine.persistence_ok());
        assert(!engine.enqueue(scratch.path / "missing.wav", output, Preset::AudioMp3).ok);
        assert(!engine.enqueue(input, scratch.path / "bad.mp4", Preset::AudioMp3).ok);
        assert(!engine.enqueue(input, input, Preset::AudioWav).ok);
        write_file(output, "do-not-overwrite");
        assert(!engine.enqueue(input, output, Preset::AudioMp3).ok);
        assert(fs::file_size(output) == 16U);
        fs::remove(output);
    }

    // The worker attempts a job, records an encoder failure, preserves the
    // input, and leaves no output or hidden export behind.
    std::uint64_t failed_id = 0;
    {
        Engine engine(queue, "/no/such/ffmpeg");
        auto created = engine.enqueue(input, output, Preset::AudioMp3, 5000);
        assert(created.ok && created.id > 0);
        failed_id = created.id;
        for (int count = 0; count < 200; ++count) {
            const Job job = find_job(engine, failed_id);
            if (job.state == JobState::Failed) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        assert(find_job(engine, failed_id).state == JobState::Failed);
        assert(!fs::exists(output));
        assert(fs::file_size(input) > 0U);
    }

    // Failure history survives restart and can be retried deliberately.
    {
        Engine engine(queue, "/no/such/ffmpeg");
        const Job before = find_job(engine, failed_id);
        assert(before.state == JobState::Failed);
        assert(!before.error.empty());
        assert(engine.retry(failed_id));
        for (int count = 0; count < 200; ++count) {
            if (find_job(engine, failed_id).state == JobState::Failed) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        assert(find_job(engine, failed_id).state == JobState::Failed);
        assert(!fs::exists(output));
    }

    // Batch insertion is all-or-nothing and rejects collisions before writing.
    {
        const auto second = scratch.path / "second.wav";
        write_file(second, "another test input");
        Engine engine(queue, "/no/such/ffmpeg");
        const std::size_t before = engine.snapshot().size();
        const BatchItem one{input, scratch.path / "first.flac", 5000};
        const BatchItem two{second, scratch.path / "second.flac", 5000};
        const auto invalid = engine.enqueue_batch({one, one}, Preset::AudioFlac);
        assert(!invalid.ok);
        assert(engine.snapshot().size() == before);
        const auto invalid_extension = engine.enqueue_batch(
            {one, {second, scratch.path / "third.mp3", 5000}}, Preset::AudioFlac);
        assert(!invalid_extension.ok);
        assert(engine.snapshot().size() == before);
        const auto accepted = engine.enqueue_batch({one, two}, Preset::AudioFlac);
        assert(accepted.ok);
        assert(accepted.ids.size() == 2U);
        assert(accepted.ids[0] < accepted.ids[1]);
    }
    {
        Engine engine(queue, "/no/such/ffmpeg");
        // Any unstarted jobs saved at shutdown must be explicitly retried.
        // Finished failures remain in history, not relaunched automatically.
        for (const auto& job : engine.snapshot()) {
            assert(job.state != JobState::Queued);
            assert(job.state != JobState::Running);
        }
    }

    // A forgotten finished record must never cause old job IDs or encoder
    // log filenames to be reused after restart. Outputs/logs remain intact.
    std::uint64_t high_water = 0;
    const auto history_out = scratch.path / "history.mp3";
    {
        Engine engine(queue, "/no/such/ffmpeg");
        const auto created = engine.enqueue(input, history_out, Preset::AudioMp3);
        assert(created.ok);
        high_water = created.id;
        for (int i = 0; i < 200; ++i) {
            if (find_job(engine, high_water).state == JobState::Failed) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        assert(find_job(engine, high_water).state == JobState::Failed);
        const auto logs = queue.parent_path() / "studio-logs";
        fs::create_directories(logs);
        write_file(logs / ("job-" + std::to_string(high_water) + ".log"),
                   "FFmpeg diagnostic:\nerror\033[31m here\n");
        const std::string diagnostic = engine.recent_log(high_water, 4096);
        assert(diagnostic.find("FFmpeg diagnostic:") != std::string::npos);
        assert(diagnostic.find('\033') == std::string::npos);
        assert(!engine.forget(0));
        assert(engine.forget(high_water));
        assert(!engine.forget(high_water));
        assert(fs::exists(logs / ("job-" + std::to_string(high_water) + ".log")));
        assert(!fs::exists(history_out));
    }
    {
        Engine engine(queue, "/no/such/ffmpeg");
        assert(engine.persistence_ok());
        const auto next = engine.enqueue(input, history_out, Preset::AudioMp3);
        assert(next.ok && next.id > high_water);
        const auto logs = queue.parent_path() / "studio-logs";
        const auto fake = scratch.path / "external-log";
        write_file(fake, "Do not follow this symlink");
        const auto symlink_path = logs / ("job-" + std::to_string(next.id) + ".log");
        std::error_code ec;
        fs::create_symlink(fake, symlink_path, ec);
        if (!ec) {
            assert(engine.recent_log(next.id).find("Do not follow") == std::string::npos);
        }
    }

    // Upgrade a valid V1 queue on load without relaunching old work.
    {
        const fs::path old = scratch.path / "legacy-state" / "queue.txt";
        fs::create_directories(old.parent_path());
        const auto encode = [](const std::string& value) {
            static constexpr char hex[] = "0123456789ABCDEF";
            std::string out;
            for (unsigned char c : value) { out += hex[c >> 4U]; out += hex[c & 15U]; }
            return out;
        };
        write_file(old,"NOUGAT_STUDIO_QUEUE_V1\n7\t4\t3\t0\t"+
                       encode(input.string())+"\t"+
                       encode((scratch.path / "legacy.mp3").string())+"\t\n");
        Engine engine(old, "/no/such/ffmpeg");
        assert(engine.persistence_ok());
        assert(engine.snapshot().size() == 1U);
        assert(engine.snapshot()[0].state == JobState::Failed);
        std::ifstream upgraded(old);
        std::string magic;
        std::getline(upgraded, magic);
        assert(magic.rfind("NOUGAT_STUDIO_QUEUE_V2\t", 0U) == 0U);
        const auto newer = engine.enqueue(input, scratch.path / "next.mp3", Preset::AudioMp3);
        assert(newer.ok && newer.id > 7U);
    }

    // A malformed queue is rejected, not overwritten or silently executed.
    {
        const fs::path invalid = scratch.path / "bad-state" / "queue.txt";
        fs::create_directories(invalid.parent_path());
        write_file(invalid, "NOUGAT_STUDIO_QUEUE_V1\n999\tnot-an-integer\n");
        Engine engine(invalid, "/no/such/ffmpeg");
        assert(!engine.persistence_ok());
        assert(!engine.enqueue(input, output, Preset::AudioMp3).ok);
        std::ifstream read(invalid);
        std::string magic;
        std::getline(read, magic);
        assert(magic == "NOUGAT_STUDIO_QUEUE_V1");
    }
    return 0;
}
