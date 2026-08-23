#!/usr/bin/env bash
# Nougat Media Suite v0.0.34 changed-files installer.
# Base: accepted v0.0.33 commit 6763a42bf5c125974e5a2882234fb2ee2e04c512.
# Focus: exact-sheet tabs/player controls + Home/Discover/UI repair.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v34.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_34_EXACT_SHEET_UI_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_34-$stamp-$$"
expected_head="6763a42bf5c125974e5a2882234fb2ee2e04c512"
canonical_id="com.elderredsoftworks.NougatMediaSuite"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
pinned_model="$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_bytes="84106624"
pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
canonical_launcher="$HOME/.local/share/applications/${canonical_id}.desktop"
launcher_unversioned="$HOME/.local/share/applications/NougatMediaSuite.desktop"
launcher_v33="$HOME/.local/share/applications/NougatMediaSuite_v33.desktop"
launcher_v34="$HOME/.local/share/applications/NougatMediaSuite_v34.desktop"
applied=0

phase_start(){ printf '\n=== %s ===\n' "$1"; }
phase_pass(){ printf 'PASS: %s\n' "$1"; }
require_command(){ command -v "$1" >/dev/null 2>&1 || { printf 'FAIL: required command missing: %s\n' "$1"; return 1; }; }

verify_file_exact(){ python3 - "$1" "$2" "$3" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); size=int(sys.argv[2]); expected=sys.argv[3]
if not p.is_file() or p.stat().st_size!=size: raise SystemExit(1)
h=hashlib.sha256()
with p.open('rb') as f:
    for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
raise SystemExit(0 if h.hexdigest()==expected else 1)
PY
}

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
    # Never delete caches inside generated runtime environments; those are runtime-owned.
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

verify_git_state(){
  local branch head
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  [[ "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "${branch:-unknown}"; return 1; }
  [[ "$head" == "$expected_head" ]] || { printf 'FAIL: expected accepted v0.0.33 HEAD %s, found %s\n' "$expected_head" "${head:-unknown}"; return 1; }
  git -C "$project_root" diff --quiet || { printf 'FAIL: tracked worktree changes exist.\n'; return 1; }
  git -C "$project_root" diff --cached --quiet || { printf 'FAIL: staged changes exist.\n'; return 1; }
  python3 - "$project_root" <<'PY'
import subprocess,sys
root=sys.argv[1]
allowed=('components/ai/runtime/','components/jellyfin/runtime/','components/security/runtime/')
items=subprocess.check_output(['git','-C',root,'ls-files','--others','--exclude-standard'],text=True).splitlines()
bad=[x for x in items if not x.startswith(allowed)]
if bad:
 print('FAIL: unexpected untracked project files:')
 for x in bad: print('  '+x)
 raise SystemExit(1)
print('Git preflight PASS: accepted v0.0.33 HEAD with generated runtime directories only.')
PY
}

verify_manifest(){ python3 - "$payload_root" "$manifest_path" <<'PY'
import hashlib,json,pathlib,sys
root=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2])
if not mp.is_file(): print('FAIL: v34 manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text())
if m.get('target_version')!='0.0.34' or m.get('accepted_base_commit')!='6763a42bf5c125974e5a2882234fb2ee2e04c512':
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

verify_protected_state(){
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 - "$project_root" <<'PY'
import hashlib,pathlib,sys
root=pathlib.Path(sys.argv[1]); expected={
'LICENSE':'640f0f231aef885a21da0ff4eaf2cc29efda72a5d0702c52cc62476317090d84',
'COPYRIGHT.md':'f0f741eabd0e861a88fd2e2d3c8fc59a0c51ab53379e7f2be0b799b7a7a4ee31',
'CONTRIBUTING.md':'7e31d96229c25a287f22fe508180c2a94dd022ba5c6f6f2256f456de926bcfcb',
'THIRD_PARTY_NOTICES.md':'9def5008c33b202695a52d10772f7836bbd2939826da004f188f787b5dcddf1f',
'docs/LICENSING_POLICY.md':'e7fd56582d8f32154845b3e87a8fe0ed609a8ca626065800d9d8dd14128c50ff'}
for rel,want in expected.items():
 got=hashlib.sha256((root/rel).read_bytes()).hexdigest()
 if got!=want: print('FAIL: protected file changed:',rel); raise SystemExit(1)
print('Protected licensing state preserved.')
PY
}

manifest_paths(){ python3 - "$manifest_path" "$1" <<'PY'
import json,sys
m=json.load(open(sys.argv[1])); key=sys.argv[2]
if key=='payload':
 for x in m.get('payload',{}): print(x)
else:
 for x in m.get(key,[]): print(x)
PY
}

save_rollback(){
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell/applications" || return 1
  local rel src
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    if [[ -e "$project_root/$rel" ]]; then mkdir -p "$rollback_root/project/$(dirname "$rel")"; cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1; fi
  done < <(manifest_paths payload)
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    if [[ -e "$project_root/$rel" ]]; then mkdir -p "$rollback_root/project/$(dirname "$rel")"; cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1; fi
  done < <(manifest_paths remove_after_success)
  for src in "$launcher_unversioned" "$launcher_v33" "$launcher_v34" "$canonical_launcher"; do
    [[ -e "$src" ]] && cp -a "$src" "$rollback_root/user-shell/applications/$(basename "$src")"
  done
  printf 'Accepted base: v0.0.33 %s\nTarget: v0.0.34\n' "$expected_head" > "$rollback_root/ROLLBACK_INFO.txt"
}

apply_payload(){
  local rel
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || return 1
  done < <(manifest_paths payload)
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_34.sh" "$project_root/tools/test_nougat_media_suite_v34.py" "$project_root/tools/test_installer_rollback_v34.py" || return 1
  applied=1
}

restore_rollback(){
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring accepted v0.0.33 touched state\n'
  local rel backup dest
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    if [[ -e "$backup" ]]; then mkdir -p "$project_root/$(dirname "$rel")"; cp -a "$backup" "$project_root/$rel"; else rm -rf -- "$project_root/$rel"; fi
  done < <(manifest_paths payload)
  rm -f -- "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v34.json" "$project_root/Nougat_Media_Suite_v34"
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    [[ -e "$backup" ]] && { mkdir -p "$project_root/$(dirname "$rel")"; cp -a "$backup" "$project_root/$rel"; }
  done < <(manifest_paths remove_after_success)
  for dest in "$launcher_unversioned" "$launcher_v33" "$launcher_v34" "$canonical_launcher"; do
    backup="$rollback_root/user-shell/applications/$(basename "$dest")"
    if [[ -e "$backup" ]]; then mkdir -p "$(dirname "$dest")"; cp -a "$backup" "$dest"; else rm -f -- "$dest"; fi
  done
  printf 'ROLLBACK PASS: accepted v0.0.33 touched state restored. Generated runtimes/user data preserved.\n'
}

run_source_tests(){
  verify_protected_state || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v34.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v34.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_security_analysis_v33.py" "$project_root" || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_diagnostics_v26.py" "$project_root" || return 1
}

build_stub(){
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  local exe="$build_root/stub/Nougat_Media_Suite_v34"
  [[ "$("$exe" --version)" == "Nougat Media Suite v0.0.34" ]] || return 1
  "$exe" --discover-ai-self-test || return 1
  "$exe" --v25-ui-state-self-test || return 1
  "$exe" --v28-ui-state-self-test || return 1
  "$exe" --v29-tv-reliability-self-test || return 1
  "$exe" --v30-ui-library-player-self-test || return 1
  "$exe" --v31-ui-sheet-self-test || return 1
  "$exe" --v32-p2p-player-repair-self-test || return 1
  HOME="$build_root/stub-home" "$exe" --v33-integration-self-test || return 1
  HOME="$build_root/stub-home-v34" "$exe" --v34-ui-polish-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v34.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v33.py" "$project_root" "$exe" || return 1
}

verify_security_runtime(){
  if python3 "$project_root/tools/install_nougat_security_runtime_v33.py" "$project_root" --check; then
    python3 "$project_root/tools/test_nougat_security_analysis_v33.py" "$project_root" || return 1
    return 0
  fi
  printf 'Retained v0.0.33 security runtime is missing or incomplete; rebuilding the free one-shot runtime.\n'
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
  local exe="$build_root/full/Nougat_Media_Suite_v34"
  verify_relative_ai_rpath "$exe" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.34" ]] || return 1
  env -u LD_LIBRARY_PATH "$exe" --discover-ai-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v25-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v28-ui-state-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v29-tv-reliability-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v30-ui-library-player-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v31-ui-sheet-self-test || return 1
  env -u LD_LIBRARY_PATH "$exe" --v32-p2p-player-repair-self-test || return 1
  HOME="$build_root/full-home" env -u LD_LIBRARY_PATH "$exe" --v33-integration-self-test || return 1
  HOME="$build_root/full-home-v34" env -u LD_LIBRARY_PATH "$exe" --v34-ui-polish-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v34.py" "$project_root" "$exe" || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
}

install_launchers(){
  mkdir -p "$HOME/.local/share/applications" || return 1
  cp -a "$project_root/NougatMediaSuite.desktop" "$launcher_unversioned" || return 1
  cp -a "$project_root/NougatMediaSuite_v34.desktop" "$launcher_v34" || return 1
  cp -a "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" "$canonical_launcher" || return 1
  rm -f -- "$launcher_v33"
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
}

apply_raw_icon(){
  local exe="$1" info
  gio set -t string "$exe" metadata::custom-icon "file://$master_icon" || return 1
  info="$(gio info -a metadata::custom-icon "$exe" 2>&1)" || return 1
  [[ "$info" == *"file://$master_icon"* ]] || return 1
  printf 'Raw executable approved N custom-icon metadata verified after final write.\n'
}

install_final(){
  cp "$build_root/full/Nougat_Media_Suite_v34" "$project_root/Nougat_Media_Suite_v34" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v34" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v34" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v34" --version 2>/dev/null)" == "Nougat Media Suite v0.0.34" ]] || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v34" --v34-ui-polish-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v34.py" "$project_root" "$project_root/Nougat_Media_Suite_v34" || return 1
  install_launchers || return 1
  apply_raw_icon "$project_root/Nougat_Media_Suite_v34" || return 1
  local rel
  while IFS= read -r rel; do [[ -n "$rel" ]] && rm -f -- "$project_root/$rel"; done < <(manifest_paths remove_after_success)
  command -v nautilus >/dev/null 2>&1 && nautilus -q >/dev/null 2>&1 || true
}

main(){
  phase_start "accepted v0.0.33 base and v0.0.34 prerequisites"
  [[ -d "$project_root" ]] || { printf 'FINAL FAIL: project not found. Terminal remains open.\n'; return 1; }
  local cmd
  for cmd in cmake g++ pkg-config python3 gio git env readelf sha256sum tar xvfb-run xwininfo xprop ffmpeg; do require_command "$cmd" || { printf 'FINAL FAIL: prerequisites incomplete. Terminal remains open.\n'; return 1; }; done
  pkg-config --exists libtorrent-rasterbar || { printf 'FAIL: libtorrent-rasterbar development package unavailable.\n'; return 1; }
  cleanup_generated_python_caches || { printf 'FINAL FAIL: generated Python cache cleanup failed. Terminal remains open.\n'; return 1; }
  verify_git_state || { printf 'FINAL FAIL: Git/base preflight failed. Terminal remains open.\n'; return 1; }
  [[ -x "$project_root/Nougat_Media_Suite_v33" ]] || { printf 'FAIL: accepted v0.0.33 root executable missing.\n'; return 1; }
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v33" --version 2>/dev/null)" == "Nougat Media Suite v0.0.33" ]] || { printf 'FAIL: accepted v0.0.33 executable version mismatch.\n'; return 1; }
  [[ -f "$project_root/components/ai/runtime/include/llama.h" && -e "$project_root/components/ai/runtime/lib/libllama.so.0" ]] || { printf 'FAIL: AI runtime missing.\n'; return 1; }
  [[ -x "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]] || { printf 'FAIL: integrated Jellyfin runtime missing.\n'; return 1; }
  verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha" || { printf 'FAIL: pinned Nomic model missing/changed.\n'; return 1; }
  verify_manifest || return 1
  verify_protected_state || return 1
  phase_pass "accepted v0.0.33 base and v0.0.34 prerequisites"

  phase_start "save exact v0.0.33 touched-state rollback snapshot"
  save_rollback || { printf 'FINAL FAIL: rollback snapshot failed. Terminal remains open.\n'; return 1; }
  phase_pass "save exact v0.0.33 touched-state rollback snapshot"

  phase_start "apply v0.0.34 changed files"
  apply_payload || { restore_rollback; printf 'FINAL FAIL: payload application failed. Terminal remains open.\n'; return 1; }
  run_source_tests || { restore_rollback; printf 'FINAL FAIL: source/regression tests failed. Terminal remains open.\n'; return 1; }
  phase_pass "apply v0.0.34 changed files and source/regression tests"

  phase_start "warnings-as-errors stub build and X11 smoke"
  mkdir -p "$build_root" && build_stub || { restore_rollback; printf 'FINAL FAIL: stub/UI validation failed. Terminal remains open.\n'; return 1; }
  phase_pass "warnings-as-errors stub build and X11 smoke"

  phase_start "verify retained one-shot security runtime"
  verify_security_runtime || { restore_rollback; printf 'FINAL FAIL: retained security runtime verification failed. Terminal remains open.\n'; return 1; }
  phase_pass "retained YARA-X/capa/Magika one-shot security runtime"

  phase_start "full native v0.0.34 build"
  build_full || { restore_rollback; printf 'FINAL FAIL: full native build failed. Terminal remains open.\n'; return 1; }
  phase_pass "full native v0.0.34 build"

  phase_start "install final v0.0.34 executable and launchers"
  install_final || { restore_rollback; printf 'FINAL FAIL: final installation failed. Terminal remains open.\n'; return 1; }
  phase_pass "install final v0.0.34 executable and launchers"

  rm -rf -- "$build_root"
  printf '\nFINAL PASS: Nougat Media Suite v0.0.34 installed and validated.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v34"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'UI: actual-sheet top tabs, seek bar, and housed volume control active.\n'
  printf 'HOME: fixed section card geometry and direct non-coasting scrollbar dragging active.\n'
  printf 'DISCOVER: Live TV selector active; TMDb-backed sources labeled TMDb Movie / TMDb TV.\n'
  printf 'FOUNDATIONS: accepted v0.0.33 server/security/P2P/tuner systems retained; server lifecycle is not altered by this installer.\n'
  printf 'OWNER CHECK REQUIRED: top-tab sheet fidelity/left placement, seek/volume sheet fidelity, Home card alignment, scroll dragging, Live TV header spacing, Discover selectors, and page-frame corners.\n'
  printf 'CANDIDATE STATUS: v0.0.34 remains unaccepted until owner real-machine approval.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v34\n' "$project_root"
}

main "$@"
