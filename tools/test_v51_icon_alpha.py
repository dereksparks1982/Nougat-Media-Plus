#!/usr/bin/env python3
from __future__ import annotations
import re
import struct
import sys
import zlib
from pathlib import Path

ROOT=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()


def need(ok: bool, message: str) -> None:
    if not ok:
        raise RuntimeError(message)


def rgba_png(path: Path):
    data=path.read_bytes()
    need(data.startswith(b'\x89PNG\r\n\x1a\n'),f'not PNG: {path}')
    pos=8; width=height=None; color=None; depth=None; raw=bytearray()
    while pos+12<=len(data):
        length=struct.unpack('>I',data[pos:pos+4])[0]
        kind=data[pos+4:pos+8]
        payload=data[pos+8:pos+8+length]
        pos+=12+length
        if kind==b'IHDR':
            width,height,depth,color,_,_,_=struct.unpack('>IIBBBBB',payload)
        elif kind==b'IDAT': raw.extend(payload)
        elif kind==b'IEND': break
    need(width is not None and height is not None,'PNG missing IHDR')
    need(depth==8 and color==6,f'expected 8-bit RGBA PNG: {path}')
    packed=zlib.decompress(bytes(raw)); stride=width*4
    need(len(packed)==height*(stride+1),f'unexpected PNG data length: {path}')
    rows=[]; prior=bytearray(stride); off=0
    for _y in range(height):
        f=packed[off]; off+=1
        scan=bytearray(packed[off:off+stride]); off+=stride
        recon=bytearray(stride)
        for x,val in enumerate(scan):
            a=recon[x-4] if x>=4 else 0
            b=prior[x]
            c=prior[x-4] if x>=4 else 0
            if f==0: out=val
            elif f==1: out=(val+a)&255
            elif f==2: out=(val+b)&255
            elif f==3: out=(val+((a+b)//2))&255
            elif f==4:
                p=a+b-c; pa=abs(p-a); pb=abs(p-b); pc=abs(p-c)
                pr=a if pa<=pb and pa<=pc else (b if pb<=pc else c)
                out=(val+pr)&255
            else: raise RuntimeError(f'unsupported PNG filter {f}: {path}')
            recon[x]=out
        rows.append(recon); prior=recon
    return width,height,rows



def embedded_icon_corner_alpha(header: str, size: int) -> tuple[int,int]:
    match=re.search(rf"kIcon{size}\[{size*size}\] = \{{(.*?)\n\}};",header,re.S)
    need(match is not None,f'missing embedded kIcon{size}')
    values=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{8})u',match.group(1))]
    need(len(values)==size*size,f'unexpected embedded kIcon{size} length')
    return (values[(size-1)*size]>>24)&255,(values[size*size-1]>>24)&255

def alpha_at(rows,x,y): return rows[y][x*4+3]

files=[ROOT/'assets/branding/nougat-media-suite-v51-master-N.png']
files += [ROOT/f'assets/icons/nougat-media-suite-v51-{size}.png' for size in (16,32,48,64,128,256,512)]
files += [ROOT/'assets/icons/nougat-media-suite-v51.png']
for path in files:
    need(path.is_file(),f'missing icon asset: {path.relative_to(ROOT)}')
    w,h,rows=rgba_png(path)
    need(w==h,f'icon is not square: {path.relative_to(ROOT)}')
    bl=alpha_at(rows,0,h-1); br=alpha_at(rows,w-1,h-1)
    need(bl==0 and br==0,
         f'bottom-corner alpha leak in {path.relative_to(ROOT)}: BL={bl} BR={br}')
    print(f'PASS: transparent bottom corners {path.relative_to(ROOT)}')
header=(ROOT/'src/nougat_media_suite_icon_data.hpp').read_text(encoding='utf-8')
for size in (16,32,64):
    bl,br=embedded_icon_corner_alpha(header,size)
    need(bl==0 and br==0,f'embedded kIcon{size} bottom-corner alpha leak: BL={bl} BR={br}')
    print(f'PASS: transparent embedded kIcon{size} bottom corners')
print('PASS: v0.0.51 N icon bottom-corner alpha gate')
