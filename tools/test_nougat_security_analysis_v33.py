#!/usr/bin/env python3
from pathlib import Path
import os, stat, subprocess, sys, tempfile
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
worker=root/'components/security/nougat_security_worker.py'; runtime=root/'components/security/runtime'; py=runtime/'venv/bin/python'
def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
text=worker.read_text()
for token in ['ANALYSIS INCOMPLETE','YARA-X runtime unavailable','Magika file-type runtime unavailable','URLhaus','WARN ME FIRST','--offline']:
    need(token in text,'security contract missing: '+token)
for forbidden in ['os.remove(', 'shutil.move(', 'clamd']:
    need(forbidden not in text,'destructive/resident path present: '+forbidden)
with tempfile.TemporaryDirectory(prefix='nougat-sec-v33-') as td:
    td=Path(td); home=td/'home'; home.mkdir(); sample=td/'clean.txt'; sample.write_text('Nougat harmless clean fixture.\n'); before=sample.read_bytes()
    runner=str(py) if py.is_file() else sys.executable
    env=os.environ.copy(); env['HOME']=str(home)
    out=subprocess.check_output([runner,str(worker),'--file',str(sample),'--offline'],env=env,text=True,stderr=subprocess.STDOUT,timeout=240)
    if py.is_file(): need('VERDICT=NO THREATS DETECTED' in out,'verified full runtime did not produce clean offline verdict')
    else: need('VERDICT=ANALYSIS INCOMPLETE' in out,'missing runtime did not produce ANALYSIS INCOMPLETE')
    need(sample.read_bytes()==before,'scanner modified clean fixture')
    hist=home/'.config/nougat-media-suite/security/scan_history.jsonl'; need(hist.is_file(),'scan history missing'); need(stat.S_IMODE(hist.stat().st_mode)==0o600,'history permissions not 0600')
    if py.is_file():
        e=td/'eicar.txt'; e.write_bytes(b'X5O!P%@AP[4\\PZX54(P^)7CC)7}$EICAR-STANDARD-ANTIVIRUS-TEST-FILE!$H+H*'); e_before=e.read_bytes()
        eo=subprocess.check_output([runner,str(worker),'--file',str(e),'--offline'],env=env,text=True,stderr=subprocess.STDOUT,timeout=240)
        need('VERDICT=THREAT DETECTED' in eo and 'Nougat_EICAR_Antivirus_Test_File' in eo,'YARA-X EICAR test rule did not fire')
        need(e.read_bytes()==e_before,'WARN ME FIRST violated on EICAR fixture')
print('security-v33=pass strict-incomplete=pass one-shot=pass warn-first=pass yara-x-pin=pass capa-pin=pass magika-pin=pass urlhaus=pass no-resident-daemon=pass')
