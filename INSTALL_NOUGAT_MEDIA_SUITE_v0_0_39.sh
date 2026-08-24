#!/usr/bin/env bash
# Nougat Media Suite v0.0.39 rejected-build repair installer.
# Base commit: pushed/accepted v0.0.38 (9957aa6a4ba439d86cd5b35f580d9cb3a9be1ed1).
# Repairs v0.0.39 in place, preserving the rejected candidate's newer artwork/assets.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v39.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_39_REJECTED_BUILD_REPAIR_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_39-repair-$stamp-$$"
base_commit="9957aa6a4ba439d86cd5b35f580d9cb3a9be1ed1"
canonical_id="com.elderredsoftworks.NougatMediaSuite"
canonical_launcher="$HOME/.local/share/applications/${canonical_id}.desktop"
launcher_unversioned="$HOME/.local/share/applications/NougatMediaSuite.desktop"
launcher_v38="$HOME/.local/share/applications/NougatMediaSuite_v38.desktop"
launcher_v39="$HOME/.local/share/applications/NougatMediaSuite_v39.desktop"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"
applied=0

phase_start(){ printf '\n=== %s ===\n' "$1"; }
phase_pass(){ printf 'PASS: %s\n' "$1"; }
require_command(){ command -v "$1" >/dev/null 2>&1 || { printf 'FAIL: required command missing: %s\n' "$1"; return 1; }; }

atomic_replace_file(){
  local src="$1" dest="$2" tmp
  tmp="${dest}.nougat-v39-replace-$$"
  rm -f -- "$tmp"
  cp -a "$src" "$tmp" || return 1
  mv -f -- "$tmp" "$dest" || { rm -f -- "$tmp"; return 1; }
}

verify_manifest(){
  python3 - "$payload_root" "$manifest_path" <<'PY'
import hashlib,json,pathlib,sys
root=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2])
if not mp.is_file(): print('FAIL: v39 manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text())
if m.get('target_version')!='0.0.39' or m.get('base_commit')!='9957aa6a4ba439d86cd5b35f580d9cb3a9be1ed1':
    print('FAIL: v39 manifest identity/base mismatch'); raise SystemExit(1)
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

verify_install_base(){
  local branch head version_text
  [[ -d "$project_root/.git" ]] || { printf 'FAIL: Nougat Git project root missing: %s\n' "$project_root"; return 1; }
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  [[ "$branch" == "main" ]] || { printf 'FAIL: expected branch main, found %s\n' "$branch"; return 1; }
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)" || return 1
  [[ "$head" == "$base_commit" ]] || {
    printf 'FAIL: local HEAD is not pushed v0.0.38 baseline.\nExpected: %s\nFound:    %s\n' "$base_commit" "$head"
    printf 'No project files were changed.\n'
    return 1
  }
  if ! git -C "$project_root" diff --cached --quiet --; then
    printf 'FAIL: staged Git changes are present. The rejected-v0.0.39 repair requires an unstaged working candidate so rollback remains exact.\n'
    printf 'No project files were changed.\n'
    return 1
  fi
  for rel in src/main.cpp src/live_tv/tuner_backend.hpp src/live_tv/tuner_backend.cpp CMakeLists.txt assets/ui/nougat_volume_sheet_frames.bin docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png; do
    [[ -f "$project_root/$rel" ]] || { printf 'FAIL: required project file missing: %s\n' "$rel"; return 1; }
  done
  version_text="$(grep -Eo 'v0\.0\.(38|39)' "$project_root/src/main.cpp" | head -1)"
  [[ "$version_text" == "v0.0.38" || "$version_text" == "v0.0.39" ]] || {
    printf 'FAIL: source is neither the pushed v0.0.38 state nor the rejected v0.0.39 working state.\n'
    return 1
  }
  python3 - "$project_root/assets/ui/nougat_volume_sheet_frames.bin" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); expected='38197798a97e9ecadf3934daca692446bea586b36e2038c533aa5c92f51077e2'
actual=hashlib.sha256(p.read_bytes()).hexdigest()
if actual!=expected:
    print('FAIL: protected exact-sheet VOLUME asset is not the accepted v0.0.38 asset.'); raise SystemExit(1)
print('Protected VOLUME preflight PASS:',actual)
PY
  printf 'Install base verified: pushed v0.0.38 HEAD with %s source state; rejected-v39 repair-in-place is allowed.\n' "$version_text"
}

dry_run_repair_preflight(){
  local dry="$build_root/source-preflight"
  local dry_build="$build_root/source-preflight-build"
  rm -rf -- "$dry" "$dry_build"
  mkdir -p "$dry" || return 1

  # Build a no-write mirror of the exact rejected working tree. Files that the
  # v39 repair may modify are copied; everything else is symlinked read-only to
  # the current project so the preflight compiles against the real source set
  # without copying large runtimes or changing the active project.
  python3 - "$project_root" "$dry" <<'PYDRY'
import os,pathlib,shutil,sys
src=pathlib.Path(sys.argv[1]).resolve()
dst=pathlib.Path(sys.argv[2]).resolve()

def link_or_copy_tree_children(src_dir,dst_dir,copy_names=()):
    dst_dir.mkdir(parents=True,exist_ok=True)
    copies=set(copy_names)
    for child in src_dir.iterdir():
        target=dst_dir/child.name
        if child.name in copies:
            if child.is_dir(): shutil.copytree(child,target,symlinks=True)
            else: shutil.copy2(child,target)
        else:
            target.symlink_to(child, target_is_directory=child.is_dir())

# Top level: copy every file the repair can edit; symlink other project state.
copy_top={
    'CMakeLists.txt','README.md','CHANGELOG.md','ROADMAP.md','docs',
    'NougatMediaSuite.desktop','NougatMediaSuite_v39.desktop',
    'com.elderredsoftworks.NougatMediaSuite.desktop'
}
for child in src.iterdir():
    if child.name in {'.git','src','tools'}: continue
    target=dst/child.name
    if child.name in copy_top:
        if child.is_dir(): shutil.copytree(child,target,symlinks=True)
        else: shutil.copy2(child,target)
    else:
        target.symlink_to(child,target_is_directory=child.is_dir())

# Source tree: exact rejected main/tuner sources are copied because the patcher
# edits them. Every unrelated source path is a symlink to the real tree.
src_out=dst/'src'; src_out.mkdir()
for child in (src/'src').iterdir():
    if child.name in {'main.cpp','live_tv','diagnostics'}:
        continue
    (src_out/child.name).symlink_to(child,target_is_directory=child.is_dir())
shutil.copy2(src/'src/main.cpp',src_out/'main.cpp')
lt=src_out/'live_tv'; lt.mkdir()
for child in (src/'src/live_tv').iterdir():
    target=lt/child.name
    if child.name in {'tuner_backend.hpp','tuner_backend.cpp'}:
        shutil.copy2(child,target)
    else:
        target.symlink_to(child,target_is_directory=child.is_dir())

# Project tools not replaced by the repair remain available to CMake/tests.
tools_out=dst/'tools'; tools_out.mkdir()
for child in (src/'tools').iterdir():
    if child.name in {'apply_v39_repair.py','test_nougat_diagnostics_v26.py','test_nougat_media_suite_v39.py','test_v39_diagnostic_api_compat.py'}:
        continue
    (tools_out/child.name).symlink_to(child,target_is_directory=child.is_dir())
PYDRY
  [[ $? -eq 0 ]] || { printf 'FAIL: could not create exact no-write rejected-v39 source mirror.\n'; return 1; }

  mkdir -p "$dry/src/diagnostics" || return 1
  cp -a "$payload_root/src/diagnostics/." "$dry/src/diagnostics/" || return 1
  cp -a "$payload_root/tools/apply_v39_repair.py" "$dry/tools/" || return 1
  cp -a "$payload_root/tools/test_nougat_diagnostics_v26.py" "$dry/tools/" || return 1
  cp -a "$payload_root/tools/test_nougat_media_suite_v39.py" "$dry/tools/" || return 1
  cp -a "$payload_root/tools/test_v39_diagnostic_api_compat.py" "$dry/tools/" || return 1
  mkdir -p "$dry/docs/design" || return 1
  cp -a "$payload_root/docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png" "$dry/docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png" || return 1

  python3 "$dry/tools/apply_v39_repair.py" "$dry" || {
    printf 'FAIL: dry-run source repair did not match the current rejected-v0.0.39 source shape.\n'
    return 1
  }

  python3 "$dry/tools/test_nougat_media_suite_v39.py" "$dry" || {
    printf 'FAIL: dry-run v0.0.39 source contract failed.\n'
    return 1
  }

  python3 "$dry/tools/test_nougat_diagnostics_v26.py" "$dry" || {
    printf 'FAIL: dry-run retained diagnostic regression failed.\n'
    return 1
  }

  python3 "$dry/tools/test_v39_diagnostic_api_compat.py" "$dry" || {
    printf 'FAIL: rejected-v39 diagnostic API compatibility compile failed before project modification.\n'
    return 1
  }

  # Critical gate added after the rejected repair exposed a header/main API
  # mismatch: compile the ENTIRE exact mirrored v39 program before touching the
  # active project. A source-contract grep or tuner-only compile is insufficient.
  cmake -S "$dry" -B "$dry_build" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || {
    printf 'FAIL: dry-run full stub configure failed before project modification.\n'
    return 1
  }
  cmake --build "$dry_build" -j"$(nproc)" || {
    printf 'FAIL: dry-run full stub compile failed before project modification.\n'
    return 1
  }
  local dry_exe="$dry_build/Nougat_Media_Suite_v39"
  [[ -x "$dry_exe" ]] || { printf 'FAIL: dry-run v39 executable missing after full compile.\n'; return 1; }
  [[ "$("$dry_exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.39" ]] || {
    printf 'FAIL: dry-run compiled executable reports the wrong version.\n'
    return 1
  }
  HOME="$build_root/dry-home-v39" "$dry_exe" --v39-diagnostic-self-test || {
    printf 'FAIL: dry-run compiled v39 diagnostic self-test failed.\n'
    return 1
  }

  printf 'Dry-run repair preflight PASS: exact rejected working-tree mirror patched, full v39 stub executable compiled with project warnings-as-errors, and diagnostic runtime self-test passed before touching project files.\n'
  return 0
}

snapshot_paths(){
  cat <<'PATHS'
src/diagnostics/diagnostic_types.hpp
src/diagnostics/diagnostic_engine.hpp
src/diagnostics/diagnostic_engine.cpp
src/main.cpp
src/live_tv/tuner_backend.hpp
src/live_tv/tuner_backend.cpp
CMakeLists.txt
README.md
CHANGELOG.md
ROADMAP.md
NougatMediaSuite.desktop
NougatMediaSuite_v39.desktop
com.elderredsoftworks.NougatMediaSuite.desktop
tools/apply_v39_repair.py
tools/test_nougat_diagnostics_v26.py
tools/test_nougat_media_suite_v39.py
tools/test_v39_diagnostic_api_compat.py
INSTALL_NOUGAT_MEDIA_SUITE_v0_0_39.sh
NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v39.json
APPLY_COMMAND.txt
docs/builds/NOUGAT_MEDIA_SUITE_v0_0_39_REJECTED_BUILD_REPAIR_VALIDATION.md
docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png
Nougat_Media_Suite_v39
PATHS
}

save_rollback(){
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell/applications" || return 1
  : > "$rollback_root/EXISTED_BEFORE.txt" || return 1
  local rel src
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    src="$project_root/$rel"
    if [[ -e "$src" ]]; then
      printf '%s\n' "$rel" >> "$rollback_root/EXISTED_BEFORE.txt"
      mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1
      cp -a "$src" "$rollback_root/project/$rel" || return 1
    fi
  done < <(snapshot_paths)
  for src in "$launcher_unversioned" "$launcher_v38" "$launcher_v39" "$canonical_launcher"; do
    [[ -e "$src" ]] && cp -a "$src" "$rollback_root/user-shell/applications/$(basename "$src")"
  done
  git -C "$project_root" status --short --branch > "$rollback_root/GIT_STATUS_BEFORE.txt" 2>/dev/null || true
  git -C "$project_root" diff --binary > "$rollback_root/UNCOMMITTED_DIFF_BEFORE.patch" 2>/dev/null || true
  printf 'Base commit: %s\nTarget: same-version rejected v0.0.39 repair\n' "$base_commit" > "$rollback_root/ROLLBACK_INFO.txt"
  return 0
}

existed_before(){ grep -Fqx -- "$1" "$rollback_root/EXISTED_BEFORE.txt" 2>/dev/null; }

restore_rollback(){
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring exact pre-v0.0.39-repair touched state.\n'
  local rel backup dest
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    dest="$project_root/$rel"
    if existed_before "$rel"; then
      [[ -e "$backup" ]] || { printf 'ROLLBACK FAIL: missing backup %s\n' "$rel"; return 1; }
      mkdir -p "$(dirname "$dest")" || return 1
      rm -rf -- "$dest"
      cp -a "$backup" "$dest" || return 1
    else
      rm -rf -- "$dest"
    fi
  done < <(snapshot_paths)
  for dest in "$launcher_unversioned" "$launcher_v39" "$canonical_launcher"; do
    backup="$rollback_root/user-shell/applications/$(basename "$dest")"
    if [[ -e "$backup" ]]; then
      mkdir -p "$(dirname "$dest")" || return 1
      cp -a "$backup" "$dest" || return 1
    else
      rm -f -- "$dest"
    fi
  done
  printf 'ROLLBACK PASS: rejected-build pre-repair touched state restored.\n'
  return 0
}

apply_repair_payload(){
  local rel
  # Rollback protection begins before the first active-project write.
  applied=1
  for rel in \
    src/diagnostics/diagnostic_types.hpp \
    src/diagnostics/diagnostic_engine.hpp \
    src/diagnostics/diagnostic_engine.cpp \
    tools/apply_v39_repair.py \
    tools/test_nougat_diagnostics_v26.py \
    tools/test_nougat_media_suite_v39.py \
    tools/test_v39_diagnostic_api_compat.py \
    INSTALL_NOUGAT_MEDIA_SUITE_v0_0_39.sh \
    APPLY_COMMAND.txt \
    docs/builds/NOUGAT_MEDIA_SUITE_v0_0_39_REJECTED_BUILD_REPAIR_VALIDATION.md \
    docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png; do
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || return 1
  done
  cp -a "$manifest_path" "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v39.json" || return 1
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_39.sh" \
           "$project_root/tools/apply_v39_repair.py" \
           "$project_root/tools/test_nougat_diagnostics_v26.py" \
           "$project_root/tools/test_nougat_media_suite_v39.py" \
           "$project_root/tools/test_v39_diagnostic_api_compat.py" || return 1
  python3 "$project_root/tools/apply_v39_repair.py" "$project_root" || return 1
  cp -a "$project_root/NougatMediaSuite.desktop" "$project_root/NougatMediaSuite_v39.desktop" || return 1
  sed -i 's/Nougat_Media_Suite_v38/Nougat_Media_Suite_v39/g; s/v0\.0\.38/v0.0.39/g' "$project_root/NougatMediaSuite_v39.desktop" || return 1
  return 0
}

verify_protected_state(){
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  printf 'Protected licensing/Search state preserved.\n'
}

run_source_tests(){
  verify_protected_state || return 1
  python3 "$project_root/tools/test_p2p_stream_server_v32.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_diagnostics_v26.py" "$project_root" || return 1
  python3 "$project_root/tools/test_v39_diagnostic_api_compat.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v39.py" "$project_root" || return 1
}

verify_relative_ai_rpath(){
  local exe="$1" dynamic
  dynamic="$(readelf -d "$exe" 2>/dev/null)" || return 1
  [[ "$dynamic" == *'$ORIGIN/components/ai/runtime/lib'* ]] || { printf 'FAIL: relative AI RPATH missing.\n'; return 1; }
  [[ "$dynamic" != *'/DKLab/Projects/ReddMedia/'* ]] || { printf 'FAIL: obsolete absolute RPATH leaked.\n'; return 1; }
  if ldd "$exe" 2>/dev/null | grep -q 'not found'; then
    printf 'FAIL: unresolved shared-library dependency detected.\n'
    ldd "$exe" 2>/dev/null | grep 'not found' || true
    return 1
  fi
  printf 'Relative runtime/RPATH dependency check PASS.\n'
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
  HOME="$build_root/test-home-v38" "$exe" --v38-library-live-tv-player-self-test || return 1
  HOME="$build_root/test-home-v39" "$exe" --v39-diagnostic-self-test || return 1
}

build_stub(){
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  local exe="$build_root/stub/Nougat_Media_Suite_v39"
  [[ -x "$exe" ]] || { printf 'FAIL: stub v39 executable missing.\n'; return 1; }
  [[ "$("$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.39" ]] || return 1
  run_regression_exe_tests "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v39.py" "$project_root" "$exe" || return 1
}

verify_security_runtime(){
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
  local exe="$build_root/full/Nougat_Media_Suite_v39"
  [[ -x "$exe" ]] || { printf 'FAIL: full native v39 executable missing.\n'; return 1; }
  verify_relative_ai_rpath "$exe" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$exe" --version 2>/dev/null)" == "Nougat Media Suite v0.0.39" ]] || return 1
  run_regression_exe_tests "$exe" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v39.py" "$project_root" "$exe" || return 1
  "$exe" --embedding-model-test || return 1
}

install_final(){
  local built="$build_root/full/Nougat_Media_Suite_v39"
  [[ -x "$built" ]] || return 1
  atomic_replace_file "$built" "$project_root/Nougat_Media_Suite_v39" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v39" || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v39" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v39" --version 2>/dev/null)" == "Nougat Media Suite v0.0.39" ]] || return 1
  run_regression_exe_tests "$project_root/Nougat_Media_Suite_v39" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v39.py" "$project_root" "$project_root/Nougat_Media_Suite_v39" || return 1
  # Owner rule: a successful candidate may not leave any persisted Live TV
  # channel with blank/text/invented artwork. This uses the real current lineup.
  "$project_root/Nougat_Media_Suite_v39" --v39-channel-logo-audit || return 1
}

install_launchers(){
  mkdir -p "$HOME/.local/share/applications" || return 1
  cp -a "$project_root/NougatMediaSuite.desktop" "$launcher_unversioned" || return 1
  cp -a "$project_root/NougatMediaSuite_v39.desktop" "$launcher_v39" || return 1
  cp -a "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" "$canonical_launcher" || return 1
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
}

apply_raw_icon(){
  local exe="$1"
  if command -v gio >/dev/null 2>&1 && [[ -f "$master_icon" ]]; then
    gio set -t string "$exe" metadata::custom-icon "file://$master_icon" >/dev/null 2>&1 || return 1
  fi
  return 0
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

main(){
  phase_start "v0.0.39 rejected-build repair preflight"
  require_command git || return 1
  require_command python3 || return 1
  require_command cmake || return 1
  require_command g++ || return 1
  require_command readelf || return 1
  require_command ldd || return 1
  verify_manifest || return 1
  verify_install_base || return 1
  phase_pass "pushed v0.0.38 / rejected-v0.0.39 base verified"

  phase_start "dry-run rejected-build repair against current source"
  dry_run_repair_preflight || return 1
  phase_pass "repair patch and source gates verified before project modification"

  phase_start "save exact rejected-build touched-state rollback snapshot"
  save_rollback || return 1
  phase_pass "rollback snapshot saved"

  phase_start "apply complete v0.0.39 repair"
  if ! apply_repair_payload || ! run_source_tests; then
    restore_rollback
    return 1
  fi
  phase_pass "diagnostics, Live TV, regression-test and source repair applied"

  phase_start "warnings-as-errors stub build and retained runtime tests"
  if ! build_stub; then
    restore_rollback
    return 1
  fi
  phase_pass "stub build and retained v25-v39 runtime gates"

  phase_start "retained security runtime verification"
  if ! verify_security_runtime; then
    restore_rollback
    return 1
  fi
  phase_pass "security runtime preserved"

  phase_start "full native v0.0.39 build"
  if ! build_full; then
    restore_rollback
    return 1
  fi
  phase_pass "full native build, AI model probe and v39 diagnostic self-test"

  phase_start "install root executable, launchers and real-channel-art acceptance gate"
  if ! install_final || ! install_launchers || ! apply_raw_icon "$project_root/Nougat_Media_Suite_v39"; then
    restore_rollback
    return 1
  fi
  cleanup_generated_python_caches || true
  phase_pass "root v0.0.39 candidate installed and validated"

  printf '\nFINAL PASS: Nougat Media Suite v0.0.39 rejected build repaired and installed as a new candidate.\n'
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v39"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'DIAGNOSTICS: Passed / Needs Attention / Problem / Not Tested / Information; evidence, expected/observed, repair guidance, history, TXT/JSON/support bundle.\n'
  printf 'LIVE TV: cached guide preserved; current multiplex PSIP can harvest during playback; full other-frequency sweep waits for tuner idle.\n'
  printf 'CHANNEL ART: successful install requires every persisted channel to resolve to actual artwork; no text/number fallback is accepted.\n'
  printf 'REGRESSION: retained executable gates through v0.0.38 plus v0.0.39 diagnostics passed.\n'
  printf 'UI SHEET: docs/design/NOUGAT_UI_COMPONENT_SHEET_APPROVED.png is the owner-supplied sole approved UI authority; screenshots are reference evidence only.\n'
  printf 'VOLUME: protected accepted exact-sheet asset verified unchanged.\n'
  printf 'This is still a candidate until owner visual/hardware acceptance.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v39\n' "$project_root"
  return 0
}

main "$@"
