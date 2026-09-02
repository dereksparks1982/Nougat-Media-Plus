#!/usr/bin/env python3
from __future__ import annotations

import binascii
from pathlib import Path
import re
import struct
import sys
import zlib

PNG_SIG = b"\x89PNG\r\n\x1a\n"

class IconError(RuntimeError):
    pass

def chunks(data: bytes):
    pos = 8
    while pos + 12 <= len(data):
        size = struct.unpack(">I", data[pos:pos+4])[0]
        kind = data[pos+4:pos+8]
        payload = data[pos+8:pos+8+size]
        yield kind, payload
        pos += 12 + size
        if kind == b"IEND":
            break

def paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa, pb, pc = abs(p-a), abs(p-b), abs(p-c)
    if pa <= pb and pa <= pc: return a
    if pb <= pc: return b
    return c

def read_rgba(path: Path):
    data = path.read_bytes()
    if not data.startswith(PNG_SIG):
        raise IconError(f"not PNG: {path}")
    ihdr = None
    compressed = bytearray()
    for kind, payload in chunks(data):
        if kind == b"IHDR": ihdr = payload
        elif kind == b"IDAT": compressed.extend(payload)
    if ihdr is None:
        raise IconError("PNG missing IHDR")
    width, height, depth, color_type, compression, filtering, interlace = struct.unpack(">IIBBBBB", ihdr)
    if depth != 8 or color_type != 6 or compression != 0 or filtering != 0 or interlace != 0:
        raise IconError("v53 icon repair requires non-interlaced 8-bit RGBA PNG")
    raw = zlib.decompress(bytes(compressed))
    stride = width * 4
    rows = []
    offset = 0
    previous = bytearray(stride)
    for _ in range(height):
        f = raw[offset]; offset += 1
        scan = bytearray(raw[offset:offset+stride]); offset += stride
        recon = bytearray(stride)
        for i, value in enumerate(scan):
            left = recon[i-4] if i >= 4 else 0
            up = previous[i]
            ul = previous[i-4] if i >= 4 else 0
            if f == 0: predictor = 0
            elif f == 1: predictor = left
            elif f == 2: predictor = up
            elif f == 3: predictor = (left + up) // 2
            elif f == 4: predictor = paeth(left, up, ul)
            else: raise IconError("unsupported PNG filter")
            recon[i] = (value + predictor) & 0xff
        rows.append(recon)
        previous = recon
    return width, height, rows

def write_rgba(path: Path, width: int, height: int, rows):
    raw = bytearray()
    for row in rows:
        raw.append(0)
        raw.extend(row)
    def chunk(kind: bytes, payload: bytes):
        body = kind + payload
        return struct.pack(">I", len(payload)) + body + struct.pack(">I", binascii.crc32(body) & 0xffffffff)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    data = PNG_SIG + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)

def clear_rounded_exterior(width: int, height: int, rows):
    if width != height:
        raise IconError("approved Nougat N master must be square")
    radius = max(2, round(width * 0.19))
    r2 = radius * radius
    changed = 0
    for y in range(height):
        for x in range(width):
            cx = cy = None
            if x < radius and y < radius: cx, cy = radius, radius
            elif x >= width-radius and y < radius: cx, cy = width-radius-1, radius
            elif x < radius and y >= height-radius: cx, cy = radius, height-radius-1
            elif x >= width-radius and y >= height-radius: cx, cy = width-radius-1, height-radius-1
            if cx is None: continue
            dx, dy = x-cx, y-cy
            if dx*dx + dy*dy > r2:
                ai = x*4+3
                if rows[y][ai] != 0:
                    rows[y][ai] = 0
                    changed += 1
    return changed

def resize_rgba(rows, sw: int, sh: int, size: int):
    # Bilinear sampling in premultiplied-alpha space avoids dark/white corner halos.
    out = []
    for oy in range(size):
        sy = ((oy + 0.5) * sh / size) - 0.5
        y0 = max(0, min(sh-1, int(sy)))
        y1 = max(0, min(sh-1, y0 + 1))
        fy = max(0.0, min(1.0, sy - int(sy)))
        row = bytearray(size*4)
        for ox in range(size):
            sx = ((ox + 0.5) * sw / size) - 0.5
            x0 = max(0, min(sw-1, int(sx)))
            x1 = max(0, min(sw-1, x0 + 1))
            fx = max(0.0, min(1.0, sx - int(sx)))
            samples = ((x0,y0,(1-fx)*(1-fy)), (x1,y0,fx*(1-fy)),
                       (x0,y1,(1-fx)*fy), (x1,y1,fx*fy))
            a = pr = pg = pb = 0.0
            for x,y,w in samples:
                i=x*4
                r,g,b,aa=rows[y][i:i+4]
                af=aa/255.0
                a += aa*w
                pr += r*af*w; pg += g*af*w; pb += b*af*w
            alpha=max(0,min(255,int(round(a))))
            if alpha > 0:
                af=alpha/255.0
                r=max(0,min(255,int(round(pr/af))))
                g=max(0,min(255,int(round(pg/af))))
                b=max(0,min(255,int(round(pb/af))))
            else: r=g=b=0
            i=ox*4
            row[i:i+4]=bytes((r,g,b,alpha))
        out.append(row)
    # Re-assert the rounded exterior after scaling, so every output size is clean.
    clear_rounded_exterior(size,size,out)
    return out

def argb_words(path: Path):
    width, height, rows = read_rgba(path)
    if width != height: raise IconError("embedded WM icon must be square")
    words=[]
    for row in rows:
        for i in range(0,len(row),4):
            r,g,b,a=row[i:i+4]
            words.append((a<<24)|(r<<16)|(g<<8)|b)
    return width, words

def replace_icon_array(text: str, size: int, words):
    array_name=f"kIcon{size}"
    size_name=f"kIcon{size}Size"
    text,count=re.subn(rf"inline constexpr int {size_name}\s*=\s*\d+\s*;",
                       f"inline constexpr int {size_name} = {size};",text,count=1)
    if count != 1: raise IconError(f"could not find {size_name}")
    lines=[]
    for start in range(0,len(words),8):
        lines.append("    "+", ".join(f"0x{w:08x}u" for w in words[start:start+8])+",")
    replacement=f"inline constexpr std::uint32_t {array_name}[{size*size}] = {{\n"+"\n".join(lines)+"\n};"
    pattern=rf"inline constexpr std::uint32_t {array_name}\s*\[[^\]]+\]\s*=\s*\{{.*?\n\}};"
    text,count=re.subn(pattern,replacement,text,count=1,flags=re.S)
    if count != 1: raise IconError(f"could not find {array_name} array")
    return text

def main() -> int:
    project=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
    master=project/'assets/branding/nougat-media-suite-v51-master-N.png'
    if not master.is_file():
        raise IconError(f"approved owner N master missing: {master}")
    sw,sh,rows=read_rgba(master)
    if sw != sh: raise IconError(f"approved owner N master is not square: {sw}x{sh}")
    changed=clear_rounded_exterior(sw,sh,rows)
    assets=project/'assets/icons'
    cleaned_master=assets/'nougat-media-suite-v53-master-clean.png'
    write_rgba(cleaned_master,sw,sh,rows)
    print(f"MASTER: cleared {changed} exterior alpha pixel(s) from approved v51 N master")

    generated={}
    for size in (16,32,48,64,128,256,512):
        scaled=resize_rgba(rows,sw,sh,size)
        destination=assets/f'nougat-media-suite-v53-{size}.png'
        write_rgba(destination,size,size,scaled)
        generated[size]=destination
        print(f"ICON {size}: regenerated from corrected approved master")
    (assets/'nougat-media-suite-v53.png').write_bytes(generated[512].read_bytes())

    header=project/'src/nougat_media_suite_icon_data.hpp'
    text=header.read_text(encoding='utf-8')
    for size in (16,32,64):
        actual,words=argb_words(generated[size])
        if actual != size: raise IconError('embedded icon size mismatch')
        text=replace_icon_array(text,size,words)
    header.write_text(text,encoding='utf-8')
    print('PASS: one corrected approved N master regenerated all icon sizes and embedded WM icon data.')
    return 0

if __name__=='__main__':
    try:
        raise SystemExit(main())
    except IconError as exc:
        print(f'STOP: {exc}',file=sys.stderr)
        raise SystemExit(1)
