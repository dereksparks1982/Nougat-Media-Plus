#!/usr/bin/env bash
# Nougat Media Suite v0.0.40 rejected-build repair v7.

payload_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
project_root="$HOME/DKLab/Projects/Nougat Media Suite"
archive_parent="$HOME/DKLab/Archives/ReddMedia Archive"
base_commit="e9468cc0a98eefd9efd6f3dfd9ec32851dca8c26"
stamp="$(date +%Y%m%d_%H%M%S)"
candidate_root="${TMPDIR:-/tmp}/nougat-v40-repair-source-$stamp-$$"
build_root="${TMPDIR:-/tmp}/nougat-v40-repair-build-$stamp-$$"
rollback_root="$archive_parent/Nougat_Media_Suite_pre_v0_0_40_REJECTED_BUILD_REPAIR_v7_$stamp"
manifest="$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v40_REPAIR_v7.json"
runtime_candidate="$project_root/.Nougat_Media_Suite_v40.runtime-candidate-$$"
applied=0

canonical_id="com.elderredsoftworks.NougatMediaSuite"
user_apps="$HOME/.local/share/applications"
canonical_launcher="$user_apps/${canonical_id}.desktop"
unversioned_launcher="$user_apps/NougatMediaSuite.desktop"
v39_launcher="$user_apps/NougatMediaSuite_v39.desktop"
v40_launcher="$user_apps/NougatMediaSuite_v40.desktop"
master_icon="$project_root/assets/icons/nougat-media-suite-concept-sheet-v24.png"

phase(){ printf '\n=== %s ===\n' "$1"; }

verify_manifest(){
  python3 - "$payload_root" "$manifest" <<'PY'
import hashlib,json,pathlib,sys
root=pathlib.Path(sys.argv[1]); mp=pathlib.Path(sys.argv[2])
if not mp.is_file():
    print("FAIL: repair manifest missing"); raise SystemExit(1)
m=json.loads(mp.read_text())
if m.get("target_version")!="0.0.40" or m.get("base_commit")!="e9468cc0a98eefd9efd6f3dfd9ec32851dca8c26":
    print("FAIL: repair manifest identity mismatch"); raise SystemExit(1)
for rel,rec in m.get("payload",{}).items():
    p=root/rel
    if not p.is_file():
        print("FAIL: repair payload missing:",rel); raise SystemExit(1)
    data=p.read_bytes()
    if len(data)!=rec["bytes"] or hashlib.sha256(data).hexdigest()!=rec["sha256"]:
        print("FAIL: repair payload mismatch:",rel); raise SystemExit(1)
print("PASS: repair package manifest verified:",len(m.get("payload",{})),"files")
PY
}

verify_repository(){
  [[ -d "$project_root/.git" ]] || { echo "FAIL: Nougat Git project root missing."; return 1; }
  local branch head
  branch="$(git -C "$project_root" branch --show-current 2>/dev/null)"
  head="$(git -C "$project_root" rev-parse HEAD 2>/dev/null)"
  [[ "$branch" == "main" ]] || { echo "FAIL: expected branch main, found $branch"; return 1; }
  [[ "$head" == "$base_commit" ]] || {
    echo "FAIL: repair must still sit on accepted v0.0.39 HEAD."
    echo "Expected: $base_commit"
    echo "Found:    $head"
    return 1
  }
  if ! git -C "$project_root" diff --cached --quiet --; then
    echo "FAIL: staged changes are present."
    echo "STOP: repair will not overwrite staged work."
    return 1
  fi
  [[ -f "$master_icon" ]] || { echo "FAIL: approved Nougat N icon missing: $master_icon"; return 1; }
  command -v gio >/dev/null 2>&1 || { echo "FAIL: gio is required for root executable icon metadata."; return 1; }
  echo "PASS: accepted v0.0.39 Git baseline verified. Rejected working candidate may be present."
}

create_candidate(){
  rm -rf -- "$candidate_root" "$build_root"
  mkdir -p "$candidate_root/src" "$candidate_root/components/nougat" "$candidate_root/tools" || return 1

  for rel in CMakeLists.txt src/main.cpp components/nougat/nougat_engine.py \
             NougatMediaSuite.desktop NougatMediaSuite_v39.desktop \
             com.elderredsoftworks.NougatMediaSuite.desktop; do
    mkdir -p "$candidate_root/$(dirname "$rel")" || return 1
    git -C "$project_root" show "$base_commit:$rel" > "$candidate_root/$rel" || {
      echo "FAIL: could not materialize accepted baseline file: $rel"
      return 1
    }
  done

  python3 - "$project_root" "$candidate_root" <<'PY'
import pathlib,sys
src=pathlib.Path(sys.argv[1]).resolve()
dst=pathlib.Path(sys.argv[2]).resolve()

for child in src.iterdir():
    if child.name in {".git","CMakeLists.txt","src","components","NougatMediaSuite.desktop",
                      "NougatMediaSuite_v39.desktop","NougatMediaSuite_v40.desktop",
                      "Nougat_Media_Suite_v39","Nougat_Media_Suite_v40",
                      "com.elderredsoftworks.NougatMediaSuite.desktop","tools"}:
        continue
    target=dst/child.name
    if not target.exists() and not target.is_symlink():
        target.symlink_to(child,target_is_directory=child.is_dir())

for top in ("src","components","tools"):
    src_top=src/top
    dst_top=dst/top
    dst_top.mkdir(exist_ok=True)
    for child in src_top.iterdir():
        rel=f"{top}/{child.name}"
        if rel=="src/main.cpp" or rel=="components/nougat":
            continue
        target=dst_top/child.name
        if not target.exists() and not target.is_symlink():
            target.symlink_to(child,target_is_directory=child.is_dir())

src_nougat=src/"components/nougat"
dst_nougat=dst/"components/nougat"
for child in src_nougat.iterdir():
    if child.name=="nougat_engine.py":
        continue
    target=dst_nougat/child.name
    if not target.exists() and not target.is_symlink():
        target.symlink_to(child,target_is_directory=child.is_dir())
PY
  [[ $? -eq 0 ]] || return 1

  python3 "$payload_root/tools/apply_v40_rejected_build_repair.py" "$candidate_root" || return 1
  python3 -m py_compile "$candidate_root/components/nougat/nougat_engine.py" || {
    echo "FAIL: repaired Nougat search engine does not compile as Python."
    return 1
  }
  echo "PASS: repaired Nougat search engine Python compile gate."
  cp -a "$payload_root/tools/test_nougat_media_suite_v40_repair.py" \
        "$candidate_root/tools/test_nougat_media_suite_v40_repair.py" || return 1
  chmod +x "$candidate_root/tools/test_nougat_media_suite_v40_repair.py"
  python3 "$candidate_root/tools/test_nougat_media_suite_v40_repair.py" "$candidate_root" || return 1
  echo "PASS: exact accepted v0.0.39 source mirror patched to repaired v0.0.40."
}

build_candidate(){
  cmake -S "$candidate_root" -B "$build_root" -DCMAKE_BUILD_TYPE=Release || return 1
  cmake --build "$build_root" -j"$(nproc)" || return 1
  local built="$build_root/Nougat_Media_Suite_v40"
  [[ -x "$built" ]] || { echo "FAIL: native v0.0.40 executable missing after build."; return 1; }

  rm -f -- "$candidate_root/Nougat_Media_Suite_v40"
  cp -L -- "$built" "$candidate_root/Nougat_Media_Suite_v40" || return 1
  chmod +x "$candidate_root/Nougat_Media_Suite_v40" || return 1

  if [[ -L "$candidate_root/Nougat_Media_Suite_v40" ]]; then
    echo "FAIL: staged v0.0.40 executable is a symlink."
    return 1
  fi
  if [[ ! -f "$candidate_root/Nougat_Media_Suite_v40" ]]; then
    echo "FAIL: staged v0.0.40 executable is not a regular file."
    return 1
  fi
  if command -v file >/dev/null 2>&1; then
    file "$candidate_root/Nougat_Media_Suite_v40"
    file "$candidate_root/Nougat_Media_Suite_v40" | grep -q "ELF" || {
      echo "FAIL: staged v0.0.40 executable is not an ELF binary."
      return 1
    }
  fi
  echo "PASS: staged v0.0.40 is a real regular executable, not a symlink."

  local observed
  observed="$("$candidate_root/Nougat_Media_Suite_v40" --version 2>&1)"
  [[ "$observed" == "Nougat Media Suite v0.0.40" ]] || {
    echo "FAIL: candidate version mismatch."
    printf 'Observed: %s\n' "$observed"
    return 1
  }

  phase "retained runtime gates"
  "$candidate_root/Nougat_Media_Suite_v40" --v35-cleanup-self-test || return 1
  "$candidate_root/Nougat_Media_Suite_v40" --v36-library-ui-player-self-test || return 1
  "$candidate_root/Nougat_Media_Suite_v40" --v37-live-tv-system-self-test || return 1
  "$candidate_root/Nougat_Media_Suite_v40" --v38-library-live-tv-player-self-test || return 1
  "$candidate_root/Nougat_Media_Suite_v40" --v39-diagnostic-self-test || return 1
  python3 "$candidate_root/tools/test_nougat_media_suite_v40_repair.py" \
          "$candidate_root" "$candidate_root/Nougat_Media_Suite_v40" || return 1
  echo "PASS: full native repaired candidate passed before active project modification."
}

snapshot_paths(){
cat <<'PATHS'
src/main.cpp
components/nougat/nougat_engine.py
CMakeLists.txt
NougatMediaSuite.desktop
NougatMediaSuite_v39.desktop
NougatMediaSuite_v40.desktop
com.elderredsoftworks.NougatMediaSuite.desktop
Nougat_Media_Suite_v39
Nougat_Media_Suite_v40
tools/test_nougat_media_suite_v40_repair.py
INSTALL_NOUGAT_MEDIA_SUITE_v0_0_40_REJECTED_BUILD_REPAIR_v7.sh
NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v40_REPAIR_v7.json
docs/builds/NOUGAT_MEDIA_SUITE_v0_0_40_REJECTED_BUILD_REPAIR_v7_VALIDATION.md
PATHS
}

save_rollback(){
  mkdir -p "$rollback_root/project" "$rollback_root/user-shell" || return 1
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

  for src in "$canonical_launcher" "$unversioned_launcher" "$v39_launcher" "$v40_launcher"; do
    [[ -e "$src" ]] && cp -a "$src" "$rollback_root/user-shell/$(basename "$src")"
  done
  git -C "$project_root" status --short --branch > "$rollback_root/GIT_STATUS_BEFORE.txt" 2>/dev/null
  git -C "$project_root" diff --binary > "$rollback_root/UNCOMMITTED_DIFF_BEFORE.patch" 2>/dev/null
  echo "PASS: rollback snapshot saved at $rollback_root"
}

existed_before(){ grep -Fqx -- "$1" "$rollback_root/EXISTED_BEFORE.txt" 2>/dev/null; }

restore_rollback(){
  [[ "$applied" == "1" ]] || return 0
  rm -f -- "$runtime_candidate"
  echo
  echo "ROLLBACK START: restoring exact pre-repair working state."
  local rel backup dest
  while IFS= read -r rel; do
    [[ -n "$rel" ]] || continue
    backup="$rollback_root/project/$rel"
    dest="$project_root/$rel"
    if existed_before "$rel"; then
      mkdir -p "$(dirname "$dest")" || return 1
      rm -rf -- "$dest"
      cp -a "$backup" "$dest" || return 1
    else
      rm -rf -- "$dest"
    fi
  done < <(snapshot_paths)

  mkdir -p "$user_apps"
  for dest in "$canonical_launcher" "$unversioned_launcher" "$v39_launcher" "$v40_launcher"; do
    backup="$rollback_root/user-shell/$(basename "$dest")"
    if [[ -e "$backup" ]]; then
      rm -f -- "$dest"
      cp -a "$backup" "$dest" || return 1
    else
      rm -f -- "$dest"
    fi
  done
  echo "ROLLBACK PASS: pre-repair project state restored."
}

atomic_copy(){
  local src="$1" dest="$2" tmp="${2}.nougat-v40repair-$$"
  rm -f -- "$tmp"
  cp -a "$src" "$tmp" || return 1
  mv -f -- "$tmp" "$dest" || { rm -f -- "$tmp"; return 1; }
}

apply_executable_icon(){
  local executable="$1" icon_uri actual
  icon_uri="$(python3 - "$master_icon" <<'PY'
import pathlib,sys
print(pathlib.Path(sys.argv[1]).resolve().as_uri())
PY
)" || return 1

  gio set -t string "$executable" metadata::custom-icon "$icon_uri" || {
    echo "FAIL: could not apply approved custom icon to root executable."
    return 1
  }
  actual="$(gio info -a metadata::custom-icon "$executable" 2>/dev/null)"
  printf '%s\n' "$actual" | grep -F -- "$icon_uri" >/dev/null || {
    echo "FAIL: root executable custom icon verification failed."
    printf 'Expected: %s\nObserved:\n%s\n' "$icon_uri" "$actual"
    return 1
  }
  echo "PASS: root executable carries approved Nougat N custom icon metadata."
}

install_candidate(){
  phase "package + repository preflight"
  verify_manifest || return 1
  verify_repository || return 1

  phase "no-write accepted-baseline candidate"
  create_candidate || return 1

  phase "full native candidate build"
  build_candidate || return 1

  phase "snapshot active project before repair"
  mkdir -p "$archive_parent" || return 1
  save_rollback || return 1
  applied=1

  phase "install repaired v0.0.40 source + binary"
  atomic_copy "$candidate_root/src/main.cpp" "$project_root/src/main.cpp" || return 1
  atomic_copy "$candidate_root/components/nougat/nougat_engine.py" "$project_root/components/nougat/nougat_engine.py" || return 1
  atomic_copy "$candidate_root/CMakeLists.txt" "$project_root/CMakeLists.txt" || return 1
  atomic_copy "$candidate_root/NougatMediaSuite.desktop" "$project_root/NougatMediaSuite.desktop" || return 1
  atomic_copy "$candidate_root/com.elderredsoftworks.NougatMediaSuite.desktop" \
              "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" || return 1
  atomic_copy "$candidate_root/NougatMediaSuite_v40.desktop" "$project_root/NougatMediaSuite_v40.desktop" || return 1

  rm -f -- "$runtime_candidate"
  cp -L -- "$candidate_root/Nougat_Media_Suite_v40" "$runtime_candidate" || return 1
  chmod +x "$runtime_candidate" || return 1

  if [[ -L "$runtime_candidate" || ! -f "$runtime_candidate" ]]; then
    echo "FAIL: project-root runtime candidate is not a regular executable file."
    return 1
  fi

  rm -f -- "$project_root/Nougat_Media_Suite_v40"
  mv -f -- "$runtime_candidate" "$project_root/Nougat_Media_Suite_v40" || return 1
  chmod +x "$project_root/Nougat_Media_Suite_v40" || return 1

  if [[ -L "$project_root/Nougat_Media_Suite_v40" || ! -f "$project_root/Nougat_Media_Suite_v40" ]]; then
    echo "FAIL: installed root v0.0.40 executable is not a regular file."
    return 1
  fi

  cp -a "$payload_root/tools/test_nougat_media_suite_v40_repair.py" \
        "$project_root/tools/test_nougat_media_suite_v40_repair.py" || return 1
  cp -a "$payload_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_40_REJECTED_BUILD_REPAIR_v7.sh" \
        "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_40_REJECTED_BUILD_REPAIR_v7.sh" || return 1
  cp -a "$payload_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v40_REPAIR_v7.json" \
        "$project_root/NOUGAT_MEDIA_SUITE_PATCH_MANIFEST_v40_REPAIR_v7.json" || return 1
  cp -a "$payload_root/docs/builds/NOUGAT_MEDIA_SUITE_v0_0_40_REJECTED_BUILD_REPAIR_v7_VALIDATION.md" \
        "$project_root/docs/builds/NOUGAT_MEDIA_SUITE_v0_0_40_REJECTED_BUILD_REPAIR_v7_VALIDATION.md" || return 1
  chmod +x "$project_root/tools/test_nougat_media_suite_v40_repair.py" \
           "$project_root/INSTALL_NOUGAT_MEDIA_SUITE_v0_0_40_REJECTED_BUILD_REPAIR_v7.sh"

  rm -f -- "$project_root/Nougat_Media_Suite_v39" "$project_root/NougatMediaSuite_v39.desktop"

  phase "install launchers + required executable icon"
  mkdir -p "$user_apps" || return 1
  atomic_copy "$project_root/NougatMediaSuite.desktop" "$unversioned_launcher" || return 1
  atomic_copy "$project_root/com.elderredsoftworks.NougatMediaSuite.desktop" "$canonical_launcher" || return 1
  atomic_copy "$project_root/NougatMediaSuite_v40.desktop" "$v40_launcher" || return 1
  rm -f -- "$v39_launcher"

  apply_executable_icon "$project_root/Nougat_Media_Suite_v40" || return 1

  if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$user_apps" >/dev/null 2>&1
  fi

  phase "final installed verification"
  [[ -x "$project_root/Nougat_Media_Suite_v40" ]] || {
    echo "FAIL: root v0.0.40 executable missing or not executable."; return 1;
  }
  [[ "$("$project_root/Nougat_Media_Suite_v40" --version 2>&1)" == "Nougat Media Suite v0.0.40" ]] || {
    echo "FAIL: installed root executable version gate failed."; return 1;
  }
  python3 "$project_root/tools/test_nougat_media_suite_v40_repair.py" \
          "$project_root" "$project_root/Nougat_Media_Suite_v40" || return 1
  apply_executable_icon "$project_root/Nougat_Media_Suite_v40" || return 1

  echo
  echo "FINAL PASS: repaired Nougat Media Suite v0.0.40 candidate installed."
  echo "Owner acceptance is still required. Nothing was committed, tagged, or pushed."
  echo "Root executable: $project_root/Nougat_Media_Suite_v40"
  echo "Rollback snapshot: $rollback_root"
  echo
  echo "Launch:"
  echo "cd \"$project_root\" && ./Nougat_Media_Suite_v40"
  applied=0
  return 0
}

echo "=== NOUGAT MEDIA SUITE v0.0.40 REJECTED-BUILD REPAIR v7 ==="
if install_candidate; then
  rm -rf -- "$candidate_root" "$build_root"
else
  echo
  echo "FAIL: repaired v0.0.40 candidate did not complete."
  restore_rollback
  rm -rf -- "$candidate_root" "$build_root"
  echo "STOP: pre-repair project state restored."
fi
