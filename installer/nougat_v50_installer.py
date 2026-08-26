#!/usr/bin/env python3
"""Nougat Media Suite v0.0.50 installer.

Core law: the video player is always installed. Everything else is an optional
plugin. The installer supports Default, Custom, and Advanced Custom modes.
Optional plugins are installed per-user under the Nougat XDG data tree so a
user can add/remove capabilities without making the source tree or Downloads
folder part of the runtime contract.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

APP_VERSION = "0.0.50"
APP_BINARY = "Nougat_Media_Suite_v50"
APP_PREFIX = Path("/opt/nougat-media-suite")
PLUGIN_FORMAT = "NOUGAT_PLUGIN"
PLUGIN_FORMAT_VERSION = 1


class InstallError(RuntimeError):
    pass


def run(args: list[str], *, capture: bool = False) -> subprocess.CompletedProcess[str]:
    kwargs: dict[str, object] = {"text": True}
    if capture:
        kwargs.update(stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return subprocess.run(args, **kwargs)  # type: ignore[arg-type]


def xdg_data_home() -> Path:
    value = os.environ.get("XDG_DATA_HOME", "").strip()
    if value:
        return Path(value).expanduser()
    return Path.home() / ".local" / "share"


def user_plugin_root() -> Path:
    override = os.environ.get("NOUGAT_PLUGIN_ROOT", "").strip()
    if override:
        return Path(override).expanduser()
    return xdg_data_home() / "nougat" / "plugins"


def load_plugin_catalog(source_root: Path) -> dict[str, dict]:
    catalog: dict[str, dict] = {}
    plugin_dir = source_root / "plugins"
    if not plugin_dir.is_dir():
        return catalog
    for manifest_path in sorted(plugin_dir.glob("*/plugin.json")):
        data = json.loads(manifest_path.read_text(encoding="utf-8"))
        if data.get("format") != PLUGIN_FORMAT or data.get("format_version") != PLUGIN_FORMAT_VERSION:
            raise InstallError(f"Unsupported plugin manifest: {manifest_path}")
        plugin_id = str(data.get("id", "")).strip()
        if not plugin_id or "/" in plugin_id or "\\" in plugin_id or plugin_id in {".", ".."}:
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
    print("The video player is always installed. Optional plugins are your choice.\n")
    print("1) Default        Player + recommended plugins")
    print("2) Custom         Player + selected feature plugins")
    print("3) Advanced Custom Player + individually selected plugins/components")
    while True:
        choice = input("Choose installation mode [1/2/3]: ").strip()
        if choice in {"1", "default", "Default"}:
            return "default"
        if choice in {"2", "custom", "Custom"}:
            return "custom"
        if choice in {"3", "advanced", "Advanced"}:
            return "advanced"
        print("Choose 1, 2, or 3.")


def interactive_plugins(catalog: dict[str, dict], *, advanced: bool) -> list[str]:
    selected: list[str] = []
    if not catalog:
        print("No optional plugins are currently available.")
        return selected
    title = "individual plugin/component" if advanced else "feature plugin"
    print(f"\nSelect optional {title}s. Press Enter for No.")
    for plugin_id, data in catalog.items():
        name = data.get("display_name", plugin_id)
        description = data.get("description", "")
        print(f"\n{name} [{plugin_id}]")
        if description:
            print(f"  {description}")
        answer = input("Install this plugin? [y/N]: ").strip().lower()
        if answer in {"y", "yes"}:
            selected.append(plugin_id)
    return selected


def choose_plugins(mode: str, catalog: dict[str, dict], requested: list[str] | None) -> list[str]:
    if requested is not None:
        selected = requested
    elif mode == "default":
        selected = [
            plugin_id for plugin_id, data in catalog.items()
            if data.get("recommended_by_default") is True
        ]
    else:
        selected = interactive_plugins(catalog, advanced=(mode == "advanced"))

    unknown = sorted(set(selected) - set(catalog))
    if unknown:
        raise InstallError("Unknown plugin(s): " + ", ".join(unknown))
    return sorted(set(selected))


def copy_file_atomic(source: Path, destination: Path, mode: int | None = None) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(prefix=destination.name + ".", dir=destination.parent, delete=False) as handle:
        temp = Path(handle.name)
    try:
        shutil.copy2(source, temp)
        if mode is not None:
            temp.chmod(mode)
        os.replace(temp, destination)
    except Exception:
        try:
            temp.unlink()
        except OSError:
            pass
        raise


def install_core_staged(binary: Path, staging_root: Path) -> Path:
    destination = staging_root / "opt" / "nougat-media-suite" / APP_BINARY
    copy_file_atomic(binary, destination, 0o755)
    return destination


def install_core_system(binary: Path) -> Path:
    destination = APP_PREFIX / APP_BINARY
    command = ["install", "-Dm755", str(binary), str(destination)]
    if os.geteuid() != 0:
        command.insert(0, "sudo")
    result = run(command)
    if result.returncode != 0:
        raise InstallError("Unable to install Nougat player core to /opt")
    return destination


def install_desktop_identity(source_root: Path, *, staging_root: Path | None) -> None:
    desktop_source = source_root / "NougatMediaSuite.desktop"
    icon_source = source_root / "assets" / "icons" / "nougat-media-suite-concept-sheet-v24.png"
    if not desktop_source.is_file() or not icon_source.is_file():
        raise InstallError("Nougat desktop identity assets are missing")

    if staging_root is not None:
        data_home = staging_root / "user-data"
    else:
        data_home = xdg_data_home()

    desktop_destination = data_home / "applications" / "NougatMediaSuite.desktop"
    icon_destination = data_home / "icons" / "hicolor" / "256x256" / "apps" / "nougat-media-suite-concept-sheet-v24.png"
    copy_file_atomic(desktop_source, desktop_destination, 0o644)
    copy_file_atomic(icon_source, icon_destination, 0o644)


def install_plugin(source_root: Path, plugin_id: str, data: dict, *, staging_root: Path | None) -> Path:
    if staging_root is not None:
        plugin_root = staging_root / "user-data" / "nougat" / "plugins"
    else:
        plugin_root = user_plugin_root()
    destination = plugin_root / plugin_id
    temporary = plugin_root / ("." + plugin_id + ".installing")

    if temporary.exists():
        shutil.rmtree(temporary)
    temporary.mkdir(parents=True, exist_ok=True)

    try:
        manifest_out = {key: value for key, value in data.items() if not key.startswith("_")}
        resources = manifest_out.get("resources", [])
        for resource in resources:
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
            if resource.get("kind") == "python-worker":
                target.chmod(0o755)

        (temporary / "plugin.json").write_text(
            json.dumps(manifest_out, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )

        if destination.exists():
            shutil.rmtree(destination)
        os.replace(temporary, destination)
    except Exception:
        if temporary.exists():
            shutil.rmtree(temporary)
        raise

    return destination


def remove_plugin(plugin_id: str, *, staging_root: Path | None) -> None:
    if "/" in plugin_id or "\\" in plugin_id or plugin_id in {"", ".", ".."}:
        raise InstallError("Unsafe plugin id")
    if staging_root is not None:
        root = staging_root / "user-data" / "nougat" / "plugins"
    else:
        root = user_plugin_root()
    destination = root / plugin_id
    if destination.exists():
        shutil.rmtree(destination)
    print(f"Removed plugin: {plugin_id}")


def verify_core(binary: Path) -> None:
    result = run([str(binary), "--version"], capture=True)
    output = (result.stdout or "").strip()
    if result.returncode != 0 or output != "Nougat Media Suite v0.0.50":
        raise InstallError(f"Installed player identity check failed: {output!r}")


def write_install_state(mode: str, plugins: list[str], *, staging_root: Path | None) -> Path:
    if staging_root is not None:
        data_root = staging_root / "user-data" / "nougat"
    else:
        data_root = xdg_data_home() / "nougat"
    data_root.mkdir(parents=True, exist_ok=True)
    state = {
        "format": "NOUGAT_INSTALL_STATE",
        "format_version": 1,
        "application_version": APP_VERSION,
        "core": "core-player",
        "mode": mode,
        "plugins": plugins,
    }
    destination = data_root / "install-state.json"
    destination.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return destination


def main() -> int:
    parser = argparse.ArgumentParser(description="Install Nougat Media Suite v0.0.50")
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--mode", choices=["default", "custom", "advanced"])
    parser.add_argument("--plugins", help="Comma-separated optional plugin IDs. Empty string means player only.")
    parser.add_argument("--list-plugins", action="store_true")
    parser.add_argument("--remove-plugin")
    parser.add_argument("--staging-root", type=Path, help="Test installation root; never invokes sudo.")
    args = parser.parse_args()

    try:
        source_root = args.source_root.expanduser().resolve()
        catalog = load_plugin_catalog(source_root)

        if args.list_plugins:
            print("Available optional plugins:")
            for plugin_id, data in catalog.items():
                marker = "recommended" if data.get("recommended_by_default") else "optional"
                print(f"  {plugin_id}: {data.get('display_name', plugin_id)} ({marker})")
            return 0

        staging_root = args.staging_root.expanduser().resolve() if args.staging_root else None
        if args.remove_plugin:
            remove_plugin(args.remove_plugin, staging_root=staging_root)
            return 0

        mode = args.mode or interactive_mode()
        requested = parse_plugins(args.plugins) if args.plugins is not None else None
        selected = choose_plugins(mode, catalog, requested)

        binary = (args.binary or (source_root / APP_BINARY)).expanduser().resolve()
        if not binary.is_file():
            raise InstallError(f"Built Nougat player core is missing: {binary}")
        verify_core(binary)

        if staging_root is not None:
            installed_core = install_core_staged(binary, staging_root)
        else:
            installed_core = install_core_system(binary)
        verify_core(installed_core)
        install_desktop_identity(source_root, staging_root=staging_root)

        installed_plugins: list[str] = []
        for plugin_id in selected:
            destination = install_plugin(source_root, plugin_id, catalog[plugin_id], staging_root=staging_root)
            manifest = destination / "plugin.json"
            if not manifest.is_file():
                raise InstallError(f"Plugin install verification failed: {plugin_id}")
            installed_plugins.append(plugin_id)
            print(f"Installed plugin: {plugin_id}")

        state_path = write_install_state(mode, installed_plugins, staging_root=staging_root)

        print("=== NOUGAT MEDIA SUITE v0.0.50 INSTALL PASS ===")
        print("Core player:", installed_core)
        print("Mode:", mode)
        print("Plugins:", ", ".join(installed_plugins) if installed_plugins else "none (player only)")
        print("State:", state_path)
        return 0
    except Exception as exc:
        print("INSTALL FAIL:", exc)
        print("Nougat v0.0.50 was not accepted or tagged by this installer.")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
