#!/usr/bin/env bash

# Nougat Media Suite v0.0.24 same-version true app-wide exact concept-sheet N repair.
# Approved scope only: replace every active Nougat N identity surface with the literal
# N extracted from the owner-supplied full-resolution concept sheet. Background/quilt,
# Search, playback, licensing, Library, Discover, Stream, and diagnostics are preserved.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_24_TRUE_APP_WIDE_EXACT_N_REPAIR_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_24-true-exact-n-$stamp-$$"
expected_head="870808f38352efeda13ac2c83e99f53c6a5e3fb4"
canonical_id="com.elderredsoftworks.NougatMediaSuite"
icon_key="nougat-media-suite-concept-sheet-v24"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
canonical_launcher="$HOME/.local/share/applications/${canonical_id}.desktop"
launcher_unversioned="$HOME/.local/share/applications/NougatMediaSuite.desktop"
launcher_v22="$HOME/.local/share/applications/NougatMediaSuite_v22.desktop"
launcher_v23="$HOME/.local/share/applications/NougatMediaSuite_v23.desktop"
launcher_v24="$HOME/.local/share/applications/NougatMediaSuite_v24.desktop"
pinned_model="$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_bytes="84106624"
pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
applied=0

modified_paths=(
  "APPLY_COMMAND.txt"
  "CHANGELOG.md"
  "ROADMAP.md"
  "NougatMediaSuite.desktop"
  "NougatMediaSuite_v22.desktop"
  "NougatMediaSuite_v23.desktop"
  "NougatMediaSuite_v24.desktop"
  "com.elderredsoftworks.NougatMediaSuite.desktop"
  "assets/icons/nougat-media-suite.png"
  "assets/icons/nougat-media-suite-14.png"
  "assets/icons/nougat-media-suite-16.png"
  "assets/icons/nougat-media-suite-32.png"
  "assets/icons/nougat-media-suite-48.png"
  "assets/icons/nougat-media-suite-64.png"
  "assets/icons/nougat-media-suite-128.png"
  "assets/icons/nougat-media-suite-256.png"
  "assets/icons/nougat-media-suite-512.png"
  "src/nougat_media_suite_icon_data.hpp"
  "tools/test_nougat_media_suite_v24.py"
  "tools/test_nougat_visual_assets_v24.py"
  "tools/test_installer_rollback_v24.py"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_24_SEARCH_PAGE_UI_POLISH_HANDSHAKE.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_24_SEARCH_PAGE_UI_POLISH_VALIDATION.md"
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_24.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json"
)
added_paths=(
  "assets/icons/nougat-media-suite-concept-sheet-v24.png"
)
deleted_on_success=(
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
s=socket.socket()
s.settimeout(.25)
used=s.connect_ex(('127.0.0.1',8096))==0
s.close()
if used:
    print('FAIL: localhost port 8096 is in use. Close Nougat Media Suite/Jellyfin before applying this repair.')
    raise SystemExit(1)
print('Port 8096 is free.')
PY
}

verify_file_exact() {
  python3 - "$1" "$2" "$3" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); size=int(sys.argv[2]); expected=sys.argv[3]
if not p.is_file() or p.stat().st_size != size:
    raise SystemExit(1)
h=hashlib.sha256()
with p.open('rb') as f:
    for b in iter(lambda:f.read(1024*1024),b''):
        h.update(b)
raise SystemExit(0 if h.hexdigest()==expected else 1)
PY
}

verify_relative_ai_rpath() {
  local exe="$1" dynamic
  dynamic="$(readelf -d "$exe" 2>/dev/null)" || return 1
  [[ "$dynamic" == *'$ORIGIN/components/ai/runtime/lib'* ]] || {
    printf 'FAIL: relative AI runtime RPATH missing from %s\n' "$exe"
    return 1
  }
  if [[ "$dynamic" == *'/DKLab/Projects/ReddMedia/'* || "$dynamic" == *'/DKLab/Projects/Nougat Media Suite/components/ai/runtime/'* ]]; then
    printf 'FAIL: absolute project AI runtime path leaked into %s\n' "$exe"
    return 1
  fi
  printf 'Relative AI runtime RPATH verified: $ORIGIN/components/ai/runtime/lib[64].\n'
}

verify_manifest() {
  python3 - "$payload_root" "$manifest_path" "$project_root" <<'PY'
import hashlib,json,pathlib,sys
payload=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2]); project=pathlib.Path(sys.argv[3])
if not mp.is_file():
    print('FAIL: package manifest missing')
    raise SystemExit(1)
m=json.loads(mp.read_text(encoding='utf-8'))
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):
            h.update(b)
    return h.hexdigest()
for rel,record in m.get('payload',{}).items():
    p=payload/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: payload mismatch: {rel}')
        raise SystemExit(1)
for rel,record in m.get('base_files',{}).items():
    p=project/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: installed rejected v0.0.24 base differs: {rel}')
        raise SystemExit(1)
for rel in m.get('required_absent',[]):
    if (project/rel).exists():
        print(f'FAIL: expected absent base path exists: {rel}')
        raise SystemExit(1)
print(f"Manifest verified: {len(m.get('payload',{}))} payload files, {len(m.get('base_files',{}))} exact base files, {len(m.get('required_absent',[]))} required-absent paths.")
PY
}

verify_git_state() {
  local branch head staged line path
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  staged="$(git -C "$project_root" diff --cached --name-only)"
  [[ "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "${branch:-unknown}"; return 1; }
  [[ "$head" == "$expected_head" ]] || { printf 'FAIL: expected accepted v0.0.23 HEAD %s, found %s\n' "$expected_head" "${head:-unknown}"; return 1; }
  [[ -z "$staged" ]] || { printf 'FAIL: staged Git changes already exist:\n%s\n' "$staged"; return 1; }
  while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    path="${line:3}"
    case "$path" in
      1.zip|2.zip|3.zip|Nougat_Media_Suite_v24|Nougat_Media_Suite_v23|NougatMediaSuite.desktop|NougatMediaSuite_v22.desktop|NougatMediaSuite_v23.desktop|NougatMediaSuite_v24.desktop|com.elderredsoftworks.NougatMediaSuite.desktop|INSTALL_NOUGAT_MEDIA_SUITE_v0_0_24.sh|NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json|APPLY_COMMAND.txt|CHANGELOG.md|CMakeLists.txt|COMPANY_BIBLE.md|DEPENDENCIES.md|README.md|ROADMAP.md|src/main.cpp|src/nougat_media_suite_icon_data.hpp|src/nougat_quilt_texture_data.hpp|src/ytdlp_stream_server.cpp|assets/icons/*|assets/ui/*|docs/NOUGAT_MEDIA_SUITE_BRAND_PALETTE.md|docs/builds/*|tools/test_installer_rollback_v22.py|tools/test_installer_rollback_v23.py|tools/test_installer_rollback_v24.py|tools/test_license_protection_v22.py|tools/test_nougat_media_suite_retained_v22.py|tools/test_nougat_media_suite_ui_smoke_v22.py|tools/test_nougat_media_suite_ui_smoke_v23.py|tools/test_nougat_media_suite_ui_smoke_v24.py|tools/test_nougat_media_suite_v22.py|tools/test_nougat_media_suite_v23.py|tools/test_nougat_media_suite_v24.py|tools/test_nougat_visual_assets_v24.py) ;;
      *) printf 'FAIL: unexpected worktree path present before repair: %s\n' "$path"; return 1 ;;
    esac
  done < <(git -C "$project_root" status --porcelain)
  printf 'Git preflight PASS: main remains at accepted v0.0.23; rejected v0.0.24 working state is permitted.\n'
}

verify_license_state() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  printf 'Protected licensing state verified unchanged.\n'
}

verify_search_engine_state() {
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  printf 'Search-engine behavior files verified unchanged.\n'
}

save_rollback_snapshot() {
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell/applications" "$rollback_root/user-shell/icons" "$rollback_root/user-shell/pixmaps" || return 1
  local rel size key src
  for rel in "${modified_paths[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1
    fi
  done
  if [[ -e "$project_root/Nougat_Media_Suite_v24" ]]; then
    cp -a "$project_root/Nougat_Media_Suite_v24" "$rollback_root/project/Nougat_Media_Suite_v24" || return 1
  fi
  for src in "$launcher_unversioned" "$launcher_v22" "$launcher_v23" "$launcher_v24" "$canonical_launcher"; do
    if [[ -e "$src" ]]; then
      cp -a "$src" "$rollback_root/user-shell/applications/$(basename "$src")" || return 1
    fi
  done
  for size in 16 32 48 64 128 256 512; do
    for key in "${icon_alias_keys[@]}"; do
      src="$HOME/.local/share/icons/hicolor/${size}x${size}/apps/${key}.png"
      if [[ -e "$src" ]]; then
        mkdir -p "$rollback_root/user-shell/icons/$size" || return 1
        cp -a "$src" "$rollback_root/user-shell/icons/$size/${key}.png" || return 1
      fi
    done
  done
  for key in "${icon_alias_keys[@]}"; do
    src="$HOME/.local/share/pixmaps/${key}.png"
    if [[ -e "$src" ]]; then cp -a "$src" "$rollback_root/user-shell/pixmaps/${key}.png" || return 1; fi
  done
  if command -v gsettings >/dev/null 2>&1; then
    gsettings get org.gnome.shell favorite-apps > "$rollback_root/user-shell/favorite-apps.txt" 2>/dev/null || true
  fi
  cat > "$rollback_root/ROLLBACK_INFO.txt" <<INFO
Pre-v0.0.24 true app-wide exact concept-sheet N repair snapshot
Created: $(date -Is)
Git HEAD remains accepted v0.0.23: $expected_head
Base: owner-rejected v0.0.24 candidate
Repair: literal concept-sheet N replaces every active icon identity surface
INFO
}

apply_payload() {
  local rel
  for rel in "${modified_paths[@]}" "${added_paths[@]}"; do
    [[ "$rel" == "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json" ]] && continue
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || { printf 'FAIL: could not apply %s\n' "$rel"; return 1; }
  done
  cp -a "$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v24.json" || return 1
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_24.sh" "$project_root/tools/test_installer_rollback_v24.py" "$project_root/tools/test_nougat_visual_assets_v24.py" "$project_root/tools/test_nougat_media_suite_v24.py" || return 1
  applied=1
}

restore_rollback() {
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring exact rejected pre-repair v0.0.24 state\n'
  local rel size key dest backup
  for rel in "${modified_paths[@]}"; do
    if [[ -e "$rollback_root/project/$rel" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$rollback_root/project/$rel" "$project_root/$rel"
    else
      rm -rf -- "$project_root/$rel"
    fi
  done
  for rel in "${added_paths[@]}"; do
    rm -rf -- "$project_root/$rel"
  done
  if [[ -e "$rollback_root/project/Nougat_Media_Suite_v24" ]]; then
    cp -a "$rollback_root/project/Nougat_Media_Suite_v24" "$project_root/Nougat_Media_Suite_v24"
  fi
  for dest in "$launcher_unversioned" "$launcher_v22" "$launcher_v23" "$launcher_v24" "$canonical_launcher"; do
    backup="$rollback_root/user-shell/applications/$(basename "$dest")"
    if [[ -e "$backup" ]]; then
      mkdir -p "$(dirname "$dest")"
      cp -a "$backup" "$dest"
    else
      rm -f -- "$dest"
    fi
  done
  for size in 16 32 48 64 128 256 512; do
    for key in "${icon_alias_keys[@]}"; do
      dest="$HOME/.local/share/icons/hicolor/${size}x${size}/apps/${key}.png"
      backup="$rollback_root/user-shell/icons/$size/${key}.png"
      if [[ -e "$backup" ]]; then
        mkdir -p "$(dirname "$dest")"
        cp -a "$backup" "$dest"
      else
        rm -f -- "$dest"
      fi
    done
  done
  for key in "${icon_alias_keys[@]}"; do
    dest="$HOME/.local/share/pixmaps/${key}.png"
    backup="$rollback_root/user-shell/pixmaps/${key}.png"
    if [[ -e "$backup" ]]; then cp -a "$backup" "$dest"; else rm -f -- "$dest"; fi
  done
  if [[ -s "$rollback_root/user-shell/favorite-apps.txt" ]] && command -v gsettings >/dev/null 2>&1; then
    gsettings set org.gnome.shell favorite-apps "$(cat "$rollback_root/user-shell/favorite-apps.txt")" >/dev/null 2>&1 || true
  fi
  command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
  printf 'ROLLBACK PASS: rejected pre-repair v0.0.24 state restored.\n'
}

run_source_tests() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_media_server_lifecycle_v17.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_visual_assets_v24.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v24.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v24.py" "$project_root" || return 1
}

build_stub() {
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  [[ "$("$build_root/stub/Nougat_Media_Suite_v24" --version)" == "Nougat Media Suite v0.0.24" ]] || { printf 'FAIL: stub version mismatch\n'; return 1; }
  "$build_root/stub/Nougat_Media_Suite_v24" --discover-ai-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v24.py" "$project_root" "$build_root/stub/Nougat_Media_Suite_v24" || return 1
}

build_full() {
  cmake -S "$project_root" -B "$build_root/full" -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/full" -j"$(nproc)" || return 1
  mkdir -p "$build_root/full/components/ai" || return 1
  rm -rf -- "$build_root/full/components/ai/runtime" "$build_root/full/components/ai/models"
  ln -s "$project_root/components/ai/runtime" "$build_root/full/components/ai/runtime" || return 1
  ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models" || return 1
  verify_relative_ai_rpath "$build_root/full/Nougat_Media_Suite_v24" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$build_root/full/Nougat_Media_Suite_v24" --version 2>/dev/null)" == "Nougat Media Suite v0.0.24" ]] || { printf 'FAIL: full native version/runtime check failed\n'; return 1; }
  env -u LD_LIBRARY_PATH "$build_root/full/Nougat_Media_Suite_v24" --discover-ai-self-test || return 1
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
  for key in "${icon_alias_keys[@]}"; do
    cp -a "$master_icon" "$HOME/.local/share/pixmaps/${key}.png" || return 1
  done
  command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
}

apply_raw_icon() {
  local exe="$1" info
  [[ -f "$master_icon" ]] || { printf 'FAIL: exact concept-sheet master icon missing: %s\n' "$master_icon"; return 1; }
  gio set -t string "$exe" metadata::custom-icon "file://$master_icon" || return 1
  info="$(gio info -a metadata::custom-icon "$exe" 2>&1)"
  [[ "$info" == *"file://$master_icon"* ]] || { printf 'FAIL: raw executable custom-icon readback mismatch\n%s\n' "$info"; return 1; }
  printf 'Raw executable exact concept-sheet N custom-icon metadata verified.\n'
}

refresh_nougat_favorite() {
  command -v gsettings >/dev/null 2>&1 || return 0
  python3 - <<'PY'
import ast, subprocess
schema='org.gnome.shell'; key='favorite-apps'
canonical='com.elderredsoftworks.NougatMediaSuite.desktop'
legacy={
    'NougatMediaSuite.desktop','NougatMediaSuite_v21.desktop','NougatMediaSuite_v22.desktop',
    'NougatMediaSuite_v23.desktop','NougatMediaSuite_v24.desktop',canonical
}
try:
    raw=subprocess.check_output(['gsettings','get',schema,key],text=True,stderr=subprocess.DEVNULL).strip()
    if raw.startswith('@as '):
        raw=raw[4:]
    apps=ast.literal_eval(raw)
    if not isinstance(apps,list):
        raise ValueError
    positions=[i for i,a in enumerate(apps) if a in legacy]
    if not positions:
        raise SystemExit(0)
    pos=min(positions)
    clean=[a for a in apps if a not in legacy]
    subprocess.run(['gsettings','set',schema,key,str(clean)],check=False,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    clean.insert(min(pos,len(clean)),canonical)
    subprocess.run(['gsettings','set',schema,key,str(clean)],check=False,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    print('GNOME dock favorite rebound to canonical launcher with fresh concept-sheet icon key.')
except Exception:
    pass
PY
}

install_desktop_identity() {
  mkdir -p "$HOME/.local/share/applications" || return 1
  cp -a "$project_root/NougatMediaSuite.desktop" "$launcher_unversioned" || return 1
  cp -a "$project_root/NougatMediaSuite_v22.desktop" "$launcher_v22" || return 1
  cp -a "$project_root/NougatMediaSuite_v23.desktop" "$launcher_v23" || return 1
  cp -a "$project_root/NougatMediaSuite_v24.desktop" "$launcher_v24" || return 1
  cp -a "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" "$canonical_launcher" || return 1
  chmod 0644 "$launcher_unversioned" "$launcher_v22" "$launcher_v23" "$launcher_v24" "$canonical_launcher" || return 1

  python3 - "$HOME/.local/share/applications" "$icon_key" <<'PY'
import pathlib,sys
root=pathlib.Path(sys.argv[1]); key=sys.argv[2]
candidates=list(root.glob('NougatMediaSuite*.desktop'))+[root/'com.elderredsoftworks.NougatMediaSuite.desktop']
for p in candidates:
    if not p.is_file():
        continue
    try:
        s=p.read_text(encoding='utf-8')
    except Exception:
        continue
    if 'Nougat Media Suite' not in s and 'Nougat_Media_Suite_' not in s:
        continue
    lines=[]; found=False
    for line in s.splitlines():
        if line.startswith('Icon='):
            line='Icon='+key
            found=True
        lines.append(line)
    if not found:
        lines.append('Icon='+key)
    p.write_text('\n'.join(lines)+'\n',encoding='utf-8')
PY

  local f
  for f in "$launcher_unversioned" "$launcher_v22" "$launcher_v23" "$launcher_v24" "$canonical_launcher"; do
    grep -Fq "Icon=$icon_key" "$f" || { printf 'FAIL: launcher still references old icon: %s\n' "$f"; return 1; }
    grep -Fq 'StartupWMClass=NougatMediaSuite' "$f" || { printf 'FAIL: StartupWMClass missing: %s\n' "$f"; return 1; }
    grep -Fq 'X-GNOME-Application-ID=com.elderredsoftworks.NougatMediaSuite' "$f" || { printf 'FAIL: GNOME app ID missing: %s\n' "$f"; return 1; }
    if command -v desktop-file-validate >/dev/null 2>&1; then desktop-file-validate "$f" || return 1; fi
    touch "$f"
  done

  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
  command -v xdg-desktop-menu >/dev/null 2>&1 && xdg-desktop-menu forceupdate >/dev/null 2>&1 || true
  refresh_nougat_favorite
}

verify_installed_identity() {
  local size key src dest
  for size in 16 32 48 64 128 256 512; do
    src="$project_root/assets/icons/nougat-media-suite-$size.png"
    for key in "${icon_alias_keys[@]}"; do
      dest="$HOME/.local/share/icons/hicolor/${size}x${size}/apps/${key}.png"
      cmp -s "$src" "$dest" || { printf 'FAIL: installed icon alias does not match exact concept-sheet asset: %s\n' "$dest"; return 1; }
    done
  done
  local f
  for f in "$launcher_unversioned" "$launcher_v22" "$launcher_v23" "$launcher_v24" "$canonical_launcher"; do
    grep -Fq "Icon=$icon_key" "$f" || { printf 'FAIL: installed launcher icon key mismatch: %s\n' "$f"; return 1; }
  done
  local info
  info="$(gio info -a metadata::custom-icon "$project_root/Nougat_Media_Suite_v24" 2>&1)" || return 1
  [[ "$info" == *"file://$master_icon"* ]] || { printf 'FAIL: active raw executable metadata does not point to exact concept-sheet master\n'; return 1; }
  return 0
}

install_final_candidate() {
  cp "$build_root/full/Nougat_Media_Suite_v24" "$project_root/Nougat_Media_Suite_v24" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v24" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v24" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v24" --version 2>/dev/null)" == "Nougat Media Suite v0.0.24" ]] || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v24" --discover-ai-self-test || return 1
  apply_raw_icon "$project_root/Nougat_Media_Suite_v24" || return 1
  install_icon_aliases || return 1
  install_desktop_identity || return 1
  verify_installed_identity || return 1
  command -v nautilus >/dev/null 2>&1 && nautilus -q >/dev/null 2>&1 || true
  return 0
}

main() {
  phase_start "rejected v0.0.24 base and literal concept-sheet N repair prerequisites"
  [[ -d "$project_root" ]] || { printf 'FAIL: project not found: %s\n' "$project_root"; printf '\nFINAL FAIL: repair not installed. Terminal remains open.\n'; return 1; }
  local cmd
  for cmd in cmake g++ pkg-config python3 gio git env readelf sha256sum cmp xvfb-run xwininfo xprop; do
    require_command "$cmd" || { printf '\nFINAL FAIL: prerequisites incomplete. Terminal remains open.\n'; return 1; }
  done
  pkg-config --exists libtorrent-rasterbar || { printf 'FAIL: libtorrent-rasterbar development package unavailable.\n'; return 1; }
  verify_port_8096_free || { printf '\nFINAL FAIL: runtime preflight failed. Terminal remains open.\n'; return 1; }
  verify_git_state || { printf '\nFINAL FAIL: Git/worktree preflight failed. Terminal remains open.\n'; return 1; }
  verify_license_state || { printf '\nFINAL FAIL: protected license state mismatch. Terminal remains open.\n'; return 1; }
  verify_search_engine_state || { printf '\nFINAL FAIL: Search-engine state mismatch. Terminal remains open.\n'; return 1; }
  [[ -x "$project_root/Nougat_Media_Suite_v24" ]] || { printf 'FAIL: installed Nougat_Media_Suite_v24 missing/not executable.\n'; return 1; }
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v24" --version 2>/dev/null)" == "Nougat Media Suite v0.0.24" ]] || { printf 'FAIL: installed v0.0.24 executable version/runtime check failed.\n'; return 1; }
  [[ -f "$project_root/components/ai/runtime/include/llama.h" && -e "$project_root/components/ai/runtime/lib/libllama.so.0" ]] || { printf 'FAIL: AI runtime missing.\n'; return 1; }
  [[ -f "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]] || { printf 'FAIL: integrated Jellyfin runtime missing.\n'; return 1; }
  verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha" || { printf 'FAIL: pinned Nomic model missing/changed.\n'; return 1; }
  verify_manifest || { printf '\nFINAL FAIL: repair package/base verification failed. Terminal remains open.\n'; return 1; }
  phase_pass "rejected v0.0.24 base and literal concept-sheet N repair prerequisites"

  phase_start "save exact pre-repair rejected v0.0.24 rollback snapshot"
  save_rollback_snapshot || { printf '\nFINAL FAIL: rollback snapshot failed before changes. Terminal remains open.\n'; return 1; }
  phase_pass "save exact pre-repair rejected v0.0.24 rollback snapshot"

  phase_start "replace literal concept-sheet N in project icons, in-app header data, window icon data, and every launcher"
  apply_payload || { restore_rollback; printf '\nFINAL FAIL: exact concept-sheet icon payload application failed. Terminal remains open.\n'; return 1; }
  phase_pass "replace literal concept-sheet N in project icons, in-app header data, window icon data, and every launcher"

  phase_start "license/Search preservation and exact concept-sheet N regression tests"
  run_source_tests || { restore_rollback; printf '\nFINAL FAIL: source/regression validation failed; rejected pre-repair v24 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "license/Search preservation and exact concept-sheet N regression tests"

  phase_start "warnings-as-errors stub build and X11 identity smoke"
  mkdir -p "$build_root" && build_stub || { restore_rollback; printf '\nFINAL FAIL: deterministic build/X11 identity smoke failed; rejected pre-repair v24 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "warnings-as-errors stub build and X11 identity smoke"

  phase_start "full native Nougat Media Suite v0.0.24 rebuild"
  build_full || { restore_rollback; printf '\nFINAL FAIL: full native build failed; rejected pre-repair v24 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "full native Nougat Media Suite v0.0.24 rebuild"

  phase_start "install literal concept-sheet N app-wide and refresh GNOME/Files identity"
  install_final_candidate || { restore_rollback; printf '\nFINAL FAIL: final icon identity installation failed; rejected pre-repair v24 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "install literal concept-sheet N app-wide and refresh GNOME/Files identity"

  rm -rf -- "$build_root"
  printf '\nFINAL PASS: Nougat Media Suite v0.0.24 literal concept-sheet N app-wide replacement installed and validated.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v24"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'ICON STATUS: exact concept-sheet N now drives the in-app header, embedded X11 window icon, project icon family, every Nougat launcher alias, GNOME icon aliases, dock favorite identity, and raw executable metadata.\n'
  printf 'BACKGROUND STATUS: existing concept-sheet quilt and per-tab tints preserved unchanged.\n'
  printf 'LICENSE STATUS: protected PolyForm Noncommercial licensing files preserved unchanged.\n'
  printf 'SEARCH ENGINE STATUS: Search engine/bridge behavior preserved unchanged.\n'
  printf 'OWNER CHECK REQUIRED: confirm the concept-sheet N appears in the left in-app header, Ubuntu dock/sidebar, app switcher/window, and Files executable with no white/cream halo.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v24\n' "$project_root"
}

main "$@"
