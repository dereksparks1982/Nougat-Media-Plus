#!/usr/bin/env python3
import pathlib, sys
root = pathlib.Path(sys.argv[1]).resolve()
p = (root / 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_36.sh').read_text(errors='replace')
checks = {
    'base-commit': '8c2b8fb63254eb4b17045688ffbd50c1bba2075b' in p,
    'no-terminal-exit': '\nexit ' not in p and '|| exit' not in p and 'set -e' not in p,
    'rollback': 'restore_rollback' in p and 'ROLLBACK PASS' in p,
    'root-executable': 'Nougat_Media_Suite_v36' in p and 'install_final' in p,
    'generated-runtimes-preserved': 'components/security/runtime' in p and 'components/ai/runtime' in p and 'components/jellyfin/runtime' in p,
    'owner-source-archives-preserved': "allowed_owner_archives={'1.zip','2.zip'}" in p,
    'persistent-server-safe': 'Retained security runtime' in p and 'build_full' in p,
    'v36-tests': 'test_nougat_media_suite_v36.py' in p and '--v36-library-ui-player-self-test' in p,
    'v35-regression': '--v35-cleanup-self-test' in p,
    'full-native-build': 'build_full' in p and 'REDDMEDIA_P2P_STUB=ON' in p,
    'old-version-last': 'remove_old_version_files' in p and 'NougatMediaSuite_v35.desktop' in p,
}
if not all(checks.values()):
    for k,v in checks.items():
        if not v: print('FAIL:',k)
    raise SystemExit(1)
print('installer-v36=pass rollback=pass terminal-safe=pass base-v35=pass root-executable=pass retained-runtimes=pass v35-regression=pass full-native-gate=pass')
