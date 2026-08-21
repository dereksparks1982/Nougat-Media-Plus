#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys

root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
ytdlp=(root/'src/ytdlp_stream_server.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
desktop=(root/'NougatMediaSuite.desktop').read_text(encoding='utf-8')

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)

def block(text,start,end):
    a=text.find(start); need(a>=0,'missing '+start)
    b=text.find(end,a+len(start)); need(b>=0,'missing end marker '+end)
    return text[a:b]

need('VERSION 0.0.23' in cmake, 'CMake version is not 0.0.23')
need('add_executable(Nougat_Media_Suite_v23' in cmake, 'v23 executable target missing')
need('Nougat_Media_Suite_v22' not in cmake, 'v22 target leaked into CMake')
need('Nougat Media Suite v0.0.23' in main, 'CLI v0.0.23 marker missing')
need('const std::string versionLabel = "v0.0.23"' in main, 'in-app v0.0.23 marker missing')
need('Nougat_Media_Suite_v23' in desktop, 'desktop launcher does not target v23')

# Owner-approved top order stays exact.
order=['videoPlayerTab','libraryTab','discoverTab','nougatTab','ytdlpTab','debugTab']
positions=[main.find(x+' = {topX') for x in order]
need(all(x>=0 for x in positions) and positions==sorted(positions), 'top-level tab order changed')

# Stream has one shared URL field and no duplicate Play button.
need('Rect ytdlpUrlRect' in main, 'shared Stream URL field missing')
need('ytdlpPlayBtn' not in main, 'redundant Stream Play button still exists')
need('button_on(target,ytdlpDirectWatchBtn,"Direct Watch")' in main, 'Direct Watch button missing')
need('layout_button_row({&ytdlpDownloadBtn,&ytdlpDirectWatchBtn,&ytdlpWebpageBtn,&ytdlpClearBtn}' in main,
     'Stream action row is not the approved Download/Direct Watch/Open Webpage/Clear layout')
for forbidden in ('Vimeo','Dailymotion','Twitch','Kick','TikTok','Bilibili','Niconico'):
    need(forbidden not in block(main,'enum class StreamPlatform','enum class YtDlpJob'), f'unapproved Stream service leaked into runtime enum: {forbidden}')

# Active tab point/notch and wide-window centering are real layout behavior.
top=block(main,'void layout()','void update_video_prompt_layout()')
need('(W - topControlTotalW) / 2' in top, 'top navigation wide-window centering missing')
need('(W - controlTotalW) / 2' in top, 'player-control wide-window centering missing')
bar=block(main,'void draw_top_bar','void update_chapter_marks')
need('XFillPolygon' in bar and 'if (active)' in bar, 'active-tab point/notch drawing missing')
need('pointerWindowX = e.xmotion.x' in main and 'pointerWindowY = e.xmotion.y' in main, 'concept hover-state pointer tracking missing')

# Old red player fills are not allowed in seek or volume.
seek=block(main,'void draw_seek_time_row','void draw_volume_bar')
vol=block(main,'void draw_volume_bar','void draw_controls')
for name,region in [('seek',seek),('volume',vol)]:
    need('companyRed' not in region, f'old red {name} fill still present')
    need('caramel' in region and 'creamTrack' in region, f'concept palette missing from {name}')
need('std::min(280, W / 3)' in top, 'compact volume width cap missing')
need('normalX = volRect.x + volRect.w / 2' in vol, '100% volume marker logic missing')

# All main pages share the quilted material with approved tint branches.
need('void draw_quilted_background' in main, 'quilted page material missing')
for view in ('VideoPlayer','Library','Discover','Stream','Debug'):
    need(f'ViewMode::{view}' in block(main,'void draw_quilted_background','void draw_concept_field'), f'quilt tint missing for {view}')

# Exact N concept icon is the active asset, not the old candy icon.
icon=root/'assets/icons/nougat-media-suite.png'
need(icon.is_file(), 'active Nougat icon missing')
need(hashlib.sha256(icon.read_bytes()).hexdigest()=='c9e8f046938b04054a6840352c6bc1276f67198d691ba46a0ff5a533d9236b25',
     'active icon is not the approved concept-sheet N emblem')
need((root/'src/nougat_media_suite_icon_data.hpp').is_file(), 'embedded X11 icon data missing')

# Current YouTube integration recognizes modern external JS runtimes.
need('--js-runtimes' in ytdlp, 'yt-dlp JavaScript-runtime support missing')
need('find_executable_in_path("deno")' in ytdlp, 'Deno detection missing')
need('find_executable_in_path("node")' in ytdlp, 'Node detection missing')
need('supported_node_runtime_present()' in ytdlp and 'major >= 22' in ytdlp, 'Node 22+ compatibility gate missing')
need('find_executable_in_path("qjs")' in ytdlp, 'QuickJS detection missing')

# Licensing files must stay byte-identical to the already-pushed license-only state.
expected={
 'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
 'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
 'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
 'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
 'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff',
}
for rel,want in expected.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest()
    need(got==want, f'protected licensing file changed in v23: {rel}')

print('v23-contract=pass exact-concept-ui=pass active-tab-point=pass centered-controls=pass nougat-bars=pass compact-volume=pass shared-direct-play=pass direct-watch=pass exact-N-icon=pass license-preserved=pass')
