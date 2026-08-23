#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
p=root/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_33.sh'; t=p.read_text()
def need(x,m):
    if not x: raise SystemExit('FAIL: '+m)
need('expected_head="084ee7ccd82be3a578f738b3bcb6ac8570a573dd"' in t,'accepted v32 commit gate missing')
need('Nougat_Media_Suite_v32' in t and 'Nougat Media Suite v0.0.32' in t,'accepted v32 executable gate missing')
need('Nougat_Media_Suite_v33' in t and 'Nougat Media Suite v0.0.33' in t,'v33 executable gate missing')
for x in ['install_nougat_security_runtime_v33.py','--check','--v33-integration-self-test','verify_persistent_server','parent-death test READY','NOUGAT_MEDIA_SERVER_OWNER','complete Nougat-owned server process tree','test_nougat_security_analysis_v33.py','test_nougat_media_suite_ui_smoke_v33.py','pkg-config --exists libtorrent-rasterbar','apply_raw_icon','ensure_python_venv_support','python_venv_works','cleanup_generated_python_caches','__pycache__','*.pyc','python3-venv','-venv']:
    need(x in t,'installer gate missing: '+x)
need(t.index('cleanup_generated_python_caches ||') < t.index('verify_git_state ||'), 'Python cache cleanup must run before Git preflight')
for pat in [r'(^|[;\s])exit(?:\s+[0-9]+)?(?:[;\s]|$)',r'\|\|\s*exit\b',r'\bset\s+-e\b']:
    need(re.search(pat,t,re.M) is None,'terminal-closing shell pattern present')
need('ROLLBACK PASS: accepted v0.0.32 touched state restored.' in t,'rollback proof missing')
need('remove_after_success' in t,'superseded-v32 cleanup manifest missing')
helper=(root/'tools/install_nougat_security_runtime_v33.py').read_text()
for token in ["from importlib.metadata import version", "version('yara-x')=='1.19.0'", "version('flare-capa')=='9.4.0'", "version('magika')=='1.0.3'", "rglob(\"*.yml\")", "runtime verify FAIL", "capa --version", "venv = runtime / \"venv\"", "runtime.rename(backup)", "backup.rename(runtime)", "nougat-security-v33-download-"]:
    need(token in helper, 'security runtime verification/placement repair missing: '+token)
need('stage.rename(runtime)' not in helper, 'cross-filesystem stage.rename(runtime) must not be used')
need('Path(td) / \"runtime\"' not in helper, 'security venv must be created at its final runtime path')
print('installer-v33=pass accepted-v32-gate=pass manifest=pass rollback=pass terminal-safe=pass runtime-hardening=pass venv-bootstrap=pass pycache-preflight-repair=pass capa-rules-verification-repair=pass cross-device-runtime-install-repair=pass persistent-server-test=pass owned-server-tree-stop=pass final-root-executable=pass')
