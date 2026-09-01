#!/usr/bin/env python3
from pathlib import Path
import os, shutil, subprocess, sys, tempfile

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]).resolve()
TESTS = ROOT / "tools/v50_tests"
FAKE = TESTS / "fake_hdhr"

def need(c, m):
    if not c: raise RuntimeError(m)

def main():
    try:
        gpp=shutil.which("g++")
        need(gpp, "g++ is required")
        for p in (FAKE/"hdhomerun_config", FAKE/"curl"):
            need(p.is_file(), f"missing fake tool: {p}")
            p.chmod(p.stat().st_mode | 0o111)
        with tempfile.TemporaryDirectory(prefix="nougat-v50-hdhr-") as td:
            binary=Path(td)/"test_hdhomerun_provider_v50"
            cmd=[gpp,"-std=c++17","-Wall","-Wextra","-Werror","-I",str(TESTS/"include"),"-I",str(ROOT/"src"),
                 str(TESTS/"test_hdhomerun_provider_v50.cpp"),str(ROOT/"src/live_tv/hdhomerun_provider.cpp"),"-o",str(binary)]
            r=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            print(r.stdout,end="")
            need(r.returncode==0,"HDHomeRun provider test did not compile cleanly")
            r=subprocess.run([binary,str(FAKE)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            print(r.stdout,end="")
            need(r.returncode==0,"HDHomeRun provider test failed")
        return 0
    except Exception as exc:
        print("FAIL:",exc)
        return 1
if __name__=="__main__": raise SystemExit(main())
