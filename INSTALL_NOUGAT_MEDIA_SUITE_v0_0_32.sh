#!/usr/bin/env bash

# Nougat Media Suite v0.0.32 changed-files installer.
# Approved scope: native Search > P2P, Nougat Security Analysis, plus owner-reported player/UI repairs.
# Security policy is WARN ME FIRST: nothing is auto-quarantined, moved, renamed, or deleted.
# Existing accepted v0.0.31 UI-sheet component family, page/service palettes, Home cards, Library, Discover and licensing boundaries are preserved.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v32.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_32_P2P_SECURITY_ANALYSIS_AND_PLAYER_UI_REPAIR_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_32-$stamp-$$"
expected_head="b22d11d4989784dc6df56abbde08344720064790"
canonical_id="com.elderredsoftworks.NougatMediaSuite"
icon_key="nougat-media-suite-concept-sheet-v24"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
pinned_model="$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_bytes="84106624"
pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
canonical_launcher="$HOME/.local/share/applications/${canonical_id}.desktop"
launcher_unversioned="$HOME/.local/share/applications/NougatMediaSuite.desktop"
launcher_v31="$HOME/.local/share/applications/NougatMediaSuite_v31.desktop"
launcher_v32="$HOME/.local/share/applications/NougatMediaSuite_v32.desktop"
applied=0
base_mode=""

modified_paths=(
  ".gitignore"
  "APPLY_COMMAND.txt"
  "COMPANY_BIBLE.md"
  "DEPENDENCIES.md"
  "CHANGELOG.md"
  "CMakeLists.txt"
  "README.md"
  "ROADMAP.md"
  "NougatMediaSuite.desktop"
  "NougatMediaSuite_v32.desktop"
  "com.elderredsoftworks.NougatMediaSuite.desktop"
  "src/main.cpp"
  "src/p2p_engine.cpp"
  "src/p2p_engine.hpp"
  "src/p2p_stream_server.cpp"
  "src/diagnostics/diagnostic_engine.cpp"
  "src/diagnostics/diagnostic_types.hpp"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_32_NATIVE_P2P_MEDIA_AND_PLAYER_UI_REPAIR_HANDSHAKE.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_32_NATIVE_P2P_MEDIA_AND_PLAYER_UI_REPAIR_VALIDATION.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_32_NATIVE_P2P_SECURITY_ANALYSIS_REPLACEMENT_HANDSHAKE.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_32_NATIVE_P2P_SECURITY_ANALYSIS_REPLACEMENT_VALIDATION.md"
  "tools/test_nougat_media_suite_retained_v31.py"
  "tools/test_nougat_media_suite_v32.py"
  "tools/test_nougat_media_suite_ui_smoke_v32.py"
  "tools/test_p2p_stream_server_v32.py"
  "tools/test_installer_rollback_v32.py"
  "components/security/README.md"
  "components/security/nougat_security_worker.py"
  "components/security/rules/nougat_core.yar"
  "docs/security/NOUGAT_SECURITY_ANALYSIS_DEPENDENCIES.md"
  "tools/install_nougat_security_runtime_v32.py"
  "tools/test_nougat_security_analysis_v32.py"
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_32.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v32.json"
)

superseded_paths=(
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_31.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v31.json"
  "NougatMediaSuite_v31.desktop"
)

icon_alias_keys=("nougat-media-suite" "NougatMediaSuite" "$canonical_id")

phase_start() { printf '\n=== %s ===\n' "$1"; }
phase_pass() { printf 'PASS: %s\n' "$1"; }
require_command() { command -v "$1" >/dev/null 2>&1 || { printf 'FAIL: required command missing: %s\n' "$1"; return 1; }; }

verify_port_8096_free() {
  python3 - <<'PY'
import socket
s=socket.socket(); s.settimeout(.25)
used=s.connect_ex(('127.0.0.1',8096))==0
s.close()
if used:
    print('FAIL: localhost port 8096 is in use. Close Nougat Media Suite/Jellyfin before applying v0.0.32.')
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
  [[ "$head" == "$expected_head" ]] || { printf 'FAIL: expected accepted v0.0.31 HEAD %s, found %s\n' "$expected_head" "${head:-unknown}"; return 1; }
  if [[ -z "$dirty" ]]; then
    base_mode="v31"
    printf 'Git preflight PASS: clean main at accepted v0.0.31 commit %s.\n' "$expected_head"
    return 0
  fi

  # The owner may already be running the exact first v0.0.32 candidate. Accept
  # only that known overlay, never an arbitrary dirty worktree.
  python3 - "$project_root" "$manifest_path" <<'PYINNER'
import hashlib,json,pathlib,subprocess,sys
project=pathlib.Path(sys.argv[1]); manifest=json.loads(pathlib.Path(sys.argv[2]).read_text(encoding='utf-8'))
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
expected=set(manifest.get('accepted_candidate_files',{})) | set(manifest.get('accepted_candidate_removed',[])) | {'Nougat_Media_Suite_v32'}
for rel,record in manifest.get('accepted_candidate_files',{}).items():
    p=project/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print('FAIL: dirty worktree is not the exact first v0.0.32 candidate:',rel); raise SystemExit(1)
for rel in manifest.get('accepted_candidate_removed',[]):
    if (project/rel).exists():
        print('FAIL: first-v0.0.32 candidate expected removed path still exists:',rel); raise SystemExit(1)
tracked=subprocess.check_output(['git','-C',str(project),'diff','--name-only'],text=True).splitlines()
untracked=subprocess.check_output(['git','-C',str(project),'ls-files','--others','--exclude-standard'],text=True).splitlines()
actual=set(x.strip() for x in tracked+untracked if x.strip())
unexpected=sorted(actual-expected)
if unexpected:
    print('FAIL: worktree has changes outside the exact first-v0.0.32 candidate:')
    for x in unexpected: print('  '+x)
    raise SystemExit(1)
print('Exact first v0.0.32 candidate overlay verified.')
PYINNER
  [[ "$?" -eq 0 ]] || return 1
  [[ -x "$project_root/Nougat_Media_Suite_v32" ]] || { printf 'FAIL: first-v0.0.32 candidate root executable missing/not executable.\n'; return 1; }
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --version 2>/dev/null)" == "Nougat Media Suite v0.0.32" ]] || { printf 'FAIL: first-v0.0.32 candidate executable version check failed.\n'; return 1; }
  base_mode="candidate-v32"
  printf 'Git preflight PASS: exact first v0.0.32 candidate overlay at accepted v0.0.31 HEAD %s.\n' "$expected_head"
}

verify_manifest() {
  python3 - "$payload_root" "$manifest_path" "$project_root" "$base_mode" <<'PYINNER'
import hashlib,json,pathlib,sys
payload=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2]); project=pathlib.Path(sys.argv[3]); mode=sys.argv[4]
if not mp.is_file(): print('FAIL: package manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text(encoding='utf-8'))
if m.get('target_version') != '0.0.32': print('FAIL: manifest target version mismatch'); raise SystemExit(1)
if m.get('accepted_base_commit') != 'b22d11d4989784dc6df56abbde08344720064790': print('FAIL: manifest accepted base mismatch'); raise SystemExit(1)
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
for rel,record in m.get('payload',{}).items():
    p=payload/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: package payload mismatch: {rel}'); raise SystemExit(1)
if mode == 'v31':
    for rel,record in m.get('base_files',{}).items():
        p=project/rel
        if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
            print(f'FAIL: accepted v0.0.31 base differs: {rel}'); raise SystemExit(1)
    for rel in m.get('required_absent',[]):
        if (project/rel).exists(): print(f'FAIL: expected accepted-v0.0.31-absent path exists: {rel}'); raise SystemExit(1)
elif mode == 'candidate-v32':
    for rel,record in m.get('accepted_candidate_files',{}).items():
        p=project/rel
        if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
            print(f'FAIL: first-v0.0.32 candidate differs: {rel}'); raise SystemExit(1)
    for rel in m.get('accepted_candidate_removed',[]):
        if (project/rel).exists(): print(f'FAIL: first-v0.0.32 candidate removed path returned: {rel}'); raise SystemExit(1)
else:
    print('FAIL: unknown installer base mode'); raise SystemExit(1)
print(f"Manifest verified for {mode}: {len(m.get('payload',{}))} replacement payload files.")
PYINNER
}

verify_protected_state() {
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 - "$project_root" <<'PYPROTECT'
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
'src/nougat/nougat_bridge.hpp':'46a7c446fc3c8fc02bbbe9c012a589d5ee4e79d6e9641b820a949d9529c2842e'}
for rel,want in expected.items():
    got=hashlib.sha256((root/rel).read_bytes()).hexdigest()
    if got!=want: print('FAIL: protected file changed:',rel); raise SystemExit(1)
print('Licensing and decentralized Search engine/bridge protected state preserved for v0.0.32; P2P implementation is intentionally in scope.')
PYPROTECT
}

save_rollback_snapshot() {
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell/applications" "$rollback_root/user-shell/icons" "$rollback_root/user-shell/pixmaps" || return 1
  local rel src size key
  for rel in "${modified_paths[@]}" "${superseded_paths[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1
    fi
  done
  if [[ -e "$project_root/Nougat_Media_Suite_v31" ]]; then cp -a "$project_root/Nougat_Media_Suite_v31" "$rollback_root/project/Nougat_Media_Suite_v31" || return 1; fi
  if [[ -e "$project_root/Nougat_Media_Suite_v32" ]]; then cp -a "$project_root/Nougat_Media_Suite_v32" "$rollback_root/project/Nougat_Media_Suite_v32" || return 1; fi
  for src in "$launcher_unversioned" "$launcher_v31" "$launcher_v32" "$canonical_launcher"; do
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
Accepted Git base: v0.0.31 $expected_head
Encountered install base mode: $base_mode
Target candidate: v0.0.32
Package scope: native Search > P2P media expansion, Nougat Security Analysis with WARN ME FIRST behavior, plus autoplay flicker, volume geometry, Search seam contrast, Crawler status placement, Node-ID cleanup, seed availability and Stream panel-border repairs; accepted v0.0.31 UI-sheet family/page palettes retained.
INFO
}

apply_payload() {
  local rel
  for rel in "${modified_paths[@]}"; do
    [[ "$rel" == "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v32.json" ]] && continue
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || { printf 'FAIL: could not apply %s\n' "$rel"; return 1; }
  done
  cp -a "$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v32.json" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v32.json" || return 1
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_32.sh" \
    "$project_root/tools/test_nougat_media_suite_retained_v31.py" \
    "$project_root/tools/test_nougat_media_suite_v32.py" \
    "$project_root/tools/test_nougat_media_suite_ui_smoke_v32.py" \
    "$project_root/tools/test_p2p_stream_server_v32.py" \
    "$project_root/tools/test_installer_rollback_v32.py" \
    "$project_root/tools/install_nougat_security_runtime_v32.py" \
    "$project_root/tools/test_nougat_security_analysis_v32.py" \
    "$project_root/components/security/nougat_security_worker.py" || return 1
  applied=1
}

restore_rollback() {
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring exact accepted v0.0.31 touched state\n'
  local rel dest backup size key
  for rel in "${modified_paths[@]}" "${superseded_paths[@]}"; do
    if [[ -e "$rollback_root/project/$rel" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$rollback_root/project/$rel" "$project_root/$rel"
    else
      rm -rf -- "$project_root/$rel"
    fi
  done
  rm -f -- "$project_root/Nougat_Media_Suite_v32"
  if [[ -e "$rollback_root/project/Nougat_Media_Suite_v32" ]]; then cp -a "$rollback_root/project/Nougat_Media_Suite_v32" "$project_root/Nougat_Media_Suite_v32"; chmod +x "$project_root/Nougat_Media_Suite_v32" 2>/dev/null || true; fi
  if [[ -e "$rollback_root/project/Nougat_Media_Suite_v31" ]]; then cp -a "$rollback_root/project/Nougat_Media_Suite_v31" "$project_root/Nougat_Media_Suite_v31"; chmod +x "$project_root/Nougat_Media_Suite_v31" 2>/dev/null || true; fi
  for dest in "$launcher_unversioned" "$launcher_v31" "$launcher_v32" "$canonical_launcher"; do
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
  printf 'ROLLBACK PASS: accepted v0.0.31 touched state restored.\n'
}

run_source_tests() {
  verify_protected_state || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v31.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_diagnostics_v26.py" "$project_root" || return 1
  python3 "$project_root/tools/test_media_server_lifecycle_v17.py" "$project_root" || return 1
}

build_stub() {
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  local exe="$build_root/stub/Nougat_Media_Suite_v32"
  [[ "$("$exe" --version)" == "Nougat Media Suite v0.0.32" ]] || { printf 'FAIL: v32 stub version mismatch\n'; return 1; }
  "$exe" --discover-ai-self-test || return 1
  "$exe" --v25-ui-state-self-test || return 1
  "$exe" --v28-ui-state-self-test || return 1
  "$exe" --v29-tv-reliability-self-test || return 1
  "$exe" --v30-ui-library-player-self-test || return 1
  "$exe" --v31-ui-sheet-self-test || return 1
  "$exe" --v32-p2p-player-repair-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v31.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v32.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v32.py" "$project_root" "$exe" || return 1
}

build_full() {
  cmake -S "$project_root" -B "$build_root/full" -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/full" -j"$(nproc)" || return 1
  mkdir -p "$build_root/full/components/ai" || return 1
  rm -rf -- "$build_root/full/components/ai/runtime" "$build_root/full/components/ai/models"
  ln -s "$project_root/components/ai/runtime" "$build_root/full/components/ai/runtime" || return 1
  ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models" || return 1
  local exe="$build_root/full/Nougat_Media_Suite_v32"
  verify_relative_ai_rpath "$exe" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.32" ]] || { printf 'FAIL: full native v32 version/runtime check failed\n'; return 1; }
  env -u LD_LIBRARY_PATH "$exe" --discover-ai-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v25-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v28-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v29-tv-reliability-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v30-ui-library-player-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v31-ui-sheet-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v32-p2p-player-repair-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v31.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v32.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_security_analysis_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
}

install_icon_aliases() {
  local size key src
  for size in 16 32 48 64 128 256 512; do
    src="$project_root/assets/icons/nougat-media-suite-${size}.png"
    [[ -f "$src" ]] || src="$master_icon"
    mkdir -p "$HOME/.local/share/icons/hicolor/${size}x${size}/apps" || return 1
    for key in "${icon_alias_keys[@]}"; do cp -a "$src" "$HOME/.local/share/icons/hicolor/${size}x${size}/apps/${key}.png" || return 1; done
  done
  mkdir -p "$HOME/.local/share/pixmaps" || return 1
  for key in "${icon_alias_keys[@]}"; do cp -a "$master_icon" "$HOME/.local/share/pixmaps/${key}.png" || return 1; done
  command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
}

install_launchers() {
  mkdir -p "$HOME/.local/share/applications" || return 1
  cp -a "$project_root/NougatMediaSuite.desktop" "$launcher_unversioned" || return 1
  cp -a "$project_root/NougatMediaSuite_v32.desktop" "$launcher_v32" || return 1
  cp -a "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" "$canonical_launcher" || return 1
  rm -f -- "$launcher_v31"
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
}

refresh_nougat_favorite() {
  command -v gsettings >/dev/null 2>&1 || return 0
  python3 - <<'PY'
import ast, subprocess
schema='org.gnome.shell'; key='favorite-apps'; canonical='com.elderredsoftworks.NougatMediaSuite.desktop'
legacy={'NougatMediaSuite.desktop','NougatMediaSuite_v31.desktop','NougatMediaSuite_v32.desktop',canonical}
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
if changed: subprocess.run(['gsettings','set',schema,key,str(out)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
PY
}

apply_raw_icon() {
  local exe="$1" info
  gio set -t string "$exe" metadata::custom-icon "file://$master_icon" || return 1
  info="$(gio info -a metadata::custom-icon "$exe" 2>&1)" || return 1
  [[ "$info" == *"file://$master_icon"* ]] || { printf 'FAIL: v32 executable custom-icon readback mismatch\n%s\n' "$info"; return 1; }
  printf 'Raw executable approved N custom-icon metadata verified after final write.\n'
}

install_final_candidate() {
  cp "$build_root/full/Nougat_Media_Suite_v32" "$project_root/Nougat_Media_Suite_v32" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v32" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v32" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --version 2>/dev/null)" == "Nougat Media Suite v0.0.32" ]] || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --discover-ai-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --v25-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --v28-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --v29-tv-reliability-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --v30-ui-library-player-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --v31-ui-sheet-self-test || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --v32-p2p-player-repair-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v31.py" "$project_root" "$project_root/Nougat_Media_Suite_v32" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v32.py" "$project_root" "$project_root/Nougat_Media_Suite_v32" || return 1
  python3 "$project_root/tools/test_nougat_security_analysis_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v32.py" "$project_root" "$project_root/Nougat_Media_Suite_v32" || return 1
  install_icon_aliases || return 1
  install_launchers || return 1
  refresh_nougat_favorite || return 1
  apply_raw_icon "$project_root/Nougat_Media_Suite_v32" || return 1
  rm -f -- "$project_root/Nougat_Media_Suite_v31" || return 1
  local rel
  for rel in "${superseded_paths[@]}"; do rm -f -- "$project_root/$rel" || return 1; done
  command -v nautilus >/dev/null 2>&1 && nautilus -q >/dev/null 2>&1 || true
  return 0
}

main() {
  phase_start "accepted v0.0.31 base and v0.0.32 prerequisites"
  [[ -d "$project_root" ]] || { printf 'FAIL: project not found: %s\nFINAL FAIL: v0.0.32 not installed. Terminal remains open.\n' "$project_root"; return 1; }
  local cmd
  for cmd in cmake g++ pkg-config python3 gio git env readelf sha256sum tar xvfb-run xwininfo xprop ffmpeg; do require_command "$cmd" || { printf 'FINAL FAIL: prerequisites incomplete. Terminal remains open.\n'; return 1; }; done
  pkg-config --exists libtorrent-rasterbar || { printf 'FAIL: libtorrent-rasterbar development package unavailable.\nFINAL FAIL: prerequisites incomplete. Terminal remains open.\n'; return 1; }
  verify_port_8096_free || { printf 'FINAL FAIL: runtime preflight failed. Terminal remains open.\n'; return 1; }
  verify_git_state || { printf 'FINAL FAIL: Git/worktree preflight failed. Terminal remains open.\n'; return 1; }
  if [[ "$base_mode" == "v31" ]]; then
    [[ -x "$project_root/Nougat_Media_Suite_v31" ]] || { printf 'FAIL: accepted v0.0.31 root executable missing/not executable.\n'; return 1; }
    [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v31" --version 2>/dev/null)" == "Nougat Media Suite v0.0.31" ]] || { printf 'FAIL: accepted v0.0.31 executable version/runtime check failed.\n'; return 1; }
  elif [[ "$base_mode" == "candidate-v32" ]]; then
    [[ -x "$project_root/Nougat_Media_Suite_v32" ]] || { printf 'FAIL: first-v0.0.32 candidate root executable missing/not executable.\n'; return 1; }
    [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v32" --version 2>/dev/null)" == "Nougat Media Suite v0.0.32" ]] || { printf 'FAIL: first-v0.0.32 candidate executable version/runtime check failed.\n'; return 1; }
  else
    printf 'FAIL: unknown installer base mode: %s\n' "$base_mode"
    return 1
  fi
  [[ -f "$project_root/components/ai/runtime/include/llama.h" && -e "$project_root/components/ai/runtime/lib/libllama.so.0" ]] || { printf 'FAIL: AI runtime missing.\n'; return 1; }
  [[ -f "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]] || { printf 'FAIL: integrated Jellyfin runtime missing.\n'; return 1; }
  verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha" || { printf 'FAIL: pinned Nomic model missing/changed.\n'; return 1; }
  verify_protected_state || { printf 'FINAL FAIL: protected accepted-v0.0.31 state mismatch. Terminal remains open.\n'; return 1; }
  verify_manifest || { printf 'FINAL FAIL: v0.0.32 package/base verification failed. Terminal remains open.\n'; return 1; }
  phase_pass "accepted v0.0.31 base and v0.0.32 prerequisites"

  phase_start "save exact accepted v0.0.31 rollback snapshot"
  save_rollback_snapshot || { printf 'FINAL FAIL: rollback snapshot failed before changes. Terminal remains open.\n'; return 1; }
  phase_pass "save exact accepted v0.0.31 rollback snapshot"

  phase_start "apply approved Nougat Media Suite v0.0.32 P2P/player/UI changed files"
  apply_payload || { restore_rollback; printf 'FINAL FAIL: v0.0.32 payload application failed. Terminal remains open.\n'; return 1; }
  phase_pass "apply approved Nougat Media Suite v0.0.32 P2P/player/UI changed files"

  phase_start "protected-state, retained v0.0.31 behavior, v0.0.32 P2P/player/UI and HTTP Range tests"
  run_source_tests || { restore_rollback; printf 'FINAL FAIL: source/regression validation failed; accepted v0.0.31 touched state restored. Terminal remains open.\n'; return 1; }
  phase_pass "protected-state, retained v0.0.31 behavior, v0.0.32 P2P/security/player/UI and HTTP Range tests"

  phase_start "verify one-shot Nougat Security Analysis scaffolding"
  python3 -m py_compile "$project_root/components/security/nougat_security_worker.py" "$project_root/tools/install_nougat_security_runtime_v32.py" || { restore_rollback; printf 'FINAL FAIL: security scaffold syntax check failed; accepted v0.0.31 touched state restored. Terminal remains open.\n'; return 1; }
  python3 "$project_root/tools/test_nougat_security_analysis_v32.py" "$project_root" || { restore_rollback; printf 'FINAL FAIL: security scaffold validation failed; accepted v0.0.31 touched state restored. Terminal remains open.\n'; return 1; }
  phase_pass "verify one-shot Nougat Security Analysis scaffolding (full YARA-X/capa/Magika runtime intentionally deferred)"

  phase_start "warnings-as-errors stub build, P2P/security contract tests and X11 v0.0.32 identity/UI smoke"
  mkdir -p "$build_root" && build_stub || { restore_rollback; printf 'FINAL FAIL: deterministic build/UI validation failed; accepted v0.0.31 touched state restored. Terminal remains open.\n'; return 1; }
  phase_pass "warnings-as-errors stub build, P2P/security contract tests and X11 v0.0.32 identity/UI smoke"

  phase_start "full native Nougat Media Suite v0.0.32 rebuild"
  build_full || { restore_rollback; printf 'FINAL FAIL: full native v0.0.32 build failed; accepted v0.0.31 touched state restored. Terminal remains open.\n'; return 1; }
  phase_pass "full native Nougat Media Suite v0.0.32 rebuild"

  phase_start "install and verify final v32 executable, launchers, and N identity"
  install_final_candidate || { restore_rollback; printf 'FINAL FAIL: final v0.0.32 installation failed; accepted v0.0.31 touched state restored. Terminal remains open.\n'; return 1; }
  phase_pass "install and verify final v32 executable, launchers, and N identity"

  rm -rf -- "$build_root"
  printf '\nFINAL PASS: Nougat Media Suite v0.0.32 P2P + Nougat Security Analysis + Player/UI Repair installed and validated.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v32"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'PALETTE STATUS: accepted v0.0.31 page/service palette values preserved; Search cream seam ink darkened only.\n'
  printf 'BEHAVIOR STATUS: accepted v0.0.31 Home cards, Library/cache, Discover, Stream providers and licensing retained; P2P, Virus Scan/Security Analysis and approved repair paths intentionally changed.\n'
  printf 'ROADMAP STATUS: Live TV/HDHomeRun/ATSC 3.0 and Radio/SDR/CB reception recorded for future builds only.\n'
  printf 'SECURITY STATUS: one-shot scanner scaffolding active; no resident daemon; WARN ME FIRST; no automatic quarantine/move/delete; full pinned YARA-X/capa/Magika runtime deferred to the next security hardening pass.\n'
printf 'OWNER CHECK REQUIRED: verify P2P magnet/.torrent playback and seek behavior, seeding/availability display, Virus Scan file/folder scans and scanner process exit, autoplay countdown no-flash, seek-style volume bar, Home fixed-header clipping plus right-side vertical and Continue Watching horizontal scrollbars, brown Search seam contrast, Crawler status placement, Node-ID cleanup, and bottom-only Stream activity panel border across YouTube/Vimeo/Rumble/RuTube/VK/OK.\n'
  printf 'CANDIDATE STATUS: v0.0.32 remains unaccepted until owner real-machine visual/use approval.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v32\n' "$project_root"
}

main "$@"
