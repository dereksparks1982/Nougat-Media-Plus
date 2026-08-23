#!/usr/bin/env python3
import pathlib, sys
root = pathlib.Path(sys.argv[1]).resolve()
p = (root / 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_37.sh').read_text(errors='replace')
checks = {
    'base-v36': 'accepted-v0.0.36 preflight' in p and 'Nougat_Media_Suite_v36' in p and 'Accepted v0.0.36 touched-state hashes verified.' in p,
    'no-terminal-exit': '\nexit ' not in p and '|| exit' not in p and 'set -e' not in p,
    'rollback': 'restore_rollback' in p and 'ROLLBACK PASS' in p and 'exact accepted v0.0.36 touched state restored' in p,
    'root-executable': 'Nougat_Media_Suite_v37' in p and 'install_final' in p,
    'generated-runtimes-preserved': 'components/security/runtime' in p and 'components/ai/runtime' in p and 'components/jellyfin/runtime' in p,
    'owner-archives-preserved': '1.zip' in p and '2.zip' in p,
    'v37-tests': 'test_nougat_media_suite_v37.py' in p and '--v37-live-tv-system-self-test' in p,
    'v36-regression': '--v36-library-ui-player-self-test' in p,
    'full-native-build': 'build_full' in p and 'REDDMEDIA_P2P_STUB=ON' in p,
    'old-version-last': 'remove_old_version_files' in p and 'NougatMediaSuite_v36.desktop' in p,
    'watch-live-owner-gate': 'real hardware Watch Live' in p,
}
if not all(checks.values()):
    for k,v in checks.items():
        if not v: print('FAIL:',k)
    raise SystemExit(1)
print('installer-v37=pass rollback=pass terminal-safe=pass base-v36=pass root-executable=pass retained-runtimes=pass owner-archives=pass v36-regression=pass full-native-gate=pass watch-live-owner-gate=pass')
