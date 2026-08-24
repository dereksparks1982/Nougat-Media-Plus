#!/usr/bin/env python3
# v0.0.39 update of the retained v0.0.26 diagnostic regression.
# The v26 export/redaction contract remains; obsolete severity expectations do not.
from __future__ import annotations
import errno,http.server,json,pathlib,shutil,socketserver,subprocess,sys,tempfile,threading,tarfile
ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
HARNESS=r'''
#include "diagnostics/diagnostic_engine.hpp"
#include <filesystem>
#include <string>
int main(int argc,char**argv){
 if(argc!=8) return 90;
 std::string root=argv[1],runtime=argv[2],model=argv[3],media=argv[4],log=argv[5],out=argv[6],mode=argv[7],error;
 reddmedia::DiagnosticInput in;
 in.app_version="Nougat Media Suite v0.0.26 retained regression";
 in.executable_path=argv[0]; in.project_root=root; in.current_view="System";
 in.server_state=reddmedia::MediaServerState::Ready; in.server_owned=true; in.server_api_ready=true;
 in.runtime_path=runtime; in.data_path=root+"/data"; in.config_path=root+"/config"; in.cache_path=root+"/cache"; in.log_path=log;
 in.library_full_scan=true;
 reddmedia::LibraryNode movie; movie.kind=reddmedia::LibraryNodeKind::Movie; movie.name="Healthy Movie"; movie.path=media; // metadata intentionally incomplete
 in.library_nodes.push_back(movie);
 in.vlc_probe_attempted=true; in.vlc_loaded=true; in.vlc_version="libVLC test"; in.playback_state="Idle"; in.volume_percent=100;
 in.search_test_requested=false; in.search_node_running=false;
 in.tmdb_configured=true; in.ai_model_path=model; in.ai_runtime_path=runtime;
 in.stream_engine_path=root+"/yt-dlp"; in.stream_engine_version="test"; in.stream_provider="YouTube"; in.stream_status="Ready";
 in.diagnostic_history_path=out+"/history.jsonl";
 reddmedia::DiagnosticEngine engine; auto report=engine.evaluate(in);
 bool idle=false, metadata=false;
 for(const auto& issue:report.issues){
   if(issue.code=="SEARCH_IDLE" && issue.severity==reddmedia::DiagnosticSeverity::NotTested) idle=true;
   if(issue.code=="LIBRARY_METADATA_COMPLETENESS" && issue.severity==reddmedia::DiagnosticSeverity::Information) metadata=true;
 }
 if(report.problem_count!=0) return 10;
 if(!idle) return 11;
 if(!metadata) return 12;
 const std::string text=reddmedia::DiagnosticEngine::report_text(report,in);
 if(text.find("Expected:")==std::string::npos || text.find("Observed:")==std::string::npos || text.find("Evidence:")==std::string::npos) return 13;
 if(!reddmedia::DiagnosticEngine::write_text_report(report,in,out+"/report.txt",error)) return 14;
 if(!reddmedia::DiagnosticEngine::write_json_report(report,in,out+"/report.json",error)) return 15;
 if(!reddmedia::DiagnosticEngine::write_support_bundle(report,in,out+"/support.tar.gz",error)) return 16;
 return mode=="new-model" ? 0 : 91;
}
'''
class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self): self.send_response(200); self.end_headers(); self.wfile.write(b'{}')
    def log_message(self,*_): pass
class Server(socketserver.ThreadingMixIn,http.server.HTTPServer): daemon_threads=True
with tempfile.TemporaryDirectory(prefix='nougat-v26-diag-new-model-') as raw:
    temp=pathlib.Path(raw); compiler=shutil.which('g++'); need(compiler is not None,'g++ required')
    for name in ('data','config','cache'): (temp/name).mkdir()
    source=temp/'h.cpp'; source.write_text(HARNESS); binary=temp/'harness'
    subprocess.run([compiler,'-std=c++17','-Wall','-Wextra','-Werror',f'-I{ROOT/"src"}',str(source),str(ROOT/'src/diagnostics/diagnostic_engine.cpp'),'-o',str(binary)],check=True)
    server=None; thread=None
    try:
        server=Server(('127.0.0.1',8096),Handler)
    except OSError as exc:
        if exc.errno not in (errno.EADDRINUSE,98): raise
    if server:
        thread=threading.Thread(target=server.serve_forever,daemon=True); thread.start()
    try:
        runtime=temp/'runtime'; runtime.mkdir(); model=temp/'model.gguf'; model.write_bytes(b'model'); media=temp/'movie.mkv'; media.write_bytes(b'video')
        ytdlp=temp/'yt-dlp'; ytdlp.write_text('#!/bin/sh\n'); ytdlp.chmod(0o755)
        logs=temp/'logs'; logs.mkdir(); (logs/'jellyfin.log').write_text('ordinary line\nAuthorization: Bearer super-secret-token\napi_key=private-value\n')
        out=temp/'out'; out.mkdir()
        subprocess.run([str(binary),str(temp),str(runtime),str(model),str(media),str(logs),str(out),'new-model'],check=True,timeout=30)
        data=json.loads((out/'report.json').read_text()); need(data.get('report')=='Nougat Media Suite Diagnostic Report','JSON identity wrong')
        need(data.get('counts',{}).get('problems')==0,'healthy retained fixture reported Problems')
        archive=out/'support.tar.gz'; need(archive.is_file(),'support bundle missing')
        with tarfile.open(archive,'r:gz') as tf:
            content=b'\n'.join(tf.extractfile(m).read() for m in tf.getmembers() if m.isfile())
        need(b'super-secret-token' not in content and b'private-value' not in content,'support bundle leaked credentials')
    finally:
        if server: server.shutdown(); server.server_close()
        if thread: thread.join(timeout=2)
print('diagnostics-v26=pass new-severity-model=pass search-idle-not-tested=pass metadata-information=pass exports=pass redaction=pass')
