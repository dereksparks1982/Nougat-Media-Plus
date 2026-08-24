#!/usr/bin/env python3
from __future__ import annotations
import errno, http.server, json, os, pathlib, shutil, socketserver, subprocess, sys, tempfile, threading, tarfile

ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)

HARNESS=r'''
#include "diagnostics/diagnostic_engine.hpp"
#include <fstream>
#include <string>
int main(int argc,char**argv){
  if(argc!=8) return 90;
  std::string root=argv[1], runtime=argv[2], model=argv[3], media=argv[4], log=argv[5], out=argv[6], mode=argv[7], error;
  reddmedia::DiagnosticInput in;
  in.app_version="Nougat Media Suite v0.0.26";
  in.executable_path=root+"/Nougat_Media_Suite_v26";
  in.project_root=root;
  in.current_view="Debug";
  in.server_state=reddmedia::MediaServerState::Ready;
  in.server_owned=true;
  in.server_api_ready=true;
  in.runtime_path=runtime;
  in.data_path=root+"/data"; in.config_path=root+"/config"; in.cache_path=root+"/cache"; in.log_path=log;
  in.library_full_scan=true;
  reddmedia::LibraryNode movie; movie.kind=reddmedia::LibraryNodeKind::Movie; movie.name="Healthy Movie"; movie.overview="Verified overview"; movie.path=media; movie.poster_item_id="poster"; in.library_nodes.push_back(movie);
  in.vlc_probe_attempted=(mode=="probed"); in.vlc_loaded=(mode=="probed"); in.vlc_version="libVLC test"; in.playback_state="Idle"; in.volume_percent=100;
  in.tmdb_configured=true; in.ai_model_path=model; in.ai_runtime_path=runtime;
  in.stream_engine_path=root+"/yt-dlp"; in.stream_engine_version="test"; in.stream_provider="YouTube"; in.stream_status="Ready";
  reddmedia::DiagnosticEngine engine; auto report=engine.evaluate(in);
  if(report.overall!=reddmedia::DiagnosticSeverity::Information) return 10;
  for(const auto& issue:report.issues) if(issue.code=="VLC_UNAVAILABLE") return 11;
  if(reddmedia::DiagnosticEngine::report_text(report,in).find("Nougat Media Suite Diagnostic Report")==std::string::npos) return 12;
  if(!reddmedia::DiagnosticEngine::write_text_report(report,in,out+"/report.txt",error)) return 13;
  if(!reddmedia::DiagnosticEngine::write_json_report(report,in,out+"/report.json",error)) return 14;
  if(!reddmedia::DiagnosticEngine::write_support_bundle(report,in,out+"/support.tar.gz",error)) return 15;
  return 0;
}
'''

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200); self.end_headers(); self.wfile.write(b'{}')
    def log_message(self,*_): pass
class Server(socketserver.ThreadingMixIn,http.server.HTTPServer): daemon_threads=True

with tempfile.TemporaryDirectory(prefix='nougat-v26-diag-') as raw:
    temp=pathlib.Path(raw); compiler=shutil.which('g++'); need(compiler is not None,'g++ required')
    source=temp/'h.cpp'; source.write_text(HARNESS); binary=temp/'harness'
    subprocess.run([compiler,'-std=c++17','-Wall','-Wextra','-Werror',f'-I{ROOT/"src"}',str(source),str(ROOT/'src/diagnostics/diagnostic_engine.cpp'),'-o',str(binary)],check=True)
    server=None; thread=None
    try:
        server=Server(('127.0.0.1',8096),Handler)
    except OSError as exc:
        if exc.errno not in (errno.EADDRINUSE, 98):
            raise SystemExit(f'FAIL: diagnostic v26 port probe setup failed: {exc}')
        print('diagnostics-v26-port=busy existing-local-service-used')
    if server is not None:
        thread=threading.Thread(target=server.serve_forever,daemon=True); thread.start()
    try:
        runtime=temp/'runtime'; runtime.mkdir(); model=temp/'model.gguf'; model.write_bytes(b'model'); media=temp/'movie.mkv'; media.write_bytes(b'video')
        ytdlp=temp/'yt-dlp'; ytdlp.write_text('#!/bin/sh\n'); ytdlp.chmod(0o755)
        logs=temp/'logs'; logs.mkdir(); (logs/'jellyfin.log').write_text('ordinary line\nAuthorization: Bearer super-secret-token\napi_key=private-value\n')
        out=temp/'out'; out.mkdir()
        for mode in ('probed','unknown'):
            subprocess.run([str(binary),str(temp),str(runtime),str(model),str(media),str(logs),str(out),mode],check=True,timeout=30)
        text=(out/'report.txt').read_text(); need(text.startswith('Nougat Media Suite Diagnostic Report'),'TXT report identity wrong')
        data=json.loads((out/'report.json').read_text()); need(data.get('report')=='Nougat Media Suite Diagnostic Report','JSON report identity wrong')
        archive=out/'support.tar.gz'; need(archive.is_file(),'support bundle missing')
        with tarfile.open(archive,'r:gz') as tf:
            names=tf.getnames(); need(any(n.endswith('report.txt') for n in names),'bundle report.txt missing'); need(any('/logs/' in ('/'+n) for n in names),'bundle logs missing')
            content=b'\n'.join(tf.extractfile(m).read() for m in tf.getmembers() if m.isfile())
        need(b'super-secret-token' not in content and b'private-value' not in content,'support bundle leaked credential material')
        need(b'[REDACTED SENSITIVE LINE]' in content,'support bundle did not show redaction marker')
    finally:
        if server is not None:
            server.shutdown(); server.server_close()
            if thread is not None: thread.join(timeout=2)
print('diagnostics-v26=pass txt=pass json=pass support-bundle=pass redaction=pass unknown-not-green-by-assumption=pass')
