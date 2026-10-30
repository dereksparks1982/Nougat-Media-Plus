// Named Studio profile persistence and corruption checks. No FFmpeg run,
// app launch, codec, user media or network resource is required.
#include "studio/profile_library.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
namespace fs=std::filesystem;
using namespace nougat::studio;
struct Scratch {
    fs::path dir;
    Scratch() {
        std::string p=(fs::temp_directory_path()/"nougat-profiles-XXXXXX").string();
        std::vector<char> bytes(p.begin(),p.end());bytes.push_back('\0');
        char* result=mkdtemp(bytes.data());assert(result);
        dir=result;
    }
    ~Scratch() { std::error_code ec;fs::remove_all(dir,ec); }
};
void write(const fs::path& name,const std::string& data) {
    std::ofstream out(name); assert(out.good());out<<data;
}
int main() {
    Scratch scratch;
    const fs::path file=scratch.dir/"private/studio-profiles.v1";
    NamedProfile film;
    film.name="Home 1080p";
    film.preset=Preset::VideoMp4H264;
    film.edits.width=1920; film.edits.height=1080; film.edits.crf=20;
    film.edits.audio_kbps=192; film.edits.normalize_audio=true;
    NamedProfile podcast;
    podcast.name="Podcast MP3";
    podcast.preset=Preset::AudioMp3;
    podcast.edits.normalize_audio=true;
    podcast.edits.audio_kbps=160;
    {
        ProfileLibrary lib(file);
        assert(lib.available());
        std::string error;
        assert(lib.upsert(film,error));
        assert(lib.upsert(podcast,error));
        assert(lib.list().size()==2U);
        auto selected=lib.find("Home 1080p");
        assert(selected && selected->edits.width==1920);
        assert(!lib.find("Missing profile"));
        NamedProfile invalid=film;
        invalid.name="injected\tname";
        assert(!lib.upsert(invalid,error));
        invalid=film;
        invalid.edits.rotation=17;
        assert(!lib.upsert(invalid,error));
        assert(lib.list().size()==2U);
    }
    {
        ProfileLibrary lib(file);
        assert(lib.available());
        assert(lib.list().size()==2U);
        auto selected=lib.find("Podcast MP3");
        assert(selected && selected->edits.normalize_audio);
        std::string error;
        assert(lib.erase("Home 1080p",error));
        assert(!lib.find("Home 1080p"));
        assert(!lib.erase("Home 1080p",error));
    }
    {
        ProfileLibrary lib(file);
        assert(lib.available());
        assert(lib.list().size()==1U);
        const auto source=scratch.dir/"not-a-profile";
        write(source,"not a profile");
        const auto link=scratch.dir/"bad-link";
        std::error_code ec;
        fs::create_symlink(source,link,ec);
        if (!ec) {
            ProfileLibrary linked(link);
            assert(!linked.available());
        }
    }
    {
        const fs::path corrupt=scratch.dir/"corrupt";
        write(corrupt,"INVALID\n");
        ProfileLibrary lib(corrupt);
        assert(!lib.available());
        std::string error;
        assert(!lib.upsert(podcast,error));
        std::ifstream source(corrupt);
        std::string header;
        std::getline(source,header);
        assert(header=="INVALID");
    }
    return 0;
}
