#!/usr/bin/env python3
from __future__ import annotations
import hashlib
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import textwrap

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]


def need(ok: bool, msg: str) -> None:
    if not ok:
        raise SystemExit('FAIL: ' + msg)

worker = root/'components/security/nougat_security_worker.py'
runtime_installer = root/'tools/install_nougat_security_runtime_v32.py'
rules = root/'components/security/rules/nougat_core.yar'
main = (root/'src/main.cpp').read_text(encoding='utf-8')
p2ph = (root/'src/p2p_engine.hpp').read_text(encoding='utf-8')
worker_text = worker.read_text(encoding='utf-8')
runtime_text = runtime_installer.read_text(encoding='utf-8')

need(worker.is_file() and os.access(worker, os.X_OK), 'one-shot security worker missing/not executable')
need(runtime_installer.is_file(), 'security runtime installer missing')
need(rules.is_file(), 'built-in YARA-X rules missing')
for token in ['The worker intentionally has no daemon mode', 'WARN ME FIRST', 'never moves, deletes, quarantines', '--file', '--folder', '--history', '--offline']:
    need(token.lower() in worker_text.lower(), 'worker policy/CLI missing: '+token)
for forbidden in ['watchdog', 'inotify', 'clamd', 'os.remove(', 'shutil.move(', 'unlink(', 'rename(']:
    need(forbidden not in worker_text, 'worker contains resident/destructive behavior: '+forbidden)
need('yara_x.Scanner' in worker_text and 'scan_file' in worker_text, 'YARA-X file streaming path missing')
need('Magika().identify_path' in worker_text, 'Magika content identification missing')
need('clamscan' in worker_text and 'clamd' not in worker_text, 'optional one-shot ClamAV contract missing')
need('MalwareBazaar' in worker_text and 'ThreatFox' in worker_text, 'free community telemetry integration missing')
need('abusech.key' in worker_text and 'scan_history.jsonl' in worker_text, 'private key/history storage paths missing')
need('os.chmod(HISTORY, 0o600)' in worker_text, 'scan history owner-only permissions missing')
for token in ['YARA_X = "1.19.0"', 'CAPA = "9.4.0"', 'MAGIKA = "1.0.3"', 'capa-rules-v9.4.0']:
    need(token in runtime_text, 'pinned security runtime missing: '+token)
need('daemon' in runtime_text.lower() and 'components" / "security" / "runtime' in runtime_text, 'generated one-shot runtime contract missing')

# UI + user-approved behavior.
for token in ['NougatPanel { Search, Crawler, P2P, VirusScan }', '"Virus Scan"', '"Scan File"', '"Scan Folder"', '"Scan Again"', '"Community Key', '"History"']:
    need(token in main, 'Virus Scan UI missing: '+token)
need('metadata::custom-icon' in (root/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_32.sh').read_text(encoding='utf-8'), 'installer icon identity contract disappeared')
need('"Node "+node' not in main, 'stray ordinary Search Node identifier is still rendered')
need('"Node ID: "+node' in main, 'Node ID not retained in Network/advanced location')
need('text(target,28,174,status,searchPalette.text);' in main, 'Crawler status sentence was not moved to exact approved Y position')
need('nougatCrawlLogBox = {28, 190,' in main, 'Crawler results box geometry was changed')
need('"Ready. Crawl a site or add a peer, then search."' in main, 'Crawler status wording changed')
for token in ['known_peers', 'known_seeds', 'swarm_availability', 'announcing_trackers', 'announcing_dht', 'announcing_lsd']:
    need(token in p2ph, 'P2P availability evidence field missing: '+token)
for token in ['You: Seed ✓', 'Seeding: ', 'Remote seeds:', 'your complete local copy = 1.00']:
    need(token in main, 'P2P seeding clarity UI missing: '+token)

# Harmless offline scan must finish, hash correctly, leave file untouched, and write history.
with tempfile.TemporaryDirectory(prefix='nougat-security-v32-test-') as td:
    td = Path(td)
    home = td/'home'; home.mkdir()
    sample = td/'sample.txt'
    payload = b'Nougat harmless security test fixture.\n'
    sample.write_bytes(payload)
    before = sample.read_bytes()
    env = os.environ.copy(); env['HOME'] = str(home)
    proc = subprocess.run([sys.executable, str(worker), '--file', str(sample), '--offline'], env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    need(proc.returncode == 0, 'harmless one-shot worker scan failed: '+proc.stdout)
    need('VERDICT=NO THREATS DETECTED' in proc.stdout, 'harmless scan verdict incorrect')
    need(hashlib.sha256(payload).hexdigest() in proc.stdout, 'SHA-256 evidence missing/incorrect')
    need(sample.read_bytes() == before, 'worker modified harmless file')
    history = home/'.config/nougat-media-suite/security/scan_history.jsonl'
    need(history.is_file(), 'scan history not written')
    mode = stat.S_IMODE(history.stat().st_mode)
    need(mode == 0o600, f'scan history permissions are {oct(mode)}, expected 0600')

# Fake YARA-X module makes the harmless EICAR standard test string deterministic without installing/downloading an AV engine here.
with tempfile.TemporaryDirectory(prefix='nougat-security-v32-yara-') as td:
    td = Path(td); fake = td/'fake'; fake.mkdir(); home = td/'home'; home.mkdir()
    (fake/'yara_x.py').write_text(textwrap.dedent('''
        __version__='1.19.0'
        class Compiler:
            def __init__(self): self.sources=[]
            def new_namespace(self,n): pass
            def add_source(self,s): self.sources.append(s)
            def build(self): return self.sources
        class R:
            def __init__(self,i): self.identifier=i
        class Scan:
            def __init__(self,m): self.matching_rules=m
        class Scanner:
            def __init__(self,r): pass
            def set_timeout(self,t): pass
            def scan_file(self,p):
                b=open(p,'rb').read()
                return Scan([R('Nougat_EICAR_Standard_Test')]) if b.startswith(b'X5O!P%@AP') else Scan([])
    '''), encoding='utf-8')
    eicar = td/'eicar.com.txt'
    eicar_bytes = b'X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*'
    eicar.write_bytes(eicar_bytes)
    env = os.environ.copy(); env['HOME'] = str(home); env['PYTHONPATH'] = str(fake)
    proc = subprocess.run([sys.executable, str(worker), '--file', str(eicar), '--offline'], env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
    need(proc.returncode == 0, 'EICAR fixture worker run failed: '+proc.stdout)
    need('VERDICT=THREAT DETECTED' in proc.stdout and 'Nougat_EICAR_Standard_Test' in proc.stdout, 'YARA-X threat path not surfaced')
    need(eicar.read_bytes() == eicar_bytes, 'WARN ME FIRST violated: EICAR fixture was modified/moved/deleted')
    need('did not move, delete, quarantine, rename, or open' in proc.stdout, 'WARN ME FIRST report missing')


# Home scrolling controls are explicit and sheet-styled.
need('XRectangle homeClip{0, static_cast<short>(viewport_top)' in main, 'Home fixed-header clip missing')
need('homeVerticalScrollTrack = {W - 18, viewport_top + 4, 12, verticalTrackH};' in main, 'Home vertical scrollbar missing')
need('homeContinueScrollTrack = {homeContinueArea.x, scrollY, homeContinueArea.w, 14};' in main, 'Continue Watching horizontal scrollbar missing')
need('handle_home_scrollbar_press' in main, 'Home scrollbar click/drag routing missing')
need('homeVerticalScrollDragging = false;' in main, 'Home scrollbar release cleanup missing')
print('security-v32=pass one-shot=pass warn-first=pass no-auto-quarantine=pass yara-x=pass capa=pass magika=pass optional-clamscan=pass free-telemetry-hook=pass manual-scan=pass history=pass seed-availability=pass crawler-position-only=pass')
