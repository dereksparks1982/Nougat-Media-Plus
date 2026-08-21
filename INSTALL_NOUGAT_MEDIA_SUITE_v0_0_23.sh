#!/usr/bin/env bash

# Nougat Media Suite v0.0.23 exact-concept UI and Stream repair candidate installer.
# No `set -e` and no `exit`: failures print STOP/FAIL and leave the owner's terminal open.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v23.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_23_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_23-build-$stamp-$$"
expected_head="755c1ac"
expected_base_exe_sha="651ba56069f97170b7f94cce2d019de703c18fa7f8d8e51775a61caf92e843a5"
user_launcher="$HOME/.local/share/applications/NougatMediaSuite.desktop"
pinned_model="$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_bytes="84106624"
pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
applied=0

modified_paths=(
  "APPLY_COMMAND.txt"
  "CHANGELOG.md"
  "CMakeLists.txt"
  "COMPANY_BIBLE.md"
  "DEPENDENCIES.md"
  "NougatMediaSuite.desktop"
  "README.md"
  "ROADMAP.md"
  "assets/icons/nougat-media-suite.png"
  "assets/icons/nougat-media-suite-16.png"
  "assets/icons/nougat-media-suite-32.png"
  "assets/icons/nougat-media-suite-48.png"
  "assets/icons/nougat-media-suite-64.png"
  "assets/icons/nougat-media-suite-128.png"
  "assets/icons/nougat-media-suite-256.png"
  "assets/icons/nougat-media-suite-512.png"
  "docs/NOUGAT_MEDIA_SUITE_BRAND_PALETTE.md"
  "src/main.cpp"
  "src/nougat_media_suite_icon_data.hpp"
  "src/ytdlp_stream_server.cpp"
)

added_paths=(
  "assets/icons/nougat-media-suite-14.png"
  "NougatMediaSuite_v23.desktop"
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_23.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v23.json"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_23_EXACT_CONCEPT_UI_STREAM_REPAIR_HANDSHAKE.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_23_EXACT_CONCEPT_UI_STREAM_REPAIR_VALIDATION.md"
  "tools/test_installer_rollback_v23.py"
  "tools/test_nougat_media_suite_ui_smoke_v23.py"
  "tools/test_nougat_media_suite_v23.py"
)

deleted_on_success=(
  "NougatMediaSuite_v22.desktop"
)

phase_start() { printf '\nPHASE START: %s\n' "$1"; }
phase_pass() { printf 'PHASE PASS: %s\n' "$1"; }

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then printf 'FAIL: required command not found: %s\n' "$1"; return 1; fi
  return 0
}

sha256_file() { sha256sum "$1" | awk '{print $1}'; }

verify_port_8096_free() {
  python3 - <<'PY'
import socket
s=socket.socket(); s.settimeout(.25); used=s.connect_ex(('127.0.0.1',8096))==0; s.close()
if used:
    print('FAIL: localhost port 8096 is in use. Close Nougat Media Suite/Jellyfin before applying this candidate.')
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
  [[ "$dynamic" == *'$ORIGIN/components/ai/runtime/lib'* ]] || {
    printf 'FAIL: relative AI runtime RPATH missing from %s\n' "$exe"; return 1; }
  if [[ "$dynamic" == *'/DKLab/Projects/ReddMedia/'* || "$dynamic" == *'/DKLab/Projects/Nougat Media Suite/components/ai/runtime/'* ]]; then
    printf 'FAIL: absolute project AI runtime path leaked into %s\n' "$exe"; return 1
  fi
  printf 'Relative AI runtime RPATH verified: $ORIGIN/components/ai/runtime/lib[64].\n'
}

verify_manifest() {
  python3 - "$payload_root" "$manifest_path" "$project_root" <<'PY'
import hashlib,json,pathlib,sys
payload=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2]); project=pathlib.Path(sys.argv[3])
if not mp.is_file(): print('FAIL: package manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text(encoding='utf-8'))
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
for rel,record in m.get('payload',{}).items():
    p=payload/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: payload mismatch: {rel}'); raise SystemExit(1)
for rel,record in m.get('base_files',{}).items():
    p=project/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: current project base file differs from the approved v23 base: {rel}'); raise SystemExit(1)
print(f"Manifest verified: {len(m.get('payload',{}))} payload files and {len(m.get('base_files',{}))} exact base files.")
PY
}

verify_git_state() {
  local branch head staged line path
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse --short=7 HEAD 2>/dev/null)"
  staged="$(git -C "$project_root" diff --cached --name-only)"
  [[ "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "${branch:-unknown}"; return 1; }
  [[ "$head" == "$expected_head" ]] || { printf 'FAIL: expected license-only HEAD %s, found %s\n' "$expected_head" "${head:-unknown}"; return 1; }
  [[ -z "$staged" ]] || { printf 'FAIL: staged Git changes already exist:\n%s\n' "$staged"; return 1; }

  # The owner is intentionally carrying the uncommitted v0.0.22 candidate plus
  # the three uploaded source ZIPs. Stop only if another unexpected path appears.
  while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    path="${line:3}"
    case "$path" in
      APPLY_COMMAND.txt|CHANGELOG.md|CMakeLists.txt|COMPANY_BIBLE.md|DEPENDENCIES.md|INSTALL_NOUGAT_MEDIA_SUITE_v0_0_21.sh|NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v21.json|NougatMediaSuite.desktop|NougatMediaSuite_v21.desktop|Nougat_Media_Suite_v21|README.md|ROADMAP.md|src/main.cpp|tools/test_installer_rollback_v21.py|tools/test_nougat_media_suite_retained_v21.py|tools/test_nougat_media_suite_ui_smoke_v21.py|tools/test_nougat_media_suite_v21.py|.github/|1.zip|2.zip|3.zip|CONTRIBUTING.md|COPYRIGHT.md|INSTALL_NOUGAT_MEDIA_SUITE_v0_0_22.sh|NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v22.json|NougatMediaSuite_v22.desktop|Nougat_Media_Suite_v22|docs/builds/NOUGAT_MEDIA_SUITE_v0_0_22_LICENSE_PROTECTION_HANDSHAKE.md|docs/builds/NOUGAT_MEDIA_SUITE_v0_0_22_LICENSE_PROTECTION_VALIDATION.md|tools/test_installer_rollback_v22.py|tools/test_license_protection_v22.py|tools/test_nougat_media_suite_retained_v22.py|tools/test_nougat_media_suite_ui_smoke_v22.py|tools/test_nougat_media_suite_v22.py)
        ;;
      *) printf 'FAIL: unexpected pre-v23 worktree path: %s\n' "$path"; return 1;;
    esac
  done < <(git -C "$project_root" status --porcelain)
  printf 'Git preflight PASS: main at license-only commit %s; no staged or unexpected paths.\n' "$expected_head"
}

verify_license_state() {
  python3 - "$project_root" <<'PY'
from pathlib import Path
import hashlib,sys
root=Path(sys.argv[1])
expected={
 'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
 'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
 'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
 'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
 'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff',
}
for rel,want in expected.items():
    p=root/rel
    got=hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else ''
    if got!=want:
        print(f'FAIL: protected license state changed before v23: {rel}'); raise SystemExit(1)
print('Protected v0.0.22 license-only state verified unchanged.')
PY
}

save_rollback_snapshot() {
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell" || return 1
  local rel
  for rel in "${modified_paths[@]}" "${deleted_on_success[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1
    fi
  done
  if [[ -e "$project_root/Nougat_Media_Suite_v22" ]]; then cp -a "$project_root/Nougat_Media_Suite_v22" "$rollback_root/project/Nougat_Media_Suite_v22" || return 1; fi
  if [[ -e "$user_launcher" ]]; then cp -a "$user_launcher" "$rollback_root/user-shell/NougatMediaSuite.desktop" || return 1; fi
  cat > "$rollback_root/ROLLBACK_INFO.txt" <<INFO
Pre-v0.0.23 exact working-state rollback snapshot
Created: $(date -Is)
Git HEAD: $expected_head
Base executable: Nougat_Media_Suite_v22
Candidate executable: Nougat_Media_Suite_v23
The already-pushed licensing files, user media, runtime data, Search data, and the owner-uploaded 1.zip/2.zip/3.zip are preserved.
INFO
}

apply_payload() {
  local rel
  for rel in "${modified_paths[@]}" "${added_paths[@]}"; do
    [[ "$rel" == "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v23.json" ]] && continue
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || { printf 'FAIL: could not apply %s\n' "$rel"; return 1; }
  done
  cp -a "$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v23.json" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v23.json" || return 1
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_23.sh" \
    "$project_root/tools/test_installer_rollback_v23.py" \
    "$project_root/tools/test_nougat_media_suite_ui_smoke_v23.py" \
    "$project_root/tools/test_nougat_media_suite_v23.py" || return 1
  applied=1
}

restore_rollback() {
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring exact pre-v0.0.23 touched state\n'
  local rel
  for rel in "${added_paths[@]}"; do rm -rf -- "$project_root/$rel"; done
  rm -f -- "$project_root/Nougat_Media_Suite_v23"
  for rel in "${modified_paths[@]}" "${deleted_on_success[@]}"; do
    if [[ -e "$rollback_root/project/$rel" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$rollback_root/project/$rel" "$project_root/$rel"
    else
      rm -rf -- "$project_root/$rel"
    fi
  done
  if [[ -e "$rollback_root/project/Nougat_Media_Suite_v22" ]]; then cp -a "$rollback_root/project/Nougat_Media_Suite_v22" "$project_root/Nougat_Media_Suite_v22"; fi
  if [[ -e "$rollback_root/user-shell/NougatMediaSuite.desktop" ]]; then mkdir -p "$(dirname "$user_launcher")"; cp -a "$rollback_root/user-shell/NougatMediaSuite.desktop" "$user_launcher"; fi
  printf 'ROLLBACK PASS: exact pre-v0.0.23 touched state restored; licensing/user data/runtimes preserved.\n'
}

run_source_tests() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_media_server_lifecycle_v17.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v23.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v23.py" "$project_root" || return 1
}

build_stub() {
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  [[ "$("$build_root/stub/Nougat_Media_Suite_v23" --version)" == "Nougat Media Suite v0.0.23" ]] || { printf 'FAIL: stub version mismatch\n'; return 1; }
  "$build_root/stub/Nougat_Media_Suite_v23" --discover-ai-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v23.py" "$project_root" "$build_root/stub/Nougat_Media_Suite_v23" || return 1
}

build_full() {
  cmake -S "$project_root" -B "$build_root/full" -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/full" -j"$(nproc)" || return 1
  mkdir -p "$build_root/full/components/ai" || return 1
  rm -rf -- "$build_root/full/components/ai/runtime" "$build_root/full/components/ai/models"
  ln -s "$project_root/components/ai/runtime" "$build_root/full/components/ai/runtime" || return 1
  ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models" || return 1
  verify_relative_ai_rpath "$build_root/full/Nougat_Media_Suite_v23" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$build_root/full/Nougat_Media_Suite_v23" --version 2>/dev/null)" == "Nougat Media Suite v0.0.23" ]] || { printf 'FAIL: full native version/runtime check failed\n'; return 1; }
  env -u LD_LIBRARY_PATH "$build_root/full/Nougat_Media_Suite_v23" --discover-ai-self-test || return 1
}

install_icon_theme() {
  local size src dest
  for size in 16 32 48 64 128 256 512; do
    src="$project_root/assets/icons/nougat-media-suite-$size.png"
    dest="$HOME/.local/share/icons/hicolor/${size}x${size}/apps/nougat-media-suite.png"
    mkdir -p "$(dirname "$dest")" || return 1
    cp -a "$src" "$dest" || return 1
  done
  command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
  return 0
}

apply_raw_icon() {
  local exe="$1" icon="$project_root/assets/icons/nougat-media-suite.png" info
  [[ -f "$icon" ]] || { printf 'FAIL: approved concept-sheet N icon missing: %s\n' "$icon"; return 1; }
  gio set -t string "$exe" metadata::custom-icon "file://$icon" || return 1
  info="$(gio info -a metadata::custom-icon "$exe" 2>&1)"
  [[ "$info" == *"file://$icon"* ]] || { printf 'FAIL: raw executable custom-icon readback mismatch\n%s\n' "$info"; return 1; }
  printf 'Raw executable concept-sheet N icon metadata verified.\n'
}

install_final_candidate() {
  cp "$build_root/full/Nougat_Media_Suite_v23" "$project_root/Nougat_Media_Suite_v23" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v23" || return 1
  [[ -x "$project_root/Nougat_Media_Suite_v23" ]] || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v23" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v23" --version 2>/dev/null)" == "Nougat Media Suite v0.0.23" ]] || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v23" --discover-ai-self-test || return 1

  # Identity gate comes after the final executable write.
  apply_raw_icon "$project_root/Nougat_Media_Suite_v23" || return 1
  install_icon_theme || return 1
  mkdir -p "$HOME/.local/share/applications" || return 1
  cp "$project_root/NougatMediaSuite.desktop" "$user_launcher" || return 1
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
  command -v nautilus >/dev/null 2>&1 && nautilus -q >/dev/null 2>&1 || true

  rm -f -- "$project_root/Nougat_Media_Suite_v22"
  rm -f -- "$project_root/NougatMediaSuite_v22.desktop"
  return 0
}

main() {
  phase_start "exact current v0.0.22 working base and prerequisites"
  if [[ ! -d "$project_root" ]]; then printf 'FAIL: project not found: %s\n' "$project_root"; printf '\nFINAL FAIL: v0.0.23 not installed. Terminal remains open.\n'; return 1; fi
  local cmd
  for cmd in cmake g++ pkg-config python3 gio git env readelf sha256sum xvfb-run xwininfo; do require_command "$cmd" || { printf '\nFINAL FAIL: prerequisites incomplete. Terminal remains open.\n'; return 1; }; done
  pkg-config --exists libtorrent-rasterbar || { printf 'FAIL: libtorrent-rasterbar development package unavailable.\n'; return 1; }
  verify_port_8096_free || { printf '\nFINAL FAIL: runtime preflight failed. Terminal remains open.\n'; return 1; }
  verify_git_state || { printf '\nFINAL FAIL: Git/worktree preflight failed. Terminal remains open.\n'; return 1; }
  verify_license_state || { printf '\nFINAL FAIL: protected license state mismatch. Terminal remains open.\n'; return 1; }
  [[ -x "$project_root/Nougat_Media_Suite_v22" ]] || { printf 'FAIL: current root Nougat_Media_Suite_v22 missing/not executable.\n'; return 1; }
  [[ "$(sha256_file "$project_root/Nougat_Media_Suite_v22")" == "$expected_base_exe_sha" ]] || { printf 'FAIL: current v22 executable hash does not match the uploaded base.\n'; return 1; }
  [[ -f "$project_root/components/ai/runtime/include/llama.h" && -e "$project_root/components/ai/runtime/lib/libllama.so.0" ]] || { printf 'FAIL: AI runtime missing.\n'; return 1; }
  [[ -f "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]] || { printf 'FAIL: integrated Jellyfin runtime missing.\n'; return 1; }
  verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha" || { printf 'FAIL: pinned Nomic model missing/changed.\n'; return 1; }
  verify_manifest || { printf '\nFINAL FAIL: package/base verification failed. Terminal remains open.\n'; return 1; }
  [[ ! -e "$project_root/Nougat_Media_Suite_v23" ]] || { printf 'FAIL: Nougat_Media_Suite_v23 already exists; do not overwrite an unknown candidate.\n'; return 1; }
  phase_pass "exact current v0.0.22 working base and prerequisites"

  phase_start "save exact pre-v0.0.23 rollback snapshot"
  save_rollback_snapshot || { printf '\nFINAL FAIL: rollback snapshot failed before source changes. Terminal remains open.\n'; return 1; }
  phase_pass "save exact pre-v0.0.23 rollback snapshot"

  phase_start "apply approved v0.0.23 concept UI and Stream repair files"
  apply_payload || { restore_rollback; printf '\nFINAL FAIL: payload application failed. Terminal remains open.\n'; return 1; }
  phase_pass "apply approved v0.0.23 concept UI and Stream repair files"

  phase_start "license boundary, retained behavior, and v0.0.23 regression tests"
  run_source_tests || { restore_rollback; printf '\nFINAL FAIL: source/regression validation failed; pre-v23 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "license boundary, retained behavior, and v0.0.23 regression tests"

  phase_start "warnings-as-errors stub build and X11 smoke"
  mkdir -p "$build_root" && build_stub || { restore_rollback; printf '\nFINAL FAIL: deterministic build/UI smoke failed; pre-v23 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "warnings-as-errors stub build and X11 smoke"

  phase_start "full native Nougat Media Suite v0.0.23 build"
  build_full || { restore_rollback; printf '\nFINAL FAIL: full native build failed; pre-v23 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "full native Nougat Media Suite v0.0.23 build"

  phase_start "install and verify root Nougat_Media_Suite_v23 plus exact N identity"
  install_final_candidate || { restore_rollback; printf '\nFINAL FAIL: final executable/icon/launcher verification failed; pre-v23 state restored. Terminal remains open.\n'; return 1; }
  phase_pass "install and verify root Nougat_Media_Suite_v23 plus exact N identity"

  rm -rf -- "$build_root"
  printf '\nFINAL PASS: Nougat Media Suite v0.0.23 exact-concept UI and Stream repair candidate installed and validated.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v23"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'LICENSE STATUS: the already-pushed PolyForm Noncommercial licensing files were preserved unchanged.\n'
  printf 'OWNER VISUAL CHECK REQUIRED: exact concept-sheet buttons/notch/quilt/bar colors/compact volume, centered wide controls, and exact N icon in Files/dock.\n'
  printf 'OWNER STREAM CHECK REQUIRED: one shared Direct Play URL, no redundant Stream Play button, Direct Watch, and the reported YouTube URL.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v23\n' "$project_root"
  return 0
}

main "$@"
