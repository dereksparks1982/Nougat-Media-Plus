#!/usr/bin/env bash

# Nougat Media Suite v0.0.30 changed-files installer.
# Approved scope: UI cohesion, cache-first Library performance/progress, multi-row poster geometry,
# native Previous/Next episode navigation, and volume/control proportion polish.
# Focused P2P streaming expansion remains v0.0.31.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v30.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_30_UI_LIBRARY_PLAYER_POLISH_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_30-$stamp-$$"
expected_head="91ced5349a71de34cbc7d3d95d32c4de56645c7b"
canonical_id="com.elderredsoftworks.NougatMediaSuite"
icon_key="nougat-media-suite-concept-sheet-v24"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
pinned_model="$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_bytes="84106624"
pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
canonical_launcher="$HOME/.local/share/applications/${canonical_id}.desktop"
launcher_unversioned="$HOME/.local/share/applications/NougatMediaSuite.desktop"
launcher_v22="$HOME/.local/share/applications/NougatMediaSuite_v22.desktop"
launcher_v23="$HOME/.local/share/applications/NougatMediaSuite_v23.desktop"
launcher_v24="$HOME/.local/share/applications/NougatMediaSuite_v24.desktop"
launcher_v25="$HOME/.local/share/applications/NougatMediaSuite_v25.desktop"
launcher_v26="$HOME/.local/share/applications/NougatMediaSuite_v26.desktop"
launcher_v27="$HOME/.local/share/applications/NougatMediaSuite_v27.desktop"
launcher_v28="$HOME/.local/share/applications/NougatMediaSuite_v28.desktop"
launcher_v29="$HOME/.local/share/applications/NougatMediaSuite_v29.desktop"
launcher_v30="$HOME/.local/share/applications/NougatMediaSuite_v30.desktop"
applied=0

modified_paths=(
  "APPLY_COMMAND.txt"
  "CHANGELOG.md"
  "CMakeLists.txt"
  "README.md"
  "ROADMAP.md"
  "NougatMediaSuite.desktop"
  "NougatMediaSuite_v22.desktop"
  "NougatMediaSuite_v23.desktop"
  "NougatMediaSuite_v24.desktop"
  "NougatMediaSuite_v25.desktop"
  "NougatMediaSuite_v26.desktop"
  "NougatMediaSuite_v27.desktop"
  "NougatMediaSuite_v28.desktop"
  "NougatMediaSuite_v29.desktop"
  "NougatMediaSuite_v30.desktop"
  "com.elderredsoftworks.NougatMediaSuite.desktop"
  "src/main.cpp"
  "src/media_server/library_metadata_cache.hpp"
  "src/media_server/library_metadata_cache.cpp"
  "tools/test_nougat_media_suite_retained_v29.py"
  "tools/test_nougat_media_suite_v30.py"
  "tools/test_nougat_media_suite_ui_smoke_v30.py"
  "tools/test_installer_rollback_v30.py"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_30_UI_LIBRARY_PLAYER_POLISH_HANDSHAKE.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_30_UI_LIBRARY_PLAYER_POLISH_VALIDATION.md"
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_30.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v30.json"
)

icon_alias_keys=(
  "nougat-media-suite-concept-sheet-v24"
  "nougat-media-suite-exact-n"
  "nougat-media-suite"
  "com.elderredsoftworks.NougatMediaSuite"
  "NougatMediaSuite"
)

phase_start() { printf '\nPHASE START: %s\n' "$1"; }
phase_pass() { printf 'PHASE PASS: %s\n' "$1"; }

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf 'FAIL: required command not found: %s\n' "$1"
    return 1
  fi
  return 0
}

verify_port_8096_free() {
  python3 - <<'PY'
import socket
s=socket.socket(); s.settimeout(.25)
used=s.connect_ex(('127.0.0.1',8096))==0
s.close()
if used:
    print('FAIL: localhost port 8096 is in use. Close Nougat Media Suite/Jellyfin before applying v0.0.30.')
    raise SystemExit(1)
print('Port 8096 is free.')
PY
}

verify_file_exact() {
  python3 - "$1" "$2" "$3" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); size=int(sys.argv[2]); expected=sys.argv[3]
if not p.is_file() or p.stat().st_size != size: raise SystemExit(1)
h=hashlib.sha256()
with p.open('rb') as f:
    for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
raise SystemExit(0 if h.hexdigest()==expected else 1)
PY
}

verify_relative_ai_rpath() {
  local exe="$1" dynamic
  dynamic="$(readelf -d "$exe" 2>/dev/null)" || return 1
  [[ "$dynamic" == *'$ORIGIN/components/ai/runtime/lib'* ]] || { printf 'FAIL: relative AI runtime RPATH missing from %s\n' "$exe"; return 1; }
  if [[ "$dynamic" == *'/DKLab/Projects/ReddMedia/'* || "$dynamic" == *'/DKLab/Projects/Nougat Media Suite/components/ai/runtime/'* ]]; then
    printf 'FAIL: absolute project AI runtime path leaked into %s\n' "$exe"
    return 1
  fi
  printf 'Relative AI runtime RPATH verified: $ORIGIN/components/ai/runtime/lib[64].\n'
}

verify_git_state() {
  local branch head dirty
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  dirty="$(git -C "$project_root" status --porcelain)"
  [[ "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "${branch:-unknown}"; return 1; }
  [[ "$head" == "$expected_head" ]] || { printf 'FAIL: expected accepted v0.0.29 HEAD %s, found %s\n' "$expected_head" "${head:-unknown}"; return 1; }
  [[ -z "$dirty" ]] || { printf 'FAIL: accepted v0.0.29 worktree must be clean before v0.0.30:\n%s\n' "$dirty"; return 1; }
  printf 'Git preflight PASS: clean main at accepted v0.0.29 commit %s.\n' "$expected_head"
}

verify_manifest() {
  python3 - "$payload_root" "$manifest_path" "$project_root" <<'PY'
import hashlib,json,pathlib,sys
payload=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2]); project=pathlib.Path(sys.argv[3])
if not mp.is_file(): print('FAIL: package manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text(encoding='utf-8'))
if m.get('target_version') != '0.0.30': print('FAIL: manifest target version mismatch'); raise SystemExit(1)
if m.get('accepted_base_commit') != '91ced5349a71de34cbc7d3d95d32c4de56645c7b': print('FAIL: manifest accepted base mismatch'); raise SystemExit(1)
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
for rel,record in m.get('payload',{}).items():
    p=payload/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: package payload mismatch: {rel}'); raise SystemExit(1)
for rel,record in m.get('base_files',{}).items():
    p=project/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: accepted v0.0.29 base differs: {rel}'); raise SystemExit(1)
for rel in m.get('required_absent',[]):
    if (project/rel).exists(): print(f'FAIL: expected accepted-v0.0.29-absent path exists: {rel}'); raise SystemExit(1)
print(f"Manifest verified: {len(m.get('payload',{}))} payload files, {len(m.get('base_files',{}))} exact accepted-v0.0.29 base files, {len(m.get('required_absent',[]))} required-absent paths.")
PY
}

verify_protected_state() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 - "$project_root" <<'PY'
import hashlib,pathlib,sys
root=pathlib.Path(sys.argv[1])
expected={
'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff',
'components/nougat/nougat_engine.py':'ea40f22f77561c3c18ccd58dd01a69f6741cd3b02f6a56a522730c2918240993',
'src/nougat/nougat_bridge.cpp':'15bc81a969986d8bcbeef8e8e452f04c5c6e06a0b9824f2b9e3e05fd9c57b944',
'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e',
'src/p2p_engine.cpp':'1ad8dec1a454f5809c9afb1647b51d461110d01ce8647cfdabeeb57e9b3137a5',
'src/p2p_engine.hpp':'3d21670ffb49616c011d008efe292340ee1e6e55a004260d193c3e864c2a283f',
'src/p2p_stream_server.cpp':'110a9d7dd5036f8adcc45def2f1853c51a1ec928cca88a6132de6d5c205c4a29',
'src/p2p_stream_server.hpp':'68b92de25a138c5e78f8655ac6a28316d375743a13c65ed28369a2b5880b77d7'}
for rel,want in expected.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest()
    if got!=want: print('FAIL: protected file changed:',rel); raise SystemExit(1)
print('Licensing, Search engine/bridge, and existing P2P implementation preserved unchanged for v0.0.30; focused P2P expansion remains v0.0.31.')
PY
}

save_rollback_snapshot() {
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell/applications" "$rollback_root/user-shell/icons" "$rollback_root/user-shell/pixmaps" || return 1
  local rel src size key
  for rel in "${modified_paths[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1
    fi
  done
  if [[ -e "$project_root/Nougat_Media_Suite_v29" ]]; then cp -a "$project_root/Nougat_Media_Suite_v29" "$rollback_root/project/Nougat_Media_Suite_v29" || return 1; fi
  for src in "$launcher_unversioned" "$launcher_v22" "$launcher_v23" "$launcher_v24" "$launcher_v25" "$launcher_v26" "$launcher_v27" "$launcher_v28" "$launcher_v29" "$launcher_v30" "$canonical_launcher"; do
    if [[ -e "$src" ]]; then cp -a "$src" "$rollback_root/user-shell/applications/$(basename "$src")" || return 1; fi
  done
  for size in 16 32 48 64 128 256 512; do
    for key in "${icon_alias_keys[@]}"; do
      src="$HOME/.local/share/icons/hicolor/${size}x${size}/apps/${key}.png"
      if [[ -e "$src" ]]; then mkdir -p "$rollback_root/user-shell/icons/$size" || return 1; cp -a "$src" "$rollback_root/user-shell/icons/$size/${key}.png" || return 1; fi
    done
  done
  for key in "${icon_alias_keys[@]}"; do
    src="$HOME/.local/share/pixmaps/${key}.png"
    if [[ -e "$src" ]]; then cp -a "$src" "$rollback_root/user-shell/pixmaps/${key}.png" || return 1; fi
  done
  if command -v gsettings >/dev/null 2>&1; then gsettings get org.gnome.shell favorite-apps > "$rollback_root/user-shell/favorite-apps.txt" 2>/dev/null || true; fi
  cat > "$rollback_root/ROLLBACK_INFO.txt" <<INFO
Project: Nougat Media Suite
Created: $(date -Is)
Accepted base: v0.0.29 $expected_head
Target candidate: v0.0.30
Package scope: UI cohesion, Library cache/progress/multi-row polish, Previous/Next, volume proportion; focused P2P expansion deferred to v0.0.31
INFO
}

apply_payload() {
  local rel
  for rel in "${modified_paths[@]}"; do
    [[ "$rel" == "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v30.json" ]] && continue
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || { printf 'FAIL: could not apply %s
' "$rel"; return 1; }
  done
  cp -a "$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v30.json" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v30.json" || return 1
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_30.sh" \
    "$project_root/tools/test_nougat_media_suite_retained_v29.py" \
    "$project_root/tools/test_nougat_media_suite_v30.py" \
    "$project_root/tools/test_nougat_media_suite_ui_smoke_v30.py" \
    "$project_root/tools/test_installer_rollback_v30.py" || return 1
  applied=1
}

restore_rollback() {
  [[ "$applied" == "1" ]] || return 0
  printf '
ROLLBACK START: restoring exact accepted v0.0.29 touched state
'
  local rel dest backup size key
  for rel in "${modified_paths[@]}"; do
    if [[ -e "$rollback_root/project/$rel" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$rollback_root/project/$rel" "$project_root/$rel"
    else
      rm -rf -- "$project_root/$rel"
    fi
  done
  rm -f -- "$project_root/Nougat_Media_Suite_v30"
  if [[ -e "$rollback_root/project/Nougat_Media_Suite_v29" ]]; then cp -a "$rollback_root/project/Nougat_Media_Suite_v29" "$project_root/Nougat_Media_Suite_v29"; chmod +x "$project_root/Nougat_Media_Suite_v29" 2>/dev/null || true; fi
  for dest in "$launcher_unversioned" "$launcher_v22" "$launcher_v23" "$launcher_v24" "$launcher_v25" "$launcher_v26" "$launcher_v27" "$launcher_v28" "$launcher_v29" "$launcher_v30" "$canonical_launcher"; do
    backup="$rollback_root/user-shell/applications/$(basename "$dest")"
    if [[ -e "$backup" ]]; then mkdir -p "$(dirname "$dest")"; cp -a "$backup" "$dest"; else rm -f -- "$dest"; fi
  done
  for size in 16 32 48 64 128 256 512; do
    for key in "${icon_alias_keys[@]}"; do
      dest="$HOME/.local/share/icons/hicolor/${size}x${size}/apps/${key}.png"; backup="$rollback_root/user-shell/icons/$size/${key}.png"
      if [[ -e "$backup" ]]; then mkdir -p "$(dirname "$dest")"; cp -a "$backup" "$dest"; else rm -f -- "$dest"; fi
    done
  done
  for key in "${icon_alias_keys[@]}"; do
    dest="$HOME/.local/share/pixmaps/${key}.png"; backup="$rollback_root/user-shell/pixmaps/${key}.png"
    if [[ -e "$backup" ]]; then cp -a "$backup" "$dest"; else rm -f -- "$dest"; fi
  done
  if [[ -s "$rollback_root/user-shell/favorite-apps.txt" ]] && command -v gsettings >/dev/null 2>&1; then gsettings set org.gnome.shell favorite-apps "$(cat "$rollback_root/user-shell/favorite-apps.txt")" >/dev/null 2>&1 || true; fi
  command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
  printf 'ROLLBACK PASS: accepted v0.0.29 touched state restored.
'
}

run_source_tests() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_media_server_lifecycle_v17.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_diagnostics_v26.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v29.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v30.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v30.py" "$project_root" || return 1
}

build_stub() {
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  local exe="$build_root/stub/Nougat_Media_Suite_v30"
  [[ "$("$exe" --version)" == "Nougat Media Suite v0.0.30" ]] || { printf 'FAIL: v30 stub version mismatch
'; return 1; }
  "$exe" --discover-ai-self-test || return 1
  "$exe" --v25-ui-state-self-test || return 1
  "$exe" --v28-ui-state-self-test || return 1
  "$exe" --v29-tv-reliability-self-test || return 1
  "$exe" --v30-ui-library-player-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v29.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v30.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v30.py" "$project_root" "$exe" || return 1
}

build_full() {
  cmake -S "$project_root" -B "$build_root/full" -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/full" -j"$(nproc)" || return 1
  mkdir -p "$build_root/full/components/ai" || return 1
  rm -rf -- "$build_root/full/components/ai/runtime" "$build_root/full/components/ai/models"
  ln -s "$project_root/components/ai/runtime" "$build_root/full/components/ai/runtime" || return 1
  ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models" || return 1
  local exe="$build_root/full/Nougat_Media_Suite_v30"
  verify_relative_ai_rpath "$exe" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.30" ]] || { printf 'FAIL: full native v30 version/runtime check failed
'; return 1; }
  env -u LD_LIBRARY_PATH "$exe" --discover-ai-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v25-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v28-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v29-tv-reliability-self-test || return 1
  "$exe" --v30-ui-library-player-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v29.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v30.py" "$project_root" "$exe" || return 1
}

install_icon_aliases() {
  local size key src dest
  for size in 16 32 48 64 128 256 512; do
    src="$project_root/assets/icons/nougat-media-suite-$size.png"
    for key in "${icon_alias_keys[@]}"; do
      dest="$HOME/.local/share/icons/hicolor/${size}x${size}/apps/${key}.png"
      mkdir -p "$(dirname "$dest")" || return 1
      cp -a "$src" "$dest" || return 1
      cmp -s "$src" "$dest" || { printf 'FAIL: installed icon alias mismatch: %s\n' "$dest"; return 1; }
    done
  done
  mkdir -p "$HOME/.local/share/pixmaps" || return 1
  for key in "${icon_alias_keys[@]}"; do cp -a "$master_icon" "$HOME/.local/share/pixmaps/${key}.png" || return 1; done
  command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
}

install_launchers() {
  mkdir -p "$HOME/.local/share/applications" || return 1
  local file
  for file in NougatMediaSuite.desktop NougatMediaSuite_v22.desktop NougatMediaSuite_v23.desktop NougatMediaSuite_v24.desktop NougatMediaSuite_v25.desktop NougatMediaSuite_v26.desktop NougatMediaSuite_v27.desktop NougatMediaSuite_v28.desktop NougatMediaSuite_v29.desktop NougatMediaSuite_v30.desktop com.elderredsoftworks.NougatMediaSuite.desktop; do
    cp -a "$project_root/$file" "$HOME/.local/share/applications/$file" || return 1
  done
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
}

refresh_nougat_favorite() {
  command -v gsettings >/dev/null 2>&1 || return 0
  python3 - <<'PY'
import ast, subprocess
schema='org.gnome.shell'; key='favorite-apps'; canonical='com.elderredsoftworks.NougatMediaSuite.desktop'
legacy={'NougatMediaSuite.desktop','NougatMediaSuite_v22.desktop','NougatMediaSuite_v23.desktop','NougatMediaSuite_v24.desktop','NougatMediaSuite_v25.desktop','NougatMediaSuite_v26.desktop','NougatMediaSuite_v27.desktop','NougatMediaSuite_v28.desktop','NougatMediaSuite_v29.desktop','NougatMediaSuite_v30.desktop',canonical}
try:
    raw=subprocess.check_output(['gsettings','get',schema,key],text=True,stderr=subprocess.DEVNULL).strip(); values=ast.literal_eval(raw)
except Exception: raise SystemExit(0)
if not isinstance(values,list): raise SystemExit(0)
changed=False; out=[]
for item in values:
    if item in legacy:
        if canonical not in out: out.append(canonical)
        changed = changed or item != canonical
    else: out.append(item)
if changed:
    subprocess.run(['gsettings','set',schema,key,str(out)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
PY
}

apply_raw_icon() {
  local exe="$1" info
  gio set -t string "$exe" metadata::custom-icon "file://$master_icon" || return 1
  info="$(gio info -a metadata::custom-icon "$exe" 2>&1)" || return 1
  [[ "$info" == *"file://$master_icon"* ]] || { printf 'FAIL: v30 executable custom-icon readback mismatch\n%s\n' "$info"; return 1; }
  printf 'Raw executable approved N custom-icon metadata verified after final write.\n'
}

install_final_candidate() {
  cp "$build_root/full/Nougat_Media_Suite_v30" "$project_root/Nougat_Media_Suite_v30" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v30" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v30" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v30" --version 2>/dev/null)" == "Nougat Media Suite v0.0.30" ]] || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v30" --discover-ai-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v30" --v25-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v30" --v28-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v30" --v29-tv-reliability-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v30" --v30-ui-library-player-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v29.py" "$project_root" "$project_root/Nougat_Media_Suite_v30" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v30.py" "$project_root" "$project_root/Nougat_Media_Suite_v30" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v30.py" "$project_root" "$project_root/Nougat_Media_Suite_v30" || return 1
  install_icon_aliases || return 1
  install_launchers || return 1
  refresh_nougat_favorite || return 1
  apply_raw_icon "$project_root/Nougat_Media_Suite_v30" || return 1
  rm -f -- "$project_root/Nougat_Media_Suite_v29" || return 1
  command -v nautilus >/dev/null 2>&1 && nautilus -q >/dev/null 2>&1 || true
  return 0
}

main() {
  phase_start "accepted v0.0.29 base and v0.0.30 prerequisites"
  [[ -d "$project_root" ]] || { printf 'FAIL: project not found: %s
' "$project_root"; printf '
FINAL FAIL: v0.0.30 not installed. Terminal remains open.
'; return 1; }
  local cmd
  for cmd in cmake g++ pkg-config python3 gio git env readelf sha256sum tar xvfb-run xwininfo xprop ffmpeg; do require_command "$cmd" || { printf '
FINAL FAIL: prerequisites incomplete. Terminal remains open.
'; return 1; }; done
  pkg-config --exists libtorrent-rasterbar || { printf 'FAIL: libtorrent-rasterbar development package unavailable.
'; printf '
FINAL FAIL: prerequisites incomplete. Terminal remains open.
'; return 1; }
  verify_port_8096_free || { printf '
FINAL FAIL: runtime preflight failed. Terminal remains open.
'; return 1; }
  verify_git_state || { printf '
FINAL FAIL: Git/worktree preflight failed. Terminal remains open.
'; return 1; }
  [[ -x "$project_root/Nougat_Media_Suite_v29" ]] || { printf 'FAIL: accepted v0.0.29 root executable missing/not executable.
'; return 1; }
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v29" --version 2>/dev/null)" == "Nougat Media Suite v0.0.29" ]] || { printf 'FAIL: accepted v0.0.29 executable version/runtime check failed.
'; return 1; }
  [[ -f "$project_root/components/ai/runtime/include/llama.h" && -e "$project_root/components/ai/runtime/lib/libllama.so.0" ]] || { printf 'FAIL: AI runtime missing.
'; return 1; }
  [[ -f "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]] || { printf 'FAIL: integrated Jellyfin runtime missing.
'; return 1; }
  verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha" || { printf 'FAIL: pinned Nomic model missing/changed.
'; return 1; }
  verify_protected_state || { printf '
FINAL FAIL: protected accepted-v0.0.29 state mismatch. Terminal remains open.
'; return 1; }
  verify_manifest || { printf '
FINAL FAIL: v0.0.30 package/base verification failed. Terminal remains open.
'; return 1; }
  phase_pass "accepted v0.0.29 base and v0.0.30 prerequisites"

  phase_start "save exact accepted v0.0.29 rollback snapshot"
  save_rollback_snapshot || { printf '
FINAL FAIL: rollback snapshot failed before changes. Terminal remains open.
'; return 1; }
  phase_pass "save exact accepted v0.0.29 rollback snapshot"

  phase_start "apply approved Nougat Media Suite v0.0.30 changed files"
  apply_payload || { restore_rollback; printf '
FINAL FAIL: v0.0.30 payload application failed. Terminal remains open.
'; return 1; }
  phase_pass "apply approved Nougat Media Suite v0.0.30 changed files"

  phase_start "license, Search/P2P preservation, retained v0.0.29 behavior, and v0.0.30 regression tests"
  run_source_tests || { restore_rollback; printf '
FINAL FAIL: source/regression validation failed; accepted v0.0.29 touched state restored. Terminal remains open.
'; return 1; }
  phase_pass "license, Search/P2P preservation, retained v0.0.29 behavior, and v0.0.30 regression tests"

  phase_start "warnings-as-errors stub build and X11 v0.0.30 identity/UI smoke"
  mkdir -p "$build_root" && build_stub || { restore_rollback; printf '
FINAL FAIL: deterministic build/UI validation failed; accepted v0.0.29 touched state restored. Terminal remains open.
'; return 1; }
  phase_pass "warnings-as-errors stub build and X11 v0.0.30 identity/UI smoke"

  phase_start "full native Nougat Media Suite v0.0.30 rebuild"
  build_full || { restore_rollback; printf '
FINAL FAIL: full native v0.0.30 build failed; accepted v0.0.29 touched state restored. Terminal remains open.
'; return 1; }
  phase_pass "full native Nougat Media Suite v0.0.30 rebuild"

  phase_start "install and verify final v30 executable, launchers, and N identity"
  install_final_candidate || { restore_rollback; printf '
FINAL FAIL: final v0.0.30 installation failed; accepted v0.0.29 touched state restored. Terminal remains open.
'; return 1; }
  phase_pass "install and verify final v30 executable, launchers, and N identity"

  rm -rf -- "$build_root"
  printf '
FINAL PASS: Nougat Media Suite v0.0.30 UI Cohesion, Library Performance, and Player Navigation installed and validated.
'
  printf 'Executable: %s
' "$project_root/Nougat_Media_Suite_v30"
  printf 'Rollback snapshot: %s
' "$rollback_root"
  printf 'LICENSE STATUS: protected PolyForm Noncommercial licensing files preserved unchanged.
'
  printf 'SEARCH/P2P STATUS: Search engine/bridge and existing P2P implementation preserved unchanged; focused P2P expansion remains roadmap v0.0.31.
'
  printf 'OWNER CHECK REQUIRED: rounded large content panels; complete selected-tab arrows during busy states; portrait Home movie/series poster geometry; multiple Library rows visible at once; cache-first Library return; truthful determinate/indeterminate progress; Refresh Server versus Refresh Library semantics; Previous/Next episode navigation; larger volume knob; retained v0.0.29 behavior.
'
  printf 'CANDIDATE STATUS: v0.0.30 remains unaccepted until owner real-machine visual/use approval.
'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v30
' "$project_root"
}

main "$@"
