#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import datetime as dt
import os
import shutil
import subprocess
import sys

BASE = '8e346237928d4d358136b926f70e27729b6bd731'
TARGET = 'Nougat_Media_Suite_v53'
PREVIOUS = 'Nougat_Media_Suite_v52'

TRACKED_TARGETS = (
    'CMakeLists.txt',
    'NougatMediaSuite.desktop',
    'com.elderredsoftworks.NougatMediaSuite.desktop',
    'src/main.cpp',
    'src/live_tv/hdhomerun_provider.cpp',
    'src/world_tv/world_tv_service.hpp',
    'src/world_tv/world_tv_service.cpp',
    'components/world_tv/nougat_world_tv_worker.py',
    'components/games/artwork_cache_worker.py',
    'src/nougat_media_suite_icon_data.hpp',
)

class BuildError(RuntimeError):
    pass

def run(argv, cwd: Path, capture=False):
    result = subprocess.run(argv, cwd=cwd, text=True,
                            stdout=subprocess.PIPE if capture else None,
                            stderr=subprocess.STDOUT if capture else None)
    if result.returncode != 0:
        detail = f'\n{result.stdout}' if capture and result.stdout else ''
        raise BuildError(f'command failed ({result.returncode}): {" ".join(map(str,argv))}{detail}')
    return (result.stdout or '').strip() if capture else ''

def baseline_file_bytes(root: Path, rel: str) -> bytes:
    result = subprocess.run(['git','show',f'{BASE}:{rel}'], cwd=root, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise BuildError(f'could not read baseline file from Git: {rel}')
    return result.stdout

def verify_baseline(root: Path) -> None:
    # README/documentation-only commits after the accepted v0.0.52 release are
    # allowed. The actual files v0.0.53 will modify must still match the accepted
    # v0.0.52 bytes exactly before anything is overwritten.
    head = run(['git','rev-parse','HEAD'], root, capture=True)
    print(f'Current HEAD: {head}')
    for rel in TRACKED_TARGETS:
        path = root / rel
        if not path.is_file():
            raise BuildError(f'baseline target is missing: {rel}')
        if path.read_bytes() != baseline_file_bytes(root, rel):
            raise BuildError(f'v0.0.53 target differs from accepted v0.0.52 and will not be overwritten: {rel}')
    print('PASS: all v0.0.53 target files still match the accepted v0.0.52 code base.')

def backup_targets(root: Path) -> Path:
    stamp = dt.datetime.now().strftime('%Y%m%d_%H%M%S')
    archive = Path.home() / 'DKLab' / 'Archive' / 'Nougat Media Suite' / f'v0.0.53-preapply-{stamp}'
    archive.mkdir(parents=True, exist_ok=False)
    for rel in TRACKED_TARGETS:
        dest = archive / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(root/rel, dest)
    previous = root / PREVIOUS
    if previous.is_file():
        shutil.copy2(previous, archive/PREVIOUS)
    return archive

def install_desktop_identity(root: Path) -> None:
    appdir = Path.home()/'.local/share/applications'
    appdir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root/'com.elderredsoftworks.NougatMediaSuite.desktop', appdir/'com.elderredsoftworks.NougatMediaSuite.desktop')
    for size in (16,32,48,64,128,256,512):
        src = root/f'assets/icons/nougat-media-suite-v53-{size}.png'
        if not src.is_file():
            raise BuildError(f'generated icon missing: {src}')
        destdir = Path.home()/f'.local/share/icons/hicolor/{size}x{size}/apps'
        destdir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, destdir/'nougat-media-suite.png')
    for command in ('update-desktop-database','gtk-update-icon-cache'):
        exe=shutil.which(command)
        if not exe: continue
        if command == 'update-desktop-database':
            subprocess.run([exe,str(appdir)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            subprocess.run([exe,'-f','-t',str(Path.home()/'.local/share/icons/hicolor')], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def main() -> int:
    package = Path(__file__).resolve().parents[1]
    root = Path(sys.argv[1] if len(sys.argv)>1 else Path.home()/'DKLab/Projects/Nougat Media Suite').resolve()
    if not (root/'.git').exists():
        raise BuildError(f'Nougat Git project not found: {root}')

    print('=== NOUGAT MEDIA SUITE v0.0.53 CANDIDATE BUILD ===')
    print(f'Project: {root}')
    print('Preflight: exact accepted v0.0.52 target files...')
    verify_baseline(root)
    archive = backup_targets(root)
    print(f'Rollback snapshot: {archive}')

    run([sys.executable, str(package/'tools/apply_v53.py'), str(root)], root)
    run([sys.executable, str(root/'tools/test_v53_static.py'), str(root)], root)

    build_dir = root/'build-v53'
    if build_dir.exists():
        shutil.rmtree(build_dir)
    print('Configuring release build...')
    run(['cmake','-S',str(root),'-B',str(build_dir),'-DCMAKE_BUILD_TYPE=Release'], root)
    print('Compiling v0.0.53...')
    jobs = max(1, min(4, os.cpu_count() or 1))
    run(['cmake','--build',str(build_dir),'-j',str(jobs)], root)

    built = build_dir/TARGET
    if not built.is_file():
        raise BuildError(f'CMake completed but did not produce {built}')
    root_target = root/TARGET
    if root_target.exists():
        root_target.unlink()
    shutil.copy2(built, root_target)
    root_target.chmod(root_target.stat().st_mode | 0o111)
    version = run([str(root_target),'--version'], root, capture=True)
    if 'v0.0.53' not in version:
        raise BuildError(f'root executable version verification failed: {version!r}')

    # Successful candidate promotion keeps one current versioned executable in root.
    previous = root/PREVIOUS
    if previous.is_file():
        rollback = archive/PREVIOUS
        if not rollback.is_file():
            shutil.copy2(previous, rollback)
        previous.unlink()

    install_desktop_identity(root)
    print('=== BUILD PASS ===')
    print(version)
    print(f'Root executable: {root_target}')
    print('No commit, tag, or push was performed.')
    return 0

if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except BuildError as exc:
        print(f'STOP: {exc}', file=sys.stderr)
        raise SystemExit(1)
