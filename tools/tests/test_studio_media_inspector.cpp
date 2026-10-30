// Standard-library-only Studio FFprobe and controller integration checks.
// No real media decoder, downloaded test data or GUI is required.
#include "studio/media_inspector.hpp"
#include "studio/studio_service.hpp"

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

struct TempDirectory {
    fs::path root;
    TempDirectory() {
        std::string pattern = (fs::temp_directory_path() / "nougat-probe-XXXXXX").string();
        std::vector<char> characters(pattern.begin(), pattern.end());
        characters.push_back('\0');
        char* result = mkdtemp(characters.data());
        assert(result);
        root = result;
    }
    ~TempDirectory() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};
void put(const fs::path& file, const std::string& data, bool executable = false) {
    std::ofstream stream(file);
    assert(stream.good());
    stream << data;
    stream.close();
    if (executable) assert(chmod(file.c_str(), 0700) == 0);
}
int main() {
    TempDirectory temp;
    const fs::path fake_probe = temp.root / "ffprobe";
    const fs::path fake_media = temp.root / "media.mkv";
    const fs::path output = temp.root / "export.mp4";
    put(fake_media, "fictional source bytes");
    put(fake_probe, "#!/bin/sh\n"
        "printf 'codec_name=h264|codec_type=video|width=1920|height=1080\\n'\n"
        "printf 'codec_name=aac|codec_type=audio\\n'\n"
        "printf 'duration=12.5\\n'\n", true);

    MediaInspector inspector(fake_probe.string());
    const MediaProbe found = inspector.inspect(fake_media);
    assert(found.ok);
    assert(found.streams.size() == 2);
    assert(found.streams[0].codec == "h264");
    assert(found.streams[0].width == 1920);
    assert(found.streams[0].height == 1080);
    assert(found.streams[1].kind == "audio");
    assert(found.duration_ms == 12500);
    assert(inspector.assess_copy(found, Preset::RemuxMp4).assessment ==
           CopyAssessment::CompatibleCandidate);
    assert(inspector.assess_copy(found, Preset::VideoMp4H264).assessment ==
           CopyAssessment::ReencodeRecommended);
    assert(!inspector.inspect(temp.root / "not-found").ok);

    // Probing fails closed when the executable is missing or misbehaving.
    assert(!MediaInspector((temp.root / "nonexistent").string()).inspect(fake_media).ok);
    const fs::path sleepy = temp.root / "sleepy-probe";
    put(sleepy, "#!/bin/sh\nsleep 2\n", true);
    const auto start = std::chrono::steady_clock::now();
    const auto timeout = MediaInspector(sleepy.string(), std::chrono::milliseconds(150))
                             .inspect(fake_media);
    assert(!timeout.ok);
    assert(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));

    const fs::path unfit = temp.root / "unfit-probe";
    put(unfit, "#!/bin/sh\nprintf 'codec_name=theora|codec_type=video\\n'\n"
               "printf 'codec_name=vorbis|codec_type=audio\\n'\n", true);
    const auto incompatible = MediaInspector(unfit.string()).inspect(fake_media);
    assert(incompatible.ok);
    assert(inspector.assess_copy(incompatible, Preset::RemuxMp4).assessment ==
           CopyAssessment::ReencodeRecommended);

    // The controller uses the inspector's duration for queued jobs without
    // launching any encoder during preflight. Invalid export is rejected.
    const fs::path queue = temp.root / "state/queue.txt";
    StudioService studio(queue, "/missing/ffmpeg", fake_probe.string());
    assert(studio.persistence_ok());
    const auto plan = studio.plan({fake_media, output, Preset::VideoMp4H264});
    assert(plan.valid && plan.media.duration_ms == 12500);
    assert(studio.plan({fake_media, temp.root / "audio.flac", Preset::AudioFlac}).valid);
    assert(!studio.plan({fake_media, output, Preset::AudioFlac}).valid);
    const auto added = studio.submit({fake_media, output, Preset::VideoMp4H264});
    assert(added.ok);
    const auto records = studio.jobs();
    assert(records.size() == 1);
    assert(records.front().duration_ms == 12500);
    assert(!studio.submit_batch({
        {fake_media, temp.root / "a.mp4", Preset::VideoMp4H264},
        {fake_media, temp.root / "b.mp3", Preset::AudioMp3}
    }).ok);
    return 0;
}
