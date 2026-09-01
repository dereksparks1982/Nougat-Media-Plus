#!/usr/bin/env python3
from __future__ import annotations
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import tempfile

SCRIPT = pathlib.Path(__file__).resolve().with_name("nougat_file_splitter.py")


def sha(path: pathlib.Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()


def run(*args: str, ok: bool=True) -> subprocess.CompletedProcess[str]:
    p=subprocess.run([sys.executable,str(SCRIPT),*args],text=True,capture_output=True)
    if ok and p.returncode != 0:
        raise AssertionError(f"command failed {args}:\nSTDOUT={p.stdout}\nSTDERR={p.stderr}")
    if not ok and p.returncode == 0:
        raise AssertionError(f"command unexpectedly passed {args}")
    return p


def main() -> int:
    with tempfile.TemporaryDirectory(prefix='nougat-splitter-test-') as td:
        root=pathlib.Path(td)
        # Multi-part byte exact round trip.
        src=root/'odd name.bin'
        src.write_bytes(bytes((i*37+11)%256 for i in range(2_700_123)))
        out=root/'parts'
        run('split-file',str(src),'--output',str(out),'--part-size-mib','1')
        manifests=list(out.glob('*.parts.json'))
        assert len(manifests)==1
        manifest=manifests[0]
        data=json.loads(manifest.read_text())
        assert data['part_count']==3
        assert data['payload_sha256']==sha(src)
        rebuilt=root/'rebuilt.bin'
        run('reassemble',str(manifest),'--output',str(rebuilt))
        assert rebuilt.read_bytes()==src.read_bytes()
        assert sha(rebuilt)==sha(src)

        # Corruption must be rejected before reconstruction.
        bad_part=out/data['parts'][1]['name']
        payload=bytearray(bad_part.read_bytes())
        payload[0] ^= 0xFF
        bad_part.write_bytes(payload)
        run('verify',str(manifest),ok=False)
        payload[0] ^= 0xFF
        bad_part.write_bytes(payload)
        run('verify',str(manifest))

        # Empty file round trip.
        empty=root/'empty.dat'; empty.write_bytes(b'')
        empty_parts=root/'empty-parts'
        run('split-file',str(empty),'--output',str(empty_parts),'--part-size-mib','1')
        empty_manifest=next(empty_parts.glob('*.parts.json'))
        empty_out=root/'empty-rebuilt.dat'
        run('reassemble',str(empty_manifest),'--output',str(empty_out))
        assert empty_out.read_bytes()==b''

        # Folder tree round trip, including spaces and a symlink when available.
        folder=root/'Project Folder'; folder.mkdir()
        (folder/'a.txt').write_text('Nougat\n',encoding='utf-8')
        nested=folder/'nested dir'; nested.mkdir(); (nested/'blob.bin').write_bytes(os.urandom(300_000))
        try:
            (folder/'link-to-a').symlink_to('a.txt')
            had_symlink=True
        except OSError:
            had_symlink=False
        folder_parts=root/'folder-parts'
        run('split-folder',str(folder),'--output',str(folder_parts),'--part-size-mib','1')
        folder_manifest=next(folder_parts.glob('*.parts.json'))
        restore_base=root/'restore'; restore_base.mkdir()
        run('reassemble',str(folder_manifest),'--output',str(restore_base))
        restored=restore_base/folder.name
        assert (restored/'a.txt').read_text(encoding='utf-8')=='Nougat\n'
        assert (restored/'nested dir'/'blob.bin').read_bytes()==(nested/'blob.bin').read_bytes()
        if had_symlink:
            assert (restored/'link-to-a').is_symlink()
            assert os.readlink(restored/'link-to-a')=='a.txt'

    print('Nougat v0.0.50 file splitter tests PASS')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
