#!/usr/bin/env python3
from __future__ import annotations
import hashlib, pathlib, subprocess, sys
root=pathlib.Path(sys.argv[1]).resolve() if len(sys.argv)>1 else pathlib.Path(__file__).resolve().parents[1]
exe=pathlib.Path(sys.argv[2]).resolve() if len(sys.argv)>2 else None
main=(root/'src/main.cpp').read_text(encoding='utf-8')
cache_h=(root/'src/media_server/library_metadata_cache.hpp').read_text(encoding='utf-8')
cache_cpp=(root/'src/media_server/library_metadata_cache.cpp').read_text(encoding='utf-8')
def need(ok,msg):
    if not ok: raise SystemExit('FAIL: '+msg)
def sha(rel): return hashlib.sha256((root/rel).read_bytes()).hexdigest()
for token in [
    'home_card_uses_portrait_poster','card_width * 3 / 2','card_width * 9 / 16',
    'const int inner_height = std::max(1, libraryListBox.h - 12);','grid.rows >= 2',
    'LibraryMetadataCache','libraryMetadataCache->load','metadata_cache->store',
    'Cached metadata ready. Checking for library changes...','Scanning Jellyfin library for changes...',
    'static_cast<double>(completed) / static_cast<double>(total)','Refreshing server status (library metadata unchanged)...',
    'button_on(target, previousBtn, "Previous")','button_on(target, nextBtn, "Next")',
    'episode_navigation_available(-1)','episode_navigation_available(1)','play_relative_episode(-1)','play_relative_episode(1)',
    'homeContinueArea.contains(x,y)','homeContinueScrollX = std::max(0, homeContinueScrollX +',
    'std::to_string(percent) + "%"','progress_determinate',
]: need(token in main,'retained v30 behavior missing: '+token)
for token in ['NOUGAT_LIBRARY_CACHE_V1','std::rename(temp_path.c_str(), final_path.c_str())','chmod(final_path.c_str(), 0600)']:
    need(token in cache_cpp,'v30 cache persistence missing: '+token)
need('class LibraryMetadataCache' in cache_h,'v30 cache header contract missing')
expected={'LICENSE': '640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84', 'COPYRIGHT.md': 'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31', 'CONTRIBUTING.md': '7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb', 'THIRD_PARTY_NOTICES.md': '9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f', 'docs/LICENSING_POLICY.md': 'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff', 'components/nougat/nougat_engine.py': 'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993', 'src/nougat/nougat_bridge.cpp': '15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944', 'src/nougat/nougat_bridge.hpp': '46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e', 'src/p2p_engine.cpp': '1ad8dec1a454f5809c9afb1647b51d461110d01ce8647cfdabeeb57e9b3137a5', 'src/p2p_engine.hpp': '3d21670ffb49616c011d008efe292340ee1e6e55a004260d193c3e864c2a283f', 'src/p2p_stream_server.cpp': '110a9d7dd5036f8adcc45def2f1853c51a1ec928cca88a6132de6d5c205c4a29', 'src/p2p_stream_server.hpp': '68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7'}
for rel,h in expected.items(): need(sha(rel)==h,'protected file changed: '+rel)
if exe is not None:
    need(subprocess.check_output([str(exe),'--version'],text=True).strip()=='Nougat Media Suite v0.0.31','v31 executable version mismatch')
    subprocess.check_call([str(exe),'--v30-ui-library-player-self-test'])
print('retained-v30=pass library-cache=pass real-progress=pass previous-next=pass home-wheel-contract=pass protected-boundaries=pass')
