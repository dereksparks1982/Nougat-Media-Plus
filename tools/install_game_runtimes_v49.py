#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]

DOS_URL = "https://github.com/dosbox-staging/dosbox-staging/releases/download/v0.82.2/dosbox-staging-linux-x86_64-v0.82.2.tar.xz"
DOS_SHA256 = "bc229df72ea103b7865cdca67324772dbffa8e58866477e69a79638b723a0442"

XENIA_URL = "https://github.com/xenia-canary/xenia-canary/releases/download/1e834f8/xenia_canary_linux.AppImage"
XENIA_SHA256 = "91df919a912bd305a214c535e0ab8abee43c18eb1bab1ef5e35991d16738b05e"

STELLA_URL = "https://github.com/stella-emu/stella/releases/download/7.0/stella_7.0_amd64.deb"
STELLA_VERSION = "7.0"

BLASTEM_URL = "https://www.retrodev.com/blastem/nightlies/blastem64-0.6.3-pre-8013468ed981.tar.gz"
BLASTEM_EXPECTED_SIZE = 6053889
BLASTEM_REVISION = "8013468ed981"


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url: str, target: Path, expected: str | None = None) -> str:
    print("Downloading:", url)
    request = urllib.request.Request(url, headers={"User-Agent": "Nougat-Media-Suite-v0.0.49"})
    with urllib.request.urlopen(request, timeout=120) as response, target.open("wb") as out:
        shutil.copyfileobj(response, out)
    actual = sha256(target)
    if expected is not None and actual != expected:
        target.unlink(missing_ok=True)
        raise RuntimeError(f"SHA-256 mismatch for {url}: {actual}")
    return actual


def safe_extract_tar(archive: Path, destination: Path, mode: str, label: str) -> None:
    root = destination.resolve()
    with tarfile.open(archive, mode) as tar:
        members = tar.getmembers()
        for member in members:
            resolved = (destination / member.name).resolve()
            if resolved != root and root not in resolved.parents:
                raise RuntimeError(f"Unsafe path in {label} archive: {member.name}")
            if member.issym() or member.islnk():
                link_target = (resolved.parent / member.linkname).resolve()
                if link_target != root and root not in link_target.parents:
                    raise RuntimeError(f"Unsafe link in {label} archive: {member.name} -> {member.linkname}")
        tar.extractall(destination, members=members)


def install_dosbox(temp: Path) -> None:
    target = ROOT / "components/games/runtime/dosbox-staging/dosbox"
    if target.is_file() and os.access(target, os.X_OK):
        print("DOSBox runtime already present:", target)
        return
    archive = temp / "dosbox.tar.xz"
    download(DOS_URL, archive, DOS_SHA256)
    extracted = temp / "dosbox"
    extracted.mkdir()
    safe_extract_tar(archive, extracted, "r:xz", "DOSBox")
    candidates = [
        p for p in extracted.rglob("*")
        if p.is_file() and p.name in {"dosbox", "dosbox-staging"} and os.access(p, os.X_OK)
    ]
    if not candidates:
        raise RuntimeError("Verified DOSBox archive did not contain an executable")
    candidates.sort(key=lambda p: (0 if p.name == "dosbox" else 1, len(p.parts)))
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(candidates[0], target)
    target.chmod(target.stat().st_mode | 0o111)
    (target.parent / "UPSTREAM.txt").write_text(
        f"DOSBox Staging v0.82.2\n{DOS_URL}\nSHA-256 {DOS_SHA256}\n",
        encoding="utf-8",
    )
    print("Installed DOSBox runtime:", target)


def install_xenia(temp: Path) -> None:
    target = ROOT / "components/games/runtime/xenia/xenia_canary"
    if target.is_file() and os.access(target, os.X_OK):
        print("Xenia runtime already present:", target)
        return
    image = temp / "xenia_canary_linux.AppImage"
    download(XENIA_URL, image, XENIA_SHA256)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(image, target)
    target.chmod(target.stat().st_mode | 0o111)
    (target.parent / "UPSTREAM.txt").write_text(
        f"Xenia Canary 1e834f8, published 2026-08-24\n{XENIA_URL}\nSHA-256 {XENIA_SHA256}\n",
        encoding="utf-8",
    )
    print("Installed Xenia Canary runtime:", target)


def stella_metadata(package: Path) -> tuple[str, str, str]:
    dpkg_deb = shutil.which("dpkg-deb")
    if not dpkg_deb:
        raise RuntimeError("dpkg-deb is required to unpack the official Stella Ubuntu package")
    result = subprocess.run(
        [dpkg_deb, "-f", str(package), "Package", "Version", "Architecture"],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        raise RuntimeError("Could not inspect Stella package: " + result.stdout.strip())

    # dpkg-deb versions differ here: some print RFC822 labels such as
    # "Package: stella", while others can emit bare field values. Accept both
    # forms, but still require all three fields before trusting the package.
    fields: dict[str, str] = {}
    bare: list[str] = []
    for raw in result.stdout.splitlines():
        line = raw.strip()
        if not line:
            continue
        if ":" in line:
            key, value = line.split(":", 1)
            key = key.strip().lower()
            if key in {"package", "version", "architecture"}:
                fields[key] = value.strip()
                continue
        bare.append(line)

    if not fields and len(bare) >= 3:
        fields = {
            "package": bare[0],
            "version": bare[1],
            "architecture": bare[2],
        }

    package_name = fields.get("package", "")
    version = fields.get("version", "")
    architecture = fields.get("architecture", "")
    if not package_name or not version or not architecture:
        raise RuntimeError(
            "Stella package metadata was incomplete: " + repr(result.stdout.splitlines())
        )
    return package_name, version, architecture


def verify_stella_dependencies(binary: Path) -> None:
    ldd = shutil.which("ldd")
    if not ldd:
        raise RuntimeError("ldd is required to validate the extracted Stella runtime")
    result = subprocess.run(
        [ldd, str(binary)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        raise RuntimeError("Could not inspect Stella shared libraries: " + result.stdout.strip())
    missing = []
    for line in result.stdout.splitlines():
        if "=> not found" in line:
            missing.append(line.split("=>", 1)[0].strip())
    if missing:
        raise RuntimeError(
            "Stella runtime dependencies are missing: " + ", ".join(sorted(set(missing)))
        )


def write_stella_wrapper(wrapper: Path) -> None:
    wrapper.parent.mkdir(parents=True, exist_ok=True)
    wrapper.write_text(
        "#!/bin/sh\n"
        "HERE=\"$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\"\n"
        "PACKAGE=\"$HERE/package\"\n"
        "export XDG_DATA_DIRS=\"$PACKAGE/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}\"\n"
        "export SDL_VIDEODRIVER=x11\n"
        "export SDL_VIDEO_DRIVER=x11\n"
        "export LD_LIBRARY_PATH=\"$PACKAGE/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}\"\n"
        "exec \"$PACKAGE/usr/bin/stella\" \"$@\"\n",
        encoding="utf-8",
    )
    wrapper.chmod(0o755)


def install_stella(temp: Path) -> None:
    wrapper = ROOT / "components/games/runtime/stella/stella"
    native = wrapper.parent / "package/usr/bin/stella"
    if wrapper.is_file() and native.is_file() and os.access(wrapper, os.X_OK) and os.access(native, os.X_OK):
        write_stella_wrapper(wrapper)
        verify_stella_dependencies(native)
        print("Stella runtime already present; SDL3 X11 wrapper refreshed:", wrapper)
        return

    package = temp / "stella_7.0_amd64.deb"
    actual_sha = download(STELLA_URL, package)
    package_name, version, architecture = stella_metadata(package)
    if package_name != "stella" or not version.startswith(STELLA_VERSION) or architecture != "amd64":
        raise RuntimeError(
            "Unexpected Stella package metadata: " +
            f"package={package_name} version={version} architecture={architecture}"
        )

    dpkg_deb = shutil.which("dpkg-deb")
    extracted = temp / "stella-package"
    extracted.mkdir()
    result = subprocess.run(
        [dpkg_deb, "-x", str(package), str(extracted)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        raise RuntimeError("Could not unpack Stella: " + result.stdout.strip())
    extracted_binary = extracted / "usr/bin/stella"
    if not extracted_binary.is_file():
        raise RuntimeError("Official Stella package did not contain usr/bin/stella")
    extracted_binary.chmod(extracted_binary.stat().st_mode | 0o111)
    verify_stella_dependencies(extracted_binary)
    print("PASS: Stella package metadata and shared-library dependencies verified")

    target_dir = wrapper.parent
    target_dir.mkdir(parents=True, exist_ok=True)
    package_dir = target_dir / "package"
    if package_dir.exists():
        shutil.rmtree(package_dir)
    shutil.copytree(extracted, package_dir, symlinks=True)

    write_stella_wrapper(wrapper)
    native = target_dir / "package/usr/bin/stella"
    native.chmod(native.stat().st_mode | 0o111)
    (target_dir / "UPSTREAM.txt").write_text(
        "Stella 7.0 official Ubuntu 22.04 amd64 package\n"
        f"{STELLA_URL}\n"
        f"Package metadata: {package_name} {version} {architecture}\n"
        f"Downloaded SHA-256 {actual_sha}\n"
        "The upstream 7.0 GitHub release metadata does not publish a checksum for this asset; "
        "Nougat records the exact downloaded digest and verifies the package identity before extraction.\n",
        encoding="utf-8",
    )
    print("Installed Stella runtime:", wrapper)
    print("Stella package SHA-256:", actual_sha)



def blastem_environment(package_root: Path) -> dict[str, str]:
    env = os.environ.copy()
    lib_dir = package_root / "lib"
    env["LD_LIBRARY_PATH"] = str(lib_dir) + (":" + env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else "")
    env["SDL_VIDEODRIVER"] = "x11"
    env["SDL_VIDEO_DRIVER"] = "x11"
    return env


def blastem_version(binary: Path, package_root: Path) -> str:
    result = subprocess.run(
        [str(binary), "-v"], cwd=package_root, env=blastem_environment(package_root),
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15,
    )
    if result.returncode != 0:
        raise RuntimeError("Could not validate BlastEm version: " + result.stdout.strip())
    return result.stdout.strip()


def verify_blastem_dependencies(binary: Path, package_root: Path) -> None:
    ldd = shutil.which("ldd")
    if not ldd:
        raise RuntimeError("ldd is required to validate the BlastEm runtime")
    result = subprocess.run(
        [ldd, str(binary)], env=blastem_environment(package_root),
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if result.returncode != 0:
        raise RuntimeError("Could not inspect BlastEm shared libraries: " + result.stdout.strip())
    missing = [line.split("=>", 1)[0].strip() for line in result.stdout.splitlines() if "=> not found" in line]
    if missing:
        raise RuntimeError("BlastEm runtime dependencies are missing: " + ", ".join(sorted(set(missing))))


def install_blastem(temp: Path) -> None:
    target_dir = ROOT / "components/games/runtime/blastem"
    wrapper = target_dir / "blastem"
    package_dir = target_dir / "package"
    native = package_dir / "blastem"
    if wrapper.is_file() and native.is_file() and os.access(wrapper, os.X_OK) and os.access(native, os.X_OK):
        try:
            version = blastem_version(native, package_dir)
            if "0.6.3-pre" in version and BLASTEM_REVISION in version:
                print("BlastEm runtime already present:", wrapper)
                return
        except Exception:
            pass

    archive = temp / "blastem.tar.gz"
    actual_sha = download(BLASTEM_URL, archive)
    actual_size = archive.stat().st_size
    if actual_size != BLASTEM_EXPECTED_SIZE:
        archive.unlink(missing_ok=True)
        raise RuntimeError(
            f"BlastEm nightly size mismatch: expected {BLASTEM_EXPECTED_SIZE}, got {actual_size}"
        )

    extracted = temp / "blastem-package"
    extracted.mkdir()
    safe_extract_tar(archive, extracted, "r:gz", "BlastEm")
    candidates = [
        candidate for candidate in extracted.rglob("blastem")
        if candidate.is_file() and os.access(candidate, os.X_OK)
    ]
    if not candidates:
        raise RuntimeError("Pinned BlastEm archive did not contain an executable named blastem")
    candidates.sort(key=lambda candidate: len(candidate.parts))
    source_binary = candidates[0]
    source_root = source_binary.parent
    for required in ("default.cfg", "rom.db"):
        if not (source_root / required).is_file():
            raise RuntimeError("Pinned BlastEm archive is missing " + required)
    version = blastem_version(source_binary, source_root)
    if "0.6.3-pre" not in version or BLASTEM_REVISION not in version:
        raise RuntimeError("Unexpected BlastEm build identity: " + version)
    verify_blastem_dependencies(source_binary, source_root)
    print("PASS: BlastEm nightly identity and shared-library dependencies verified:", version)

    target_dir.mkdir(parents=True, exist_ok=True)
    if package_dir.exists():
        shutil.rmtree(package_dir)
    shutil.copytree(source_root, package_dir, symlinks=True)
    native = package_dir / "blastem"
    native.chmod(native.stat().st_mode | 0o111)

    wrapper.write_text(
        "#!/bin/sh\n"
        "HERE=\"$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\"\n"
        "PACKAGE=\"$HERE/package\"\n"
        "mkdir -p \"${HOME}/.config/reddmedia/games/blastem-xdg\" \"${HOME}/.local/share/reddmedia/games/blastem-xdg\"\n"
        "export XDG_CONFIG_HOME=\"${HOME}/.config/reddmedia/games/blastem-xdg\"\n"
        "export XDG_DATA_HOME=\"${HOME}/.local/share/reddmedia/games/blastem-xdg\"\n"
        "export SDL_VIDEODRIVER=x11\n"
        "export SDL_VIDEO_DRIVER=x11\n"
        "export LD_LIBRARY_PATH=\"$PACKAGE/lib:${LD_LIBRARY_PATH:-}\"\n"
        "exec \"$PACKAGE/blastem\" \"$@\"\n",
        encoding="utf-8",
    )
    wrapper.chmod(0o755)
    (target_dir / "UPSTREAM.txt").write_text(
        "BlastEm official Linux x86_64 nightly 0.6.3-pre-8013468ed981\n"
        f"{BLASTEM_URL}\n"
        f"Official directory size: {BLASTEM_EXPECTED_SIZE} bytes\n"
        f"Downloaded SHA-256 {actual_sha}\n"
        f"Validated runtime identity: {version}\n"
        "The complete portable upstream package, including its COPYING/license files and bundled libraries, is preserved under package/.\n",
        encoding="utf-8",
    )
    print("Installed BlastEm runtime:", wrapper)
    print("BlastEm archive SHA-256:", actual_sha)

def main() -> int:
    try:
        with tempfile.TemporaryDirectory(prefix="nougat-v49-runtimes-") as td:
            temp = Path(td)
            install_dosbox(temp)
            install_xenia(temp)
            install_stella(temp)
            install_blastem(temp)
        print("=== v0.0.49 GAME RUNTIMES READY ===")
        return 0
    except Exception as exc:
        print("FAIL:", exc)
        print("No sudo command was used. Terminal remains open.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
