// Queue pause and ordering tests, with no FFmpeg or media content required.
#include "studio/media_job_engine.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <cstdlib>
#include <unistd.h>
namespace fs=std::filesystem;
using namespace nougat::studio;
struct Temp {
    fs::path root;
    Temp() {
        std::string pattern=(fs::temp_directory_path()/"nougat-queue-order-XXXXXX").string();
        std::vector<char> buf(pattern.begin(),pattern.end());
        buf.push_back('\0');
        char* out=mkdtemp(buf.data());
        assert(out);
        root=out;
    }
    ~Temp() { std::error_code ec; fs::remove_all(root,ec); }
};
int main() {
    Temp tmp;
    const fs::path input=tmp.root/"input.wav";
    { std::ofstream out(input); assert(out.good()); out<<"fake"; }
    const auto state=tmp.root/"state/queue";
    std::uint64_t first=0,second=0,third=0;
    {
        Engine engine(state,"/does/not/exist");
        engine.set_queue_paused(true);
        assert(engine.queue_paused());
        const auto a=engine.enqueue(input,tmp.root/"one.mp3",Preset::AudioMp3);
        const auto b=engine.enqueue(input,tmp.root/"two.mp3",Preset::AudioMp3);
        const auto c=engine.enqueue(input,tmp.root/"three.mp3",Preset::AudioMp3);
        assert(a.ok&&b.ok&&c.ok);
        first=a.id; second=b.id; third=c.id;
        assert(!engine.move_queued(first,-1));
        assert(!engine.move_queued(third,1));
        assert(!engine.move_queued(first,0));
        assert(engine.move_queued(third,-1));
        assert(engine.move_queued(third,-1));
        auto snapshot=engine.snapshot();
        assert(snapshot.size()==3);
        assert(snapshot[0].id==third);
        assert(snapshot[1].id==first);
        assert(snapshot[2].id==second);
        for (const auto& job:snapshot) assert(job.state==JobState::Queued);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (const auto& job:engine.snapshot()) assert(job.state==JobState::Queued);
    }
    {
        // Shutdown does not run paused work. Restart marks unfinished work
        // interrupted and preserves the newly chosen ordering.
        Engine restarted(state,"/does/not/exist");
        const auto jobs=restarted.snapshot();
        assert(jobs.size()==3);
        assert(jobs[0].id==third && jobs[1].id==first && jobs[2].id==second);
        for (const auto& job:jobs) assert(job.state==JobState::Interrupted);
        assert(!restarted.move_queued(first,-1));
        restarted.set_queue_paused(true);
        assert(restarted.retry(first));
        assert(restarted.retry(second));
        const auto queued=restarted.snapshot();
        assert(queued[1].state==JobState::Queued);
        assert(queued[2].state==JobState::Queued);
        restarted.set_queue_paused(false);
        assert(!restarted.queue_paused());
    }
    return 0;
}
