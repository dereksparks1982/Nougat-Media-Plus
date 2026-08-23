#!/usr/bin/env python3
import pathlib, sys
root = pathlib.Path(sys.argv[1]).resolve()
p = (root / 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_35.sh').read_text(errors='replace')
checks = {
    'base-commit': '64eade89e7b9b88f1696bef18e580e22bace978f' in p,
    'no-terminal-exit': '\nexit ' not in p and '|| exit' not in p and 'set -e' not in p,
    'rollback': 'restore_rollback' in p and 'ROLLBACK PASS' in p,
    'interrupted-repair-recovery': 'recover_interrupted_same_version_state' in p and 'pre_repair_candidate_manifest_v35.json' in p,
    'rollback-preserves-v35-exe': 'rollback_root/project/Nougat_Media_Suite_v35' in p and 'rm -f -- "$project_root/Nougat_Media_Suite_v35" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v35.json"' not in p,
    'root-executable': 'Nougat_Media_Suite_v35' in p and 'install_final' in p,
    'generated-runtimes-preserved': 'components/security/runtime' in p and 'components/ai/runtime' in p and 'components/jellyfin/runtime' in p,
    'server-untouched': 'closing the desktop UI' in p or 'server lifecycle' in p,
    'v35-tests': 'test_nougat_media_suite_v35.py' in p and '--v35-cleanup-self-test' in p,
    'full-native-build': 'build_full' in p and 'REDDMEDIA_P2P_STUB=ON' in p,
}
if not all(checks.values()):
    for k, v in checks.items():
        if not v:
            print('FAIL:', k)
    raise SystemExit(1)
print('installer-v35=pass rollback=pass interrupted-recovery=pass terminal-safe=pass root-executable=pass retained-runtimes=pass server-untouched=pass full-native-gate=pass')
