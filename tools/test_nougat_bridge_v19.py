#!/usr/bin/env python3
"""Native C++ bridge proof for the integrated Nougat engine."""
from __future__ import annotations
import os, pathlib, subprocess, sys, tempfile, textwrap
ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
with tempfile.TemporaryDirectory(prefix='reddmedia-nougat-bridge-v19.') as raw:
    t = pathlib.Path(raw)
    home = t / 'home'; home.mkdir()
    engine = ROOT / 'components/nougat/nougat_engine.py'
    env = os.environ.copy(); env['HOME'] = str(home)
    subprocess.run([sys.executable, str(engine), 'add-test-docs'], env={**env, 'NOUGAT_HOME': str(home/'.local/share/reddmedia/nougat')}, check=True, stdout=subprocess.PIPE, text=True)
    harness = t / 'bridge_test.cpp'
    harness.write_text(textwrap.dedent(r'''
        #include "nougat/nougat_bridge.hpp"
        #include <iostream>
        int main(int argc, char** argv) {
            if (argc != 2) return 90;
            reddmedia::NougatBridge bridge(argv[1]);
            std::string error;
            const std::string node = bridge.node_id(error);
            if (node.empty() || !error.empty()) return 1;
            auto ranked = bridge.search("Nougat", false, false, 100, 0);
            if (!ranked.error.empty() || ranked.results.empty() || ranked.total < 1) return 2;
            auto raw = bridge.search("Nougat", true, false, 100, 0);
            if (!raw.error.empty() || raw.results.empty() || raw.total < 1) return 3;
            std::cout << "bridge-node=" << node << " ranked=" << ranked.results.size()
                      << " raw=" << raw.results.size() << "\n";
            return 0;
        }
    '''), encoding='utf-8')
    binary = t / 'bridge_test'
    subprocess.run([
        'g++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src'),
        str(harness), str(ROOT/'src/nougat/nougat_bridge.cpp'), '-lX11','-pthread','-o',str(binary)
    ], check=True)
    result = subprocess.run([str(binary), str(engine)], env=env, check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(result.stdout.strip())
print('nougat-native-bridge=pass')
