#!/usr/bin/env bash

# Nougat Media Suite v0.0.25 same-version owner-test repair.
# Approved scope only: remove the now-unnecessary cream/white provider container
# and border behind the Stream service selectors. Provider geometry, theme, quilt
# tint, notches, Discover selection/playback, Search, licensing, and accepted
# visual assets remain unchanged.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v25.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_25_STREAM_PROVIDER_PANEL_REMOVAL_REPAIR_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_25-stream-provider-panel-repair-$stamp-$$"
expected_head="f66d35b671c9bceee6151dc63003dc3ec24578e8"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
quilt_source="$project_root/assets/ui/nougat-quilt-source.png"
pinned_model="$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_bytes="84106624"
pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
master_icon_sha="5d0239c7999a091bb4b60384b2953444a8e40a7644ca6e18dddac1cb69b00e66"
quilt_source_sha="eea284cc42f48ea2184ff3ccf8c717c9b43bad10727efe4bbdeaa8c2c025ba21"
applied=0

modified_paths=(
  "APPLY_COMMAND.txt"
  "CHANGELOG.md"
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_25.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v25.json"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_25_STREAM_SELECTION_DISCOVER_PLAY_HANDSHAKE.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_25_STREAM_SELECTION_DISCOVER_PLAY_VALIDATION.md"
  "src/main.cpp"
  "tools/test_installer_rollback_v25.py"
  "tools/test_nougat_media_suite_v25.py"
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

sha_file() {
  sha256sum "$1" | awk '{print $1}'
}

verify_port_8096_free() {
  python3 - <<'PY'
import socket
s=socket.socket(); s.settimeout(.25)
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
        print(f'FAIL: installed rejected v0.0.25 header-removal candidate differs: {rel}')
        raise SystemExit(1)
for rel in m.get('required_absent',[]):
    if (project/rel).exists():
        print(f'FAIL: expected absent path exists: {rel}')
        raise SystemExit(1)
print(f"Manifest verified: {len(m.get('payload',{}))} repair payload files and {len(m.get('base_files',{}))} exact rejected-v0.0.25 base files.")
PY
}

verify_git_state() {
  local branch head staged line path
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  staged="$(git -C "$project_root" diff --cached --name-only)"
  [[ "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "${branch:-unknown}"; return 1; }
  [[ "$head" == "$expected_head" ]] || { printf 'FAIL: expected accepted v0.0.24 HEAD %s, found %s\n' "$expected_head" "${head:-unknown}"; return 1; }
  [[ -z "$staged" ]] || { printf 'FAIL: staged Git changes already exist:\n%s\n' "$staged"; return 1; }
  while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    path="${line:3}"
    case "$path" in
      APPLY_COMMAND.txt|CHANGELOG.md|CMakeLists.txt|COMPANY_BIBLE.md|README.md|ROADMAP.md|NougatMediaSuite.desktop|NougatMediaSuite_v22.desktop|NougatMediaSuite_v23.desktop|NougatMediaSuite_v24.desktop|NougatMediaSuite_v25.desktop|com.elderredsoftworks.NougatMediaSuite.desktop|INSTALL_NOUGAT_MEDIA_SUITE_v0_0_25.sh|NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v25.json|Nougat_Media_Suite_v24|Nougat_Media_Suite_v25|src/main.cpp|src/recommendations/recommendation_engine.cpp|src/recommendations/recommendation_engine.hpp|docs/builds/NOUGAT_MEDIA_SUITE_v0_0_25_STREAM_SELECTION_DISCOVER_PLAY_HANDSHAKE.md|docs/builds/NOUGAT_MEDIA_SUITE_v0_0_25_STREAM_SELECTION_DISCOVER_PLAY_VALIDATION.md|tools/test_installer_rollback_v25.py|tools/test_nougat_media_suite_ui_smoke_v25.py|tools/test_nougat_media_suite_v25.py) ;;
      *) printf 'FAIL: unexpected worktree path present before same-version repair: %s\n' "$path"; return 1 ;;
    esac
  done < <(git -C "$project_root" status --porcelain)
  printf 'Git preflight PASS: main remains at accepted v0.0.24 commit; installed rejected v0.0.25 header-removal candidate state is permitted.\n'
}

verify_protected_state() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  [[ "$(sha_file "$master_icon")" == "$master_icon_sha" ]] || { printf 'FAIL: accepted concept-sheet N changed.\n'; return 1; }
  [[ "$(sha_file "$quilt_source")" == "$quilt_source_sha" ]] || { printf 'FAIL: accepted concept-sheet quilt source changed.\n'; return 1; }
  printf 'Protected licensing, Search behavior, concept-sheet N, and quilt source verified unchanged.\n'
}

save_rollback_snapshot() {
  mkdir -p "$rollback_root/project" || return 1
  local rel
  for rel in "${modified_paths[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1
    fi
  done
  if [[ -e "$project_root/Nougat_Media_Suite_v25" ]]; then
    cp -a "$project_root/Nougat_Media_Suite_v25" "$rollback_root/project/Nougat_Media_Suite_v25" || return 1
  fi
  cat > "$rollback_root/ROLLBACK_INFO.txt" <<INFO
Project: Nougat Media Suite
Created: $(date -Is)
Accepted Git anchor: v0.0.24 $expected_head
Rollback state: installed rejected v0.0.25 header-removal candidate before Stream provider-panel removal
INFO
}

apply_payload() {
  local rel
  for rel in "${modified_paths[@]}"; do
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || { printf 'FAIL: could not apply %s\n' "$rel"; return 1; }
  done
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_25.sh" \
    "$project_root/tools/test_installer_rollback_v25.py" \
    "$project_root/tools/test_nougat_media_suite_v25.py" || return 1
  applied=1
}

restore_rollback() {
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring exact rejected pre-panel-removal v0.0.25 candidate\n'
  local rel
  for rel in "${modified_paths[@]}"; do
    if [[ -e "$rollback_root/project/$rel" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$rollback_root/project/$rel" "$project_root/$rel"
    fi
  done
  if [[ -e "$rollback_root/project/Nougat_Media_Suite_v25" ]]; then
    cp -a "$rollback_root/project/Nougat_Media_Suite_v25" "$project_root/Nougat_Media_Suite_v25"
    chmod +x "$project_root/Nougat_Media_Suite_v25" 2>/dev/null || true
  fi
  printf 'ROLLBACK PASS: rejected pre-panel-removal v0.0.25 candidate restored.\n'
}

run_source_tests() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_media_server_lifecycle_v17.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_visual_assets_v24.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v25.py" "$project_root" || return 1
}

build_stub() {
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  local exe="$build_root/stub/Nougat_Media_Suite_v25"
  [[ "$("$exe" --version)" == "Nougat Media Suite v0.0.25" ]] || { printf 'FAIL: stub version mismatch\n'; return 1; }
  "$exe" --discover-ai-self-test || return 1
  "$exe" --v25-ui-state-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v25.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v25.py" "$project_root" "$exe" || return 1
}

build_full() {
  cmake -S "$project_root" -B "$build_root/full" -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/full" -j"$(nproc)" || return 1
  mkdir -p "$build_root/full/components/ai" || return 1
  rm -rf -- "$build_root/full/components/ai/runtime" "$build_root/full/components/ai/models"
  ln -s "$project_root/components/ai/runtime" "$build_root/full/components/ai/runtime" || return 1
  ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models" || return 1
  local exe="$build_root/full/Nougat_Media_Suite_v25"
  verify_relative_ai_rpath "$exe" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.25" ]] || { printf 'FAIL: full native version/runtime check failed\n'; return 1; }
  env -u LD_LIBRARY_PATH "$exe" --discover-ai-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v25-ui-state-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v25.py" "$project_root" "$exe" || return 1
}

apply_raw_icon() {
  local exe="$1" info
  gio set -t string "$exe" metadata::custom-icon "file://$master_icon" || return 1
  info="$(gio info -a metadata::custom-icon "$exe" 2>&1)" || return 1
  [[ "$info" == *"file://$master_icon"* ]] || { printf 'FAIL: v25 raw executable custom-icon readback mismatch\n%s\n' "$info"; return 1; }
  printf 'Raw executable accepted concept-sheet N custom-icon metadata verified.\n'
}

install_final_candidate() {
  cp "$build_root/full/Nougat_Media_Suite_v25" "$project_root/Nougat_Media_Suite_v25" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v25" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v25" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v25" --version 2>/dev/null)" == "Nougat Media Suite v0.0.25" ]] || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v25" --discover-ai-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v25" --v25-ui-state-self-test || return 1
  apply_raw_icon "$project_root/Nougat_Media_Suite_v25" || return 1
  command -v nautilus >/dev/null 2>&1 && nautilus -q >/dev/null 2>&1 || true
  return 0
}

main() {
  phase_start "installed rejected v0.0.25 header-removal candidate and Stream-provider-panel repair prerequisites"
  [[ -d "$project_root" ]] || { printf 'FAIL: project not found: %s\n' "$project_root"; printf '\nFINAL FAIL: repair not installed. Terminal remains open.\n'; return 1; }
  local cmd
  for cmd in cmake g++ pkg-config python3 gio git env readelf sha256sum xvfb-run xwininfo xprop; do
    require_command "$cmd" || { printf '\nFINAL FAIL: prerequisites incomplete. Terminal remains open.\n'; return 1; }
  done
  pkg-config --exists libtorrent-rasterbar || { printf 'FAIL: libtorrent-rasterbar development package unavailable.\n'; return 1; }
  verify_port_8096_free || { printf '\nFINAL FAIL: runtime preflight failed. Terminal remains open.\n'; return 1; }
  verify_git_state || { printf '\nFINAL FAIL: Git/worktree preflight failed. Terminal remains open.\n'; return 1; }
  [[ -x "$project_root/Nougat_Media_Suite_v25" ]] || { printf 'FAIL: installed rejected v0.0.25 root executable missing/not executable.\n'; return 1; }
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v25" --version 2>/dev/null)" == "Nougat Media Suite v0.0.25" ]] || { printf 'FAIL: installed rejected v0.0.25 executable version/runtime check failed.\n'; return 1; }
  [[ -f "$project_root/components/ai/runtime/include/llama.h" && -e "$project_root/components/ai/runtime/lib/libllama.so.0" ]] || { printf 'FAIL: AI runtime missing.\n'; return 1; }
  [[ -f "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]] || { printf 'FAIL: integrated Jellyfin runtime missing.\n'; return 1; }
  verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha" || { printf 'FAIL: pinned Nomic model missing/changed.\n'; return 1; }
  verify_protected_state || { printf '\nFINAL FAIL: protected v0.0.25 state mismatch. Terminal remains open.\n'; return 1; }
  verify_manifest || { printf '\nFINAL FAIL: repair package/base verification failed. Terminal remains open.\n'; return 1; }
  phase_pass "installed rejected v0.0.25 header-removal candidate and Stream-provider-panel repair prerequisites"

  phase_start "save exact rejected pre-repair v0.0.25 rollback snapshot"
  save_rollback_snapshot || { printf '\nFINAL FAIL: rollback snapshot failed before changes. Terminal remains open.\n'; return 1; }
  phase_pass "save exact rejected pre-repair v0.0.25 rollback snapshot"

  phase_start "remove redundant Stream provider container and border"
  apply_payload || { restore_rollback; printf '\nFINAL FAIL: repair payload application failed. Terminal remains open.\n'; return 1; }
  phase_pass "remove redundant Stream provider container and border"

  phase_start "license, Search, retained behavior, visual assets, and same-version repair regression tests"
  run_source_tests || { restore_rollback; printf '\nFINAL FAIL: source/regression validation failed; pre-repair v0.0.25 candidate restored. Terminal remains open.\n'; return 1; }
  phase_pass "license, Search, retained behavior, visual assets, and same-version repair regression tests"

  phase_start "warnings-as-errors stub build, v0.0.25 behavior tests, and X11 identity smoke"
  mkdir -p "$build_root" && build_stub || { restore_rollback; printf '\nFINAL FAIL: deterministic build/behavior validation failed; pre-repair v0.0.25 candidate restored. Terminal remains open.\n'; return 1; }
  phase_pass "warnings-as-errors stub build, v0.0.25 behavior tests, and X11 identity smoke"

  phase_start "full native Nougat Media Suite v0.0.25 rebuild"
  build_full || { restore_rollback; printf '\nFINAL FAIL: full native build failed; pre-repair v0.0.25 candidate restored. Terminal remains open.\n'; return 1; }
  phase_pass "full native Nougat Media Suite v0.0.25 rebuild"

  phase_start "install repaired v25 executable and preserve accepted N identity"
  install_final_candidate || { restore_rollback; printf '\nFINAL FAIL: final v0.0.25 repair installation failed; pre-repair candidate restored. Terminal remains open.\n'; return 1; }
  phase_pass "install repaired v25 executable and preserve accepted N identity"

  rm -rf -- "$build_root"
  printf '\nFINAL PASS: Nougat Media Suite v0.0.25 Stream provider panel removal repair installed and validated.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v25"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'STREAM STATUS: redundant label and cream/white provider panel removed; provider buttons now sit directly on the provider-tinted quilt with notch/colors preserved.\n'
  printf 'DISCOVER STATUS: v0.0.25 dual selection and local native-play repair preserved unchanged.\n'
  printf 'ICON/QUILT STATUS: accepted v0.0.24 concept-sheet N and quilt source preserved unchanged.\n'
  printf 'LICENSE STATUS: protected PolyForm Noncommercial licensing files preserved unchanged.\n'
  printf 'SEARCH ENGINE STATUS: Search engine/bridge behavior preserved unchanged.\n'
  printf 'OWNER CHECK REQUIRED: verify no cream/white box or border remains behind the Stream provider buttons and the provider row/notches/colors sit correctly on the tinted quilt.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v25\n' "$project_root"
}

main "$@"
