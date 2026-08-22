#!/usr/bin/env python3
from pathlib import Path
import hashlib, re, sys

root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
desktop=(root/'NougatMediaSuite.desktop').read_text(encoding='utf-8')
legacy_v22_desktop=(root/'NougatMediaSuite_v22.desktop').read_text(encoding='utf-8')
legacy_v23_desktop=(root/'NougatMediaSuite_v23.desktop').read_text(encoding='utf-8')
versioned_desktop=(root/'NougatMediaSuite_v24.desktop').read_text(encoding='utf-8')
canonical_desktop=(root/'com.elderredsoftworks.NougatMediaSuite.desktop').read_text(encoding='utf-8')

def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)

def block(text,start,end):
    a=text.find(start); need(a>=0,'missing '+start)
    b=text.find(end,a+len(start)); need(b>=0,'missing end marker '+end)
    return text[a:b]

need('VERSION 0.0.24' in cmake, 'CMake version is not 0.0.24')
need('add_executable(Nougat_Media_Suite_v24' in cmake, 'v24 executable target missing')
need('Nougat_Media_Suite_v23' not in cmake, 'v23 target leaked into CMake')
need('Nougat Media Suite v0.0.24' in main, 'CLI v0.0.24 marker missing')
need('const std::string versionLabel = "v0.0.24"' in main, 'in-app v0.0.24 marker missing')
need('Nougat_Media_Suite_v24' in desktop, 'desktop launcher does not target v24')

# Header identity is now exactly left-brand + centered nav + right status/version.
bar=block(main,'void draw_top_bar','void update_chapter_marks')
need('draw_suite_badge(target, 8, 5' in bar, 'approved N emblem is not at far-left header')
need('text(target, 28, 17, "NOUGAT MEDIA SUITE"' in bar, 'suite title is not immediately beside left N')
need('badgeX' not in bar, 'duplicate right-side N badge logic still exists')
need('const int serverX = versionX - 34 - text_width(serverLabel);' in bar, 'server/version right-side layout missing')

# Top-level tab order remains exact.
order=['videoPlayerTab','libraryTab','discoverTab','nougatTab','ytdlpTab','debugTab']
positions=[main.find(x+' = {topX') for x in order]
need(all(x>=0 for x in positions) and positions==sorted(positions), 'top-level tab order changed')

# Search/Crawler/P2P use the concept tab family and selected point.
search=block(main,'void draw_nougat_screen','void draw_debug_screen')
need('nougat_tab_button(target,nougatSearchPanelTab,"Search"' in search, 'Search sub-tab concept treatment missing')
need('nougat_tab_button(target,nougatCrawlerPanelTab,"Crawler"' in search, 'Crawler sub-tab concept treatment missing')
need('nougat_tab_button(target,nougatP2PPanelTab,"P2P"' in search, 'P2P sub-tab concept treatment missing')
tabhelper=block(main,'void nougat_tab_button','void draw_nougat_panel')
need('XFillPolygon' in tabhelper and 'if (!active) return;' in tabhelper, 'selected Search sub-tab point/notch missing')

# Network and SEARCH buttons must share the same exact right-column x/width.
layout=block(main,'void layout()','void update_video_prompt_layout()')
need('const int nougatRightColumnX' in layout, 'shared Search right-column anchor missing')
need('nougatNetworkAdvancedBtn = {nougatRightColumnX,54,kCompactButtonW,kCompactButtonH};' in layout,
     'Network button is not on shared right column')
need('nougatSearchBtn = {nougatRightColumnX,104,kCompactButtonW,kCompactButtonH};' in layout,
     'SEARCH button is not on shared right column')

# Search fields/panels no longer use the legacy flat slab treatment.
helper=block(main,'void nougat_button','std::string& focused_nougat_text')
need('fill_round' in helper and 'outline_round' in helper, 'concept rounded Search controls missing')
need('draw_concept_field' in helper, 'concept Search input field missing')
need('void draw_nougat_panel' in helper, 'Nougat panel helper missing')
need('draw_nougat_panel(target,nougatResultsBox);' in search, 'Search results panel not converted')
need('fill(target,nougatResultsBox,nougat_chocolate())' not in search, 'legacy dark Search results slab still present')
need('draw_nougat_panel(target,nougatCrawlLogBox);' in search, 'Crawler log panel not converted')
need('draw_nougat_panel(target,nougatPeerListBox);' in search, 'Network peer panel not converted')
need('searchPalette.text' in search, 'Search status/help contrast treatment missing')


# Crawler completion/status text must sit above the crawler panel border, not under it.
m=re.search(r'nougatCrawlLogBox = \{28,\s*(\d+),', layout)
need(m is not None, 'crawler log panel geometry missing')
panel_y=int(m.group(1))
m2=re.search(r'text\(target,28,(\d+),status,searchPalette\.text\);\n\s*\}', search)
need(m2 is not None, 'crawler status baseline missing')
status_y=int(m2.group(1))
need(panel_y-status_y >= 12, 'crawler status/panel spacing is too tight; border can cross status text')
need('if (selected) fill_round(target,{nougatCrawlLogBox.x+6,y-14' in search, 'crawler selection highlight behavior changed unexpectedly')

# Embedded P2P stays inside Search's visual family without changing P2P behavior.
p2p=block(main,'void draw_p2p_screen','std::string server_control_label')
need('embeddedInSearch' in p2p, 'embedded P2P Search-family detection missing')
need('draw_quilted_background(target, {0,86,W,H-86}, ViewMode::Nougat)' in p2p, 'embedded P2P Search quilt missing')
need('draw_concept_field' in p2p and 'draw_nougat_panel(target,fileBox)' in p2p, 'embedded P2P concept surfaces missing')

# GNOME/X11 identity must resolve to approved N instead of generic gear fallback.
identity=block(main,'void set_window_identity','void set_window_title')
need('_GTK_APPLICATION_ID' in identity and 'const char* appId = "com.elderredsoftworks.NougatMediaSuite"' in identity,
     'canonical GNOME application-ID hint missing')
need('_BAMF_DESKTOP_FILE' in identity and 'com.elderredsoftworks.NougatMediaSuite.desktop' in identity,
     'Ubuntu dock desktop-file identity hint missing')
need('StartupWMClass=NougatMediaSuite' in desktop, 'desktop StartupWMClass mismatch')
need('Icon=nougat-media-suite-concept-sheet-v24' in desktop, 'desktop does not use fresh exact-N icon-theme key')
need('X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite' in desktop, 'desktop canonical GNOME application ID missing')
for name,ds in [('unversioned',desktop),('v22',legacy_v22_desktop),('v23',legacy_v23_desktop),('v24',versioned_desktop),('canonical',canonical_desktop)]:
    need('Icon=nougat-media-suite-concept-sheet-v24' in ds, f'{name} launcher still uses old icon key')
    need('StartupWMClass=NougatMediaSuite' in ds, f'{name} launcher StartupWMClass mismatch')
    need('X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite' in ds, f'{name} launcher GNOME app ID mismatch')
icon=root/'assets/icons/nougat-media-suite.png'
need(icon.is_file(), 'approved N icon asset missing')
need(hashlib.sha256(icon.read_bytes()).hexdigest()=='5d0239c7999a091bb4b60384b2953444a8e40a7644ca6e18dddac1cb69b00e66',
     'active icon is not the literal full-resolution concept-sheet N replacement')



old_blurry_hashes={
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
for asset in (root/'assets/icons').glob('nougat-media-suite*.png'):
    need(hashlib.sha256(asset.read_bytes()).hexdigest() not in old_blurry_hashes, f'old rejected blurry N asset still active: {asset.name}')
need(hashlib.sha256((root/'src/nougat_media_suite_icon_data.hpp').read_bytes()).hexdigest()=='0fa96e1b4d79369732eedd4d8da472a8e5f72f99b44ace3885fa724834907809',
     'embedded application/window/header icon data is not the literal concept-sheet replacement')

# Rejected-v24 master-art repair: exact N comes from the full-resolution concept sheet
# with transparent outer canvas, and the page material is the exact quilt crop from
# the same sheet rather than the procedural diamond approximation.
quilt=root/'assets/ui/nougat-quilt-source.png'
need(quilt.is_file(), 'exact concept-sheet quilt source asset missing')
need(hashlib.sha256(quilt.read_bytes()).hexdigest()=='eea284cc42f48ea2184ff3ccf8c717c9b43bad10727efe4bbdeaa8c2c025ba21',
     'quilt source does not match the approved full-resolution concept-sheet crop')
need((root/'src/nougat_quilt_texture_data.hpp').is_file(), 'embedded quilt texture data missing')
need('#include "nougat_quilt_texture_data.hpp"' in main, 'main UI does not use embedded exact quilt source')
quilt_block=block(main,'static int quilt_view_index','void draw_concept_field')
need('XSetFillStyle(d, gc, FillTiled)' in quilt_block and 'XSetTile(d, gc, tile)' in quilt_block,
     'exact quilt source is not rendered as the page material')
need('constexpr int step = 34' not in quilt_block, 'legacy procedural diagonal quilt approximation still active')
for marker in [
    'case ViewMode::VideoPlayer: r=196; g=142; b=84;  blendPercent=18;',
    'case ViewMode::Library:     r=143; g=170; b=119; blendPercent=18;',
    'case ViewMode::Discover:    r=185; g=132; b=199; blendPercent=16;',
    'case ViewMode::Nougat:      r=208; g=161; b=102; blendPercent=5;',
    'case ViewMode::Stream:      r=105; g=160; b=192; blendPercent=17;',
    'case ViewMode::Debug:       r=120; g=110; b=102; blendPercent=20;',
]:
    need(marker in quilt_block, 'per-tab quilt tint contract missing: '+marker)

# The outer icon canvas must be transparent, never a white/cream square. The
# exact generated hash above pins every byte; embedded top-bar/X11 sizes must
# also carry transparent pixels from that same source.
icon_header=(root/'src/nougat_media_suite_icon_data.hpp').read_text(encoding='utf-8')
need('0x00000000u' in icon_header, 'embedded N icon has no transparent pixels')
need(icon_header.count('0x00000000u') >= 8, 'embedded N icon transparency is unexpectedly sparse')

# Search engine code is deliberately untouched in this visual-only release.
engine_expected={
 'components/nougat/nougat_engine.py':'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993',
 'src/nougat/nougat_bridge.cpp':'15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944',
 'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e',
}
for rel,want in engine_expected.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest()
    need(got==want, f'Search engine behavior file changed in UI-only v24: {rel}')

# Licensing files stay byte-identical to the already-pushed protected state.
license_expected={
 'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
 'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
 'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
 'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
 'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff',
}
for rel,want in license_expected.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest()
    need(got==want, f'protected licensing file changed in v24: {rel}')


# Same-version playback/performance repair: TV autoplay must recover across
# season routes and transient local-player startup failures, while rich concept
# hover rendering must not repaint the whole app for every raw X11 motion event.
need('bool open_media(const std::string& path, long long seek=0)' in main,
     'local media open path does not report success/failure')
need('const int play_result = api.play(mp);' in main and 'if (play_result == 0)' in main,
     'local media playback start result is not validated')
need('if (!have_series && !selected.series_id.empty())' in main,
     'TV autoplay cannot reconstruct a missing Series parent from Jellyfin SeriesId')
need('bool start_tv_autoplay_index(int index)' in main and 'poll_tv_autoplay_retry();' in main,
     'TV next-episode retry path missing')
need('tvAutoplayRetryAttempts >= 3' in main and 'tvAutoplayRetryAtMs = now_ms() + 750' in main,
     'bounded TV autoplay retry policy missing')
need('state == 5 && tvAutoplayArmed' in main and 'lastLocalPlaybackPositionMs' in main,
     'natural EOF Stopped-state fallback missing')
need('pointerFullRedrawPending = true;' in main and
     'pointer_now - lastPointerFullRedrawMs >= 50' in main and
     'pointerFullRedrawPending && !fullscreen' in main,
     'full-window pointer-motion redraw throttle missing')
need('if (moved && !fullscreen) redraw();' not in main,
     'legacy full redraw on every raw pointer motion packet still present')

# v23 Stream/playback contract remains present.
need('ytdlpPlayBtn' not in main, 'redundant Stream Play button regressed')
need('button_on(target,ytdlpDirectWatchBtn,"Direct Watch")' in main, 'Direct Watch regressed')
need('std::min(280, W / 3)' in layout, 'compact volume geometry regressed')

print('v24-contract=pass search-ui=pass crawler-spacing=pass crawler-selection-preserved=pass left-brand=pass subtab-notch=pass aligned-actions=pass concept-panels=pass embedded-p2p-theme=pass canonical-dock-identity=pass exact-master-N=pass transparent-icon-canvas=pass exact-master-quilt=pass per-tab-quilt-tints=pass tv-autoplay-repair=pass pointer-redraw-throttle=pass search-engine-unchanged=pass license-preserved=pass retained-v23-ui=pass')
