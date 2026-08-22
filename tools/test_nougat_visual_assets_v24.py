#!/usr/bin/env python3
from __future__ import annotations
import hashlib, pathlib, re, struct, sys, zlib

root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]

def need(ok,msg):
    if not ok:
        raise SystemExit('FAIL: '+msg)

def sha(p:pathlib.Path):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def png_rgba(path:pathlib.Path):
    data=path.read_bytes()
    need(data.startswith(b'\x89PNG\r\n\x1a\n'), f'not PNG: {path}')
    pos=8; ihdr=None; raw=b''
    while pos+12<=len(data):
        n=struct.unpack('>I',data[pos:pos+4])[0]
        typ=data[pos+4:pos+8]; body=data[pos+8:pos+8+n]
        pos += 12+n
        if typ==b'IHDR': ihdr=body
        elif typ==b'IDAT': raw += body
        elif typ==b'IEND': break
    need(ihdr is not None and len(ihdr)==13, f'IHDR missing: {path}')
    w,h,depth,color,comp,filt,interlace=struct.unpack('>IIBBBBB',ihdr)
    need(depth==8 and color==6 and comp==0 and filt==0 and interlace==0,
         f'expected noninterlaced RGBA8 PNG: {path}')
    bpp=4; scan=zlib.decompress(raw); stride=w*bpp
    need(len(scan)==h*(stride+1), f'bad scan length: {path}')
    rows=[]; prev=bytearray(stride); off=0
    for _ in range(h):
        ft=scan[off]; off+=1
        cur=bytearray(scan[off:off+stride]); off+=stride
        for i in range(stride):
            left=cur[i-bpp] if i>=bpp else 0
            up=prev[i]
            ul=prev[i-bpp] if i>=bpp else 0
            if ft==1: cur[i]=(cur[i]+left)&255
            elif ft==2: cur[i]=(cur[i]+up)&255
            elif ft==3: cur[i]=(cur[i]+((left+up)//2))&255
            elif ft==4:
                p=left+up-ul; pa=abs(p-left); pb=abs(p-up); pc=abs(p-ul)
                pr=left if pa<=pb and pa<=pc else (up if pb<=pc else ul)
                cur[i]=(cur[i]+pr)&255
            elif ft!=0:
                raise SystemExit(f'FAIL: unsupported PNG filter {ft}: {path}')
        rows.append(bytes(cur)); prev=cur
    return w,h,rows

expected = {
    'nougat-media-suite-14.png':'0225c7edfa314ef115236832e0b60ef03572137322a4a428dd99ef875f7c9bfd',
    'nougat-media-suite-16.png':'c07c3788d7c6c64ef8defaf0e76cb763342088f5c3edc5f7fdaab7e830f4a275',
    'nougat-media-suite-32.png':'d6f5b5d904906bb40bb75826311ad75fe03dd22ffa7a3cba681b5916637bf5fc',
    'nougat-media-suite-48.png':'81814a957a68de71727bc603f1e1b15f2bb43349355e047aad42158ef7adbd6f',
    'nougat-media-suite-64.png':'da727abe9d47d04566ca6df1beb8b061a1baa07faf0dc1a059980f658c14821d',
    'nougat-media-suite-128.png':'ba44f81c668d7ab83a32ea7d6e1d719dcbc2fd1c695ab6ea72d42c4c228a8b81',
    'nougat-media-suite-256.png':'081fa0a3dabb93dc6f9c94fddea2f78ce3b565a1f15501c58d8e2576badaeecc',
    'nougat-media-suite-512.png':'5d0239c7999a091bb4b60384b2953444a8e40a7644ca6e18dddac1cb69b00e66',
    'nougat-media-suite.png':'5d0239c7999a091bb4b60384b2953444a8e40a7644ca6e18dddac1cb69b00e66',
    'nougat-media-suite-concept-sheet-v24.png':'5d0239c7999a091bb4b60384b2953444a8e40a7644ca6e18dddac1cb69b00e66',
}
for name,hx in expected.items():
    p=root/'assets/icons'/name
    need(p.is_file(), f'missing exact concept-sheet icon asset: {name}')
    need(sha(p)==hx, f'exact concept-sheet icon hash mismatch: {name}')

master=root/'assets/icons/nougat-media-suite-concept-sheet-v24.png'
w,h,rows=png_rgba(master)
need((w,h)==(512,512),'exact concept-sheet master must be 512x512')
need(rows[0][3]==0 and rows[0][(w-1)*4+3]==0 and rows[-1][3]==0 and rows[-1][(w-1)*4+3]==0,
     'exact concept-sheet master corners are not transparent')
for y,row in enumerate(rows):
    for x in range(w):
        r,g,b,a=row[x*4:x*4+4]
        if a>0 and r>225 and g>215 and b>195 and max(r,g,b)-min(r,g,b)<45:
            nas=[]
            for nx,ny in ((x-1,y),(x+1,y),(x,y-1),(x,y+1)):
                if 0<=nx<w and 0<=ny<h:
                    nas.append(rows[ny][nx*4+3])
            need(not any(v==0 for v in nas),'white/cream concept-sheet background halo touches icon silhouette')

header=(root/'src/nougat_media_suite_icon_data.hpp').read_text(encoding='utf-8')
need(sha(root/'src/nougat_media_suite_icon_data.hpp')=='0fa96e1b4d79369732eedd4d8da472a8e5f72f99b44ace3885fa724834907809',
     'embedded icon-data header is not the exact concept-sheet replacement')
for cname,size,filename in (
    ('kTopBar14',14,'nougat-media-suite-14.png'),
    ('kIcon16',16,'nougat-media-suite-16.png'),
    ('kIcon32',32,'nougat-media-suite-32.png'),
    ('kIcon64',64,'nougat-media-suite-64.png'),
):
    m=re.search(rf'{cname}\[{size*size}\]\s*=\s*\{{(.*?)\}};',header,re.S)
    need(m is not None,f'embedded array missing: {cname}')
    vals=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{8})u',m.group(1))]
    need(len(vals)==size*size,f'embedded array length wrong: {cname}')
    pw,ph,prows=png_rgba(root/'assets/icons'/filename)
    need((pw,ph)==(size,size),f'PNG size wrong for {filename}')
    expected_vals=[]
    for prow in prows:
        for x in range(size):
            r,g,b,a=prow[x*4:x*4+4]
            expected_vals.append((a<<24)|(r<<16)|(g<<8)|b)
    need(vals==expected_vals,f'embedded {cname} does not exactly match {filename}')

main=(root/'src/main.cpp').read_text(encoding='utf-8')
need('draw_suite_badge(target, 8, 5' in main,'far-left in-app N badge is not drawn from embedded exact icon data')
need('append_net_wm_icon(data, nougat_media_suite_icon::kIcon16Size' in main,'X11 icon 16 not sourced from embedded exact icon')
need('append_net_wm_icon(data, nougat_media_suite_icon::kIcon32Size' in main,'X11 icon 32 not sourced from embedded exact icon')
need('append_net_wm_icon(data, nougat_media_suite_icon::kIcon64Size' in main,'X11 icon 64 not sourced from embedded exact icon')

icon_key='nougat-media-suite-concept-sheet-v24'
desktop_files=[
    'NougatMediaSuite.desktop',
    'NougatMediaSuite_v22.desktop',
    'NougatMediaSuite_v23.desktop',
    'NougatMediaSuite_v24.desktop',
    'com.elderredsoftworks.NougatMediaSuite.desktop',
]
for name in desktop_files:
    p=root/name
    need(p.is_file(),f'missing project launcher: {name}')
    s=p.read_text(encoding='utf-8')
    need(f'Icon={icon_key}' in s,f'project launcher still points to old/blurry icon key: {name}')
    need('StartupWMClass=NougatMediaSuite' in s,f'WM_CLASS identity missing: {name}')
    need('X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite' in s,f'GNOME app ID missing: {name}')

old_hashes={
'01a0f7a0b9e1502407648b7fe1bf7415f623fda145599a7ced6fd668b07b361c',
'0b58d64fbc6111e1d9a51d1482476a8af989af0e2caf8b96f0b184d204d94ca4',
'0cd2f6f96e4088e9822f5eefaee5edc25279efa8ae0f7a7984b5be602dc24500',
'13c3b9012d0101a98a3b5fe6d46229ba9d507b451ed0bf4dfb82a47fd1b14c1e',
'18b708407fa466491599ff5abdd10de3d0c66663f8ac5fd8064ee6972bdb1f07',
'28dbb8ddf31b12a82bf2bbe5e2b68a95548ba3f0a5168f8d4e639eefe23844e6',
'2cd34df7d01f416ff9144d80b7fd7873f83a66f00d98bc61efd71da00a6481f4',
'490098461933563fb12ccd2e6bfd8e074cb1764497c59d20a2adffde078b2cc0',
'5bf0a18afb03e992159eddd18202b02ae93c92a153e0fbde3e4bedc632a39e5a',
'5e805aafb3d3add42c9167b5c3c27b6cf817ab0d4be1472d79a27c0a43188a50',
'619b48d25cc68d6ffdaa14b85dd7f14f6e9578cf2ab4021fb684601fc556f5f5',
'673e14c90142a0fb326d0e4db0e8ba8000f8c4e78af0f947d502151bd869ff72',
'734e8e0cba4b176bcbb64e1ffecfba52a5f148ecc1422d35bcd04574380f8f29',
'813469cfaa67b2109589c85b48cc3beac248e31d778f568d0d49dce06dd1c09d',
'842a9e27c971f389e10867fc056aa4a22aec5b69bfe1face9ec201a76608104d',
'890031809ab9ec3436ba2b8f2572b36303f7aefa19f59903831ab2f851c3cf76',
'8a8e67092bf57d1fb4ba0905192e47b3dd794b0644f296cff0509d4d4498bd2b',
'962706702d9f3c7cdea032842f9bd4a62f08bb95e055b234081a90b98ec8c21a',
'9a3f456e8c5644adfc0f47acaff515ac0cdb93a06de2e498d44b4a717e70c2d2',
'9e5ad06df05460e99dcb294d73bc3a62626952f1a241299c3c5c3198db1cb987',
'a02991e0fce4e137a6de31b05d44ae32d52d15ec63b2435995602745eb17203c',
'b6c0210e99eab69feb2c24e1a2729ad2d35b7d763f1a19556bfc1dc0a3be7ce2',
'c18b6de5d60375dd96f0bb9964f8e0cf88b0f62460c50afc87d5bcb1006aef7c',
'c38af0c36eb4f97f1463c8287265b2d7ffc44f5b16d6a696dbcffcbe04684f75',
'c6ba28d75bee5dd340b8d6d62586af4db7a040af8594d86cebd7ade970f31e82',
'd2260a3d929a223723af577502bf8382f391d687c61e48820a016fb98b749516',
'd32f278090422352b86a91c7f0c5540431f6ff8628b9415298f359a81780b870',
'd91860c1d2c89ba3a17f6151d6a11e6954403a4c3647e51426147de2d881235b',
'dbdda26d3770c02d2b4b3ec2e29ef1a5fbdcbc91619cacb7aaa6044820478e52',
'dffc74a8200434f2de978122f12fe49747ef12005ece687dba86104d753a69eb',
'eac0ac78fec25497b35912185f7f6982020e9fb163f1c130b11f5aa4f2e29dfb',
'f1066437345f3ec67d8585050adaf6b67f68c5053a6c67671a46b931efe8f82d',
'f548b9520b88ac78c76e106cfcbb08ab9a88eee7a5c9a6018c19d50cb4aab71e',
'fdd45e2550cb915cb27355d0bb3afcb27e87fee7bf64aa86f6b114c7a9761f14',
}
for p in (root/'assets/icons').glob('nougat-media-suite*.png'):
    need(sha(p) not in old_hashes,f'rejected old/blurry icon asset remains in project: {p.name}')

quilt=root/'assets/ui/nougat-quilt-source.png'
need(quilt.is_file(),'concept-sheet quilt source missing')
need(sha(quilt)=='eea284cc42f48ea2184ff3ccf8c717c9b43bad10727efe4bbdeaa8c2c025ba21',
     'background/quilt changed during icon-only repair')
print('exact-concept-sheet-N=pass in-app-header-exact=pass x11-window-icon-exact=pass launcher-family-exact=pass transparent-corners=pass no-white-halo=pass rejected-blurry-family-absent=pass quilt-preserved=pass')
