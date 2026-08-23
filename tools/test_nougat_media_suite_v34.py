#!/usr/bin/env python3
import pathlib,re,subprocess,sys
root=pathlib.Path(sys.argv[1]).resolve()
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None

def need(cond,msg):
    if not cond:
        raise SystemExit('FAIL: '+msg)

cm=(root/'CMakeLists.txt').read_text(errors='replace')
main=(root/'src/main.cpp').read_text(errors='replace')
types=(root/'src/recommendations/recommendation_types.hpp').read_text(errors='replace')
readme=(root/'README.md').read_text(errors='replace')

need('VERSION 0.0.34' in cm and 'Nougat_Media_Suite_v34' in cm,'v0.0.34 CMake identity missing')
need('kTopTabW = 106' in main and 'kTopTabGap = 3' in main and 'kTopTabH = 40' in main,'actual-sheet top-tab geometry missing')
need('draw_top_nav_tab_surface' in main and 'outline_round_dashed(target, seam, 2' in main,'actual-sheet tab construction missing')
need('top_nav_left_bound()' in main and 'W - topStatusReserve' in main,'left-shifted nav/right fixed status contract missing')
need('draw_sheet_volume_housing' in main and 'draw_speaker_icon(target, volumeHousingRect.x + 15' in main,'actual-sheet volume housing/speaker controls missing')
need('seekRect = {seekX, seekY, std::max(220, W-seekX-seekRightPad), 16}' in main,'actual-sheet seek track geometry missing')
need('enum class RecommendationSource { Local, External, LiveTV };' in types,'Discover Live TV source missing')
need('"TMDb Movie"' in main and '"TMDb TV"' in main and '"Live TV"' in main,'Discover source labels missing')
need('home_card_height(bool continue_card' in main and 'card_width * 9 / 16' in main and 'card_width * 3 / 2' in main,'fixed Home section card geometry missing')
need('XCheckTypedWindowEvent' in main and 'Button1Mask' in main,'direct scrollbar drag/coalescing repair missing')
for name in ('Home','Library','Discover','LiveTV','Stream','Debug'):
    need(f'ViewMode::{name}' in main,'affected page frame contract missing '+name)
need('!app.page_uses_connected_square_frame(ViewMode::Nougat)' in main and '!app.page_uses_connected_square_frame(ViewMode::VideoPlayer)' in main,'Search/Video Player frame preservation test missing')
need('actual-sheet top tabs/seek/volume' in main,'v34 self-test wording missing')
need('TMDb Movie' in readme and 'exact concept-sheet' in readme.lower(),'v34 docs missing')
for rel in ('NougatMediaSuite.desktop','NougatMediaSuite_v34.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
    text=(root/rel).read_text(errors='replace')
    need('Nougat_Media_Suite_v34' in text,f'v34 launcher identity missing: {rel}')

if exe:
    need(exe.is_file(),'v34 executable missing')
    out=subprocess.check_output([str(exe),'--version'],text=True,stderr=subprocess.STDOUT).strip()
    need(out=='Nougat Media Suite v0.0.34',f'v34 version mismatch: {out!r}')
    subprocess.run([str(exe),'--v34-ui-polish-self-test'],check=True)

print('v34-contract=pass exact-sheet-tabs=pass exact-sheet-seek=pass exact-sheet-volume=pass nav-left=pass nav-right-preserved=pass home-fixed-cards=pass direct-scroll=pass live-tv-discover=pass tmdb-names=pass page-corners=pass')
