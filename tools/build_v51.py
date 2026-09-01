#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time

BASE='45f163752e5f1e0ed00f7d6d851bb6f6a5abf96e'
TARGET='Nougat_Media_Suite_v51'
PREVIOUS='Nougat_Media_Suite_v50'
PREVIOUS_SHA='cd1cf65c0e31772b0d7890f7243020b390f4409ba7e0605e870c0290ea506a6d'


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def run(args, *, cwd=None, capture=False, env=None):
    cmd=[str(x) for x in args]
    print('+',' '.join(cmd),flush=True)
    result=subprocess.run(cmd,cwd=cwd,env=env,text=True,
                          stdout=subprocess.PIPE if capture else None,
                          stderr=subprocess.STDOUT if capture else None)
    if capture and result.stdout:
        print(result.stdout,end='' if result.stdout.endswith('\n') else '\n')
    return result


def git(root: Path, *args: str) -> str:
    result=subprocess.run(['git',*args],cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    need(result.returncode==0,result.stderr.strip() or 'git failed')
    return result.stdout.strip()


def sha256(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda:handle.read(1024*1024),b''):
            h.update(block)
    return h.hexdigest()


def clean_output(text: str) -> bool:
    lower=text.lower()
    return 'warning:' not in lower and 'clock skew' not in lower


def candidate_changed_paths() -> list[str]:
    return [
        'src/main.cpp','src/nougat_media_suite_icon_data.hpp','src/live_tv/hdhomerun_provider.cpp',
        'src/world_tv/world_tv_service.cpp','components/world_tv/nougat_world_tv_worker.py',
        'src/lan/lan_media_service.hpp','src/lan/lan_media_service.cpp','CMakeLists.txt',
        'NougatMediaSuite.desktop','com.elderredsoftworks.NougatMediaSuite.desktop',
        'CHANGELOG.md','ROADMAP.md','DEPENDENCIES.md','docs/builds/NOUGAT_MEDIA_SUITE_v0_0_51_SCOPE.md',
        'tools/nougat_file_splitter.py','tools/test_nougat_file_splitter_v51.py','tools/test_v51_static.py',
        'tools/test_v51_icon_alpha.py','tools/apply_v51.py','tools/build_v51.py',
        'assets/branding/nougat-media-suite-v51-lockup-master.png',
        'assets/branding/nougat-media-suite-v51-lockup-header.png',
        'assets/branding/nougat-media-suite-v51-master-N.png',
        'assets/icons/nougat-media-suite-v51.png','assets/icons/nougat-media-suite-v51-16.png',
        'assets/icons/nougat-media-suite-v51-32.png','assets/icons/nougat-media-suite-v51-48.png',
        'assets/icons/nougat-media-suite-v51-64.png','assets/icons/nougat-media-suite-v51-128.png',
        'assets/icons/nougat-media-suite-v51-256.png','assets/icons/nougat-media-suite-v51-512.png',
    ]


def verify_package_manifest(package: Path) -> None:
    manifest_path=package/'MANIFEST.json'
    need(manifest_path.is_file(),'candidate MANIFEST.json is missing')
    manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    need(manifest.get('base_commit')==BASE,'candidate manifest base mismatch')
    listed=manifest.get('files')
    need(isinstance(listed,list) and listed,'candidate manifest file list is invalid')
    expected={}
    for entry in listed:
        need(isinstance(entry,dict),'candidate manifest entry is invalid')
        rel=entry.get('path'); size=entry.get('bytes'); digest=entry.get('sha256')
        need(isinstance(rel,str) and rel and rel!='MANIFEST.json',f'invalid manifest path: {rel!r}')
        need(rel not in expected,f'duplicate manifest path: {rel}')
        expected[rel]=(size,digest)
    actual=[]
    for path in package.rglob('*'):
        if not path.is_file(): continue
        rel=path.relative_to(package).as_posix()
        if rel=='MANIFEST.json': continue
        need('__pycache__/' not in rel and not rel.endswith('.pyc'),f'build debris in package: {rel}')
        actual.append(rel)
    need(sorted(actual)==sorted(expected),
         f'candidate manifest file-set mismatch; listed={sorted(expected)} actual={sorted(actual)}')
    for rel in sorted(actual):
        path=package/rel
        size,digest=expected[rel]
        need(size==path.stat().st_size,f'manifest byte-count mismatch: {rel}')
        need(digest==sha256(path),f'manifest SHA-256 mismatch: {rel}')
    print(f'PASS: candidate manifest verified ({len(actual)} files).')


def snapshot_known_rejected_candidate(root: Path, archive_parent: Path) -> None:
    # A deleted v50 root executable is intentional in the rejected v51 state.
    # It is not an unexpected source edit and must never be restored beside v51.
    known=set(candidate_changed_paths()) | {PREVIOUS}
    unstaged=set(filter(None,git(root,'diff','--name-only').splitlines()))
    staged=set(filter(None,git(root,'diff','--cached','--name-only').splitlines()))
    dirty=unstaged|staged
    unexpected=sorted(dirty-known)
    need(not unexpected,'STOP: tracked changes outside the known rejected v51 candidate exist: '+', '.join(unexpected))

    untracked=set(filter(None,git(root,'ls-files','--others','--exclude-standard').splitlines()))
    rejected_untracked=sorted(path for path in untracked if path in known)
    has_v51=(root/TARGET).is_file()
    if not dirty and not rejected_untracked and not has_v51:
        return

    rejected=archive_parent/('rejected-v51-before-r5-'+time.strftime('%Y%m%d-%H%M%S'))
    rejected.mkdir(parents=True,exist_ok=False)
    status=subprocess.run(['git','status','--short'],cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (rejected/'status.txt').write_text(status.stdout or '',encoding='utf-8')
    for args,name in ((['git','diff','--binary'],'unstaged.patch'),
                      (['git','diff','--cached','--binary'],'staged.patch')):
        result=subprocess.run(args,cwd=root,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        need(result.returncode==0,f'could not snapshot rejected candidate {name}')
        (rejected/name).write_bytes(result.stdout)
    for rel in sorted(dirty|set(rejected_untracked)):
        source=root/rel
        if source.is_file():
            target=rejected/'files'/rel
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(source,target)
    if has_v51:
        shutil.copy2(root/TARGET,rejected/TARGET)
    print('Rejected candidate safety snapshot:',rejected)
    print('PASS: rejected-v51 state was snapshotted without changing the project tree before compilation.')


def file_sha_or_none(path: Path) -> str | None:
    if not path.is_file():
        return None
    return sha256(path)


def merge_legacy_archive_tree(source: Path, destination: Path, stamp: str) -> None:
    destination.mkdir(parents=True,exist_ok=True)
    for item in sorted(source.iterdir(), key=lambda x: x.name):
        target=destination/item.name
        if item.is_dir() and not item.is_symlink():
            if target.exists() and not target.is_dir():
                target=destination/(item.name+'.from-Archives-'+stamp)
            if target.exists() and target.is_dir():
                merge_legacy_archive_tree(item,target,stamp)
                if not any(item.iterdir()):
                    item.rmdir()
            else:
                shutil.move(str(item),str(target))
            continue

        if not target.exists() and not target.is_symlink():
            shutil.move(str(item),str(target))
            continue

        identical=False
        if item.is_file() and target.is_file():
            identical=file_sha_or_none(item)==file_sha_or_none(target)
        if identical:
            item.unlink()
            continue

        alternate=destination/(item.name+'.from-Archives-'+stamp)
        counter=1
        while alternate.exists() or alternate.is_symlink():
            alternate=destination/(item.name+f'.from-Archives-{stamp}-{counter}')
            counter+=1
        shutil.move(str(item),str(alternate))


def migrate_wrong_archives_folder() -> Path:
    # Earlier candidate builders incorrectly created DKLab/Archives.  The owner
    # requires one canonical singular root: DKLab/Archive.  Migrate the old tree
    # losslessly, then remove the wrong plural directory.
    dklab=Path.home()/'DKLab'
    correct=dklab/'Archive'
    wrong=dklab/'Archives'
    correct.mkdir(parents=True,exist_ok=True)
    if not wrong.exists():
        return correct
    need(wrong.is_dir() and not wrong.is_symlink(),f'STOP: {wrong} is not a normal directory')
    stamp=time.strftime('%Y%m%d-%H%M%S')
    print(f'Migrating incorrect archive root {wrong} -> {correct} ...')
    merge_legacy_archive_tree(wrong,correct,stamp)
    # merge_legacy_archive_tree empties nested directories as it returns.
    for directory in sorted((p for p in wrong.rglob('*') if p.is_dir()), key=lambda p: len(p.parts), reverse=True):
        if not any(directory.iterdir()):
            directory.rmdir()
    need(not any(wrong.iterdir()),f'STOP: could not safely empty incorrect archive directory {wrong}')
    wrong.rmdir()
    need(not wrong.exists(),'STOP: incorrect DKLab/Archives directory still exists after migration')
    print('PASS: incorrect DKLab/Archives tree migrated and removed.')
    return correct


def process_alive(pid: int) -> bool:
    if pid<=1: return False
    try:
        os.kill(pid,0); return True
    except ProcessLookupError: return False
    except PermissionError: return True


def terminate(pid: int, label: str) -> None:
    if not process_alive(pid): return
    print(f'Stopping {label} PID {pid}...')
    try: os.kill(pid,signal.SIGTERM)
    except ProcessLookupError: return
    for _ in range(50):
        if not process_alive(pid): return
        time.sleep(.1)
    try: os.kill(pid,signal.SIGKILL)
    except ProcessLookupError: return
    for _ in range(20):
        if not process_alive(pid): return
        time.sleep(.1)
    raise RuntimeError(f'{label} PID {pid} did not stop')


def proc_exe(pid: int) -> str:
    try: return os.readlink(f'/proc/{pid}/exe')
    except OSError: return ''


def proc_bytes(pid: int, name: str) -> bytes:
    try: return Path(f'/proc/{pid}/{name}').read_bytes()
    except OSError: return b''


def safe_shutdown(root: Path) -> None:
    print('=== SAFE NOUGAT SHUTDOWN ===')
    prefix=str(root.resolve())+'/'
    for proc in Path('/proc').iterdir():
        if not proc.name.isdigit(): continue
        pid=int(proc.name)
        exe=proc_exe(pid)
        if exe.startswith(prefix) and Path(exe).name.startswith('Nougat_Media_Suite'):
            terminate(pid,'Nougat Media Suite')

    ownership=Path.home()/'.local/share/reddmedia/server/nougat-owned.pid'
    if ownership.is_file():
        lines=ownership.read_text(encoding='utf-8',errors='replace').splitlines()
        try: pid=int(lines[0]) if lines else -1
        except ValueError: pid=-1
        runtime=lines[1].strip() if len(lines)>1 else ''
        token=lines[2].strip() if len(lines)>2 else ''
        if process_alive(pid):
            exe=proc_exe(pid); cmd=proc_bytes(pid,'cmdline'); env=proc_bytes(pid,'environ')
            runtime_match=bool(runtime) and (exe==runtime or runtime.encode() in cmd)
            token_match=bool(token) and (('NOUGAT_MEDIA_SERVER_OWNER='+token).encode() in env)
            signature=runtime_match and b'Nougat Media Suite integrated Jellyfin' in cmd
            need(runtime_match and (token_match or signature),
                 'Running server from ownership file could not be verified as Nougat-owned; it was left untouched.')
            terminate(pid,'verified Nougat-owned Jellyfin')
    print('PASS: runtime shutdown safe.')


def copy_payload(package: Path, work: Path) -> None:
    # Exact owner-approved branding and v51 support files.
    files=[
        'src/nougat_media_suite_icon_data.hpp',
        'tools/nougat_file_splitter.py','tools/test_nougat_file_splitter_v51.py',
        'tools/test_v51_static.py','tools/test_v51_icon_alpha.py','tools/apply_v51.py','tools/build_v51.py',
        'assets/branding/nougat-media-suite-v51-lockup-master.png',
        'assets/branding/nougat-media-suite-v51-lockup-header.png',
        'assets/branding/nougat-media-suite-v51-master-N.png',
        'assets/icons/nougat-media-suite-v51.png',
        'assets/icons/nougat-media-suite-v51-16.png',
        'assets/icons/nougat-media-suite-v51-32.png',
        'assets/icons/nougat-media-suite-v51-48.png',
        'assets/icons/nougat-media-suite-v51-64.png',
        'assets/icons/nougat-media-suite-v51-128.png',
        'assets/icons/nougat-media-suite-v51-256.png',
        'assets/icons/nougat-media-suite-v51-512.png',
    ]
    for rel in files:
        src=package/rel; dst=work/rel
        need(src.is_file(),f'package payload missing: {rel}')
        dst.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(src,dst)


def link_runtime(main: Path, work: Path, rel: str) -> None:
    src=main/rel; dst=work/rel
    if src.exists() and not dst.exists():
        dst.parent.mkdir(parents=True,exist_ok=True)
        os.symlink(src,dst,target_is_directory=src.is_dir())
        print('Linked runtime:',rel)


def install_identity(root: Path) -> None:
    icon=root/'assets/icons/nougat-media-suite-v51.png'
    need(icon.is_file(),'v51 approved N icon asset missing')
    home=Path.home()
    icon_root=home/'.local/share/icons'
    icon_root.mkdir(parents=True,exist_ok=True)
    shutil.copy2(icon,icon_root/'nougat-media-suite-v51.png')
    for size in (16,32,48,64,128,256,512):
        source=root/f'assets/icons/nougat-media-suite-v51-{size}.png'
        need(source.is_file(),f'missing {size}px N icon')
        target=icon_root/'hicolor'/f'{size}x{size}'/'apps'/'nougat-media-suite-v51.png'
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target)

    apps=home/'.local/share/applications'
    apps.mkdir(parents=True,exist_ok=True)
    canonical='com.elderredsoftworks.NougatMediaSuite.desktop'
    for name in ('NougatMediaSuite.desktop',canonical):
        source=root/name
        need(TARGET in source.read_text(encoding='utf-8'),f'{name} does not target v51')
    # One canonical installed application identity prevents WM_CLASS/app-ID
    # ambiguity in GNOME's dock, switcher and process/application grouping.
    shutil.copy2(root/canonical,apps/canonical)
    stale=apps/'NougatMediaSuite.desktop'
    if stale.exists() or stale.is_symlink():
        stale.unlink()

    gio=shutil.which('gio')
    need(gio is not None,'gio is required for executable icon identity')
    uri=icon.resolve().as_uri()
    promoted=root/TARGET
    identity_paths=(promoted,root/'NougatMediaSuite.desktop',root/canonical)
    for identity_path in identity_paths:
        result=run([gio,'set','-t','string',identity_path,'metadata::custom-icon',uri],capture=True)
        need(result.returncode==0,f'could not set v51 custom icon on {identity_path.name}')
        result=run([gio,'info','-a','metadata::custom-icon',identity_path],capture=True)
        need(result.returncode==0 and uri in (result.stdout or ''),
             f'v51 icon metadata readback failed for {identity_path.name}')

    desktop_db=shutil.which('update-desktop-database')
    if desktop_db:
        result=run([desktop_db,apps],capture=True)
        need(result.returncode==0 and clean_output(result.stdout or ''),'desktop database refresh warned/failed')
    gtk=shutil.which('gtk-update-icon-cache')
    if gtk:
        result=run([gtk,'-f','-t',icon_root/'hicolor'],capture=True)
        need(result.returncode==0,'icon cache refresh failed')
    print('PASS: v51 icon assets installed and executable metadata assigned.')
    print('OWNER VISUAL ICON GATE REMAINS: Files/Nautilus, running window, dock/app switcher/sidebar.')


def main() -> int:
    try:
        package=Path(__file__).resolve().parents[1]
        root=Path(sys.argv[1] if len(sys.argv)>1 else Path.home()/'DKLab/Projects/Nougat Media Suite').resolve()
        need((root/'.git').exists() or git(root,'rev-parse','--git-dir'),f'Not a Git repo: {root}')
        need(Path(git(root,'rev-parse','--show-toplevel')).resolve()==root,'Run against Nougat repository root')
        head=git(root,'rev-parse','HEAD')
        need(head==BASE,f'STOP: v51 base must be exact accepted v0.0.50 {BASE}; current {head}')
        need(git(root,'branch','--show-current')=='main','v51 candidate must start from main')

        safe_shutdown(root)
        verify_package_manifest(package)
        archive_base=migrate_wrong_archives_folder()
        archive_root=archive_base/'Nougat Media Suite'
        archive_root.mkdir(parents=True,exist_ok=True)
        snapshot_known_rejected_candidate(root,archive_root)

        # Verify the accepted v50 executable directly from the exact Git base.
        # The owner correctly removes old root executables, so v50 is not required
        # to exist beside a rejected or promoted v51 executable.
        base_v50=subprocess.run(['git','show',f'{BASE}:{PREVIOUS}'],cwd=root,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        need(base_v50.returncode==0,'could not read accepted v0.0.50 executable from exact Git base')
        previous_sha=hashlib.sha256(base_v50.stdout).hexdigest()
        need(previous_sha==PREVIOUS_SHA,
             f'accepted Git v0.0.50 executable SHA mismatch: expected {PREVIOUS_SHA}, got {previous_sha}')
        print('PASS: accepted Git v0.0.50 SHA-256:',previous_sha)

        archive=archive_root/('v0.0.51-prebuild-'+time.strftime('%Y%m%d-%H%M%S'))
        archive.mkdir(parents=True,exist_ok=False)
        snapshot=archive/'Nougat-Media-Suite-v0.0.50-base.tar.gz'
        result=run(['git','archive','--format=tar.gz','-o',snapshot,BASE],cwd=root)
        need(result.returncode==0 and snapshot.is_file() and snapshot.stat().st_size>0,'prebuild snapshot failed')
        print('Snapshot:',snapshot)
        print('Snapshot SHA-256:',sha256(snapshot))

        temp_parent=Path(tempfile.mkdtemp(prefix='nougat-v51-candidate-'))
        work=temp_parent/'source'; build=temp_parent/'build'
        try:
            result=run(['git','worktree','add','--detach',work,BASE],cwd=root)
            need(result.returncode==0,'temporary worktree creation failed')
            for rel in ('components/ai/runtime','components/games/runtime','components/jellyfin/runtime'):
                link_runtime(root,work,rel)
            copy_payload(package,work)

            print('=== APPLY v0.0.51 ===')
            result=run([sys.executable,work/'tools/apply_v51.py',work],cwd=work,capture=True)
            need(result.returncode==0,'v51 source patch failed')

            print('=== SOURCE TESTS ===')
            for test in ('tools/test_nougat_file_splitter_v51.py','tools/test_v51_static.py','tools/test_v51_icon_alpha.py'):
                needs_root=test.endswith('static.py') or test.endswith('icon_alpha.py')
                result=run([sys.executable,work/test,work] if needs_root else [sys.executable,work/test],cwd=work,capture=True)
                need(result.returncode==0,f'{test} failed')
            result=run([sys.executable,'-m','py_compile',work/'components/world_tv/nougat_world_tv_worker.py'],cwd=work,capture=True)
            need(result.returncode==0,'World TV worker syntax failed')
            for test in ('tools/test_hdhomerun_provider_v50.py','tools/test_license_protection_v22.py'):
                if (work/test).is_file():
                    result=run([sys.executable,work/test,work],cwd=work,capture=True)
                    need(result.returncode==0,f'{test} failed')

            print('=== NATIVE v0.0.51 BUILD ===')
            result=run(['cmake','-S',work,'-B',build],capture=True)
            need(result.returncode==0,'CMake configure failed')
            need(clean_output(result.stdout or ''),'CMake configure emitted a warning')
            result=run(['cmake','--build',build,'--target',TARGET,'-j2'],capture=True)
            need(result.returncode==0,'native v0.0.51 build failed')
            need(clean_output(result.stdout or ''),'native v0.0.51 build emitted a warning')
            built=build/TARGET
            need(built.is_file() and os.access(built,os.X_OK),'v51 build output missing')

            env=dict(os.environ)
            libs=[work/'components/ai/runtime/lib',work/'components/ai/runtime/lib64']
            libs=[str(x) for x in libs if x.exists()]
            if libs:
                env['LD_LIBRARY_PATH']=':'.join(libs)+((':'+env['LD_LIBRARY_PATH']) if env.get('LD_LIBRARY_PATH') else '')
            result=run([built,'--version'],capture=True,env=env)
            need(result.returncode==0 and (result.stdout or '').strip()=='Nougat Media Suite v0.0.51','build-tree v51 identity mismatch')

            for flag in ('--v49-games-self-test','--v47-nav-self-test','--v47-fullscreen-controls-self-test','--v47-window-identity-self-test'):
                result=run([built,flag],capture=True,env=env)
                need(result.returncode==0 and 'PASS' in (result.stdout or ''),f'retained self-test failed: {flag}')

            print('=== PROMOTE VERIFIED v0.0.51 CANDIDATE ===')
            changed=candidate_changed_paths()
            prepromotion=temp_parent/'prepromotion-root-state'
            prepromotion.mkdir(parents=True,exist_ok=True)
            existed_before={}
            for rel in [*changed,TARGET,PREVIOUS]:
                source=root/rel
                existed=source.is_file() or source.is_symlink()
                existed_before[rel]=existed
                if existed:
                    backup=prepromotion/rel
                    backup.parent.mkdir(parents=True,exist_ok=True)
                    if source.is_symlink():
                        os.symlink(os.readlink(source),backup)
                    else:
                        shutil.copy2(source,backup)

            promotion_started=False
            try:
                promotion_started=True
                for rel in changed:
                    source=work/rel; target=root/rel
                    need(source.exists(),f'candidate source missing: {rel}')
                    target.parent.mkdir(parents=True,exist_ok=True)
                    shutil.copy2(source,target)
                shutil.copy2(built,root/TARGET)
                (root/TARGET).chmod((root/TARGET).stat().st_mode | 0o111)

                # One active versioned executable in the project root.  v50 is
                # preserved by the verified Git archive, never beside v51.
                previous=root/PREVIOUS
                if previous.exists() or previous.is_symlink():
                    previous.unlink()
                root_execs=sorted(x.name for x in root.glob('Nougat_Media_Suite_v[0-9]*') if x.is_file())
                need(root_execs==[TARGET],f'root executable gate failed after promotion: {root_execs}')

                clean_env=dict(os.environ); clean_env.pop('LD_LIBRARY_PATH',None)
                result=run([root/TARGET,'--version'],capture=True,env=clean_env)
                need(result.returncode==0 and (result.stdout or '').strip()=='Nougat Media Suite v0.0.51','promoted v51 executable identity mismatch')
                install_identity(root)
                root_execs=sorted(x.name for x in root.glob('Nougat_Media_Suite_v[0-9]*') if x.is_file())
                need(root_execs==[TARGET],f'final root executable gate failed: {root_execs}')
                need(not (Path.home()/'DKLab/Archives').exists(),'final archive gate failed: DKLab/Archives exists')
            except Exception:
                if promotion_started:
                    print('=== RESTORE PRE-PROMOTION PROJECT STATE AFTER FAILED PROMOTION ===')
                    for rel in [*changed,TARGET,PREVIOUS]:
                        target=root/rel
                        if target.exists() or target.is_symlink():
                            if target.is_dir() and not target.is_symlink():
                                shutil.rmtree(target)
                            else:
                                target.unlink()
                        if existed_before.get(rel):
                            backup=prepromotion/rel
                            target.parent.mkdir(parents=True,exist_ok=True)
                            if backup.is_symlink():
                                os.symlink(os.readlink(backup),target)
                            else:
                                shutil.copy2(backup,target)
                    print('PASS: exact pre-promotion project state restored after failed promotion.')
                raise

            print('=== NOUGAT v0.0.51 CANDIDATE PASS ===')
            print('Executable:',root/TARGET)
            print('SHA-256:',sha256(root/TARGET))
            print('Accepted Git v0.0.50 rollback SHA-256:',previous_sha)
            print('Root executable gate: only Nougat_Media_Suite_v51 is present.')
            print('Snapshot:',snapshot)
            print('NO GIT COMMIT, TAG, OR GITHUB PUSH PERFORMED.')
            print('OWNER TEST GATES:')
            print('  1. New exact N icon in Files/window/dock/sidebar and new exact N+cursive lockup in header.')
            print('  2. Header visibly says v0.0.51.')
            print('  3. Split folder/file/existing ZIP by chosen piece count and exact reassembly.')
            print('  4. FLEX DUO shown once with two nested tuner states; full scan finishes and channels remain.')
            print('  5. WinTV regression when connected.')
            print('  6. World TV timeline guide plus representative international playback.')
            print('  7. Floating player/channel overlays are translucent and cleanly rounded system-wide.')
            print('  8. Radio opens its independent Mulberry root, not the Video Player/resume overlay.')
            print('  9. Top navigation scrolls fully until the complete System tab is visible.')
            print(' 10. Nougat is grouped under its canonical GNOME application identity in the system Resources/process app so normal End App/End Process handling is available.')
            print(' 11. LAN endpoint diagnostics remain LAN-only/no cloud exposure.')
            return 0
        finally:
            subprocess.run(['git','worktree','remove','--force',work],cwd=root,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            shutil.rmtree(temp_parent,ignore_errors=True)
    except Exception as exc:
        print('FAIL:',exc)
        print('No Git commit, tag, or GitHub push was performed.')
        print('Terminal remains open.')
        return 1

if __name__=='__main__':
    raise SystemExit(main())
