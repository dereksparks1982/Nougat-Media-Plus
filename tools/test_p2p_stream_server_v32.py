#!/usr/bin/env python3
from __future__ import annotations
import pathlib, shutil, subprocess, sys, tempfile, textwrap
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
for rel in ('src/p2p_stream_server.cpp','src/p2p_stream_server.hpp'):
    need((root/rel).is_file(),rel+' missing')
need(shutil.which('g++') is not None,'g++ required')
with tempfile.TemporaryDirectory(prefix='nms-v32-p2p-http.') as raw:
    t=pathlib.Path(raw)
    shutil.copy2(root/'src/p2p_stream_server.cpp',t/'p2p_stream_server.cpp')
    shutil.copy2(root/'src/p2p_stream_server.hpp',t/'p2p_stream_server.hpp')
    (t/'p2p_engine.hpp').write_text(r'''#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
struct P2PStatus { bool active=true; std::string error; };
class P2PEngine {
public:
    explicit P2PEngine(std::size_t n=20000) : data(n) { for (std::size_t i=0;i<n;++i) data[i]=static_cast<char>(i%251); }
    int selected_file() const { return 0; }
    std::uint64_t selected_file_size() const { return data.size(); }
    std::string selected_file_name() const { return "fixture.mkv"; }
    void clear_stream_priority() { ++clears; }
    void prioritize_playback_window(std::uint64_t offset) { std::lock_guard<std::mutex> g(m); priority_offsets.push_back(offset); }
    bool wait_for_range(std::uint64_t offset, std::uint64_t, int timeout_ms) {
        if (stall_zero.load() && offset==0) { std::this_thread::sleep_for(std::chrono::milliseconds(std::min(timeout_ms,50))); return false; }
        return true;
    }
    P2PStatus status() const { return {}; }
    bool read_selected_range(std::uint64_t offset,char* dest,std::size_t length,std::size_t& bytes_read,std::string& error) const {
        (void)error;
        if (offset>=data.size()) { bytes_read=0; error="offset"; return false; }
        bytes_read=std::min<std::size_t>(length,data.size()-static_cast<std::size_t>(offset));
        std::memcpy(dest,data.data()+offset,bytes_read); return true;
    }
    std::vector<char> data;
    std::atomic<int> clears{0};
    std::atomic<bool> stall_zero{false};
    mutable std::mutex m;
    std::vector<std::uint64_t> priority_offsets;
};
''')
    (t/'test.cpp').write_text(r'''#include "p2p_stream_server.hpp"
#include "p2p_engine.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>
static std::string request(std::uint16_t port,const std::string& raw) {
    int fd=socket(AF_INET,SOCK_STREAM,0); if(fd<0) return {};
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port); inet_pton(AF_INET,"127.0.0.1",&a.sin_addr);
    if(connect(fd,reinterpret_cast<sockaddr*>(&a),sizeof(a))!=0){close(fd);return{};}
    send(fd,raw.data(),raw.size(),0); shutdown(fd,SHUT_WR);
    std::string out; char buf[4096]; for(;;){ssize_t n=recv(fd,buf,sizeof(buf),0); if(n<=0)break; out.append(buf,static_cast<std::size_t>(n));}
    close(fd); return out;
}
static std::string body(const std::string& r){auto p=r.find("\r\n\r\n"); return p==std::string::npos?std::string():r.substr(p+4);}
static bool has(const std::string&s,const std::string&x){return s.find(x)!=std::string::npos;}
int main(){
    P2PEngine engine; P2PStreamServer server(engine); std::string error;
    if(!server.start(error)){std::cerr<<error<<"\n";return 1;}
    const auto port=server.port(); if(!port)return 2;
    auto r=request(port,"GET /media HTTP/1.1\r\nHost: 127.0.0.1\r\nRange: bytes=100-199\r\n\r\n");
    if(!has(r,"206 Partial Content")||!has(r,"Content-Range: bytes 100-199/20000")||body(r).size()!=100)return 3;
    for(std::size_t i=0;i<100;++i)if(static_cast<unsigned char>(body(r)[i])!=static_cast<unsigned char>((100+i)%251))return 4;
    auto suffix=request(port,"GET /media HTTP/1.1\r\nHost: 127.0.0.1\r\nRange: bytes=-1000\r\n\r\n");
    if(!has(suffix,"206 Partial Content")||!has(suffix,"Content-Range: bytes 19000-19999/20000")||body(suffix).size()!=1000)return 5;
    auto head=request(port,"HEAD /media HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");
    if(!has(head,"200 OK")||!has(head,"Content-Length: 20000")||!body(head).empty())return 6;
    auto bad=request(port,"GET /media HTTP/1.1\r\nHost: 127.0.0.1\r\nRange: bytes=99999-100000\r\n\r\n");
    if(!has(bad,"416 Range Not Satisfiable"))return 7;
    {std::lock_guard<std::mutex>g(engine.m); if(engine.priority_offsets.empty()||engine.priority_offsets.front()!=100)return 8;}
    if(engine.clears.load()<2)return 9;
    engine.stall_zero=true; std::string old;
    std::thread first([&]{old=request(port,"GET /media HTTP/1.1\r\nHost: 127.0.0.1\r\nRange: bytes=0-5000\r\n\r\n");});
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    auto newer=request(port,"GET /media HTTP/1.1\r\nHost: 127.0.0.1\r\nRange: bytes=7000-7099\r\n\r\n");
    first.join(); engine.stall_zero=false;
    if(!has(newer,"206 Partial Content")||body(newer).size()!=100)return 10;
    if(body(old).size()>=5001)return 11;
    bool saw7000=false; {std::lock_guard<std::mutex>g(engine.m); for(auto x:engine.priority_offsets) if(x==7000)saw7000=true;}
    if(!saw7000)return 12;
    server.stop();
    std::cout<<"p2p-stream-v32=pass range-206=pass suffix=pass head=pass invalid-416=pass playback-window=pass seek-supersession=pass loopback=pass\n";
    return 0;
}
''')
    cmd=['g++','-std=c++17','-Wall','-Wextra','-Werror','-pthread',str(t/'p2p_stream_server.cpp'),str(t/'test.cpp'),'-o',str(t/'test')]
    subprocess.check_call(cmd)
    subprocess.check_call([str(t/'test')])
