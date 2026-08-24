#!/usr/bin/env python3
"""Compile regression for the exact rejected-v0.0.39 diagnostic API consumed by main.cpp."""
from __future__ import annotations
import pathlib, shutil, subprocess, sys, tempfile

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
compiler = shutil.which('g++')
if compiler is None:
    raise SystemExit('FAIL: g++ is required for v39 diagnostic API compatibility test')

source = r'''
#include "diagnostics/diagnostic_engine.hpp"
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>
int main() {
    reddmedia::DiagnosticInput input;
    input.playback_is_live_tv = true;
    input.live_tv_tuner_count = 1;
    input.live_tv_channel_count = 2U;
    input.live_tv_guide_channels_with_data = 1U;
    input.live_tv_guide_cache_mtime = static_cast<long long>(std::time(nullptr));
    input.live_tv_guide_refresh_busy = true;
    input.live_tv_current_mux_harvest_active = true;
    input.live_tv_full_refresh_queued = true;
    input.live_tv_tuner_name = "Self-test tuner";
    input.live_tv_tuner_backend = "Linux DVB";
    input.live_tv_tuner_status = "Watching";
    input.live_tv_frontend_path = "/dev/null";
    input.live_tv_demux_path = "/dev/null";
    input.live_tv_dvr_path = "/dev/null";
    input.live_tv_net_path = "/dev/null";
    input.live_tv_frontend_accessible = true;
    input.live_tv_demux_accessible = true;
    input.live_tv_dvr_accessible = true;
    input.live_tv_net_accessible = true;
    input.live_tv_signal_lock = true;
    input.live_tv_delivery_systems = "ATSC";
    input.live_tv_current_station = "KPTS-HD";
    input.live_tv_current_frequency = "533000000";
    input.live_tv_current_program_number = 3;
    input.live_tv_current_program_title = "Diagnostic Program";
    input.live_tv_current_program_start = static_cast<long long>(std::time(nullptr));
    input.live_tv_current_program_end = input.live_tv_current_program_start + 1800;
    input.search_status = "Idle";

    const reddmedia::DiagnosticReport report = reddmedia::DiagnosticEngine().evaluate(input);
    (void)report.attention_count;
    for (const auto& subsystem : report.subsystems) {
        const std::string label = subsystem.section;
        (void)label;
    }
    std::vector<const reddmedia::DiagnosticCheck*> ordered;
    for (const auto& check : report.checks) {
        ordered.push_back(&check);
        const std::string section = check.section;
        const std::string name = check.name;
        const std::string title = check.title;
        const std::string detail = check.detail;
        const std::string expected = check.expected;
        const std::string observed = check.observed;
        const std::string evidence = check.evidence;
        const std::string action = check.action;
        const std::string code = check.code;
        const auto severity = check.severity;
        if (name.empty() || title.empty() || name != title) return 4;
        (void)section; (void)detail; (void)expected; (void)observed; (void)evidence; (void)action; (void)code; (void)severity;
    }
    std::string error;
    const auto target = std::filesystem::temp_directory_path() / "nougat-v39-diag-api-compat" / "snapshot.json";
    if (!reddmedia::DiagnosticEngine::write_history_snapshot(report, input, target.string(), error)) return 3;
    return ordered.empty() ? 2 : 0;
}
'''

with tempfile.TemporaryDirectory(prefix='nougat-v39-diag-api-') as td:
    td_path = pathlib.Path(td)
    cpp = td_path / 'compat.cpp'
    exe = td_path / 'compat'
    cpp.write_text(source, encoding='utf-8')
    cmd = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror', f'-I{root / "src"}',
           str(cpp), str(root / 'src/diagnostics/diagnostic_engine.cpp'), '-o', str(exe), '-pthread']
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True, timeout=30)
print('v39-diagnostic-api-compat=pass exact-rejected-main-interface=pass warnings-as-errors=pass')
