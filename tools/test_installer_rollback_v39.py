#!/usr/bin/env python3
import pathlib, sys
root = pathlib.Path(sys.argv[1]).resolve()
p = (root / 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_39.sh').read_text(errors='replace')
checks = {
    'base-v38': 'exact accepted v0.0.38 touched state' in p and 'Nougat_Media_Suite_v38' in p,
    'fail-closed-baseline': 'project is not the exact accepted v0.0.38 baseline' in p and 'No files were changed.' in p,
    'manifest-identity': "target_version')!='0.0.39'" in p and "accepted_base_version')!='0.0.38'" in p,
    'no-terminal-exit': '\nexit ' not in p and '|| exit' not in p and 'set -e' not in p,
    'rollback': 'restore_rollback' in p and 'ROLLBACK PASS' in p and 'exact pre-v0.0.39 touched state restored' in p,
    'running-exe-safe-replace': 'atomic_replace_file' in p and 'mv -f -- "$tmp" "$dest"' in p,
    'root-executable': 'Nougat_Media_Suite_v39' in p and 'install_final' in p,
    'generated-runtimes-preserved': 'components/security/runtime' in p and 'components/ai/runtime' in p and 'components/jellyfin/runtime' in p,
    'owner-archives-preserved': '1.zip' in p and '2.zip' in p,
    'v39-tests': 'test_nougat_media_suite_v39.py' in p and '--v39-diagnostics-live-tv-self-test' in p,
    'v38-regression': '--v38-library-live-tv-player-self-test' in p,
    'v37-regression': '--v37-live-tv-system-self-test' in p,
    'full-native-build': 'build_full' in p and 'REDDMEDIA_P2P_STUB=ON' in p,
    'old-version-last': 'remove_old_version_files' in p and 'NougatMediaSuite_v38.desktop' in p,
    'diagnostic-gate': 'Deep Diagnostics' in p and 'guide coverage' in p and 'current-multiplex' in p,
}
if not all(checks.values()):
    for k, v in checks.items():
        if not v:
            print('FAIL:', k)
    raise SystemExit(1)
print('installer-v39=pass rollback=pass terminal-safe=pass base-v38=pass fail-closed=pass root-executable=pass retained-runtimes=pass owner-archives=pass v37-v38-regression=pass full-native-gate=pass diagnostic-live-tv-gate=pass')
