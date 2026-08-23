#!/usr/bin/env bash
# Nougat Media Suite v0.0.37 changed-files installer.
# Base: exact accepted v0.0.36 touched state.
# Focus: System/admin relocation, Home/player visual repair, one logical tuner,
#        native ATSC Watch Live, tuner ownership, PSIP EIT cache, classic guide.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v37.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_37_LIVE_TV_GUIDE_SYSTEM_REPAIR_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_37-$stamp-$$"
canonical_id="com.elderredsoftworks.NougatMediaSuite"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
canonical_launcher="$HOME/.local/share/applications/${canonical_id}.desktop"
launcher_unversioned="$HOME/.local/share/applications/NougatMediaSuite.desktop"
launcher_v36="$HOME/.local/share/applications/NougatMediaSuite_v36.desktop"
launcher_v37="$HOME/.local/share/applications/NougatMediaSuite_v37.desktop"
applied=0

phase_start(){ printf '\n=== %s ===\n' "$1"; }
phase_pass(){ printf 'PASS: %s\n' "$1"; }
require_command(){ command -v "$1" >/dev/null 2>&1 || { printf 'FAIL: required command missing: %s\n' "$1"; return 1; }; }

verify_relative_ai_rpath(){
  local exe="$1" dynamic
  dynamic="$(readelf -d "$exe" 2>/dev/null)" || return 1
  [[ "$dynamic" == *'$ORIGIN/components/ai/runtime/lib'* ]] || { printf 'FAIL: relative AI RPATH missing.\n'; return 1; }
  [[ "$dynamic" != *'/DKLab/Projects/ReddMedia/'* ]] || { printf 'FAIL: obsolete absolute RPATH leaked.\n'; return 1; }
  printf 'Relative AI runtime RPATH verified.\n'
}

cleanup_generated_python_caches(){
  python3 - "$project_root" <<'PY'
import pathlib,shutil,sys
root=pathlib.Path(sys.argv[1]); removed=0
for p in sorted(root.rglob('__pycache__'), key=lambda x: len(x.parts), reverse=True):
    rel=p.relative_to(root).as_posix()
    if rel.startswith(('components/ai/runtime/','components/jellyfin/runtime/','components/security/runtime/')): continue
    if p.is_dir(): shutil.rmtree(p,ignore_errors=True); removed+=1
for p in list(root.rglob('*.pyc')):
    rel=p.relative_to(root).as_posix()
    if rel.startswith(('components/ai/runtime/','components/jellyfin/runtime/','components/security/runtime/')): continue
    try: p.unlink(); removed+=1
    except FileNotFoundError: pass
print(f'Python cache cleanup PASS: removed {removed} project-generated cache item(s).')
PY
}

verify_manifest(){
  python3 - "$payload_root" "$manifest_path" <<'PY'
import hashlib,json,pathlib,sys
root=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2])
if not mp.is_file(): print('FAIL: v37 manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text())
if m.get('target_version')!='0.0.37' or m.get('accepted_base_version')!='0.0.36':
    print('FAIL: manifest identity/base mismatch'); raise SystemExit(1)
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
for rel,r in m.get('payload',{}).items():
    p=root/rel
    if not p.is_file() or p.stat().st_size!=r['bytes'] or sha(p)!=r['sha256']:
        print('FAIL: package payload mismatch:',rel); raise SystemExit(1)
print('Manifest verified:',len(m.get('payload',{})),'payload files.')
PY
}

verify_v36_baseline(){
  local branch
  [[ -d "$project_root" ]] || { printf 'FAIL: project root missing: %s\n' "$project_root"; return 1; }
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  [[ -z "$branch" || "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "$branch"; return 1; }
  python3 - "$project_root" "$manifest_path" <<'PY'
import hashlib,json,pathlib,sys
root=pathlib.Path(sys.argv[1]); m=json.load(open(sys.argv[2]))
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
for rel, spec in m.get('base_state',{}).items():
    p=root/rel
    if spec.get('absent'):
        if p.exists():
            print('FAIL: v0.0.37-new path already exists on baseline:',rel); raise SystemExit(1)
        continue
    if not p.is_file():
        print('FAIL: accepted v0.0.36 baseline file missing:',rel); raise SystemExit(1)
    if p.stat().st_size != spec['bytes'] or sha(p) != spec['sha256']:
        print('FAIL: accepted v0.0.36 baseline mismatch:',rel); raise SystemExit(1)
print('Accepted v0.0.36 touched-state hashes verified.')
PY
  [[ -x "$project_root/Nougat_Media_Suite_v36" ]] || { printf 'FAIL: accepted v0.0.36 root executable missing.\n'; return 1; }
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v36" --version 2>/dev/null)" == "Nougat Media Suite v0.0.36" ]] || { printf 'FAIL: root executable is not accepted v0.0.36.\n'; return 1; }
  HOME="${TMPDIR:-/tmp}/nougat-v36-preflight-home-$$" "$project_root/Nougat_Media_Suite_v36" --v36-library-ui-player-self-test || return 1
  if [[ -e "$project_root/1.zip" || -e "$project_root/2.zip" ]]; then
    printf 'Owner source archives preserved:'
    [[ -e "$project_root/1.zip" ]] && printf ' 1.zip'
    [[ -e "$project_root/2.zip" ]] && printf ' 2.zip'
    printf '\n'
  fi
  printf 'Baseline preflight PASS: exact accepted v0.0.36 touched state and root executable verified.\n'
}

manifest_paths(){
  python3 - "$manifest_path" "$1" <<'PY'
import json,sys
m=json.load(open(sys.argv[1])); key=sys.argv[2]
for x in (m.get('payload',{}) if key=='payload' else m.get(key,[])): print(x)
PY
}

verify_protected_state(){
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  printf 'Protected licensing/Search state preserved.\n'
}

save_rollback(){
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell/applications" || return 1
  local rel src
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    if [[ -e "$project_root/$rel" ]]; then
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1
    fi
  done < <(manifest_paths payload)
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    if [[ -e "$project_root/$rel" ]]; then
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1
    fi
  done < <(manifest_paths remove_after_success)
  for src in "$launcher_unversioned" "$launcher_v36" "$launcher_v37" "$canonical_launcher"; do
    [[ -e "$src" ]] && cp -a "$src" "$rollback_root/user-shell/applications/$(basename "$src")"
  done
  printf 'Base: exact accepted v0.0.36 touched state\nTarget: v0.0.37\nSnapshot preserves every v0.0.36 path touched or removed by this installer.\n' > "$rollback_root/ROLLBACK_INFO.txt"
}

apply_payload(){
  local rel
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || return 1
  done < <(manifest_paths payload)
  cp -a "$manifest_path" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v37.json" || return 1
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_37.sh" \
           "$project_root/tools/test_nougat_media_suite_v37.py" \
           "$project_root/tools/test_installer_rollback_v37.py" || return 1
  applied=1
}

restore_rollback(){
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring exact accepted v0.0.36 touched state\n'
  local rel backup dest
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    if [[ -e "$backup" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$backup" "$project_root/$rel"
    else
      rm -rf -- "$project_root/$rel"
    fi
  done < <(manifest_paths payload)
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    if [[ -e "$backup" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$backup" "$project_root/$rel"
    fi
  done < <(manifest_paths remove_after_success)
  rm -f -- "$project_root/Nougat_Media_Suite_v37" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v37.json"
  for dest in "$launcher_unversioned" "$launcher_v36" "$launcher_v37" "$canonical_launcher"; do
    backup="$rollback_root/user-shell/applications/$(basename "$dest")"
    if [[ -e "$backup" ]]; then
      mkdir -p "$(dirname "$dest")"
      cp -a "$backup" "$dest"
    else
      rm -f -- "$dest"
    fi
  done
  printf 'ROLLBACK PASS: exact accepted v0.0.36 touched state restored. Generated runtimes, owner archives, and user data preserved.\n'
}

run_source_tests(){
  verify_protected_state || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v37.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v37.py" "$project_root" || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_diagnostics_v26.py" "$project_root" || return 1
}

run_regression_exe_tests(){
  local exe="$1"
  "$exe" --discover-ai-self-test || return 1
  "$exe" --v25-ui-state-self-test || return 1
  "$exe" --v28-ui-state-self-test || return 1
  "$exe" --v29-tv-reliability-self-test || return 1
  "$exe" --v30-ui-library-player-self-test || return 1
  "$exe" --v31-ui-sheet-self-test || return 1
  "$exe" --v32-p2p-player-repair-self-test || return 1
  HOME="$build_root/test-home-v33" "$exe" --v33-integration-self-test || return 1
  HOME="$build_root/test-home-v34" "$exe" --v34-ui-polish-self-test || return 1
  HOME="$build_root/test-home-v35" "$exe" --v35-cleanup-self-test || return 1
  HOME="$build_root/test-home-v36" "$exe" --v36-library-ui-player-self-test || return 1
  HOME="$build_root/test-home-v37" "$exe" --v37-live-tv-system-self-test || return 1
}

build_stub(){
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  local exe="$build_root/stub/Nougat_Media_Suite_v37"
  [[ "$("$exe" --version)" == "Nougat Media Suite v0.0.37" ]] || return 1
  run_regression_exe_tests "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v37.py" "$project_root" "$exe" || return 1
}

verify_security_runtime(){
  if python3 "$project_root/tools/install_nougat_security_runtime_v33.py" "$project_root" --check; then
    python3 "$project_root/tools/test_nougat_security_analysis_v33.py" "$project_root" || return 1
    return 0
  fi
  printf 'Retained security runtime is missing or incomplete; rebuilding the pinned one-shot runtime.\n'
  python3 "$project_root/tools/install_nougat_security_runtime_v33.py" "$project_root" || return 1
  python3 "$project_root/tools/install_nougat_security_runtime_v33.py" "$project_root" --check || return 1
  python3 "$project_root/tools/test_nougat_security_analysis_v33.py" "$project_root" || return 1
}

build_full(){
  cmake -S "$project_root" -B "$build_root/full" -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/full" -j"$(nproc)" || return 1
  mkdir -p "$build_root/full/components/ai" "$build_root/full/components/jellyfin" || return 1
  ln -s "$project_root/components/ai/runtime" "$build_root/full/components/ai/runtime" || return 1
  ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models" || return 1
  ln -s "$project_root/components/jellyfin/runtime" "$build_root/full/components/jellyfin/runtime" || return 1
  local exe="$build_root/full/Nougat_Media_Suite_v37"
  verify_relative_ai_rpath "$exe" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.37" ]] || return 1
  run_regression_exe_tests "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v37.py" "$project_root" "$exe" || return 1
}

install_final(){
  local built="$build_root/full/Nougat_Media_Suite_v37"
  [[ -x "$built" ]] || { printf 'FAIL: full native v0.0.37 executable missing.\n'; return 1; }
  cp -a "$built" "$project_root/Nougat_Media_Suite_v37" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v37" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v37" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v37" --version 2>/dev/null)" == "Nougat Media Suite v0.0.37" ]] || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v37.py" "$project_root" "$project_root/Nougat_Media_Suite_v37" || return 1
}

install_launchers(){
  mkdir -p "$HOME/.local/share/applications" || return 1
  cp -a "$project_root/NougatMediaSuite.desktop" "$launcher_unversioned" || return 1
  cp -a "$project_root/NougatMediaSuite_v37.desktop" "$launcher_v37" || return 1
  cp -a "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" "$canonical_launcher" || return 1
  rm -f -- "$launcher_v36"
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
}

apply_raw_icon(){
  local exe="$1"
  if command -v gio >/dev/null 2>&1 && [[ -f "$master_icon" ]]; then
    gio set -t string "$exe" metadata::custom-icon "file://$master_icon" >/dev/null 2>&1 || return 1
  fi
  return 0
}

remove_old_version_files(){
  local rel
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    rm -rf -- "$project_root/$rel" || return 1
  done < <(manifest_paths remove_after_success)
}

main(){
  phase_start "v0.0.37 accepted-v0.0.36 preflight"
  require_command git || return 1
  require_command python3 || return 1
  require_command cmake || return 1
  require_command readelf || return 1
  require_command g++ || return 1
  cleanup_generated_python_caches || return 1
  verify_manifest || return 1
  verify_v36_baseline || return 1
  phase_pass "v0.0.37 accepted-v0.0.36 preflight"

  phase_start "save exact v0.0.36 touched-state rollback snapshot"
  save_rollback || return 1
  phase_pass "save exact v0.0.36 touched-state rollback snapshot"

  phase_start "apply v0.0.37 changed files"
  if ! apply_payload || ! run_source_tests; then
    restore_rollback
    return 1
  fi
  phase_pass "apply v0.0.37 changed files and source/regression tests"

  phase_start "warnings-as-errors stub build"
  if ! build_stub; then
    restore_rollback
    return 1
  fi
  phase_pass "warnings-as-errors stub build"

  phase_start "retained one-shot security runtime"
  if ! verify_security_runtime; then
    restore_rollback
    return 1
  fi
  phase_pass "retained one-shot security runtime"

  phase_start "full native v0.0.37 build"
  if ! build_full; then
    restore_rollback
    return 1
  fi
  phase_pass "full native v0.0.37 build"

  phase_start "install final v0.0.37 executable and launchers"
  if ! install_final || ! install_launchers || ! apply_raw_icon "$project_root/Nougat_Media_Suite_v37" || ! remove_old_version_files; then
    restore_rollback
    return 1
  fi
  phase_pass "install final v0.0.37 executable and launchers"

  printf '\nFINAL PASS: Nougat Media Suite v0.0.37 installed and validated.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v37"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'SYSTEM: Debug renamed to System; Start/Stop/Refresh Server moved out of Library.\n'
  printf 'LIBRARY: working Search field + Search button retained below the green action row.\n'
  printf 'HOME: Continue Watching cards now use the same card-size family as the remaining Home cards.\n'
  printf 'PLAYER: wide responsive exact-sheet seek with side timestamps; accepted VOLUME preserved; pale sprite halos suppressed.\n'
  printf 'HEADER: stitched whole-face Server state circle active.\n'
  printf 'LIVE TV: one logical DVB tuner, channel selection/double-click Watch Live, tuner ownership, persisted PSIP EIT guide cache, and first classic guide grid active.\n'
  printf 'NEXT BUILD AGENDA: unify pointer + media-title/info overlay under one 3-second activity timer in fullscreen, maximized, normal, and half-screen/resized playback.\n'
  printf 'OWNER CHECK REQUIRED: visual fidelity, Library Search, System controls, retained 66-channel scan, real hardware Watch Live, and broadcast guide population/grid behavior.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v37\n' "$project_root"
  return 0
}

main "$@"
