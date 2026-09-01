#!/usr/bin/env python3
from pathlib import Path
import sys

root=Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
hdhr=(root/'src/live_tv/hdhomerun_provider.cpp').read_text(encoding='utf-8')
world=(root/'src/world_tv/world_tv_service.cpp').read_text(encoding='utf-8')
worker=(root/'components/world_tv/nougat_world_tv_worker.py').read_text(encoding='utf-8')
roadmap=(root/'ROADMAP.md').read_text(encoding='utf-8')
scope=(root/'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_51_SCOPE.md').read_text(encoding='utf-8')
header=(root/'src/nougat_media_suite_icon_data.hpp').read_text(encoding='utf-8')
builder=(root/'tools/build_v51.py').read_text(encoding='utf-8')

assert "kTopBar14Size" not in main, "retired Nougat N header constants remain in main.cpp"
checks={
 'CMake v51': 'VERSION 0.0.51' in cmake and 'Nougat_Media_Suite_v51' in cmake,
 'visible v51 header': 'const std::string versionLabel = "v0.0.51";' in main,
 'diagnostic v51': 'input.app_version = "Nougat Media Suite v0.0.51";' in main,
 'CLI v51': 'printf("Nougat Media Suite v0.0.51\\n");' in main,
 'approved lockup embedded': 'kTopBarLockup' in header and 'draw_suite_brand' in main,
 'new N wm icon arrays': 'kIcon16' in header and 'kIcon32' in header and 'kIcon64' in header,
 'canonical GNOME application identity': '_GTK_APPLICATION_ID' in main and '_BAMF_DESKTOP_FILE' in main and 'com.elderredsoftworks.NougatMediaSuite' in main,
 'Radio top-level': 'ViewMode::Radio' in main and 'draw_tab(radioTab,"Radio",ViewMode::Radio)' in main,
 'Radio root hides video child': 'currentView == ViewMode::Radio || currentView == ViewMode::Nougat' in main,
 'dynamic top navigation source': 'Rect* const topTabs[]' in main and 'sizeof(topTabs) / sizeof(topTabs[0])' in main,
 'geometry-derived top scroll': 'int top_navigation_max_scroll() const' in main and 'debugTab.x + debugTab.w + topNavScrollX' in main,
 'multi-width nav regression': 'for (const int width : {640, 828, 1000, 1280})' in main,
 'no hard-coded top scroll cardinality': 'scroll_button_row(topNavScrollX,' not in main and 'clamp_button_scroll(100000,12,app.topNavViewportW)' not in main,
 'Radio Mulberry': 'r=112; g=38;  b=88' in main,
 'World TV resolver stays off UI thread': 'worldTvResolveWorker=std::thread' in main and 'service.resolve(station.channel_id' in main,
 'World TV shutdown does not wait on network probes': 'worldTvResolveWorker.detach()' in main and 'worldTvGuideWorker.detach()' in main and 'worldTvArtworkWorker.detach()' in main,
 'World TV guide': 'WORLD TV GUIDE' in main and 'Program guide unavailable' in main,
 'World TV timeout': 'seconds(70)' in world,
 'World TV v51 UA': 'NougatMediaSuite/0.0.51' in worker,
 'World TV evidence': 'LAST_PROBE_REASON' in worker,
 'HDHR scan phase truth': 'rf_traversal_complete' in hdhr,
 'HDHR physical grouping': 'renderedHdhrDevices' in main,
 'neutral no-tuner status': 'No compatible TV tuner detected.' in main,
 'transient opacity': '_NET_WM_WINDOW_OPACITY' in main,
 'transient rounded mask': 'apply_rounded_transient_shape' in main,
 'LAN viewer endpoint foundation': '/nougat/v1/live-tv' in (root/'src/lan/lan_media_service.cpp').read_text(encoding='utf-8'),
 'controller System roadmap': 'Unified Controller & Remote Input Framework' in roadmap and 'System tab' in roadmap and 'Drone Flight' in roadmap,
 'scope documentation': 'v0.0.51' in scope,
 'singular Archive root only': "correct=dklab/'Archive'" in builder and "archive_root=archive_base/'Nougat Media Suite'" in builder and "DKLab/Archives/Nougat Media Suite" not in builder,
 'wrong Archives migration': 'migrate_wrong_archives_folder()' in builder and 'merge_legacy_archive_tree' in builder and 'wrong.rmdir()' in builder,
 'deleted v50 accepted as rejected-v51 state': "known=set(candidate_changed_paths()) | {PREVIOUS}" in builder and "git','restore','--source',BASE,'--worktree','--',PREVIOUS" not in builder,
 'v50 never required in root before compile': "git','show',f'{BASE}:{PREVIOUS}'" in builder and "accepted v0.0.50 executable is missing" not in builder,
}
failed=[]
for name,ok in checks.items():
    print(('PASS' if ok else 'FAIL')+': '+name)
    if not ok: failed.append(name)
if failed:
    raise SystemExit(1)
