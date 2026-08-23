#!/usr/bin/env bash
# Nougat Media Suite v0.0.35 changed-files installer (rollback-recovery revision).
# Base: accepted/published v0.0.34 line plus exact README restoration commit
#       64eade89e7b9b88f1696bef18e580e22bace978f.
# Focus: code + bug cleanup, exact-sheet VOLUME sprite, app-wide layout alignment,
#        narrow-width control access, native ATSC scan, Studio foundation.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v35.json"
recovery_candidate_manifest="$payload_root/recovery/pre_repair_candidate_manifest_v35.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_35_APP_WIDE_LAYOUT_REPAIR_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_35-$stamp-$$"
expected_head="64eade89e7b9b88f1696bef18e580e22bace978f"
canonical_id="com.elderredsoftworks.NougatMediaSuite"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
canonical_launcher="$HOME/.local/share/applications/${canonical_id}.desktop"
launcher_unversioned="$HOME/.local/share/applications/NougatMediaSuite.desktop"
launcher_v34="$HOME/.local/share/applications/NougatMediaSuite_v34.desktop"
launcher_v35="$HOME/.local/share/applications/NougatMediaSuite_v35.desktop"
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

recover_interrupted_same_version_state(){
  # A failed early repair from the previous package could restore the source
  # files but then incorrectly delete the pre-repair v0.0.35 manifest and root
  # executable. Repair only that exact, proven state. Never overwrite an
  # unrelated or partially edited worktree.
  if git -C "$project_root" diff --quiet && git -C "$project_root" diff --cached --quiet; then
    return 0
  fi

  if [[ -f "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v35.json" && -x "$project_root/Nougat_Media_Suite_v35" ]]; then
    return 0
  fi

  python3 - "$project_root" "$manifest_path" "$recovery_candidate_manifest" <<'PYREC'
import hashlib,json,pathlib,shutil,subprocess,sys
root=pathlib.Path(sys.argv[1]); repair_manifest=pathlib.Path(sys.argv[2]); recovery=pathlib.Path(sys.argv[3])
m=json.loads(repair_manifest.read_text())
expected=m.get('pre_repair_candidate',{})
manifest_rel='NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v35.json'
if not expected or manifest_rel not in expected:
 print('FAIL: interrupted-repair recovery metadata is incomplete.'); raise SystemExit(1)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
 return h.hexdigest()
# Every first-candidate file except the one known to have been deleted by the
# bad rollback must still match byte-for-byte.
for rel,meta in expected.items():
 if rel==manifest_rel: continue
 p=root/rel
 if not p.is_file() or p.stat().st_size!=meta['bytes'] or sha(p)!=meta['sha256']:
  print('FAIL: state is not the known interrupted v0.0.35 rollback:',rel); raise SystemExit(1)
for rel in m.get('remove_after_success',[]):
 if (root/rel).exists():
  print('FAIL: interrupted state unexpectedly contains old-version file:',rel); raise SystemExit(1)
# No unrelated worktree edits may be present.
allowed=set(expected); allowed.update(m.get('remove_after_success',[])); allowed.add('Nougat_Media_Suite_v35')
status=subprocess.check_output(['git','-C',str(root),'status','--porcelain=v1','--untracked-files=all'],text=True).splitlines()
bad=[]
for line in status:
 if not line: continue
 path=line[3:]
 if ' -> ' in path: path=path.split(' -> ',1)[1]
 if path.startswith(('components/ai/runtime/','components/jellyfin/runtime/','components/security/runtime/')): continue
 if path not in allowed: bad.append(line)
if bad:
 print('FAIL: unrelated worktree changes exist; interrupted-state recovery refused:')
 for line in bad: print('  '+line)
 raise SystemExit(1)
meta=expected[manifest_rel]
if not recovery.is_file() or recovery.stat().st_size!=meta['bytes'] or sha(recovery)!=meta['sha256']:
 print('FAIL: packaged recovery manifest does not match the original v0.0.35 candidate.'); raise SystemExit(1)
cur=root/manifest_rel
if cur.exists():
 if cur.stat().st_size!=meta['bytes'] or sha(cur)!=meta['sha256']:
  print('FAIL: current v0.0.35 manifest exists but is not the expected candidate manifest; refusing overwrite.'); raise SystemExit(1)
else:
 shutil.copy2(recovery,cur)
 print('RECOVERY PASS: restored exact first-candidate v0.0.35 manifest deleted by the prior rollback bug.')
PYREC
  [[ $? -eq 0 ]] || return 1

  if [[ ! -x "$project_root/Nougat_Media_Suite_v35" ]]; then
    printf 'RECOVERY: rebuilding the missing pre-repair v0.0.35 root executable from the verified candidate source.\n'
    rm -rf -- "$build_root/recovery-pre-repair"
    cmake -S "$project_root" -B "$build_root/recovery-pre-repair" -DCMAKE_BUILD_TYPE=Release || return 1
    cmake --build "$build_root/recovery-pre-repair" -j"$(nproc)" || return 1
    local recovered="$build_root/recovery-pre-repair/Nougat_Media_Suite_v35"
    [[ -x "$recovered" ]] || { printf 'FAIL: recovery build did not produce Nougat_Media_Suite_v35.\n'; return 1; }
    cp -a "$recovered" "$project_root/Nougat_Media_Suite_v35" || return 1
    chmod +x "$project_root/Nougat_Media_Suite_v35" || return 1
    [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v35" --version 2>/dev/null)" == "Nougat Media Suite v0.0.35" ]] || {
      printf 'FAIL: recovered pre-repair executable does not identify as v0.0.35.\n'; return 1;
    }
    printf 'RECOVERY PASS: rebuilt missing first-candidate v0.0.35 executable.\n'
  fi
}

verify_git_state(){
  local branch head
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  [[ "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "${branch:-unknown}"; return 1; }
  [[ "$head" == "$expected_head" ]] || { printf 'FAIL: expected v0.0.34+README baseline %s, found %s\n' "$expected_head" "${head:-unknown}"; return 1; }

  if git -C "$project_root" diff --quiet && git -C "$project_root" diff --cached --quiet; then
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
print('Git preflight PASS: clean accepted v0.0.34+README baseline; generated runtime directories only.')
PY
    return $?
  fi

  # Same-version v0.0.35 repair path: the first v0.0.35 candidate may already
  # be installed. Accept it only when every known candidate file exactly
  # matches the hashes recorded in this repair manifest and no unrelated edits
  # are present. This avoids forcing a rollback just to install the repair.
  python3 - "$project_root" "$manifest_path" <<'PY'
import hashlib,json,pathlib,subprocess,sys
root=pathlib.Path(sys.argv[1]); manifest=json.load(open(sys.argv[2]))
expected=manifest.get('pre_repair_candidate',{})
if not expected:
 print('FAIL: repair manifest lacks pre_repair_candidate verification data'); raise SystemExit(1)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
 return h.hexdigest()
for rel,meta in expected.items():
 p=root/rel
 if not p.is_file() or p.stat().st_size!=meta['bytes'] or sha(p)!=meta['sha256']:
  print('FAIL: current v0.0.35 candidate does not match expected pre-repair file:',rel); raise SystemExit(1)
for rel in manifest.get('remove_after_success',[]):
 if (root/rel).exists():
  print('FAIL: current v0.0.35 candidate unexpectedly still contains old-version file:',rel); raise SystemExit(1)
allowed=set(expected)
allowed.update(manifest.get('remove_after_success',[]))
allowed.add('Nougat_Media_Suite_v35')
status=subprocess.check_output(['git','-C',str(root),'status','--porcelain=v1','--untracked-files=all'],text=True).splitlines()
bad=[]
for line in status:
 if not line: continue
 path=line[3:]
 if ' -> ' in path: path=path.split(' -> ',1)[1]
 if path.startswith(('components/ai/runtime/','components/jellyfin/runtime/','components/security/runtime/')): continue
 if path not in allowed: bad.append(line)
if bad:
 print('FAIL: unrelated worktree changes exist:')
 for line in bad: print('  '+line)
 raise SystemExit(1)
print('Git preflight PASS: exact first v0.0.35 candidate detected; safe same-version repair path active.')
PY
  [[ -x "$project_root/Nougat_Media_Suite_v35" ]] || { printf 'FAIL: current v0.0.35 candidate executable missing.\n'; return 1; }
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v35" --version 2>/dev/null)" == "Nougat Media Suite v0.0.35" ]] || {
    printf 'FAIL: current candidate executable does not identify as v0.0.35.\n'; return 1;
  }
}
verify_manifest(){
  python3 - "$payload_root" "$manifest_path" <<'PY'
import hashlib,json,pathlib,sys
root=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2])
if not mp.is_file(): print('FAIL: v35 manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text())
if m.get('target_version')!='0.0.35' or m.get('accepted_base_commit')!='64eade89e7b9b88f1696bef18e580e22bace978f':
 print('FAIL: manifest identity/base mismatch'); raise SystemExit(1)
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
 return h.hexdigest()
for group,label in ((m.get('payload',{}),'payload'),(m.get('recovery_assets',{}),'recovery asset')):
 for rel,r in group.items():
  p=root/rel
  if not p.is_file() or p.stat().st_size!=r['bytes'] or sha(p)!=r['sha256']:
   print(f'FAIL: package {label} mismatch:',rel); raise SystemExit(1)
print('Manifest verified:',len(m.get('payload',{})),'payload files and',len(m.get('recovery_assets',{})),'recovery asset(s).')
PY
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
  printf 'Protected licensing/search state preserved.\n'
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
  if [[ -e "$project_root/Nougat_Media_Suite_v35" ]]; then
    cp -a "$project_root/Nougat_Media_Suite_v35" "$rollback_root/project/Nougat_Media_Suite_v35" || return 1
  fi
  for src in "$launcher_unversioned" "$launcher_v34" "$launcher_v35" "$canonical_launcher"; do
    [[ -e "$src" ]] && cp -a "$src" "$rollback_root/user-shell/applications/$(basename "$src")"
  done
  printf 'Accepted base HEAD: %s\nTarget: v0.0.35 same-version repair\nSnapshot preserves the exact touched state present before this installer.\n' "$expected_head" > "$rollback_root/ROLLBACK_INFO.txt"
}

apply_payload(){
  local rel
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || return 1
  done < <(manifest_paths payload)
  cp -a "$manifest_path" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v35.json" || return 1
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_35.sh" \
           "$project_root/tools/test_nougat_media_suite_v35.py" \
           "$project_root/tools/test_installer_rollback_v35.py" || return 1
  applied=1
}

restore_rollback(){
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring exact pre-repair touched state\n'
  local rel backup dest
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    if [[ -e "$backup" ]]; then mkdir -p "$project_root/$(dirname "$rel")"; cp -a "$backup" "$project_root/$rel"; else rm -rf -- "$project_root/$rel"; fi
  done < <(manifest_paths payload)
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    [[ -e "$backup" ]] && { mkdir -p "$project_root/$(dirname "$rel")"; cp -a "$backup" "$project_root/$rel"; }
  done < <(manifest_paths remove_after_success)
  backup="$rollback_root/project/Nougat_Media_Suite_v35"
  if [[ -e "$backup" ]]; then
    cp -a "$backup" "$project_root/Nougat_Media_Suite_v35"
    chmod +x "$project_root/Nougat_Media_Suite_v35" 2>/dev/null || true
  else
    rm -f -- "$project_root/Nougat_Media_Suite_v35"
  fi
  # NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v35.json is restored/removed above by
  # the payload rollback loop according to whether it existed pre-repair.
  for dest in "$launcher_unversioned" "$launcher_v34" "$launcher_v35" "$canonical_launcher"; do
    backup="$rollback_root/user-shell/applications/$(basename "$dest")"
    if [[ -e "$backup" ]]; then mkdir -p "$(dirname "$dest")"; cp -a "$backup" "$dest"; else rm -f -- "$dest"; fi
  done
  printf 'ROLLBACK PASS: exact pre-repair touched state restored. Generated runtimes and user data preserved.\n'
}

run_source_tests(){
  verify_protected_state || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v35.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v35.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_security_analysis_v33.py" "$project_root" || return 1
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
}

build_stub(){
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  local exe="$build_root/stub/Nougat_Media_Suite_v35"
  [[ "$("$exe" --version)" == "Nougat Media Suite v0.0.35" ]] || return 1
  run_regression_exe_tests "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v35.py" "$project_root" "$exe" || return 1
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
  local exe="$build_root/full/Nougat_Media_Suite_v35"
  verify_relative_ai_rpath "$exe" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.35" ]] || return 1
  run_regression_exe_tests "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v35.py" "$project_root" "$exe" || return 1
}

install_final(){
  local built="$build_root/full/Nougat_Media_Suite_v35"
  [[ -x "$built" ]] || { printf 'FAIL: full native v0.0.35 executable missing.\n'; return 1; }
  cp -a "$built" "$project_root/Nougat_Media_Suite_v35" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v35" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v35" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v35" --version 2>/dev/null)" == "Nougat Media Suite v0.0.35" ]] || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v35.py" "$project_root" "$project_root/Nougat_Media_Suite_v35" || return 1
}

install_launchers(){
  mkdir -p "$HOME/.local/share/applications" || return 1
  cp -a "$project_root/NougatMediaSuite.desktop" "$launcher_unversioned" || return 1
  cp -a "$project_root/NougatMediaSuite_v35.desktop" "$launcher_v35" || return 1
  cp -a "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" "$canonical_launcher" || return 1
  rm -f -- "$launcher_v34"
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
  phase_start "v0.0.35 base/candidate repair preflight"
  require_command git || return 1
  require_command python3 || return 1
  require_command cmake || return 1
  require_command readelf || return 1
  require_command g++ || return 1
  cleanup_generated_python_caches || return 1
  recover_interrupted_same_version_state || return 1
  verify_git_state || return 1
  verify_manifest || return 1
  phase_pass "v0.0.35 base/candidate repair preflight"

  phase_start "save exact pre-repair touched-state rollback snapshot"
  save_rollback || return 1
  phase_pass "save exact pre-repair touched-state rollback snapshot"

  phase_start "apply v0.0.35 changed files"
  if ! apply_payload || ! run_source_tests; then
    restore_rollback
    return 1
  fi
  phase_pass "apply v0.0.35 changed files and source/regression tests"

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

  phase_start "full native v0.0.35 build"
  if ! build_full; then
    restore_rollback
    return 1
  fi
  phase_pass "full native v0.0.35 build"

  phase_start "install final v0.0.35 executable and launchers"
  if ! install_final || ! install_launchers || ! apply_raw_icon "$project_root/Nougat_Media_Suite_v35" || ! remove_old_version_files; then
    restore_rollback
    return 1
  fi
  phase_pass "install final v0.0.35 executable and launchers"

  printf '\nFINAL PASS: Nougat Media Suite v0.0.35 installed and validated.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v35"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'CLEANUP: Nougat Search/Crawler lifetime repair and stronger v0.0.35 validation active.\n'
  printf 'UI: exact sheet-pixel VOLUME control; app-wide top-row alignment; centered full-width player transport group; Library one-row/far-right view toggles; enlarged selected-tab pointers; Debug/player full narrow-width scrolling.\n'
  printf 'LIVE TV: native Linux DVB ATSC 1.0 channel scan is ready for owner hardware testing.\n'
  printf 'STUDIO: the top-level tab remains Studio between Stream and Debug; the page is Gold Studio with a yellow/gold family and brown stitched borders; full tools are roadmap work.\n'
  printf 'SERVER: persistent server lifecycle remains untouched by UI close.\n'
  printf 'OWNER CHECK REQUIRED: exact VOLUME appearance, no player black repaint band, full half-screen player/Debug/Library scrolling, app-wide alignment, larger selected-tab pointers, tuner scan, and Studio palette.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v35\n' "$project_root"
  return 0
}

main "$@"
