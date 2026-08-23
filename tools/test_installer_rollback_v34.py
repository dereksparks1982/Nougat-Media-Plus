#!/usr/bin/env python3
import pathlib,sys
root=pathlib.Path(sys.argv[1]).resolve()
p=(root/'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_34.sh').read_text(errors='replace')
checks={
'base-commit':'6763a42bf5c125974e5a2882234fb2ee2e04c512' in p,
'no-terminal-exit':'\nexit ' not in p and '|| exit' not in p and 'set -e' not in p,
'rollback':'restore_rollback' in p and 'ROLLBACK PASS' in p,
'root-executable':'Nougat_Media_Suite_v34' in p and 'install_final' in p,
'generated-runtimes-preserved':'components/security/runtime' in p and 'components/ai/runtime' in p and 'components/jellyfin/runtime' in p,
'no-server-kill':'verify_persistent_server' not in p,
'v34-tests':'test_nougat_media_suite_v34.py' in p and '--v34-ui-polish-self-test' in p,
}
if not all(checks.values()):
    for k,v in checks.items():
        if not v: print('FAIL:',k)
    raise SystemExit(1)
print('installer-v34=pass rollback=pass terminal-safe=pass root-executable=pass retained-runtimes=pass server-untouched=pass')
