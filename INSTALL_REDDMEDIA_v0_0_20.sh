#!/usr/bin/env bash

# ReddMedia v0.0.20 Stream / palettes / volume / Library-view candidate installer.
# Accepted-base manifest repair: requires the exact final accepted v0.0.19 manifest and pinned AI model; the unchanged model is not re-shipped.
# Deliberately avoids `set -e` and `exit` so a failed pasted command never closes the owner's terminal.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/ReddMedia"
manifest_path="$payload_root/REDDMEDIA_PATCH_MANIFEST_v20.json"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
stamp="$(date +%Y%m%d_%H%M%S)"
rollback_root="$archive_parent/ReddMedia_pre_v0_0_20_$stamp"
build_root="${TMPDIR:-/tmp}/reddmedia-v0_0_20-build-$stamp-$$"
expected_base_commit="e1cf0aa1ca55757c8048e5f142f9c1d92f23cb26"
accepted_v19_manifest_sha256="26106d40b8878d08c7fb759217c6a56ad8c5221df1a732028261450c272f8a47"
applied=0
launcher_backup_present=0
launcher_backup="$rollback_root/user-launcher/ReddMedia.desktop"
user_launcher="$HOME/.local/share/applications/ReddMedia.desktop"
pinned_model_rel="components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
pinned_model_target="$project_root/$pinned_model_rel"
pinned_model_expected_bytes="84106624"
pinned_model_expected_sha256="d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"

modified_paths=(
  "APPLY_COMMAND.txt"
  "CHANGELOG.md"
  "CMakeLists.txt"
  "COMPANY_BIBLE.md"
  "DEPENDENCIES.md"
  "README.md"
  "ROADMAP.md"
  "ReddMedia.desktop"
  "src/main.cpp"
)

added_paths=(
  "INSTALL_REDDMEDIA_v0_0_20.sh"
  "REDDMEDIA_PATCH_MANIFEST_v20.json"
  "ReddMedia_v20.desktop"
  "docs/builds/REDDMEDIA_v0_0_20_STREAM_PALETTES_VOLUME_LIBRARY_VIEWS_HANDSHAKE.md"
  "docs/builds/REDDMEDIA_v0_0_20_STREAM_PALETTES_VOLUME_LIBRARY_VIEWS_VALIDATION.md"
  "tools/test_reddmedia_ui_smoke_v20.py"
  "tools/test_reddmedia_v20.py"
  "tools/test_installer_rollback_v20.py"
)

deleted_on_success=(
  "INSTALL_REDDMEDIA_v0_0_19.sh"
  "REDDMEDIA_PATCH_MANIFEST_v19.json"
  "ReddMedia_v19.desktop"
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

ensure_ui_smoke_tools() {
  local missing=()
  if ! command -v xvfb-run >/dev/null 2>&1; then missing+=(xvfb); fi
  if ! command -v xwininfo >/dev/null 2>&1; then missing+=(x11-utils); fi
  if [[ "${#missing[@]}" -eq 0 ]]; then
    printf 'X11 UI smoke tools OK: xvfb-run and xwininfo\n'
    return 0
  fi
  if ! command -v apt-get >/dev/null 2>&1 || ! command -v sudo >/dev/null 2>&1; then
    printf 'FAIL: missing UI smoke tools and automatic Ubuntu package installation is unavailable: %s\n' "${missing[*]}"
    return 1
  fi
  printf 'INFO: installing missing Ubuntu UI smoke packages: %s\n' "${missing[*]}"
  phase_start "apt package refresh"
  if ! sudo apt-get update; then
    printf 'FAIL: apt package refresh failed\n'
    return 1
  fi
  phase_pass "apt package refresh"
  phase_start "install missing UI smoke prerequisites"
  if ! sudo apt-get install -y "${missing[@]}"; then
    printf 'FAIL: could not install required UI smoke packages\n'
    return 1
  fi
  phase_pass "install missing UI smoke prerequisites"
  if ! command -v xvfb-run >/dev/null 2>&1 || ! command -v xwininfo >/dev/null 2>&1; then
    printf 'FAIL: xvfb-run/xwininfo still unavailable after package installation\n'
    return 1
  fi
  printf 'X11 UI smoke tools OK: xvfb-run and xwininfo\n'
  return 0
}

verify_port_8096_free() {
  python3 - <<'PY'
import socket
s=socket.socket(); s.settimeout(0.25)
used=s.connect_ex(('127.0.0.1',8096)) == 0
s.close()
if used:
    print('FAIL: localhost port 8096 is in use. Close ReddMedia/Jellyfin before applying this candidate.')
    raise SystemExit(1)
print('Port 8096 is free.')
PY
}

verify_fts5() {
  python3 - <<'PY'
import sqlite3
con=sqlite3.connect(':memory:')
try:
    con.execute('CREATE VIRTUAL TABLE t USING fts5(body)')
except Exception as exc:
    print(f'FAIL: Python sqlite3 does not provide FTS5: {exc}')
    raise SystemExit(1)
finally:
    con.close()
print('Python SQLite FTS5 OK')
PY
}

verify_git_base() {
  if [[ ! -d "$project_root/.git" ]]; then
    printf 'FAIL: ReddMedia Git repository is missing: %s\n' "$project_root/.git"
    return 1
  fi
  local branch head tracked_changes
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  if [[ "$branch" != "main" ]]; then
    printf 'FAIL: ReddMedia Git branch is %s, expected main.\n' "${branch:-unknown}"
    return 1
  fi
  if [[ "$head" != "$expected_base_commit" ]]; then
    printf 'FAIL: ReddMedia HEAD is %s, expected accepted v0.0.19 commit %s.\n' "${head:-unknown}" "$expected_base_commit"
    return 1
  fi
  tracked_changes="$(git -C "$project_root" status --porcelain --untracked-files=no)"
  if [[ -n "$tracked_changes" ]]; then
    printf 'FAIL: tracked ReddMedia files differ from accepted v0.0.19.\n%s\n' "$tracked_changes"
    return 1
  fi
  printf 'Git preflight PASS: main at accepted v0.0.19 commit %s; tracked tree clean.\n' "$expected_base_commit"
  return 0
}

verify_manifest_and_base() {
  python3 - "$payload_root" "$project_root" "$manifest_path" "$accepted_v19_manifest_sha256" <<'PY'
import hashlib, json, pathlib, sys
payload=pathlib.Path(sys.argv[1]); project=pathlib.Path(sys.argv[2]); manifest_path=pathlib.Path(sys.argv[3]); accepted_manifest_sha=sys.argv[4]
if not manifest_path.is_file():
    print('FAIL: package manifest missing')
    raise SystemExit(1)
manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
accepted_rec=manifest.get('base_expected',{}).get('REDDMEDIA_PATCH_MANIFEST_v19.json',{})
if accepted_rec.get('sha256') != accepted_manifest_sha:
    print('FAIL: package carries a stale v0.0.19 base-manifest hash')
    raise SystemExit(1)

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''): h.update(block)
    return h.hexdigest()

for rel, record in manifest.get('payload',{}).items():
    path=payload/rel
    if not path.is_file():
        print(f'FAIL: packaged payload file missing: {rel}')
        raise SystemExit(1)
    if path.stat().st_size != record['bytes'] or sha(path) != record['sha256']:
        print(f'FAIL: packaged payload hash mismatch: {rel}')
        raise SystemExit(1)
for rel, record in manifest.get('base_expected',{}).items():
    path=project/rel
    if not path.is_file():
        print(f'FAIL: required v0.0.19 base file missing: {rel}')
        raise SystemExit(1)
    if path.stat().st_size != record['bytes'] or sha(path) != record['sha256']:
        print(f'FAIL: accepted v0.0.19 base file differs: {rel}')
        raise SystemExit(1)
print(f"Manifest verified: {len(manifest.get('payload',{}))} payload files; exact final accepted v0.0.19 base verified.")
PY
}

verify_file_exact() {
  local path="$1" expected_bytes="$2" expected_sha="$3"
  python3 - "$path" "$expected_bytes" "$expected_sha" <<'PY'
import hashlib, pathlib, sys
path=pathlib.Path(sys.argv[1]); expected_bytes=int(sys.argv[2]); expected_sha=sys.argv[3]
if not path.is_file() or path.stat().st_size != expected_bytes:
    raise SystemExit(1)
h=hashlib.sha256()
with path.open('rb') as f:
    for block in iter(lambda:f.read(1024*1024),b''): h.update(block)
raise SystemExit(0 if h.hexdigest()==expected_sha else 1)
PY
}

verify_existing_pinned_ai_model() {
  if ! verify_file_exact "$pinned_model_target" "$pinned_model_expected_bytes" "$pinned_model_expected_sha256"; then
    printf 'FAIL: accepted v0.0.19 pinned Nomic model is missing or does not match the accepted runtime asset: %s\n' "$pinned_model_target"
    printf 'Expected bytes: %s\n' "$pinned_model_expected_bytes"
    printf 'Expected SHA-256: %s\n' "$pinned_model_expected_sha256"
    return 1
  fi
  printf 'Accepted pinned offline embedding model verified in place: %s\n' "$pinned_model_target"
  return 0
}

verify_candidate_paths_absent() {
  local rel
  for rel in "${added_paths[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then
      printf 'FAIL: v0.0.20 candidate-only path already exists: %s\n' "$rel"
      return 1
    fi
  done
  if [[ -e "$project_root/ReddMedia_v20" ]]; then
    printf 'FAIL: ReddMedia_v20 already exists. This installer will not overwrite an unknown candidate.\n'
    return 1
  fi
  return 0
}

save_rollback_snapshot() {
  if ! mkdir -p "$rollback_root/project" "$rollback_root/user-launcher"; then return 1; fi
  local rel
  for rel in "${modified_paths[@]}" "${deleted_on_success[@]}"; do
    if [[ -e "$project_root/$rel" ]]; then
      if ! mkdir -p "$rollback_root/project/$(dirname "$rel")"; then return 1; fi
      if ! cp -a "$project_root/$rel" "$rollback_root/project/$rel"; then return 1; fi
    fi
  done
  if [[ -e "$project_root/ReddMedia_v19" ]]; then
    if ! cp -a "$project_root/ReddMedia_v19" "$rollback_root/project/ReddMedia_v19"; then return 1; fi
  fi
  if [[ -e "$user_launcher" ]]; then
    launcher_backup_present=1
    if ! cp -a "$user_launcher" "$launcher_backup"; then return 1; fi
  fi
  cat > "$rollback_root/ROLLBACK_INFO.txt" <<EOF_INFO
ReddMedia pre-v0.0.20 rollback snapshot
Created: $(date -Is)
Project: $project_root
Accepted base commit: $expected_base_commit
Base executable: ReddMedia_v19
Candidate: ReddMedia_v20 Stream / palettes / volume / Library views
Generated runtimes and user data are intentionally outside this changed-file snapshot.
EOF_INFO
  return 0
}

apply_payload() {
  local rel
  for rel in "${modified_paths[@]}" "${added_paths[@]}"; do
    if ! mkdir -p "$project_root/$(dirname "$rel")"; then return 1; fi
    if ! cp -a "$payload_root/$rel" "$project_root/$rel"; then
      printf 'FAIL: could not apply payload path: %s\n' "$rel"
      return 1
    fi
  done
  if ! chmod +x "$project_root/INSTALL_REDDMEDIA_v0_0_20.sh" \
           "$project_root/tools/test_reddmedia_v20.py" \
           "$project_root/tools/test_reddmedia_ui_smoke_v20.py" \
           "$project_root/tools/test_installer_rollback_v20.py" 2>/dev/null; then return 1; fi
  applied=1
  return 0
}

restore_rollback() {
  if [[ "$applied" != "1" ]]; then return 0; fi
  printf '\nROLLBACK START: restoring exact accepted v0.0.19 touched paths\n'
  local rel
  for rel in "${added_paths[@]}"; do rm -rf -- "$project_root/$rel"; done
  rm -f -- "$project_root/ReddMedia_v20"
  for rel in "${modified_paths[@]}" "${deleted_on_success[@]}"; do
    if [[ -e "$rollback_root/project/$rel" ]]; then
      mkdir -p "$project_root/$(dirname "$rel")"
      cp -a "$rollback_root/project/$rel" "$project_root/$rel"
    else
      rm -rf -- "$project_root/$rel"
    fi
  done
  if [[ -e "$rollback_root/project/ReddMedia_v19" ]]; then
    cp -a "$rollback_root/project/ReddMedia_v19" "$project_root/ReddMedia_v19"
  fi
  if [[ "$launcher_backup_present" == "1" && -e "$launcher_backup" ]]; then
    mkdir -p "$(dirname "$user_launcher")"
    cp -a "$launcher_backup" "$user_launcher"
  fi
  printf 'ROLLBACK PASS: accepted v0.0.19 touched source/executable restored; generated runtimes and user data preserved.\n'
  return 0
}

run_source_tests() {
  if ! python3 "$project_root/tools/test_nougat_v19.py" "$project_root"; then return 1; fi
  if ! python3 "$project_root/tools/test_nougat_bridge_v19.py" "$project_root"; then return 1; fi
  if ! python3 "$project_root/tools/test_reddmedia_v20.py" "$project_root"; then return 1; fi
  if ! python3 "$project_root/tools/test_media_server_lifecycle_v17.py" "$project_root"; then return 1; fi
  if ! python3 "$project_root/tools/test_installer_rollback_v20.py" "$project_root"; then return 1; fi
  return 0
}

build_stub() {
  if ! cmake -S "$project_root" -B "$build_root/stub" -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON; then return 1; fi
  if ! cmake --build "$build_root/stub" -j"$(nproc)"; then return 1; fi
  if [[ "$("$build_root/stub/ReddMedia_v20" --version)" != "ReddMedia v0.0.20" ]]; then
    printf 'FAIL: stub executable version mismatch\n'
    return 1
  fi
  if ! "$build_root/stub/ReddMedia_v20" --discover-ai-self-test; then return 1; fi
  if ! python3 "$project_root/tools/test_reddmedia_ui_smoke_v20.py" "$project_root" "$build_root/stub/ReddMedia_v20"; then return 1; fi
  return 0
}

build_full() {
  if ! cmake -S "$project_root" -B "$build_root/full"; then return 1; fi
  if ! cmake --build "$build_root/full" -j"$(nproc)"; then return 1; fi
  if [[ "$("$build_root/full/ReddMedia_v20" --version)" != "ReddMedia v0.0.20" ]]; then
    printf 'FAIL: full native executable version mismatch\n'
    return 1
  fi
  if ! mkdir -p "$build_root/full/components/ai"; then return 1; fi
  rm -rf -- "$build_root/full/components/ai/models"
  if ! ln -s "$project_root/components/ai/models" "$build_root/full/components/ai/models"; then return 1; fi
  if ! "$build_root/full/ReddMedia_v20" --discover-ai-self-test; then return 1; fi
  return 0
}

apply_raw_executable_icon() {
  local executable="$1"
  local icon="$project_root/assets/icons/reddmedia.png"
  if [[ ! -f "$icon" ]]; then
    printf 'FAIL: approved red-tree icon missing: %s\n' "$icon"
    return 1
  fi
  if ! gio set -t string "$executable" metadata::custom-icon "file://$icon"; then
    printf 'FAIL: could not assign red-tree metadata to raw executable\n'
    return 1
  fi
  local info
  info="$(gio info -a metadata::custom-icon "$executable" 2>&1)"
  if [[ "$info" != *"file://$icon"* ]]; then
    printf 'FAIL: red-tree metadata readback did not match final executable\n%s\n' "$info"
    return 1
  fi
  printf 'Raw executable icon metadata verified: %s\n' "$icon"
  return 0
}

install_final_candidate() {
  if ! cp "$build_root/full/ReddMedia_v20" "$project_root/ReddMedia_v20"; then return 1; fi
  if ! chmod +x "$project_root/ReddMedia_v20"; then return 1; fi
  if [[ ! -x "$project_root/ReddMedia_v20" ]]; then
    printf 'FAIL: root ReddMedia_v20 is missing or not executable\n'
    return 1
  fi
  if [[ "$("$project_root/ReddMedia_v20" --version)" != "ReddMedia v0.0.20" ]]; then
    printf 'FAIL: root ReddMedia_v20 smoke/version check failed\n'
    return 1
  fi
  if ! "$project_root/ReddMedia_v20" --discover-ai-self-test; then
    printf 'FAIL: root ReddMedia_v20 could not load and use the pinned offline embedding model.\n'
    return 1
  fi
  if ! apply_raw_executable_icon "$project_root/ReddMedia_v20"; then return 1; fi

  if ! mkdir -p "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor/256x256/apps"; then return 1; fi
  if ! cp "$project_root/ReddMedia.desktop" "$user_launcher"; then return 1; fi
  if [[ -f "$project_root/assets/icons/reddmedia-256.png" ]]; then
    cp "$project_root/assets/icons/reddmedia-256.png" "$HOME/.local/share/icons/hicolor/256x256/apps/reddmedia.png" || return 1
  elif [[ -f "$project_root/assets/icons/reddmedia.png" ]]; then
    cp "$project_root/assets/icons/reddmedia.png" "$HOME/.local/share/icons/hicolor/256x256/apps/reddmedia.png" || return 1
  fi
  if command -v update-desktop-database >/dev/null 2>&1; then update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true; fi
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then gtk-update-icon-cache -f "$HOME/.local/share/icons/hicolor" >/dev/null 2>&1 || true; fi
  if command -v nautilus >/dev/null 2>&1; then nautilus -q >/dev/null 2>&1 || true; fi

  local rel
  for rel in "${deleted_on_success[@]}"; do rm -f -- "$project_root/$rel"; done
  rm -f -- "$project_root/ReddMedia_v19"
  return 0
}

main() {
  phase_start "accepted v0.0.19 base and prerequisites"
  if [[ ! -d "$project_root" ]]; then
    printf 'FAIL: ReddMedia project not found: %s\n' "$project_root"
    printf '\nFINAL FAIL: ReddMedia v0.0.20 was not installed. Terminal remains open.\n'
    return 1
  fi
  for cmd in cmake g++ pkg-config python3 rsync gio git; do
    if ! require_command "$cmd"; then
      printf '\nFINAL FAIL: prerequisites are incomplete. Terminal remains open.\n'
      return 1
    fi
  done
  if ! ensure_ui_smoke_tools; then
    printf '\nFINAL FAIL: UI smoke prerequisites are incomplete. Terminal remains open.\n'
    return 1
  fi
  if ! pkg-config --exists libtorrent-rasterbar; then
    printf 'FAIL: libtorrent-rasterbar development package is unavailable to pkg-config.\n'
    printf '\nFINAL FAIL: prerequisites are incomplete. Terminal remains open.\n'
    return 1
  fi
  if ! verify_fts5 || ! verify_port_8096_free; then
    printf '\nFINAL FAIL: runtime preflight failed. Terminal remains open.\n'
    return 1
  fi
  if [[ ! -x "$project_root/ReddMedia_v19" ]] || [[ "$("$project_root/ReddMedia_v19" --version 2>/dev/null)" != "ReddMedia v0.0.19" ]]; then
    printf 'FAIL: required accepted root base executable ReddMedia_v19 does not report ReddMedia v0.0.19.\n'
    return 1
  fi
  if [[ ! -f "$project_root/components/ai/runtime/include/llama.h" ]]; then
    printf 'FAIL: accepted pinned AI runtime is missing.\n'
    return 1
  fi
  if [[ ! -f "$project_root/components/jellyfin/runtime/jellyfin/jellyfin" ]]; then
    printf 'FAIL: accepted integrated Jellyfin runtime is missing.\n'
    return 1
  fi
  if ! verify_existing_pinned_ai_model; then
    printf '\nFINAL FAIL: accepted v0.0.19 pinned AI model is missing or changed. No source was modified. Terminal remains open.\n'
    return 1
  fi
  if ! verify_git_base; then
    printf '\nFINAL FAIL: accepted Git base preflight failed. Terminal remains open.\n'
    return 1
  fi
  if ! verify_manifest_and_base; then
    printf '\nFINAL FAIL: package/base verification failed. Terminal remains open.\n'
    return 1
  fi
  if ! verify_candidate_paths_absent; then
    printf '\nFINAL FAIL: candidate path preflight failed. Terminal remains open.\n'
    return 1
  fi
  phase_pass "accepted v0.0.19 base and prerequisites"

  phase_start "save exact pre-v0.0.20 rollback snapshot"
  if ! save_rollback_snapshot; then
    printf '\nFINAL FAIL: rollback snapshot could not be created. No source was changed. Terminal remains open.\n'
    return 1
  fi
  phase_pass "save exact pre-v0.0.20 rollback snapshot"

  phase_start "apply v0.0.20 changed files"
  if ! apply_payload; then
    restore_rollback
    printf '\nFINAL FAIL: payload application failed. Terminal remains open.\n'
    return 1
  fi
  phase_pass "apply v0.0.20 changed files"

  phase_start "v0.0.20 and retained ReddMedia behavior tests"
  if ! run_source_tests; then
    restore_rollback
    printf '\nFINAL FAIL: behavior validation failed and accepted v0.0.19 was restored. Terminal remains open.\n'
    return 1
  fi
  phase_pass "v0.0.20 and retained ReddMedia behavior tests"

  phase_start "deterministic native stub build and UI smoke"
  if ! mkdir -p "$build_root" || ! build_stub; then
    restore_rollback
    printf '\nFINAL FAIL: deterministic native build/UI smoke failed and accepted v0.0.19 was restored. Terminal remains open.\n'
    return 1
  fi
  phase_pass "deterministic native stub build and UI smoke"

  phase_start "full native ReddMedia v0.0.20 build"
  if ! build_full; then
    restore_rollback
    printf '\nFINAL FAIL: full native build failed and accepted v0.0.19 was restored. Terminal remains open.\n'
    return 1
  fi
  phase_pass "full native ReddMedia v0.0.20 build"

  phase_start "install and verify root ReddMedia_v20"
  if ! install_final_candidate; then
    restore_rollback
    printf '\nFINAL FAIL: final root executable/launcher verification failed and accepted v0.0.19 was restored. Terminal remains open.\n'
    return 1
  fi
  phase_pass "install and verify root ReddMedia_v20"

  rm -rf -- "$build_root"
  printf '\nFINAL PASS: ReddMedia v0.0.20 Stream/palette/volume/Library-view candidate installed and validated.\n'
  printf 'Project: %s\n' "$project_root"
  printf 'Executable: %s\n' "$project_root/ReddMedia_v20"
  printf 'Rollback snapshot: %s\n' "$rollback_root"
  printf 'OWNER CHECK REQUIRED: visually/functionally accept or reject this candidate.\n'
  printf 'Launch: cd "%s" && ./ReddMedia_v20\n' "$project_root"
  return 0
}

main "$@"
