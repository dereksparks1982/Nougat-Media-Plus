// Folder batch preview contracts: no encoders, no network and no user media.
// Creates disposable source/output directories inside a private temporary root.
#include "studio/folder_batch_planner.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace nougat::studio;

struct Scratch {
    fs::path root;
    Scratch() {
        std::string pattern=(fs::temp_directory_path()/"nougat-folder-preview-XXXXXX").string();
        std::vector<char> buf(pattern.begin(),pattern.end());
        buf.push_back('\0');
        char* created=mkdtemp(buf.data());
        assert(created);
        root=created;
    }
    ~Scratch() {
        std::error_code ec;
        fs::remove_all(root,ec);
    }
};
void write(const fs::path& p) {
    std::ofstream out(p,std::ios::binary);
    assert(out.good());
    out<<"fixture";
}
int main() {
    Scratch root;
    const auto in=root.root/"in";
    const auto out=root.root/"out";
    fs::create_directory(in);
    fs::create_directory(out);
    write(in/"movie.mkv");
    write(in/"song.wav");
    write(in/"other.ogg");
    write(in/"notes.txt");
    write(in/".hidden.mp4");
    fs::create_directory(in/"season");
    write(in/"season"/"episode.mp4");
    write(out/"other-nougat.mp3"); // collision must never overwrite
    std::error_code ec;
    fs::create_symlink(in/"movie.mkv",in/"alias.mp4",ec);

    FolderBatchOptions opts;
    opts.source_directory=in;
    opts.output_directory=out;
    opts.preset=Preset::AudioMp3;
    opts.max_files=10;
    auto plan=plan_folder_batch(opts);
    assert(plan.ok);
    assert(plan.exports.size()==2);
    assert(!plan.skipped.empty());
    assert(plan.scanned_entries>=5);
    assert(plan.source_bytes>0);
    for(const auto& job:plan.exports) {
        assert(job.output.extension()==".mp3");
        assert(job.output.parent_path()==out);
        assert(!fs::exists(job.output));
    }

    opts.recursive=true;
    opts.max_depth=3;
    plan=plan_folder_batch(opts);
    assert(plan.ok && plan.exports.size()==3);
    assert(plan.exports[0].input.filename()=="movie.mkv");
    assert(plan.exports.back().input.filename()=="song.wav");

    opts.max_files=1;
    plan=plan_folder_batch(opts);
    assert(!plan.ok);
    assert(plan.error.find("limit")!=std::string::npos);
    opts.max_files=10;

    opts.output_directory=in;
    assert(!plan_folder_batch(opts).ok);
    opts.output_directory=in/"season";
    assert(!plan_folder_batch(opts).ok);
    opts.output_directory=out;

    // A queued job reserves a destination even if no final file exists yet.
    Job queued;
    queued.id=55;
    queued.output=out/"movie-nougat.mp3";
    queued.state=JobState::Queued;
    plan=plan_folder_batch(opts,{queued});
    assert(plan.ok);
    assert(plan.exports.size()==2);
    bool found_collision=false;
    for(const auto& skipped:plan.skipped)
        if(skipped.reason.find("reserves")!=std::string::npos) found_collision=true;
    assert(found_collision);

    // Same base filename in different source formats cannot silently overwrite.
    write(in/"movie.mp4");
    plan=plan_folder_batch(opts);
    assert(plan.ok);
    assert(!plan.skipped.empty());

    // Refuse a source folder that is itself a symlink.
    const auto source_link=root.root/"source-link";
    fs::create_directory_symlink(in,source_link,ec);
    if (!ec) {
        opts.source_directory=source_link;
        assert(!plan_folder_batch(opts).ok);
    }
    return 0;
}
