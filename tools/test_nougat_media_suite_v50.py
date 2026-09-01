#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import subprocess, sys

ROOT=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()

def need(c,m):
    if not c: raise RuntimeError(m)

def text(rel):
    p=ROOT/rel; need(p.is_file(),f"missing required v50 file: {rel}"); return p.read_text(encoding='utf-8')

def blob(rel):
    r=subprocess.run(['git','hash-object',str(ROOT/rel)],cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    need(r.returncode==0,f"git hash-object failed for {rel}: {r.stdout.strip()}")
    return r.stdout.strip()

def main():
  try:
    cmake=text('CMakeLists.txt'); maincpp=text('src/main.cpp'); provider=text('src/live_tv/hdhomerun_provider.cpp')
    splitter=text('tools/nougat_file_splitter.py'); diagnostics=text('src/diagnostics/diagnostic_engine.cpp'); deps=text('DEPENDENCIES.md'); roadmap=text('ROADMAP.md'); change=text('CHANGELOG.md')
    need('VERSION 0.0.50' in cmake,'CMake v50 identity missing')
    need('add_executable(Nougat_Media_Suite_v50' in cmake,'v50 native target missing')
    need('src/live_tv/hdhomerun_provider.cpp' in cmake,'HDHomeRun provider not compiled')
    need('#include "live_tv/hdhomerun_provider.hpp"' in maincpp,'HDHomeRun provider not wired to application')
    need('Nougat Media Suite v0.0.50' in maincpp,'visible/diagnostic v50 identity missing')
    need('section_text(target, 28, 70, "STUDIO"' in maincpp,'Studio active heading not corrected')
    need('"GOLD STUDIO"' not in maincpp,'active Gold Studio wording remains')
    for marker in ('studioSplitFileBtn','studioSplitFolderBtn','studioReassembleBtn','studioVerifyBtn','launch_studio_splitter_action'):
        need(marker in maincpp,f'Studio splitter integration missing: {marker}')
    for marker in ('hdhomerun_config','/tuner','lineup.json',':5004/auto/v','HDHomeRun'):
        need(marker in provider,f'HDHomeRun provider contract missing: {marker}')
    for marker in ('format = "nougat-parts-v1"'.lower(), 'sha256', 'split-folder', 'reassemble', 'studio-gui'):
        need(marker in splitter.lower(),f'File Splitter contract missing: {marker}')
    need('LIVE_TV_TUNER_ACCESS' in diagnostics and 'unreachable_network_tuners' in diagnostics, 'HDHomeRun diagnostic access evidence missing')
    need('hdhomerun-config' in deps,'v50 HDHomeRun dependency note missing')
    need('motion capture' in roadmap.lower() and 'green-screen' in roadmap.lower() and 'animation' in roadmap.lower(),
         'Studio production roadmap missing approved directions')
    for marker in ('aerial production', 'dualsense', 'drone-camera', 'telemetry', 'px4/ardupilot', 'dji', 'custom drone software/firmware'):
        need(marker in roadmap.lower(), f'Studio Aerial Production roadmap missing: {marker}')
    need('no aerial production implementation' in roadmap.lower(),
         'v50 Aerial Production code boundary missing')
    need('v0.0.51' in roadmap and 'remaining emulator support' in roadmap.lower() and 'web viewer' in roadmap.lower(),
         'v51 emulator-completion + Web Viewer foundation boundary missing')
    need('v0.0.50 - Studio File Splitter and Unified Tuners' in change,'v50 changelog entry missing')
    for rel in ('NougatMediaSuite.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
        need('Nougat_Media_Suite_v50' in text(rel),f'{rel}: v50 executable target missing')
    # Owner-protected governance/licensing files are explicitly outside v50 scope.
    need(blob('COMPANY_BIBLE.md')=='43b55b0cde56b2d53c3a29cfc1f8950d7780c4fd','Company Bible changed unexpectedly')
    need(blob('THIRD_PARTY_NOTICES.md')=='135545738e2615c78fb0444de5e8d07cd1c2331c','THIRD_PARTY_NOTICES changed unexpectedly')
    print('Nougat v0.0.50 static contract PASS: Studio splitter + unified tuners; v51 emulator + Web Viewer boundary preserved')
    return 0
  except Exception as exc:
    print('FAIL:',exc)
    return 1
if __name__=='__main__': raise SystemExit(main())
