// Pure command planning and serialization tests; no codecs, FFmpeg runtime
// or user media folders are required. Uses private disposable SRT fixture.
#include "studio/advanced_export.hpp"
#include "studio/media_job_engine.hpp"

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
namespace fs = std::filesystem;
using namespace nougat::studio;
bool has(const std::vector<std::string>& a, const std::string& value) {
    return std::find(a.begin(),a.end(),value) != a.end();
}
bool has_text(const std::vector<std::string>& a, const std::string& value) {
    return std::any_of(a.begin(),a.end(),[&](const std::string& s) {
        return s.find(value) != std::string::npos;
    });
}
int main() {
    const fs::path in="/tmp/fixture-video.mkv", out="/tmp/fixture-output.mp4";
    ExportEdits default_edits;
    const auto regular=build_export_command("/usr/bin/ffmpeg",Preset::VideoMp4H264,
                                            default_edits,in,out);
    assert(regular.ok);
    assert(has(regular.argv,"libx264"));
    assert(has(regular.argv,"yuv420p"));
    assert(has(regular.argv,"pipe:1"));
    assert(!has(regular.argv,"-vf"));
    ExportEdits edit;
    edit.start_ms=2500;
    edit.duration_ms=6000;
    edit.width=1920; edit.height=1080; edit.fps=30;
    edit.crop_width=1280; edit.crop_height=720; edit.crop_x=4; edit.crop_y=8;
    edit.rotation=90; edit.crf=19; edit.video_kbps=6000;
    edit.audio_kbps=192; edit.gain_tenths_db=-25;
    edit.normalize_audio=true; edit.audio_stream=1;
    const auto planned=build_export_command("/usr/bin/ffmpeg",
        Preset::VideoMp4H264,edit,in,out);
    assert(planned.ok);
    assert(has(planned.argv,"2.500"));
    assert(has(planned.argv,"6.000"));
    assert(has_text(planned.argv,"scale=1920:1080"));
    assert(has_text(planned.argv,"crop=1280:720:4:8"));
    assert(has_text(planned.argv,"transpose=clock"));
    assert(has_text(planned.argv,"loudnorm=I=-16"));
    assert(has_text(planned.argv,"volume=-2.5dB"));
    assert(has(planned.argv,"0:a:1?"));
    assert(has(planned.argv,"6000k"));
    const std::string saved=serialize_export_edits(edit);
    ExportEdits loaded;
    assert(deserialize_export_edits(saved,loaded));
    assert(loaded.width==1920 && loaded.duration_ms==6000);
    assert(loaded.audio_stream==1 && loaded.gain_tenths_db==-25);
    assert(serialize_export_edits(loaded)==saved);
    assert(!deserialize_export_edits(saved+"|extra",loaded));
    assert(!deserialize_export_edits("999999999999999999999",loaded));

    // Remux can trim streams and preserve metadata but cannot use filters.
    ExportEdits copy;
    copy.start_ms=1000; copy.duration_ms=2000;
    const auto remux=build_export_command("/usr/bin/ffmpeg",Preset::RemuxMkv,
                                          copy,in,"/tmp/result.mkv");
    assert(remux.ok && has(remux.argv,"copy"));
    copy.rotation=90;
    std::string message;
    assert(!validate_export_edits(copy,Preset::RemuxMkv,message));
    copy.rotation=0;
    copy.drop_subtitles=true;
    assert(build_export_command("/usr/bin/ffmpeg",Preset::RemuxMp4,
                                copy,in,out).ok);
    assert(has_text(build_export_command("/usr/bin/ffmpeg",Preset::RemuxMp4,
                                copy,in,out).argv,"-0:s?"));

    // Audio-only exports can pick tracks and normalize; no video filters.
    ExportEdits audio;
    audio.audio_stream=2; audio.normalize_audio=true;
    assert(has(build_export_command("/usr/bin/ffmpeg",Preset::AudioFlac,
               audio,in,"/tmp/audio.flac").argv,"0:a:2"));
    audio.crop_width=120; audio.crop_height=120;
    assert(!validate_export_edits(audio,Preset::AudioFlac,message));

    ExportEdits extract;
    extract.subtitle_stream=1;
    const auto srt_extract=build_export_command(
        "/usr/bin/ffmpeg",Preset::SubtitleSrt,extract,in,"/tmp/captions.srt");
    assert(srt_extract.ok);
    assert(has(srt_extract.argv,"0:s:1"));
    assert(has(srt_extract.argv,"srt"));
    extract.normalize_audio=true;
    assert(!validate_export_edits(extract,Preset::SubtitleSrt,message));

    const fs::path temp=fs::temp_directory_path()/"nougat-advanced-SRT-XXXXXX";
    std::string pattern=temp.string();
    std::vector<char> name(pattern.begin(),pattern.end());name.push_back('\0');
    const int fd=mkstemp(name.data());assert(fd>=0);close(fd);
    const fs::path subtitle=name.data();
    // Rename to a normal SRT path without a literal "XXXXXX" suffix.
    const fs::path srt=subtitle.string()+".srt";
    fs::rename(subtitle,srt);
    { std::ofstream f(srt); assert(f.good()); f<<"1\n00:00:00,000 --> 00:00:01,000\nHello\n"; }
    ExportEdits subs;
    subs.external_srt=srt;
    auto mp4=build_export_command("/usr/bin/ffmpeg",Preset::VideoMp4H264,
                                  subs,in,out);
    assert(mp4.ok && has(mp4.argv,"mov_text") && has(mp4.argv,srt.string()));
    subs.burn_subtitles=true;
    mp4=build_export_command("/usr/bin/ffmpeg",Preset::VideoMp4H264,subs,in,out);
    assert(mp4.ok && has_text(mp4.argv,"subtitles=filename="));
    subs.drop_subtitles=true;
    assert(!validate_export_edits(subs,Preset::VideoMp4H264,message));
    subs.drop_subtitles=false;
    assert(!validate_export_edits(subs,Preset::AudioWav,message));
    fs::remove(srt);
    assert(!validate_export_edits(subs,Preset::VideoMp4H264,message));
    return 0;
}
