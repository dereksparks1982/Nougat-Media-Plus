#!/usr/bin/env python3
"""Transactional Nougat Media Suite v0.0.50 installation backend.

This backend is invoked by the native graphical installer. It remains usable
from the command line for validation and repair work, but the ordinary end-user
entry point is Nougat_Installer_v50.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from typing import Iterable

APP_VERSION = "0.0.50"
APP_BINARY = "Nougat_Media_Suite_v50"
APP_PREFIX = Path("/opt/nougat-media-suite")
PLUGIN_FORMAT = "NOUGAT_PLUGIN"
PLUGIN_FORMAT_VERSION = 1
APP_ICON_SHA256 = "681ece987dd00d9958cf953939403bd71a5ad9d70d8ad284e133272a0204d804"
APP_ICON_NAME = "nougat-media-suite-concept-sheet-v24.png"
APP_DESKTOP_NAME = "NougatMediaSuite.desktop"


class InstallError(RuntimeError):
    pass


def run(args: list[str], *, capture: bool = False, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    kwargs: dict[str, object] = {"text": True, "env": env}
    if capture:
        kwargs.update(stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return subprocess.run(args, **kwargs)  # type: ignore[arg-type]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def xdg_path(variable: str, fallback: Path) -> Path:
    value = os.environ.get(variable, "").strip()
    return Path(value).expanduser() if value else fallback


def home() -> Path:
    return Path.home()


def data_home() -> Path:
    return xdg_path("XDG_DATA_HOME", home() / ".local" / "share")


def config_home() -> Path:
    return xdg_path("XDG_CONFIG_HOME", home() / ".config")


def state_home() -> Path:
    return xdg_path("XDG_STATE_HOME", home() / ".local" / "state")


def user_plugin_root() -> Path:
    override = os.environ.get("NOUGAT_PLUGIN_ROOT", "").strip()
    return Path(override).expanduser() if override else data_home() / "nougat" / "plugins"


def proc_blob(pid: int, name: str) -> bytes:
    try:
        return (Path("/proc") / str(pid) / name).read_bytes()
    except OSError:
        return b""


def proc_exe(pid: int) -> Path | None:
    try:
        return (Path("/proc") / str(pid) / "exe").resolve(strict=True)
    except (OSError, RuntimeError):
        return None


def same_user(pid: int) -> bool:
    try:
        return (Path("/proc") / str(pid)).stat().st_uid == os.geteuid()
    except OSError:
        return False


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


def running_nougat_processes() -> list[tuple[int, Path]]:
    found: list[tuple[int, Path]] = []
    proc = Path("/proc")
    if not proc.is_dir():
        return found
    for entry in proc.iterdir():
        if not entry.name.isdigit():
            continue
        executable = proc_exe(int(entry.name))
        if executable is not None and executable.name.startswith("Nougat_Media_Suite_v"):
            found.append((int(entry.name), executable))
    return found


def terminate_verified(pids: Iterable[int], label: str) -> None:
    targets = sorted({pid for pid in pids if pid > 1 and process_alive(pid)})
    if not targets:
        return
    print(f"Stopping verified {label} before installation...")
    for pid in targets:
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        except PermissionError as exc:
            raise InstallError(f"Unable to stop verified {label} PID {pid}: {exc}") from exc

    deadline = time.monotonic() + 8.0
    while time.monotonic() < deadline:
        remaining = [pid for pid in targets if process_alive(pid)]
        if not remaining:
            return
        time.sleep(0.2)

    remaining = [pid for pid in targets if process_alive(pid)]
    if remaining:
        print(f"Verified {label} did not exit after SIGTERM; forcing shutdown of PID(s): {remaining}")
        for pid in remaining:
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
    deadline = time.monotonic() + 3.0
    while time.monotonic() < deadline:
        remaining = [pid for pid in targets if process_alive(pid)]
        if not remaining:
            return
        time.sleep(0.1)
    raise InstallError(f"Verified {label} is still running: {remaining}")


def stop_running_nougat() -> None:
    processes = running_nougat_processes()
    if not processes:
        return
    for pid, executable in processes:
        print(f"Running Nougat PID {pid}: {executable}")
    terminate_verified((pid for pid, _ in processes), "Nougat Media Suite")


def jellyfin_candidates() -> list[int]:
    candidates: list[int] = []
    proc = Path("/proc")
    if not proc.is_dir():
        return candidates
    for entry in proc.iterdir():
        if not entry.name.isdigit():
            continue
        pid = int(entry.name)
        if not same_user(pid):
            continue
        executable = proc_exe(pid)
        cmdline = proc_blob(pid, "cmdline").lower()
        exe_match = executable is not None and "jellyfin" in executable.name.lower()
        if exe_match or b"jellyfin" in cmdline:
            candidates.append(pid)
    return sorted(set(candidates))


def null_field_contains(blob: bytes, value: str) -> bool:
    encoded = value.encode("utf-8")
    return any(field == encoded for field in blob.split(b"\0") if field)


def stop_verified_owned_jellyfin() -> None:
    candidates = jellyfin_candidates()
    if not candidates:
        return

    expected_runtime = data_home() / "nougat" / "runtime" / "jellyfin" / "jellyfin" / "jellyfin"
    server_data = data_home() / "nougat" / "server"
    server_config = config_home() / "nougat" / "server"
    ownership = state_home() / "nougat" / "server" / "nougat-owned.pid"

    recorded_pid = -1
    recorded_runtime = ""
    token = ""
    try:
        lines = ownership.read_text(encoding="utf-8").splitlines()
        if lines:
            recorded_pid = int(lines[0].strip())
        if len(lines) >= 2:
            recorded_runtime = lines[1].strip()
        if len(lines) >= 3:
            token = lines[2].strip()
    except (OSError, ValueError):
        recorded_pid = -1

    if recorded_runtime and Path(recorded_runtime) != expected_runtime:
        raise InstallError(
            "STOP: Jellyfin ownership record does not match Nougat's managed runtime. "
            f"Recorded={recorded_runtime}; expected={expected_runtime}. No Jellyfin process was killed."
        )

    owned: list[int] = []
    ambiguous: list[int] = []
    for pid in candidates:
        executable = proc_exe(pid)
        cmdline = proc_blob(pid, "cmdline")
        environ = proc_blob(pid, "environ")
        runtime_match = executable == expected_runtime or null_field_contains(cmdline, str(expected_runtime))
        recorded_match = pid == recorded_pid and runtime_match
        token_match = bool(token) and null_field_contains(environ, "NOUGAT_MEDIA_SERVER_OWNER=" + token)
        signature_match = (
            runtime_match
            and null_field_contains(cmdline, str(server_data))
            and null_field_contains(cmdline, str(server_config))
            and null_field_contains(cmdline, "Nougat Media Suite integrated Jellyfin")
        )
        if recorded_match or token_match or signature_match:
            owned.append(pid)
        else:
            ambiguous.append(pid)

    if ambiguous:
        details = ", ".join(f"PID {pid} ({proc_exe(pid) or 'unknown executable'})" for pid in ambiguous)
        raise InstallError(
            "STOP: Jellyfin process ownership is ambiguous. The installer will not kill it. "
            + details
        )

    terminate_verified(owned, "Nougat-owned Jellyfin")
    try:
        ownership.unlink()
    except FileNotFoundError:
        pass
    except OSError as exc:
        raise InstallError(f"Nougat-owned Jellyfin stopped but stale ownership record could not be cleared: {exc}") from exc


def safe_runtime_shutdown() -> None:
    print("Preflight: stopping Nougat-owned runtime processes safely...")
    stop_running_nougat()
    stop_verified_owned_jellyfin()
    if running_nougat_processes():
        raise InstallError("Nougat Media Suite is still running after shutdown preflight")
    print("PASS: safe runtime shutdown preflight")


def load_plugin_catalog(source_root: Path) -> dict[str, dict]:
    catalog: dict[str, dict] = {}
    root = source_root / "plugins"
    if not root.is_dir():
        return catalog
    for manifest_path in sorted(root.glob("*/plugin.json")):
        data = json.loads(manifest_path.read_text(encoding="utf-8"))
        if data.get("format") != PLUGIN_FORMAT or data.get("format_version") != PLUGIN_FORMAT_VERSION:
            raise InstallError(f"Unsupported plugin manifest: {manifest_path}")
        plugin_id = str(data.get("id", "")).strip()
        if not plugin_id or plugin_id in {".", ".."} or "/" in plugin_id or "\\" in plugin_id:
            raise InstallError(f"Unsafe plugin id in {manifest_path}")
        if plugin_id in catalog:
            raise InstallError(f"Duplicate plugin id: {plugin_id}")
        data["_manifest_path"] = str(manifest_path)
        catalog[plugin_id] = data
    return catalog


def parse_plugins(value: str | None) -> list[str]:
    if value is None:
        return []
    return [item.strip() for item in value.split(",") if item.strip()]


def interactive_mode() -> str:
    print("Nougat Media Suite v0.0.50 Installer")
    print("The video player and plugin core are always installed.\n")
    print("1) Default")
    print("2) Custom")
    print("3) Advanced Custom")
    while True:
        choice = input("Choose installation mode [1/2/3]: ").strip().lower()
        if choice in {"1", "default"}:
            return "default"
        if choice in {"2", "custom"}:
            return "custom"
        if choice in {"3", "advanced"}:
            return "advanced"


def interactive_plugins(catalog: dict[str, dict]) -> list[str]:
    selected: list[str] = []
    for plugin_id, data in catalog.items():
        name = str(data.get("display_name", plugin_id))
        answer = input(f"Install {name} [{plugin_id}]? [y/N]: ").strip().lower()
        if answer in {"y", "yes"}:
            selected.append(plugin_id)
    return selected


def choose_plugins(mode: str, catalog: dict[str, dict], requested: list[str] | None) -> list[str]:
    if requested is not None:
        selected = requested
    elif mode == "default":
        selected = [pid for pid, data in catalog.items() if data.get("recommended_by_default") is True]
    else:
        selected = interactive_plugins(catalog)
    unknown = sorted(set(selected) - set(catalog))
    if unknown:
        raise InstallError("Unknown plugin(s): " + ", ".join(unknown))
    return sorted(set(selected))


def copy_file_atomic(source: Path, destination: Path, mode: int | None = None) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(prefix=destination.name + ".", dir=destination.parent, delete=False) as handle:
        temporary = Path(handle.name)
    try:
        shutil.copy2(source, temporary)
        if mode is not None:
            temporary.chmod(mode)
        os.replace(temporary, destination)
    except Exception:
        try:
            temporary.unlink()
        except OSError:
            pass
        raise


def privilege_command(command: list[str]) -> list[str]:
    if os.geteuid() == 0:
        return command
    if sys.stdin.isatty() and shutil.which("sudo"):
        return ["sudo", *command]
    if shutil.which("pkexec"):
        return ["pkexec", *command]
    raise InstallError("No graphical privilege helper is available. Install policykit/pkexec or run from a terminal with sudo available.")


def privileged_run(command: list[str], *, capture: bool = False) -> subprocess.CompletedProcess[str]:
    return run(privilege_command(command), capture=capture)


def prepare_bundled_core(binary: Path, source_root: Path) -> None:
    try:
        bundled = (source_root / APP_BINARY).resolve(strict=True)
    except OSError:
        return
    if binary != bundled:
        return
    mode = binary.stat().st_mode
    if mode & 0o111 == 0:
        binary.chmod(mode | 0o755)


def verify_core(binary: Path) -> None:
    result = run([str(binary), "--version"], capture=True)
    output = (result.stdout or "").strip()
    if result.returncode != 0 or output != "Nougat Media Suite v0.0.50":
        raise InstallError(f"Player identity check failed for {binary}: {output!r}")


def install_core(binary: Path, staging_root: Path | None) -> Path:
    if staging_root is not None:
        destination = staging_root / "opt" / "nougat-media-suite" / APP_BINARY
        copy_file_atomic(binary, destination, 0o755)
        return destination
    destination = APP_PREFIX / APP_BINARY
    result = privileged_run(["install", "-Dm755", str(binary), str(destination)], capture=True)
    if result.returncode != 0:
        raise InstallError("Unable to install Nougat player core to /opt: " + (result.stdout or "").strip())
    return destination


def remove_system_core(destination: Path) -> None:
    result = privileged_run(["rm", "-f", str(destination)], capture=True)
    if result.returncode != 0:
        raise InstallError("Rollback could not remove newly installed player core: " + (result.stdout or "").strip())


def install_desktop_identity(source_root: Path, installed_core: Path, staging_root: Path | None) -> tuple[Path, Path]:
    desktop_source = source_root / APP_DESKTOP_NAME
    icon_source = source_root / "assets" / "icons" / APP_ICON_NAME
    if not desktop_source.is_file() or not icon_source.is_file():
        raise InstallError("Nougat desktop identity assets are missing")
    actual_hash = sha256(icon_source)
    if actual_hash != APP_ICON_SHA256:
        raise InstallError(f"Approved Nougat icon SHA-256 mismatch: {actual_hash}")

    if staging_root is not None:
        user_data = staging_root / "user-data"
    else:
        user_data = data_home()
    desktop_destination = user_data / "applications" / APP_DESKTOP_NAME
    icon_destination = user_data / "icons" / "hicolor" / "256x256" / "apps" / APP_ICON_NAME
    copy_file_atomic(desktop_source, desktop_destination, 0o644)
    copy_file_atomic(icon_source, icon_destination, 0o644)

    desktop = desktop_destination.read_text(encoding="utf-8")
    if f"Exec={APP_PREFIX / APP_BINARY}" not in desktop:
        raise InstallError("Installed launcher does not target the v0.0.50 executable")
    if "Icon=nougat-media-suite-concept-sheet-v24" not in desktop:
        raise InstallError("Installed launcher does not use the approved Nougat icon key")
    if sha256(icon_destination) != APP_ICON_SHA256:
        raise InstallError("Installed icon does not match the approved Nougat N master")

    if staging_root is None:
        if not shutil.which("gio"):
            raise InstallError("gio is required to apply and verify the approved icon on the final raw executable")
        icon_uri = icon_destination.resolve().as_uri()
        set_result = run(["gio", "set", "-t", "string", str(installed_core), "metadata::custom-icon", icon_uri], capture=True)
        if set_result.returncode != 0:
            raise InstallError("Unable to assign the approved icon to the final raw executable: " + (set_result.stdout or "").strip())
        info = run(["gio", "info", "-a", "metadata::custom-icon", str(installed_core)], capture=True)
        output = info.stdout or ""
        if info.returncode != 0 or icon_uri not in output:
            raise InstallError("Approved icon metadata readback failed on the final raw executable")
        print("PASS: final raw executable approved-icon metadata readback")

        if shutil.which("update-desktop-database"):
            run(["update-desktop-database", str(desktop_destination.parent)], capture=True)
        icon_root = user_data / "icons" / "hicolor"
        if shutil.which("gtk-update-icon-cache") and icon_root.is_dir():
            run(["gtk-update-icon-cache", "-f", "-t", str(icon_root)], capture=True)

    return desktop_destination, icon_destination


def plugin_root(staging_root: Path | None) -> Path:
    return staging_root / "user-data" / "nougat" / "plugins" if staging_root is not None else user_plugin_root()


def install_plugin(source_root: Path, plugin_id: str, data: dict, staging_root: Path | None) -> Path:
    root = plugin_root(staging_root)
    destination = root / plugin_id
    temporary = root / ("." + plugin_id + ".installing")
    if temporary.exists():
        shutil.rmtree(temporary)
    temporary.mkdir(parents=True, exist_ok=True)
    try:
        manifest_out = {key: value for key, value in data.items() if not key.startswith("_")}
        for resource in manifest_out.get("resources", []):
            if not isinstance(resource, dict):
                raise InstallError(f"Invalid resource record for plugin {plugin_id}")
            source_rel = Path(str(resource.get("source", "")))
            install_rel = Path(str(resource.get("install_as", "")))
            if source_rel.is_absolute() or install_rel.is_absolute() or ".." in source_rel.parts or ".." in install_rel.parts:
                raise InstallError(f"Unsafe resource path for plugin {plugin_id}")
            source = source_root / source_rel
            target = temporary / install_rel
            if not source.is_file():
                raise InstallError(f"Plugin resource is missing: {source_rel}")
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
            if resource.get("kind") in {"python-worker", "executable"}:
                target.chmod(0o755)
        (temporary / "plugin.json").write_text(json.dumps(manifest_out, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        if destination.exists():
            shutil.rmtree(destination)
        os.replace(temporary, destination)
    except Exception:
        if temporary.exists():
            shutil.rmtree(temporary)
        raise
    return destination


def remove_plugin(plugin_id: str, staging_root: Path | None) -> None:
    if not plugin_id or plugin_id in {".", ".."} or "/" in plugin_id or "\\" in plugin_id:
        raise InstallError("Unsafe plugin id")
    destination = plugin_root(staging_root) / plugin_id
    if destination.exists():
        shutil.rmtree(destination)


def reconcile_plugins(source_root: Path, catalog: dict[str, dict], selected: list[str], staging_root: Path | None) -> list[str]:
    selected_set = set(selected)
    for plugin_id in sorted(catalog):
        if plugin_id not in selected_set:
            remove_plugin(plugin_id, staging_root)
    installed: list[str] = []
    for plugin_id in selected:
        destination = install_plugin(source_root, plugin_id, catalog[plugin_id], staging_root)
        if not (destination / "plugin.json").is_file():
            raise InstallError(f"Plugin install verification failed: {plugin_id}")
        installed.append(plugin_id)
        print(f"Installed plugin: {plugin_id}")
    return installed


def install_state_path(staging_root: Path | None) -> Path:
    root = staging_root / "user-data" / "nougat" if staging_root is not None else data_home() / "nougat"
    return root / "install-state.json"


def write_install_state(mode: str, plugins: list[str], staging_root: Path | None) -> Path:
    destination = install_state_path(staging_root)
    destination.parent.mkdir(parents=True, exist_ok=True)
    state = {
        "format": "NOUGAT_INSTALL_STATE",
        "format_version": 1,
        "application_version": APP_VERSION,
        "core": "core-player",
        "mode": mode,
        "plugins": plugins,
    }
    destination.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return destination


class Rollback:
    def __init__(self, catalog: dict[str, dict], staging_root: Path | None):
        self.catalog = catalog
        self.staging_root = staging_root
        self.temp = Path(tempfile.mkdtemp(prefix="nougat-v50-rollback-"))
        self.core_destination = (staging_root / "opt" / "nougat-media-suite" / APP_BINARY) if staging_root is not None else APP_PREFIX / APP_BINARY
        self.core_existed = self.core_destination.is_file()
        if self.core_existed:
            shutil.copy2(self.core_destination, self.temp / "core")
        self.user_data = staging_root / "user-data" if staging_root is not None else data_home()
        self.desktop = self.user_data / "applications" / APP_DESKTOP_NAME
        self.icon = self.user_data / "icons" / "hicolor" / "256x256" / "apps" / APP_ICON_NAME
        self.state = install_state_path(staging_root)
        self._backup_file(self.desktop, "desktop")
        self._backup_file(self.icon, "icon")
        self._backup_file(self.state, "state")
        self.plugin_backups: set[str] = set()
        root = plugin_root(staging_root)
        for plugin_id in catalog:
            source = root / plugin_id
            if source.is_dir():
                shutil.copytree(source, self.temp / "plugins" / plugin_id)
                self.plugin_backups.add(plugin_id)

    def _backup_file(self, source: Path, name: str) -> None:
        if source.is_file():
            target = self.temp / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)

    def _restore_file(self, destination: Path, name: str) -> None:
        backup = self.temp / name
        if backup.is_file():
            copy_file_atomic(backup, destination)
        else:
            try:
                destination.unlink()
            except FileNotFoundError:
                pass

    def restore(self) -> None:
        print("Installation failed; restoring the exact pre-install Nougat application files...")
        if self.staging_root is not None:
            if self.core_existed:
                copy_file_atomic(self.temp / "core", self.core_destination, 0o755)
            else:
                try:
                    self.core_destination.unlink()
                except FileNotFoundError:
                    pass
        else:
            if self.core_existed:
                result = privileged_run(["install", "-Dm755", str(self.temp / "core"), str(self.core_destination)], capture=True)
                if result.returncode != 0:
                    raise InstallError("ROLLBACK FAIL restoring previous player core: " + (result.stdout or "").strip())
            else:
                remove_system_core(self.core_destination)

        self._restore_file(self.desktop, "desktop")
        self._restore_file(self.icon, "icon")
        self._restore_file(self.state, "state")
        root = plugin_root(self.staging_root)
        for plugin_id in self.catalog:
            current = root / plugin_id
            if current.exists():
                shutil.rmtree(current)
            backup = self.temp / "plugins" / plugin_id
            if backup.is_dir():
                current.parent.mkdir(parents=True, exist_ok=True)
                shutil.copytree(backup, current)
        print("PASS: pre-install application files restored")

    def close(self) -> None:
        shutil.rmtree(self.temp, ignore_errors=True)


def verify_install(installed_core: Path, plugins: list[str], staging_root: Path | None) -> None:
    verify_core(installed_core)
    root = plugin_root(staging_root)
    for plugin_id in plugins:
        if not (root / plugin_id / "plugin.json").is_file():
            raise InstallError(f"Installed plugin disappeared during verification: {plugin_id}")
    state = json.loads(install_state_path(staging_root).read_text(encoding="utf-8"))
    if state.get("application_version") != APP_VERSION or state.get("plugins") != plugins:
        raise InstallError("Install-state verification failed")


def main() -> int:
    parser = argparse.ArgumentParser(description="Install Nougat Media Suite v0.0.50")
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--mode", choices=["default", "custom", "advanced"])
    parser.add_argument("--plugins", help="Comma-separated optional plugin IDs. Empty means player only.")
    parser.add_argument("--list-plugins", action="store_true")
    parser.add_argument("--remove-plugin")
    parser.add_argument("--staging-root", type=Path, help="Test installation root; never invokes privilege helpers.")
    args = parser.parse_args()

    rollback: Rollback | None = None
    try:
        source_root = args.source_root.expanduser().resolve()
        catalog = load_plugin_catalog(source_root)
        staging_root = args.staging_root.expanduser().resolve() if args.staging_root else None

        if staging_root is None and os.geteuid() == 0:
            raise InstallError("Run the graphical installer as your normal desktop user, not as root.")

        if args.list_plugins:
            for plugin_id, data in catalog.items():
                marker = "recommended" if data.get("recommended_by_default") else "optional"
                print(f"{plugin_id}\t{data.get('display_name', plugin_id)}\t{marker}")
            return 0

        if args.remove_plugin:
            if staging_root is None:
                safe_runtime_shutdown()
            remove_plugin(args.remove_plugin, staging_root)
            remaining = [pid for pid in catalog if (plugin_root(staging_root) / pid / "plugin.json").is_file()]
            write_install_state("custom", sorted(remaining), staging_root)
            print("PASS: plugin removed")
            return 0

        mode = args.mode or interactive_mode()
        requested = parse_plugins(args.plugins) if args.plugins is not None else None
        selected = choose_plugins(mode, catalog, requested)
        binary = (args.binary or source_root / APP_BINARY).expanduser().resolve()
        if not binary.is_file():
            raise InstallError(f"Built Nougat player core is missing: {binary}")
        prepare_bundled_core(binary, source_root)
        verify_core(binary)
        icon = source_root / "assets" / "icons" / APP_ICON_NAME
        if not icon.is_file() or sha256(icon) != APP_ICON_SHA256:
            raise InstallError("Approved Nougat N icon source is missing or has the wrong SHA-256")

        if staging_root is None:
            safe_runtime_shutdown()
        rollback = Rollback(catalog, staging_root)

        installed_core = install_core(binary, staging_root)
        verify_core(installed_core)
        install_desktop_identity(source_root, installed_core, staging_root)
        installed_plugins = reconcile_plugins(source_root, catalog, selected, staging_root)
        state = write_install_state(mode, installed_plugins, staging_root)
        verify_install(installed_core, installed_plugins, staging_root)

        print("=== NOUGAT MEDIA SUITE v0.0.50 INSTALL PASS ===")
        print("Core player:", installed_core)
        print("Mode:", mode)
        print("Plugins:", ", ".join(installed_plugins) if installed_plugins else "none (player only)")
        print("State:", state)
        print("PASS: installer transaction and final verification complete")
        rollback.close()
        rollback = None
        return 0
    except Exception as exc:
        if rollback is not None:
            try:
                rollback.restore()
            except Exception as rollback_exc:
                print("ROLLBACK FAIL:", rollback_exc)
            finally:
                rollback.close()
        print("INSTALL FAIL:", exc)
        print("Nougat v0.0.50 was not accepted or tagged by this installer.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
