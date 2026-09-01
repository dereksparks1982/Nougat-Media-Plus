#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time

BASE = "bc682de962f19b3c80f4718539467eba2aa139cf"
TARGET = "Nougat_Media_Suite_v52"
PREVIOUS = "Nougat_Media_Suite_v51"


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def run(args, *, cwd=None, capture=False, env=None):
    cmd = [str(x) for x in args]
    print("+", " ".join(cmd), flush=True)
    result = subprocess.run(
        cmd, cwd=cwd, env=env, text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
    )
    if capture and result.stdout:
        print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
    return result


def git(root: Path, *args: str) -> str:
    env = {**os.environ, "GIT_PAGER": "cat", "PAGER": "cat", "GIT_TERMINAL_PROMPT": "0"}
    result = subprocess.run(["git", "--no-pager", *args], cwd=root, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    need(result.returncode == 0, result.stderr.strip() or "git failed")
    return result.stdout.strip()


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def clean_output(text: str) -> bool:
    lowered = text.lower()
    return "warning:" not in lowered and "clock skew" not in lowered


def process_alive(pid: int) -> bool:
    if pid <= 1:
        return False
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False
    except PermissionError:
        return True


def terminate(pid: int, label: str) -> None:
    if not process_alive(pid):
        return
    print(f"Stopping {label} PID {pid}...")
    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    for _ in range(50):
        if not process_alive(pid):
            return
        time.sleep(0.1)
    try:
        os.kill(pid, signal.SIGKILL)
    except ProcessLookupError:
        return
    for _ in range(20):
        if not process_alive(pid):
            return
        time.sleep(0.1)
    raise RuntimeError(f"{label} PID {pid} did not stop")


def proc_exe(pid: int) -> str:
    try:
        return os.readlink(f"/proc/{pid}/exe")
    except OSError:
        return ""


def proc_bytes(pid: int, name: str) -> bytes:
    try:
        return Path(f"/proc/{pid}/{name}").read_bytes()
    except OSError:
        return b""


def safe_shutdown(root: Path) -> None:
    print("=== SAFE NOUGAT SHUTDOWN ===")
    prefix = str(root.resolve()) + "/"
    for proc in Path("/proc").iterdir():
        if not proc.name.isdigit():
            continue
        pid = int(proc.name)
        exe = proc_exe(pid)
        if exe.startswith(prefix) and Path(exe).name.startswith("Nougat_Media_Suite"):
            terminate(pid, "Nougat Media Suite")

    ownership = Path.home() / ".local/share/reddmedia/server/nougat-owned.pid"
    if ownership.is_file():
        lines = ownership.read_text(encoding="utf-8", errors="replace").splitlines()
        try:
            pid = int(lines[0]) if lines else -1
        except ValueError:
            pid = -1
        runtime = lines[1].strip() if len(lines) > 1 else ""
        token = lines[2].strip() if len(lines) > 2 else ""
        if process_alive(pid):
            exe = proc_exe(pid)
            cmd = proc_bytes(pid, "cmdline")
            env = proc_bytes(pid, "environ")
            runtime_match = bool(runtime) and (exe == runtime or runtime.encode() in cmd)
            token_match = bool(token) and (("NOUGAT_MEDIA_SERVER_OWNER=" + token).encode() in env)
            signature = runtime_match and b"Nougat Media Suite integrated Jellyfin" in cmd
            need(runtime_match and (token_match or signature),
                 "Running server from ownership file could not be verified as Nougat-owned; it was left untouched.")
            terminate(pid, "verified Nougat-owned Jellyfin")
    print("PASS: runtime shutdown safe.")


def link_runtime(main: Path, work: Path, rel: str) -> None:
    src = main / rel
    dst = work / rel
    if src.exists() and not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        os.symlink(src, dst, target_is_directory=src.is_dir())
        print("Linked runtime:", rel)


def copy_payload(package: Path, work: Path) -> None:
    rels = [
        "src/radio/radio_backend.hpp", "src/radio/radio_backend.cpp",
        "tools/apply_v52_radio.py", "tools/build_v52.py", "tools/vendor_radio_sources_v52.py",
        "tools/test_v52_radio_static.py",
        "components/radio/README.md", "components/radio/UPSTREAM_COMPONENTS.json",
        "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_52_SCOPE.md",
        "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md",
    ]
    for rel in rels:
        src = package / rel
        dst = work / rel
        need(src.is_file(), f"candidate payload missing: {rel}")
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def copy_tree_contents(source: Path, target: Path) -> None:
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(source, target, symlinks=True)


def install_identity(root: Path) -> None:
    # v0.0.52 is Radio-only. Reuse the current N artwork exactly as committed;
    # the known corner-alpha repair is intentionally deferred to v0.0.53.
    icon = root / "assets/icons/nougat-media-suite-v51.png"
    promoted = root / TARGET
    need(icon.is_file(), "current Nougat N icon asset missing")
    apps = Path.home() / ".local/share/applications"
    apps.mkdir(parents=True, exist_ok=True)
    for name in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
        source = root / name
        need(TARGET in source.read_text(encoding="utf-8"), f"{name} does not target v52")
        shutil.copy2(source, apps / name)

    gio = shutil.which("gio")
    need(gio is not None, "gio is required for executable icon identity")
    uri = icon.resolve().as_uri()
    result = run([gio, "set", "-t", "string", promoted, "metadata::custom-icon", uri], capture=True)
    need(result.returncode == 0, "could not set v52 executable custom icon")
    result = run([gio, "info", "-a", "metadata::custom-icon", promoted], capture=True)
    need(result.returncode == 0 and uri in (result.stdout or ""), "v52 executable icon metadata readback failed")

    desktop_db = shutil.which("update-desktop-database")
    if desktop_db:
        result = run([desktop_db, apps], capture=True)
        need(result.returncode == 0 and clean_output(result.stdout or ""), "desktop database refresh warned/failed")
    print("PASS: v52 launcher/executable identity points to the Radio candidate.")


def backup_targets(root: Path, archive: Path, paths: list[str]) -> dict[str, bool]:
    backup = archive / "pre-promotion-files"
    existed: dict[str, bool] = {}
    for rel in paths:
        src = root / rel
        existed[rel] = src.exists()
        if src.is_file() or src.is_symlink():
            dst = backup / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            if src.is_symlink():
                os.symlink(os.readlink(src), dst)
            else:
                shutil.copy2(src, dst)
        elif src.is_dir():
            copy_tree_contents(src, backup / rel)
    return existed


def restore_targets(root: Path, archive: Path, paths: list[str], existed: dict[str, bool]) -> None:
    backup = archive / "pre-promotion-files"
    for rel in paths:
        dst = root / rel
        if dst.is_symlink() or dst.is_file():
            dst.unlink(missing_ok=True)
        elif dst.is_dir():
            shutil.rmtree(dst)
        if not existed.get(rel, False):
            continue
        src = backup / rel
        if src.is_symlink():
            dst.parent.mkdir(parents=True, exist_ok=True)
            os.symlink(os.readlink(src), dst)
        elif src.is_file():
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
        elif src.is_dir():
            copy_tree_contents(src, dst)


def main() -> int:
    archive: Path | None = None
    promoted = False
    promotion_paths = [
        "CMakeLists.txt", "src/main.cpp", "src/radio", "components/radio",
        "NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop",
        "CHANGELOG.md", "ROADMAP.md", "DEPENDENCIES.md", "THIRD_PARTY_NOTICES.md",
        "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_52_SCOPE.md",
        "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md",
        "tools/apply_v52_radio.py", "tools/build_v52.py", "tools/vendor_radio_sources_v52.py",
        "tools/test_v52_radio_static.py", TARGET, PREVIOUS,
    ]
    existed: dict[str, bool] = {}
    try:
        package = Path(__file__).resolve().parents[1]
        root = Path(sys.argv[1] if len(sys.argv) > 1 else Path.home() / "DKLab/Projects/Nougat Media Suite").resolve()
        need((root / ".git").exists() or bool(git(root, "rev-parse", "--git-dir")), f"Not a Git repo: {root}")
        need(Path(git(root, "rev-parse", "--show-toplevel")).resolve() == root, "Nougat repository root mismatch")
        head = git(root, "rev-parse", "HEAD")
        need(head == BASE, f"STOP: v0.0.52 base must be exact committed v0.0.51 checkpoint {BASE}; current {head}")
        need(git(root, "branch", "--show-current") == "main", "v0.0.52 candidate must start from main")
        need(subprocess.run(["git", "diff", "--quiet"], cwd=root).returncode == 0, "tracked worktree is modified")
        need(subprocess.run(["git", "diff", "--cached", "--quiet"], cwd=root).returncode == 0, "staging area is not clean")

        safe_shutdown(root)
        previous = root / PREVIOUS
        need(previous.is_file() and os.access(previous, os.X_OK), "committed v51 checkpoint executable is missing")
        previous_sha = sha256(previous)
        print("Rejected historical v51 executable SHA-256:", previous_sha)

        archive = Path.home() / "DKLab/Archive/Nougat Media Suite" / ("v0.0.52-prebuild-" + time.strftime("%Y%m%d-%H%M%S"))
        archive.mkdir(parents=True, exist_ok=False)
        snapshot = archive / "Nougat-Media-Suite-v0.0.51-committed-base.tar.gz"
        result = run(["git", "--no-pager", "archive", "--format=tar.gz", "-o", snapshot, BASE], cwd=root)
        need(result.returncode == 0 and snapshot.is_file() and snapshot.stat().st_size > 0, "prebuild snapshot failed")
        shutil.copy2(previous, archive / PREVIOUS)
        print("Snapshot:", snapshot)
        print("Snapshot SHA-256:", sha256(snapshot))

        temp_parent = Path(tempfile.mkdtemp(prefix="nougat-v52-radio-"))
        work = temp_parent / "source"
        build = temp_parent / "build"
        try:
            result = run(["git", "--no-pager", "worktree", "add", "--detach", work, BASE], cwd=root)
            need(result.returncode == 0, "temporary worktree creation failed")
            for rel in ("components/ai/runtime", "components/games/runtime", "components/jellyfin/runtime"):
                link_runtime(root, work, rel)
            copy_payload(package, work)

            print("=== APPLY v0.0.52 RADIO-ONLY SOURCE ===")
            # Run the patcher from the immutable candidate package so its PAYLOAD
            # root is the package, not the temporary destination worktree.
            result = run([sys.executable, package / "tools/apply_v52_radio.py", work], cwd=work, capture=True)
            need(result.returncode == 0, "v0.0.52 source patch failed")

            print("=== STAGE OWNER RADIO PROJECTS / PINNED FALLBACKS ===")
            vendor_env = {**os.environ, "NOUGAT_RADIO_SOURCE_DIR": str(package)}
            result = run([sys.executable, work / "tools/vendor_radio_sources_v52.py"], cwd=work, capture=True, env=vendor_env)
            need(result.returncode == 0, "radio upstream source staging failed")

            print("=== v0.0.52 RADIO STATIC TESTS ===")
            result = run([sys.executable, work / "tools/test_v52_radio_static.py", work], cwd=work, capture=True)
            need(result.returncode == 0, "v0.0.52 Radio static contracts failed")
            result = run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", work / "src",
                          "-c", work / "src/radio/radio_backend.cpp", "-o", temp_parent / "radio_backend.o"],
                         cwd=work, capture=True)
            need(result.returncode == 0 and clean_output(result.stdout or ""), "Radio backend warnings-as-errors compile failed")

            print("=== NATIVE v0.0.52 BUILD ===")
            result = run(["cmake", "-S", work, "-B", build], capture=True)
            need(result.returncode == 0, "CMake configure failed")
            need(clean_output(result.stdout or ""), "CMake configure emitted a warning")
            result = run(["cmake", "--build", build, "--target", TARGET, "-j2"], capture=True)
            need(result.returncode == 0, "native v0.0.52 build failed")
            need(clean_output(result.stdout or ""), "native v0.0.52 build emitted a warning")
            built = build / TARGET
            need(built.is_file() and os.access(built, os.X_OK), "v52 build output missing")

            env = dict(os.environ)
            libs = [work / "components/ai/runtime/lib", work / "components/ai/runtime/lib64"]
            libs = [str(x) for x in libs if x.exists()]
            if libs:
                env["LD_LIBRARY_PATH"] = ":".join(libs) + ((":" + env["LD_LIBRARY_PATH"]) if env.get("LD_LIBRARY_PATH") else "")
            result = run([built, "--version"], capture=True, env=env)
            need(result.returncode == 0 and (result.stdout or "").strip() == "Nougat Media Suite v0.0.52",
                 "build-tree v52 identity mismatch")

            for flag in ("--v49-games-self-test", "--v47-nav-self-test", "--v47-fullscreen-controls-self-test", "--v47-window-identity-self-test"):
                result = run([built, flag], capture=True, env=env)
                need(result.returncode == 0 and "PASS" in (result.stdout or ""), f"retained self-test failed: {flag}")

            print("=== PROMOTE VERIFIED v0.0.52 RADIO CANDIDATE ===")
            existed = backup_targets(root, archive, promotion_paths)
            promoted = True
            file_paths = [
                "CMakeLists.txt", "src/main.cpp",
                "NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop",
                "CHANGELOG.md", "ROADMAP.md", "DEPENDENCIES.md", "THIRD_PARTY_NOTICES.md",
                "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_52_SCOPE.md",
                "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md",
                "tools/apply_v52_radio.py", "tools/build_v52.py", "tools/vendor_radio_sources_v52.py",
                "tools/test_v52_radio_static.py",
            ]
            for rel in file_paths:
                src = work / rel
                dst = root / rel
                need(src.is_file(), f"candidate source missing: {rel}")
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
            copy_tree_contents(work / "src/radio", root / "src/radio")
            copy_tree_contents(work / "components/radio", root / "components/radio")
            shutil.copy2(built, root / TARGET)
            (root / TARGET).chmod((root / TARGET).stat().st_mode | 0o111)

            clean_env = dict(os.environ)
            clean_env.pop("LD_LIBRARY_PATH", None)
            result = run([root / TARGET, "--version"], capture=True, env=clean_env)
            need(result.returncode == 0 and (result.stdout or "").strip() == "Nougat Media Suite v0.0.52",
                 "promoted v52 executable identity mismatch")

            # Only after the new executable is validated do we retire the old root executable.
            need(previous.is_file() and sha256(previous) == previous_sha, "v51 executable changed before retirement")
            previous.unlink()
            versioned = sorted(p.name for p in root.glob("Nougat_Media_Suite_v*") if p.is_file())
            need(versioned == [TARGET], f"root executable gate failed; found: {versioned}")

            install_identity(root)
            result = run([sys.executable, root / "tools/test_v52_radio_static.py", root], cwd=root, capture=True)
            need(result.returncode == 0, "post-promotion v52 static validation failed")

            print("=== NOUGAT v0.0.52 RADIO CANDIDATE PASS ===")
            print("Executable:", root / TARGET)
            print("SHA-256:", sha256(root / TARGET))
            print("Historical v51 rollback copy:", archive / PREVIOUS)
            print("Prebuild snapshot:", snapshot)
            print("Radio source components:", root / "components/radio/upstream")
            print("NO GIT COMMIT, TAG, OR GITHUB PUSH PERFORMED.")
            print("OWNER TEST GATES:")
            print("  1. Open Radio: simple RADIO view is understandable and independent of Video Player state.")
            print("  2. PRO exposes frequency/mode/step/gain/squelch/device/scan/record controls.")
            print("  3. Internet Radio plays through Nougat audio; Stop stops it.")
            print("  4. TV Antenna Scan hands off to the existing Live TV antenna path without fake AM/FM capability.")
            print("  5. With supported SDR hardware, Listen/Scan use the real detected receiver and remain responsive.")
            print("  6. Emergency/Weather/ISS/Shortwave presets tune truthful frequencies/modes; encrypted systems are not bypassed.")
            print("  7. TX Test is explicitly non-radiating; RF transmit remains disabled by default.")
            return 0
        finally:
            subprocess.run(["git", "--no-pager", "worktree", "remove", "--force", work], cwd=root,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           env={**os.environ, "GIT_PAGER": "cat", "PAGER": "cat", "GIT_TERMINAL_PROMPT": "0"})
            shutil.rmtree(temp_parent, ignore_errors=True)
    except Exception as exc:
        print("FAIL:", exc)
        if promoted and archive is not None and existed:
            try:
                root = Path(sys.argv[1] if len(sys.argv) > 1 else Path.home() / "DKLab/Projects/Nougat Media Suite").resolve()
                restore_targets(root, archive, promotion_paths, existed)
                print("ROLLBACK: pre-promotion tracked/candidate files restored.")
            except Exception as rollback_exc:
                print("ROLLBACK WARNING:", rollback_exc)
        print("No Git commit, tag, or GitHub push was performed.")
        print("Terminal remains open.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
