#!/usr/bin/env python3
import pathlib, sys
root = pathlib.Path(sys.argv[1]).resolve()
p = (root / 'INSTALL_NOUGAT_MEDIA_SUITE_v0_0_38.sh').read_text(errors='replace')
checks = {
    'base-v37': 'exact accepted v0.0.37 touched state' in p and 'Nougat_Media_Suite_v37' in p,
    'repair-base-v38': 'exact known rejected v0.0.38 candidate' in p and 'v38-repair' in p and 'Nougat_Media_Suite_v38' in p,
    'fail-closed-baseline': 'project is neither the exact accepted v0.0.37 baseline nor the exact known rejected v0.0.38 repair base' in p and 'No files were changed.' in p,
    'manifest-identity': "target_version')!='0.0.38'" in p and "accepted_base_version')!='0.0.37'" in p,
    'no-terminal-exit': '\nexit ' not in p and '|| exit' not in p and 'set -e' not in p,
    'rollback': 'restore_rollback' in p and 'ROLLBACK PASS' in p and 'exact pre-repair touched state restored' in p,
    'repair-exe-rollback': 'rollback_root/project/Nougat_Media_Suite_v38' in p,
    'repair-manifest-rollback': 'rollback_root/project/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v38.json' in p,
    'running-exe-safe-replace': 'atomic_replace_file' in p and 'mv -f -- "$tmp" "$dest"' in p,
    'root-executable': 'Nougat_Media_Suite_v38' in p and 'install_final' in p,
    'generated-runtimes-preserved': 'components/security/runtime' in p and 'components/ai/runtime' in p and 'components/jellyfin/runtime' in p,
    'owner-archives-preserved': '1.zip' in p and '2.zip' in p,
    'v38-tests': 'test_nougat_media_suite_v38.py' in p and '--v38-library-live-tv-player-self-test' in p,
    'v37-regression': '--v37-live-tv-system-self-test' in p,
    'v36-regression': '--v36-library-ui-player-self-test' in p,
    'full-native-build': 'build_full' in p and 'REDDMEDIA_P2P_STUB=ON' in p,
    'old-version-last': 'remove_old_version_files' in p and 'NougatMediaSuite_v37.desktop' in p,
    'sheet-owner-gate': 'exact-sheet art/stitching' in p and 'black VOLUME percent' in p,
    'live-tv-owner-gate': 'real hardware playback' in p and 'Guide-only Live TV navigation' in p and 'System tuner controls' in p,
}
if not all(checks.values()):
    for k, v in checks.items():
        if not v:
            print('FAIL:', k)
    raise SystemExit(1)
print('installer-v38=pass rollback=pass terminal-safe=pass base-v37=pass repair-base-v38=pass fail-closed=pass repair-exe-restore=pass repair-manifest-restore=pass root-executable=pass retained-runtimes=pass owner-archives=pass v36-v37-regression=pass full-native-gate=pass exact-sheet-owner-gate=pass live-tv-owner-gate=pass')
