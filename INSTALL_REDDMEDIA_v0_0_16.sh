#!/usr/bin/env bash
set -Eeuo pipefail

package_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$HOME/DKLab/Projects/ReddMedia"
required_commit="d67cf6e5e0e3ce3036adae5d9695147a7aa771e8"
snapshot_root="$HOME/DKLab/Archives/ReddMedia Archive/ReddMedia_v0_0_15_TECHNICALLY_WORKING_20260818_133901"
applied=0
build_root=""
health_root=""
health_pid=""
health_server_pid=""
recovery_started=0
recovery_backup=""

added_paths=(
  "components/ai/source/llama.cpp-pinned-source.zip"
  "components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
  "config/ai/pinned_ai_runtime.json"
  "REDDMEDIA_PATCH_MANIFEST.json"
  "docs/builds/REDDMEDIA_v0_0_16_NATIVE_LIBRARY_AND_DISCOVER_AI_VALIDATION.md"
  "docs/builds/REDDMEDIA_v0_0_16_NATIVE_LIBRARY_AND_DISCOVER_AI_HANDSHAKE.md"
  "docs/builds/REDDMEDIA_v0_0_16_NATIVE_LIBRARY_AND_DISCOVER_AI_REPAIR.md"
  "docs/builds/REDDMEDIA_v0_0_16_NATIVE_LIBRARY_AND_DISCOVER_AI_REPAIR_2.md"
  "licenses/ai/LLAMA_CPP_MIT_LICENSE.txt"
  "licenses/ai/NOMIC_EMBED_TEXT_APACHE_2_LICENSE.txt"
  "ReddMedia_v16.desktop"
  "src/media_server/library_poster.cpp"
  "src/media_server/library_poster.hpp"
  "src/recommendations/recommendation_types.hpp"
  "src/recommendations/viewing_history.cpp"
  "src/recommendations/viewing_history.hpp"
  "src/recommendations/recommendation_engine.cpp"
  "src/recommendations/recommendation_engine.hpp"
  "src/recommendations/embedding_engine.cpp"
  "src/recommendations/embedding_engine.hpp"
  "src/recommendations/tmdb_client.cpp"
  "src/recommendations/tmdb_client.hpp"
  "tools/test_reddmedia_discover_v16.py"
  "tools/test_media_server_lifecycle_v16.py"
  "tools/test_installer_rollback_v16.py"
  "INSTALL_REDDMEDIA_v0_0_16.sh"
)

modified_paths=(
  "CHANGELOG.md"
  "CMakeLists.txt"
  "DEPENDENCIES.md"
  "KNOWN_BUGS.md"
  "README.md"
  "ROADMAP.md"
  "ReddMedia.desktop"
  "THIRD_PARTY_NOTICES.md"
  "src/main.cpp"
  "src/media_server/jellyfin_api_client.cpp"
  "src/media_server/jellyfin_api_client.hpp"
  "src/media_server/media_server_manager.cpp"
  "src/media_server/media_server_manager.hpp"
)

exact_executable_pids() {
  local expected="$1"
  local proc actual
  for proc in /proc/[0-9]*; do
    [[ -e "$proc/exe" ]] || continue
    actual="$(readlink -f "$proc/exe" 2>/dev/null || true)"
    if [[ "$actual" == "$expected" ]]; then basename "$proc"; fi
  done
  return 0
}

stop_health_process() {
  if [[ -n "$health_pid" ]] && kill -0 "$health_pid" 2>/dev/null; then
    kill "$health_pid" 2>/dev/null || true
    for _ in $(seq 1 40); do
      if ! kill -0 "$health_pid" 2>/dev/null; then break; fi
      sleep 0.25
    done
    if kill -0 "$health_pid" 2>/dev/null; then kill -KILL "$health_pid" 2>/dev/null || true; fi
  fi
  if [[ -n "$health_pid" ]]; then wait "$health_pid" 2>/dev/null || true; fi
  health_pid=""
  if [[ -n "$health_server_pid" ]] && kill -0 "$health_server_pid" 2>/dev/null; then
    kill -TERM -- "-$health_server_pid" 2>/dev/null || kill -TERM "$health_server_pid" 2>/dev/null || true
    for _ in $(seq 1 40); do
      if ! kill -0 "$health_server_pid" 2>/dev/null; then break; fi
      sleep 0.25
    done
    if kill -0 "$health_server_pid" 2>/dev/null; then
      kill -KILL -- "-$health_server_pid" 2>/dev/null || kill -KILL "$health_server_pid" 2>/dev/null || true
    fi
  fi
  health_server_pid=""
}

cleanup_temporary_work() {
  stop_health_process
  if [[ -n "$build_root" && -d "$build_root" ]]; then
    case "$build_root" in
      "${TMPDIR:-/tmp}"/reddmedia-v16-build.*) rm -rf -- "$build_root" ;;
    esac
  fi
  build_root=""
  if [[ -n "$health_root" && -d "$health_root" ]]; then
    case "$health_root" in
      "${TMPDIR:-/tmp}"/reddmedia-v16-health.*) rm -rf -- "$health_root" ;;
    esac
  fi
  health_root=""
}

rollback() {
  cleanup_temporary_work
  if [[ $applied -eq 0 ]]; then
    if [[ $recovery_started -eq 1 ]]; then
      echo "ERROR: Known failed-candidate recovery did not complete." >&2
      if [[ -n "$recovery_backup" ]]; then
        echo "RECOVERY BACKUP: $recovery_backup" >&2
      fi
      return
    fi
    echo "PHASE PASS: No project files were changed."
    return
  fi
  echo "PHASE START: Rollback to committed v0.0.15 baseline"
  cd "$project_root"
  git restore --source "$required_commit" --staged --worktree -- .
  for path in "${added_paths[@]}" ReddMedia_v16 components/ai/runtime components/ai/runtime.new components/ai/runtime.old; do
    if [[ -d "$project_root/$path" ]]; then
      case "$project_root/$path" in "$project_root"/*) rm -rf -- "$project_root/$path" ;; esac
    elif [[ -e "$project_root/$path" ]]; then
      rm -f -- "$project_root/$path"
    fi
  done
  if [[ -f "$project_root/ReddMedia_v15.desktop" ]]; then
    cp "$project_root/ReddMedia_v15.desktop" "$project_root/ReddMedia.desktop"
    install -Dm644 "$project_root/ReddMedia.desktop" "$HOME/.local/share/applications/reddmedia.desktop" || true
  fi
  echo "PHASE PASS: Rollback to committed v0.0.15 baseline"
}

trap 'status=$?; if [[ $status -ne 0 ]]; then rollback; fi; exit $status' EXIT

preflight_check() {
  local message="$1"
  shift
  if ! "$@"; then
    echo "ERROR: $message" >&2
    false
  fi
}

recover_known_failed_original_candidate() {
  local -a expected_dirty=(
    "CHANGELOG.md"
    "CMakeLists.txt"
    "DEPENDENCIES.md"
    "KNOWN_BUGS.md"
    "README.md"
    "ROADMAP.md"
    "THIRD_PARTY_NOTICES.md"
    "src/main.cpp"
    "src/media_server/jellyfin_api_client.cpp"
    "src/media_server/jellyfin_api_client.hpp"
  )
  local -a actual_dirty=()
  local -a untracked=()
  local -A expected_hash=(
    ["CHANGELOG.md"]="29499f52bacdec203688aea8103226213d2e258c7ff6826b91274086da29ffe2"
    ["CMakeLists.txt"]="a90a945bb3eb91f8ffd0f219a3f6a770f5aa91bf43f801ff5f8575748e2a45bc"
    ["DEPENDENCIES.md"]="29bfa7c334dbe3723513bf4e5cf8facfd5b5590630d9a06dfddbfd20d755026c"
    ["KNOWN_BUGS.md"]="e5685c34ffc6652e773efcf4982e2d1041ecf900af1e94edaf997de1e43d604b"
    ["README.md"]="4b7a7bfc3ea605dc7f204f61a05127ef167a1b95d172d9458b7f314e228a5eb3"
    ["ROADMAP.md"]="9308e6153a1e54e30217357e27be88938915e82e12d94f8cc8228f25b1afe3af"
    ["THIRD_PARTY_NOTICES.md"]="fc2aafc31c3a886d70567b2e722c044ac0db8f71f62b59c45c7be31b64ee4b60"
    ["src/main.cpp"]="74424724c7bada91ece72d82c14157603440dbd980440e3b98a60ffcead53e89"
    ["src/media_server/jellyfin_api_client.cpp"]="d00831237081683d70bd9210370d48ed6c15d91be42e2e72ea5604201a0b0756"
    ["src/media_server/jellyfin_api_client.hpp"]="557cf0fc82d6e905742b98af8af98b56adb0949787b779aeccfde5499216f80a"
  )
  local manifest="REDDMEDIA_PATCH_MANIFEST.json"
  local path actual_expected expected_expected manifest_found=0

  mapfile -t actual_dirty < <(git diff --name-only | LC_ALL=C sort)
  if [[ ${#actual_dirty[@]} -eq 0 ]]; then return 0; fi
  if ! git diff --cached --quiet; then return 0; fi

  actual_expected="$(printf '%s\n' "${actual_dirty[@]}")"
  expected_expected="$(printf '%s\n' "${expected_dirty[@]}" | LC_ALL=C sort)"
  if [[ "$actual_expected" != "$expected_expected" ]]; then return 0; fi

  mapfile -t untracked < <(git ls-files --others --exclude-standard)
  for path in "${untracked[@]}"; do
    if [[ "$path" == "$manifest" ]]; then
      manifest_found=1
    elif [[ "$path" != components/jellyfin/runtime/* ]]; then
      return 0
    fi
  done
  if [[ $manifest_found -ne 1 ]]; then return 0; fi
  if [[ "$(sha256sum "$manifest" | cut -d' ' -f1)" != \
    "4684d45952ecc2c1e87424cc7fd16333ad92295a745546a32ee6aa465e78f6c4" ]]; then
    return 0
  fi
  for path in "${expected_dirty[@]}"; do
    if [[ ! -f "$path" ]]; then return 0; fi
    if [[ "$(sha256sum "$path" | cut -d' ' -f1)" != "${expected_hash[$path]}" ]]; then
      return 0
    fi
  done

  echo "PHASE START: Recover exact leftovers from rejected original v0.0.16 candidate"
  recovery_started=1
  mkdir -p "$HOME/.local/share/reddmedia/install-backups"
  recovery_backup="$(mktemp -d "$HOME/.local/share/reddmedia/install-backups/v16-failed-original.XXXXXX")"
  git diff --binary --output="$recovery_backup/tracked-files.patch"
  cp "$manifest" "$recovery_backup/$manifest"
  git restore --source "$required_commit" --staged --worktree -- .
  rm -f -- "$manifest"
  git diff --quiet
  git diff --cached --quiet
  recovery_started=0
  echo "RECOVERY BACKUP: $recovery_backup"
  echo "PHASE PASS: Exact failed-candidate leftovers recovered; accepted Jellyfin runtime preserved"
}

echo "PHASE START: Exact committed v0.0.15 base preflight"
preflight_check "ReddMedia project Git metadata is missing." test -d "$project_root/.git"
preflight_check "Required tool is missing: git" command -v git
cd "$project_root"
preflight_check "The checked-out commit is not the required committed v0.0.15 baseline." \
  test "$(git rev-parse HEAD)" = "$required_commit"
preflight_check "The ReddMedia repository is not on the main branch." \
  test "$(git branch --show-current)" = "main"
recover_known_failed_original_candidate
if ! git diff --quiet; then
  echo "ERROR: Tracked ReddMedia files differ from committed v0.0.15." >&2
  git status --short --untracked-files=no >&2
  false
fi
if ! git diff --cached --quiet; then
  echo "ERROR: The ReddMedia index contains staged changes." >&2
  git diff --cached --name-status >&2
  false
fi
python3 - "$project_root" <<'PY'
import pathlib, subprocess, sys
root = pathlib.Path(sys.argv[1])
untracked = subprocess.check_output(
    ["git", "-C", str(root), "ls-files", "--others", "--exclude-standard"],
    text=True,
).splitlines()
unexpected = [name for name in untracked if not name.startswith("components/jellyfin/runtime/")]
if unexpected:
    raise SystemExit("unexpected untracked files: " + ", ".join(unexpected))
PY
preflight_check "ReddMedia_v15 is missing or is not executable." \
  test -x "$project_root/ReddMedia_v15"
preflight_check "The active executable does not identify as ReddMedia v0.0.15." \
  test "$("$project_root/ReddMedia_v15" --version)" = "ReddMedia v0.0.15"
preflight_check "The integrated Jellyfin runtime is missing or is not executable." \
  test -x "$project_root/components/jellyfin/runtime/jellyfin/jellyfin"
preflight_check "The accepted v0.0.15 fallback snapshot is missing." \
  test -f "$snapshot_root/ReddMedia_v15"
preflight_check "The accepted v0.0.15 fallback executable hash does not match." \
  test "$(sha256sum "$snapshot_root/ReddMedia_v15" | cut -d' ' -f1)" = \
    "aab6981418c8fc8da0640a30be74267d4fbe98b4d2d7ecbd42fbe45330706e6d"
if [[ -n "$(exact_executable_pids "$project_root/ReddMedia_v15")" ]]; then
  echo "ERROR: Close ReddMedia before installing v0.0.16." >&2
  false
fi
for tool in sha256sum python3 unzip cmake g++ pkg-config curl find readlink stat install seq; do
  if ! command -v "$tool" >/dev/null; then
    echo "ERROR: Required tool is missing: $tool" >&2
    false
  fi
done
case "$(uname -m)" in
  x86_64) ;;
  *) echo "ERROR: ReddMedia v0.0.16 requires x86-64 Linux." >&2; false ;;
esac
if curl --silent --max-time 1 http://127.0.0.1:8096/Startup/User >/dev/null 2>&1; then
  echo "ERROR: Port 8096 is already in use. Close the other media server." >&2
  false
fi
echo "PHASE PASS: Exact committed v0.0.15 base preflight"

echo "PHASE START: Package integrity verification"
python3 - "$package_root/REDDMEDIA_PATCH_MANIFEST.json" "$package_root" <<'PY'
import hashlib, json, pathlib, sys
manifest = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
root = pathlib.Path(sys.argv[2])
for name, expected in manifest["files"].items():
    path = root / name
    if not path.is_file():
        raise SystemExit(f"missing payload file: {name}")
    data = path.read_bytes()
    if len(data) != expected["bytes"] or hashlib.sha256(data).hexdigest() != expected["sha256"]:
        raise SystemExit(f"package integrity failure: {name}")
print(f"verified {len(manifest['files'])} payload files")
PY
echo "PHASE PASS: Package integrity verification"

echo "PHASE START: Apply v0.0.16 changed files"
applied=1
for path in "${added_paths[@]}" "${modified_paths[@]}"; do
  mkdir -p "$project_root/$(dirname "$path")"
  cp -a "$package_root/$path" "$project_root/$path"
done
chmod +x "$project_root/INSTALL_REDDMEDIA_v0_0_16.sh"
echo "PHASE PASS: Apply v0.0.16 changed files"

echo "PHASE START: Pinned AI source and model proof"
test "$(sha256sum "$project_root/components/ai/source/llama.cpp-pinned-source.zip" | cut -d' ' -f1)" = \
  "8dc808f9e0166c7fe9f5ec73884392d528c1198fd2ce89f0d60971d7d55ae998"
test "$(sha256sum "$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf" | cut -d' ' -f1)" = \
  "d4e388894e09cf3816e8b0896d81d265b55e7a9fff9ab03fe8bf4ef5e11295ac"
unzip -tq "$project_root/components/ai/source/llama.cpp-pinned-source.zip" >/dev/null
test -s "$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
echo "PHASE PASS: Pinned AI source and model proof"

echo "PHASE START: Build pinned llama.cpp CPU runtime"
build_root="$(mktemp -d "${TMPDIR:-/tmp}/reddmedia-v16-build.XXXXXX")"
mkdir -p "$build_root/llama-source" "$build_root/ai-stage"
unzip -q "$project_root/components/ai/source/llama.cpp-pinned-source.zip" -d "$build_root/llama-source"
llama_source="$(find "$build_root/llama-source" -mindepth 1 -maxdepth 1 -type d -print -quit)"
test -n "$llama_source"
cmake -S "$llama_source" -B "$build_root/llama-build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$build_root/ai-stage" \
  -DBUILD_SHARED_LIBS=ON \
  -DGGML_NATIVE=OFF \
  -DGGML_OPENMP=OFF \
  -DLLAMA_BUILD_APP=OFF \
  -DLLAMA_BUILD_COMMON=OFF \
  -DLLAMA_BUILD_TESTS=OFF \
  -DLLAMA_BUILD_TOOLS=OFF \
  -DLLAMA_BUILD_EXAMPLES=OFF \
  -DLLAMA_BUILD_SERVER=OFF \
  -DLLAMA_OPENSSL=OFF \
  -DLLAMA_SUBPROCESS=OFF
cmake --build "$build_root/llama-build" --target llama --parallel
cmake --install "$build_root/llama-build"
test -f "$build_root/ai-stage/include/llama.h"
find "$build_root/ai-stage" -type f -name 'libllama.so*' -print -quit | grep -q .
rm -rf -- "$project_root/components/ai/runtime.new" "$project_root/components/ai/runtime.old"
cp -a "$build_root/ai-stage" "$project_root/components/ai/runtime.new"
if [[ -d "$project_root/components/ai/runtime" ]]; then
  mv "$project_root/components/ai/runtime" "$project_root/components/ai/runtime.old"
fi
mv "$project_root/components/ai/runtime.new" "$project_root/components/ai/runtime"
rm -rf -- "$project_root/components/ai/runtime.old"
echo "PHASE PASS: Build pinned llama.cpp CPU runtime"

echo "PHASE START: Native warnings-as-errors builds"
cmake -S "$project_root" -B "$build_root/stub" \
  -DREDDMEDIA_P2P_STUB=ON -DREDDMEDIA_AI_STUB=ON
cmake --build "$build_root/stub" --parallel
"$build_root/stub/ReddMedia_v16" --version | grep -Fx "ReddMedia v0.0.16"
"$build_root/stub/ReddMedia_v16" --discover-ai-self-test
cmake -S "$project_root" -B "$build_root/full"
cmake --build "$build_root/full" --parallel
cp "$build_root/full/ReddMedia_v16" "$project_root/ReddMedia_v16"
chmod +x "$project_root/ReddMedia_v16"
"$project_root/ReddMedia_v16" --version | grep -Fx "ReddMedia v0.0.16"
"$project_root/ReddMedia_v16" --embedding-model-test
"$project_root/ReddMedia_v16" --discover-ai-self-test
echo "PHASE PASS: Native warnings-as-errors builds"

echo "PHASE START: Deterministic Library and eight-path Discover validation"
python3 "$project_root/tools/test_reddmedia_discover_v16.py" "$project_root"
echo "PHASE PASS: Deterministic Library and eight-path Discover validation"

echo "PHASE START: Owned integrated-server lifecycle regression"
python3 "$project_root/tools/test_media_server_lifecycle_v16.py" "$project_root"
echo "PHASE PASS: Owned integrated-server lifecycle regression"

echo "PHASE START: Installer rollback regression"
python3 "$project_root/tools/test_installer_rollback_v16.py" "$project_root"
echo "PHASE PASS: Installer rollback regression"

echo "PHASE START: Integrated server graceful and parent-death shutdown proof"
health_root="$(mktemp -d "${TMPDIR:-/tmp}/reddmedia-v16-health.XXXXXX")"
"$project_root/ReddMedia_v16" --media-server-lifecycle-test \
  >"$health_root/graceful.log" 2>&1
grep -F "ReddMedia integrated server graceful shutdown PASS." \
  "$health_root/graceful.log" >/dev/null
if curl --silent --max-time 1 http://127.0.0.1:8096/health >/dev/null 2>&1; then
  cat "$health_root/graceful.log" >&2
  false
fi

"$project_root/ReddMedia_v16" --media-server-parent-death-hold \
  >"$health_root/parent-death.log" 2>&1 &
health_pid=$!
healthy=0
for _ in $(seq 1 120); do
  health_status="$(curl --silent --output /dev/null --write-out '%{http_code}' \
    --max-time 2 http://127.0.0.1:8096/health || true)"
  if [[ "$health_status" == "200" ]]; then
    healthy=1
    break
  fi
  if ! kill -0 "$health_pid" 2>/dev/null; then break; fi
  sleep 1
done
if [[ $healthy -ne 1 ]]; then
  sed -n '1,260p' "$health_root/parent-death.log" >&2
  false
fi
if [[ -r "/proc/$health_pid/task/$health_pid/children" ]]; then
  read -r health_server_pid _ < "/proc/$health_pid/task/$health_pid/children" || true
fi
test -n "$health_server_pid"
web_status="$(curl --silent --output /dev/null --write-out '%{http_code}' \
  --max-time 2 http://127.0.0.1:8096/web/index.html || true)"
test "$web_status" != "200"
kill -KILL "$health_pid"
wait "$health_pid" 2>/dev/null || true
health_pid=""
port_closed=0
for _ in $(seq 1 80); do
  if ! curl --silent --max-time 1 http://127.0.0.1:8096/health >/dev/null 2>&1; then
    port_closed=1
    break
  fi
  sleep 0.1
done
test $port_closed -eq 1
health_server_pid=""
rm -rf -- "$health_root"
health_root=""
echo "PHASE PASS: Integrated server graceful and parent-death shutdown proof"

echo "PHASE START: Desktop identity and version transition"
cp "$project_root/ReddMedia_v16.desktop" "$project_root/ReddMedia.desktop"
install -Dm644 "$project_root/ReddMedia.desktop" "$HOME/.local/share/applications/reddmedia.desktop"
rm -f -- "$project_root/ReddMedia_v15" "$project_root/ReddMedia_v15.desktop"
command -v update-desktop-database >/dev/null && \
  update-desktop-database "$HOME/.local/share/applications" || true
echo "PHASE PASS: Desktop identity and version transition"

echo "PHASE START: Final source, runtime, and UI audit"
test "$("$project_root/ReddMedia_v16" --version)" = "ReddMedia v0.0.16"
test -x "$project_root/components/jellyfin/runtime/jellyfin/jellyfin"
test -f "$project_root/components/ai/runtime/include/llama.h"
find "$project_root/components/ai/runtime" -type f -name 'libllama.so*' -print -quit | grep -q .
test -f "$project_root/components/ai/models/nomic-embed-text-v1.5-Q4_K_M.gguf"
test -f "$project_root/licenses/ai/LLAMA_CPP_MIT_LICENSE.txt"
test -f "$project_root/licenses/ai/NOMIC_EMBED_TEXT_APACHE_2_LICENSE.txt"
grep -F '"DISCOVER USUAL"' "$project_root/src/main.cpp" >/dev/null
grep -F '"DISCOVER RANDOM"' "$project_root/src/main.cpp" >/dev/null
for label in 'Local Movie' 'Local TV' 'External Movie' 'External TV'; do
  grep -F "\"$label\"" "$project_root/src/main.cpp" >/dev/null
done
for forbidden in Favorites Trailers Genres; do
  if grep -F "\"$forbidden\"" "$project_root/src/main.cpp" >/dev/null; then false; fi
done
grep -F 'open_media(selected.path, 0)' "$project_root/src/main.cpp" >/dev/null
grep -F 'draw_tree_badge' "$project_root/src/main.cpp" >/dev/null
grep -F 'mediaServer.stop();' "$project_root/src/main.cpp" >/dev/null
grep -F 'PR_SET_PDEATHSIG' "$project_root/src/media_server/media_server_manager.cpp" >/dev/null
grep -F -- '-DLLAMA_BUILD_APP=OFF' "$project_root/INSTALL_REDDMEDIA_v0_0_16.sh" >/dev/null
grep -F -- '--target llama' "$project_root/INSTALL_REDDMEDIA_v0_0_16.sh" >/dev/null
echo "PHASE PASS: Final source, runtime, and UI audit"

cleanup_temporary_work
trap - EXIT
echo "FINAL PASS: ReddMedia v0.0.16 Native Library and Discover AI installed and validated."
