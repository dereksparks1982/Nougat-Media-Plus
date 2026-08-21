#!/usr/bin/env bash

# Nougat Media Suite v0.0.22 license-protection candidate installer.
# Deliberately avoids `set -e` and `exit` so a failed pasted command never closes the owner's terminal.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
manifest_path="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v22.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
accepted_snapshot="$archive_parent/Nougat_Media_Suite_v0.0.21_ACCEPTED_20260821_025505"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_22_LICENSE_$stamp"
build_root="${TMPDIR:-/tmp}/nougat-media-suite-v0_0_22-build-$stamp-$$"
expected_base_commit="b89b0fc187bebc05f3390a76c76cbdc980713600"
expected_base_tag="v0.0.21"
applied=0
user_launcher="$HOME/.local/share/applications/NougatMediaSuite.desktop"
pinned_model="$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_bytes="84106624"
pinned_model_sha="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
ai_runtime_root="$project_root/components/ai/runtime"
ai_runtime_search_path="$ai_runtime_root/lib:$ai_runtime_root/lib64"

modified_paths=(
  "APPLY_COMMAND.txt"
  "CHANGELOG.md"
  "CMakeLists.txt"
  "COMPANY_BIBLE.md"
  "DEPENDENCIES.md"
  "LICENSE"
  "NougatMediaSuite.desktop"
  "README.md"
  "ROADMAP.md"
  "THIRD_PARTY_NOTICES.md"
  "src/main.cpp"
)

added_paths=(
  ".github/PULL_REQUEST_TEMPLATE.md"
  "CONTRIBUTING.md"
  "COPYRIGHT.md"
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_22.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v22.json"
  "NougatMediaSuite_v22.desktop"
  "docs/LICENSING_POLICY.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_22_LICENSE_PROTECTION_HANDSHAKE.md"
  "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_22_LICENSE_PROTECTION_VALIDATION.md"
  "tools/test_installer_rollback_v22.py"
  "tools/test_license_protection_v22.py"
  "tools/test_nougat_media_suite_retained_v22.py"
  "tools/test_nougat_media_suite_ui_smoke_v22.py"
  "tools/test_nougat_media_suite_v22.py"
)

deleted_on_success=(
  "INSTALL_NOUGAT_MEDIA_SUITE_v0_0_21.sh"
  "NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v21.json"
  "NougatMediaSuite_v21.desktop"
  "tools/test_installer_rollback_v21.py"
  "tools/test_nougat_media_suite_retained_v21.py"
  "tools/test_nougat_media_suite_ui_smoke_v21.py"
  "tools/test_nougat_media_suite_v21.py"
)

phase_start() { printf '\nPHASE START: %s\n' "$1"; }
phase_pass() { printf 'PHASE PASS: %s\n' "$1"; }

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then printf 'FAIL: required command not found: %s\n' "$1"; return 1; fi
  return 0
}

run_accepted_v21() {
  # v0.0.21 was accepted before the local project directory was renamed. Its
  # embedded absolute RPATH still points at the former ReddMedia path, so use
  # the accepted runtime at the new project location only for the base proof.
  env LD_LIBRARY_PATH="$ai_runtime_search_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    "$project_root/Nougat_Media_Suite_v21" "$@"
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

ensure_ui_smoke_tools() {
  local missing=()
  command -v xvfb-run >/dev/null 2>&1 || missing+=(xvfb)
  command -v xwininfo >/dev/null 2>&1 || missing+=(x11-utils)
  if [[ "${#missing[@]}" -eq 0 ]]; then printf 'X11 UI smoke tools OK: xvfb-run and xwininfo\n'; return 0; fi
  if ! command -v apt-get >/dev/null 2>&1 || ! command -v sudo >/dev/null 2>&1; then printf 'FAIL: missing UI smoke tools: %s\n' "${missing[*]}"; return 1; fi
  phase_start "install missing UI smoke prerequisites"
  if ! sudo apt-get update || ! sudo apt-get install -y "${missing[@]}"; then printf 'FAIL: UI smoke prerequisite installation failed\n'; return 1; fi
  phase_pass "install missing UI smoke prerequisites"
  command -v xvfb-run >/dev/null 2>&1 && command -v xwininfo >/dev/null 2>&1
}

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

verify_fts5() {
  python3 - <<'PY'
import sqlite3
c=sqlite3.connect(':memory:')
try: c.execute('CREATE VIRTUAL TABLE t USING fts5(body)')
except Exception as e:
    print(f'FAIL: Python sqlite3 FTS5 unavailable: {e}'); raise SystemExit(1)
finally: c.close()
print('Python SQLite FTS5 OK')
PY
}

verify_file_exact() {
  python3 - "$1" "$2" "$3" <<'PY'
import hashlib,pathlib,sys
p=pathlib.Path(sys.argv[1]); n=int(sys.argv[2]); expected=sys.argv[3]
if not p.is_file() or p.stat().st_size!=n: raise SystemExit(1)
h=hashlib.sha256()
with p.open('rb') as f:
    for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
raise SystemExit(0 if h.hexdigest()==expected else 1)
PY
}

verify_git_base() {
  if [[ ! -d "$project_root/.git" ]]; then printf 'FAIL: Git repository missing: %s\n' "$project_root/.git"; return 1; fi
  local branch head tag_commit tracked
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  tag_commit="$(git -C "$project_root" rev-list -n 1 "$expected_base_tag" 2>/dev/null)"
  tracked="$(git -C "$project_root" status --porcelain --untracked-files=no)"
  if [[ "$branch" != "main" ]]; then printf 'FAIL: expected branch main, found %s\n' "${branch:-unknown}"; return 1; fi
  if [[ "$head" != "$expected_base_commit" ]]; then printf 'FAIL: HEAD %s is not accepted v0.0.21 %s\n' "${head:-unknown}" "$expected_base_commit"; return 1; fi
  if [[ "$tag_commit" != "$expected_base_commit" ]]; then printf 'FAIL: local tag %s does not resolve to accepted v0.0.21 commit\n' "$expected_base_tag"; return 1; fi
  if [[ -n "$tracked" ]]; then printf 'FAIL: tracked files differ from accepted v0.0.21:\n%s\n' "$tracked"; return 1; fi
  if [[ ! -d "$accepted_snapshot" ]]; then printf 'FAIL: accepted v0.0.21 snapshot is missing: %s\n' "$accepted_snapshot"; return 1; fi
  printf 'Git preflight PASS: main/tag at accepted v0.0.21 commit %s; tracked tree clean.\n' "$expected_base_commit"
}

verify_manifest() {
  python3 - "$payload_root" "$manifest_path" "$expected_base_commit" "$expected_base_tag" <<'PY'
import hashlib,json,pathlib,sys
payload=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2]); expected_commit=sys.argv[3]; expected_tag=sys.argv[4]
if not mp.is_file(): print('FAIL: package manifest missing'); raise SystemExit(1)
m=json.loads(mp.read_text(encoding='utf-8'))
if m.get('expected_base_commit')!=expected_commit or m.get('expected_base_tag')!=expected_tag:
    print('FAIL: manifest base identity mismatch'); raise SystemExit(1)
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''): h.update(b)
    return h.hexdigest()
for rel,record in m.get('payload',{}).items():
    p=payload/rel
    if not p.is_file() or p.stat().st_size!=record['bytes'] or sha(p)!=record['sha256']:
        print(f'FAIL: payload mismatch: {rel}'); raise SystemExit(1)
print(f"Manifest verified: {len(m.get('payload',{}))} payload files; base commit/tag identity verified.")
PY
}

verify_candidate_absent() {
  local rel
  for rel in "${added_paths[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then printf 'FAIL: v0.0.22 added path already exists: %s\n' "$rel"; return 1; fi
  done
  if [[ -e "$project_root/Nougat_Media_Suite_v22" ]]; then printf 'FAIL: Nougat_Media_Suite_v22 already exists.\n'; return 1; fi
}

save_rollback_snapshot() {
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell" || return 1
  local rel
  for rel in "${modified_paths[@]}" "${deleted_on_success[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then mkdir -p "$rollback_root/project/$(dirname "$rel")" || return 1; cp -a "$project_root/$rel" "$rollback_root/project/$rel" || return 1; fi
  done
  if [[ -e "$project_root/Nougat_Media_Suite_v21" ]]; then cp -a "$project_root/Nougat_Media_Suite_v21" "$rollback_root/project/Nougat_Media_Suite_v21" || return 1; fi
  if [[ -e "$user_launcher" ]]; then cp -a "$user_launcher" "$rollback_root/user-shell/NougatMediaSuite.desktop" || return 1; fi
  cat > "$rollback_root/ROLLBACK_INFO.txt" <<INFO
Pre-v0.0.22 license-protection rollback snapshot
Created: $(date -Is)
Accepted base commit: $expected_base_commit
Base executable: Nougat_Media_Suite_v21
Candidate executable: Nougat_Media_Suite_v22
Generated runtimes, pinned model, user media, Search data, and user configuration are intentionally preserved outside this touched-file snapshot.
INFO
}

apply_payload() {
  local rel
  for rel in "${modified_paths[@]}" "${added_paths[@]}"; do
    mkdir -p "$project_root/$(dirname "$rel")" || return 1
    cp -a "$payload_root/$rel" "$project_root/$rel" || { printf 'FAIL: could not apply %s\n' "$rel"; return 1; }
  done
  chmod +x "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_22.sh" \
    "$project_root/tools/test_license_protection_v22.py" \
    "$project_root/tools/test_nougat_media_suite_v22.py" \
    "$project_root/tools/test_nougat_media_suite_retained_v22.py" \
    "$project_root/tools/test_nougat_media_suite_ui_smoke_v22.py" \
    "$project_root/tools/test_installer_rollback_v22.py" || return 1
  applied=1
}

restore_rollback() {
  [[ "$applied" == "1" ]] || return 0
  printf '\nROLLBACK START: restoring accepted v0.0.21 touched state\n'
  local rel
  for rel in "${added_paths[@]}"; do rm -rf -- "$project_root/$rel"; done
  rm -f -- "$project_root/Nougat_Media_Suite_v22"
  for rel in "${modified_paths[@]}" "${deleted_on_success[@]}"; do
    if [[ -e "$rollback_root/project/$rel" ]]; then mkdir -p "$project_root/$(dirname "$rel")"; cp -a "$rollback_root/project/$rel" "$project_root/$rel"; else rm -rf -- "$project_root/$rel"; fi
  done
  if [[ -e "$rollback_root/project/Nougat_Media_Suite_v21" ]]; then cp -a "$rollback_root/project/Nougat_Media_Suite_v21" "$project_root/Nougat_Media_Suite_v21"; fi
  if [[ -e "$rollback_root/user-shell/NougatMediaSuite.desktop" ]]; then mkdir -p "$(dirname "$user_launcher")"; cp -a "$rollback_root/user-shell/NougatMediaSuite.desktop" "$user_launcher"; fi
  printf 'ROLLBACK PASS: accepted v0.0.21 touched state restored; runtimes/user data preserved.\n'
}

run_source_tests() {
  python3 "$project_root/tools/test_nougat_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_nougat_media_suite_retained_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_media_server_lifecycle_v17.py" "$project_root" || return 1
  python3 "$project_root/tools/test_license_protection_v22.py" "$project_root" || return 1
  python3 "$project_root/tools/test_installer_rollback_v22.py" "$project_root" || return 1
}

build_stub() {
  cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON || return 1
  cmake --build "$build_root/stub" -j"$(nproc)" || return 1
  [[ "$("$build_root/stub/Nougat_Media_Suite_v22" --version)" == "Nougat Media Suite v0.0.22" ]] || { printf 'FAIL: stub version mismatch\n'; return 1; }
  "$build_root/stub/Nougat_Media_Suite_v22" --discover-ai-self-test || return 1
  python3 "$project_root/tools/test_nougat_media_suite_ui_smoke_v22.py" "$project_root" "$build_root/stub/Nougat_Media_Suite_v22" || return 1
}

build_full() {
  cmake -S "$project_root" -B "$build_root/full" || return 1
  cmake --build "$build_root/full" -j"$(nproc)" || return 1

  # The v22 executable deliberately resolves llama.cpp relative to itself.
  # Mirror the final project-relative runtime/model layout beside the temporary
  # build executable before running any loader-dependent checks.
  mkdir -p "$build_root/full/components/ai" || return 1
  rm -rf -- "$build_root/full/components/ai/runtime" "$build_root/full/components/ai/models"
  ln -s "$project_root/components/ai/runtime" "$build_root/full/components/ai/runtime" || return 1
  ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models" || return 1

  verify_relative_ai_rpath "$build_root/full/Nougat_Media_Suite_v22" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$build_root/full/Nougat_Media_Suite_v22" --version 2>/dev/null)" == "Nougat Media Suite v0.0.22" ]] || { printf 'FAIL: full native version/relative-runtime check failed\n'; return 1; }
  env -u LD_LIBRARY_PATH "$build_root/full/Nougat_Media_Suite_v22" --discover-ai-self-test || return 1
}

apply_raw_icon() {
  local exe="$1" icon="$project_root/assets/icons/nougat-media-suite.png" info
  [[ -f "$icon" ]] || { printf 'FAIL: approved Nougat Media Suite icon missing: %s\n' "$icon"; return 1; }
  gio set -t string "$exe" metadata::custom-icon "file://$icon" || return 1
  info="$(gio info -a metadata::custom-icon "$exe" 2>&1)"
  [[ "$info" == *"file://$icon"* ]] || { printf 'FAIL: custom-icon readback mismatch\n%s\n' "$info"; return 1; }
  printf 'Raw executable Nougat Media Suite icon metadata verified.\n'
}

install_final_candidate() {
  cp "$build_root/full/Nougat_Media_Suite_v22" "$project_root/Nougat_Media_Suite_v22" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v22" || return 1
  [[ -x "$project_root/Nougat_Media_Suite_v22" ]] || return 1
  verify_relative_ai_rpath "$project_root/Nougat_Media_Suite_v22" || return 1
  [[ "$(env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v22" --version 2>/dev/null)" == "Nougat Media Suite v0.0.22" ]] || return 1
  env -u LD_LIBRARY_PATH "$project_root/Nougat_Media_Suite_v22" --discover-ai-self-test || return 1
  apply_raw_icon "$project_root/Nougat_Media_Suite_v22" || return 1

  mkdir -p "$HOME/.local/share/applications" || return 1
  cp "$project_root/NougatMediaSuite.desktop" "$user_launcher" || return 1
  command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
  command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true
  command -v nautilus >/dev/null 2>&1 && nautilus -q >/dev/null 2>&1 || true

  local rel
  for rel in "${deleted_on_success[@]}"; do rm -f -- "$project_root/$rel"; done
  rm -f -- "$project_root/Nougat_Media_Suite_v21"
}

main() {
  phase_start "accepted Nougat Media Suite v0.0.21 base and prerequisites"
  if [[ ! -d "$project_root" ]]; then printf 'FAIL: project not found: %s\n' "$project_root"; printf '\nFINAL FAIL: v0.0.22 not installed. Terminal remains open.\n'; return 1; fi
  local cmd
  for cmd in cmake g++ pkg-config python3 rsync gio git env readelf; do require_command "$cmd" || { printf '\nFINAL FAIL: prerequisites incomplete. Terminal remains open.\n'; return 1; }; done
  ensure_ui_smoke_tools || { printf '\nFINAL FAIL: UI smoke prerequisites incomplete. Terminal remains open.\n'; return 1; }
  pkg-config --exists libtorrent-rasterbar || { printf 'FAIL: libtorrent-rasterbar development package unavailable.\n'; return 1; }
  verify_fts5 && verify_port_8096_free || { printf '\nFINAL FAIL: runtime preflight failed. Terminal remains open.\n'; return 1; }
  [[ -x "$project_root/Nougat_Media_Suite_v21" ]] || { printf 'FAIL: accepted root Nougat_Media_Suite_v21 is missing/not executable.\n'; return 1; }
  [[ -f "$project_root/components/ai/runtime/include/llama.h" && -e "$project_root/components/ai/runtime/lib/libllama.so.0" ]] || { printf 'FAIL: accepted AI runtime missing.\n'; return 1; }
  [[ "$(run_accepted_v21 --version 2>/dev/null)" == "Nougat Media Suite v0.0.21" ]] || { printf 'FAIL: accepted root Nougat_Media_Suite_v21 version check failed even with the relocated accepted AI runtime.\n'; return 1; }
  printf 'Accepted v0.0.21 executable verified with relocated AI runtime at the Nougat Media Suite project path.\n'
  [[ -f "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]] || { printf 'FAIL: integrated Jellyfin runtime missing.\n'; return 1; }
  verify_file_exact "$pinned_model" "$pinned_model_bytes" "$pinned_model_sha" || { printf 'FAIL: pinned Nomic model missing/changed.\n'; return 1; }
  verify_git_base || { printf '\nFINAL FAIL: accepted Git base preflight failed. Terminal remains open.\n'; return 1; }
  verify_manifest || { printf '\nFINAL FAIL: package verification failed. Terminal remains open.\n'; return 1; }
  verify_candidate_absent || { printf '\nFINAL FAIL: candidate-path preflight failed. Terminal remains open.\n'; return 1; }
  phase_pass "accepted Nougat Media Suite v0.0.21 base and prerequisites"

  phase_start "save exact pre-v0.0.22 rollback snapshot"
  save_rollback_snapshot || { printf '\nFINAL FAIL: rollback snapshot failed before source changes. Terminal remains open.\n'; return 1; }
  phase_pass "save exact pre-v0.0.22 rollback snapshot"

  phase_start "apply Nougat Media Suite v0.0.22 license-protection files"
  apply_payload || { restore_rollback; printf '\nFINAL FAIL: payload application failed. Terminal remains open.\n'; return 1; }
  phase_pass "apply Nougat Media Suite v0.0.22 license-protection files"

  phase_start "license boundary and retained behavior tests"
  run_source_tests || { restore_rollback; printf '\nFINAL FAIL: source/license validation failed; accepted v0.0.21 restored. Terminal remains open.\n'; return 1; }
  phase_pass "license boundary and retained behavior tests"

  phase_start "deterministic warnings-as-errors build and X11 smoke"
  mkdir -p "$build_root" && build_stub || { restore_rollback; printf '\nFINAL FAIL: deterministic build/UI smoke failed; accepted v0.0.21 restored. Terminal remains open.\n'; return 1; }
  phase_pass "deterministic warnings-as-errors build and X11 smoke"

  phase_start "full native Nougat Media Suite v0.0.22 build"
  build_full || { restore_rollback; printf '\nFINAL FAIL: full native build failed; accepted v0.0.21 restored. Terminal remains open.\n'; return 1; }
  phase_pass "full native Nougat Media Suite v0.0.22 build"

  phase_start "install and verify root Nougat_Media_Suite_v22"
  install_final_candidate || { restore_rollback; printf '\nFINAL FAIL: final executable/icon/launcher verification failed; accepted v0.0.21 restored. Terminal remains open.\n'; return 1; }
  phase_pass "install and verify root Nougat_Media_Suite_v22"

  rm -rf -- "$build_root"
  printf '\nFINAL PASS: Nougat Media Suite v0.0.22 license-protection/runtime-path-repair candidate installed and validated.\n'
  printf 'Project repository path: %s\n' "$project_root"
  printf 'Executable: %s\n' "$project_root/Nougat_Media_Suite_v22"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'OWNER CHECK REQUIRED: review LICENSE, COPYRIGHT.md, CONTRIBUTING.md, THIRD_PARTY_NOTICES.md, and confirm normal app behavior.\n'
  printf 'Launch: cd "%s" && ./Nougat_Media_Suite_v22\n' "$project_root"
  return 0
}

main "$@"
