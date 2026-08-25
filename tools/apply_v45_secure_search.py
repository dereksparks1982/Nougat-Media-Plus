#!/usr/bin/env python3
from __future__ import annotations

import re
import shutil
import subprocess
import sys
from pathlib import Path

EXPECTED_BLOBS = {
    "CMakeLists.txt": "e191405e29273ca2c9c6ed6fd5cc9c450a11b7b9",
    "src/main.cpp": "b7ee9f460a556b2985fdf1ded246826b4adf972e",
    "src/nougat/nougat_bridge.cpp": "bec8949f895baaa772cc2fedd6fa4d28264425cc",
    "src/nougat/nougat_bridge.hpp": "7ac84b1a2b574d8997174436cf61686f96439612",
    "components/nougat/nougat_engine.py": "8247a53a60ab3a437cc252768054e8eb3054aaf9",
}

V45_ROADMAP = """## v0.0.45 candidate - Secure Search Foundation, Privacy Broker & Crawler Access Architecture

- Split user Search from crawler networking: local Search uses a no-network worker and receives plaintext queries only over private local stdin IPC, never argv, URL query strings, environment variables, ordinary logs, diagnostics, or privacy receipts.
- Remove automatic DuckDuckGo/live-discovery fallback and disable plaintext remote peer Search. Secure remote Search fails closed until the Privacy Broker can provide the required private transport; there is no direct fallback.
- Add versioned `SecureSearchController`, Privacy Policy, Privacy Receipt, Privacy Broker client/protocol, and Crawler Access Manager interfaces so future OHTTP, ODoH/ECH, PIR/HE, mix-network, post-quantum, renderer-containment, and relay-directory providers can be replaced without rewriting Search.
- Split `nougat_search_worker.py` and `nougat_crawler_worker.py`. The administrative node service becomes loopback-only and no longer accepts plaintext Search queries over `/nougat/v1/search`.
- Identify crawling truthfully as `NougatSearchCrawler/0.0.45`, respect robots restrictions, and expose access states for bot-policy block, rate limit, authentication, feeds, payment-required, and temporary unavailability. Nougat never spends automatically on HTTP 402.
- Add the Rust Privacy Broker v1 scaffold as an optional, isolated security component without making Rust a hard dependency of the v0.0.45 C++ application build.
- Keep Secure Search implementation outside `src/main.cpp`; main receives only controller wiring and truthful Secure Search loading/status text.
- Candidate remains uncommitted/untagged/unpushed until owner acceptance.

"""

V46_ROADMAP = """## v0.0.46 planned - World TV Repair, Verified Broadcasts & Kaaba Live

- Make World TV the primary v0.0.46 repair/polish focus. Fix the X11/libVLC black video-child overlay that can cover the station list when the World TV surface is active, and validate clean player/list window mapping during every view transition.
- Harden native broadcaster playback, reconnect/failover behavior, stream-health reporting, source verification, station selection, scrolling, status text, and correct station/network artwork so World TV behaves like a finished Nougat surface rather than an experimental catalog.
- Double-clicking the small approved Nougat `N` icon beside the application name opens Kaaba Live immediately in Nougat's native player.
- Prefer the official Saudi Quran TV / Saudi Broadcasting Authority / ALOULA Masjid al-Haram live broadcast path for Kaaba Live. Verify the actual native stream endpoint, quality, redirects, uptime, and libVLC behavior before shipping; use only verified official distribution/fallback paths rather than mystery IPTV mirrors.
- Keep a future Madinah / Saudi Sunnah TV companion path available for later owner approval without complicating the default N double-click action.
- Candidate work begins only after v0.0.45 is owner-accepted.

"""

README_SECTION = """## v0.0.45 - Secure Search Foundation

Nougat Search now has a fail-closed privacy foundation. User queries are sent to a local no-network search worker through private stdin IPC instead of process arguments, automatic live-discovery fallback is removed, crawler networking is separated from user Search, and plaintext remote peer Search is disabled until a versioned Privacy Broker transport can satisfy Nougat's privacy policy. The crawler now uses a truthful Nougat Search identity and reports access restrictions such as robots policy, bot-policy blocks, rate limits, authentication, payment-required responses, feeds, and temporary unavailability.

The v0.0.45 architecture deliberately leaves production OHTTP/multi-relay transport, PIR/homomorphic private retrieval, mix/batching defenses, post-quantum transport, browser containment, and signed relay-directory work behind replaceable interfaces rather than hard-wiring today's mechanism into the application.

"""

CHANGELOG_SECTION = """## v0.0.45 - Secure Search Foundation, Privacy Broker & Crawler Access Architecture

- Moved plaintext Search query delivery from child-process argv to private local stdin IPC.
- Removed automatic DuckDuckGo/live-discovery Search fallback and disabled plaintext remote peer Search.
- Split local Search and crawler networking into separate workers.
- Added fail-closed Secure Search controller, Privacy Policy, Privacy Receipt, Privacy Broker client/protocol scaffold, and Crawler Access Manager.
- Made the legacy administrative node loopback-only and refused plaintext `/nougat/v1/search` requests.
- Added truthful `NougatSearchCrawler/0.0.45` identity, robots handling, crawler access classification, and no-auto-payment handling for HTTP 402.
- Added privacy-canary validation proving a unique query does not appear in worker argv, environment, output, or persistent Nougat Search files.

"""


def stop(msg: str) -> int:
    print(f"STOP: {msg}")
    return 1


def git(root: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(["git", *args], cwd=root, capture_output=True, text=True)


def git_blob(root: Path, rel: str) -> str:
    result = git(root, "hash-object", rel)
    return result.stdout.strip() if result.returncode == 0 else ""


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one baseline match, found {count}")
    return text.replace(old, new, 1)


def patch_main(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    if '#include "search/secure_search.hpp"' not in text:
        text = replace_once(
            text,
            '#include "nougat/nougat_bridge.hpp"\n',
            '#include "nougat/nougat_bridge.hpp"\n#include "search/secure_search.hpp"\n',
            "Secure Search include",
        )

    if "reddmedia::SecureSearchController::search(" not in text:
        text = replace_once(
            text,
            "reddmedia::NougatSearchResponse response = nougat.search(query, raw, include_peers, 100, offset);",
            "reddmedia::NougatSearchResponse response = reddmedia::SecureSearchController::search(nougat, query, raw, include_peers, 100, offset);",
            "Secure Search controller wiring",
        )

    if 'nougatState->status = "Searching Local Index...";' not in text:
        text = replace_once(
            text,
            'nougatState->status = "Searching Nougat...";',
            'nougatState->status = "Searching Local Index...";',
            "truthful local Search stage",
        )
    old_status = 'status << (raw ? "RAW" : "RANKED") << ": " << response.total << " matching record(s) reported";'
    new_status = 'status << "Secure Search Complete | " << (raw ? "RAW" : "RANKED") << ": " << response.total << " matching record(s) reported";'
    if old_status in text:
        text = replace_once(text, old_status, new_status, "Secure Search completion status")
    show_line = 'if (!response.results.empty()) status << " | showing " << (offset + 1) << "-" << (offset + (int)response.results.size());'
    remote_line = show_line + '\n                if (response.secure_remote_unavailable) status << " | secure remote unavailable; query not sent";'
    if "secure remote unavailable; query not sent" not in text:
        text = replace_once(text, show_line, remote_line, "fail-closed remote status")

    if re.search(r"\bbool\s+nougatSearchPeers\s*=\s*true\s*;", text):
        text, count = re.subn(r"\bbool\s+nougatSearchPeers\s*=\s*true\s*;", "bool nougatSearchPeers = false;", text, count=1)
        if count != 1:
            raise RuntimeError("could not disable remote Search by default")

    # The app-wide loading strip already keys off nougatState search/crawl busy state.
    # Its label is intentionally not user-rendered in v44, so do not anchor v45
    # installation to a nonexistent per-view loading-label assignment.

    text = text.replace("Nougat_Media_Suite_v44", "Nougat_Media_Suite_v45")
    text = text.replace("v0.0.44", "v0.0.45")
    path.write_text(text, encoding="utf-8")


def patch_desktop(path: Path) -> None:
    if not path.is_file():
        raise RuntimeError(f"missing desktop launcher: {path.name}")
    text = path.read_text(encoding="utf-8")
    if "Nougat_Media_Suite_v45" not in text:
        if "Nougat_Media_Suite_v44" not in text:
            raise RuntimeError(f"{path.name}: expected v44 executable target not found")
        text = text.replace("Nougat_Media_Suite_v44", "Nougat_Media_Suite_v45")
        path.write_text(text, encoding="utf-8")


def patch_docs(root: Path) -> None:
    readme = root / "README.md"
    if readme.is_file():
        text = readme.read_text(encoding="utf-8")
        if "## v0.0.45 - Secure Search Foundation" not in text:
            marker = "# Nougat Media Suite\n\n"
            if not text.startswith(marker):
                raise RuntimeError("README title/structure changed; refusing blind insertion")
            text = marker + README_SECTION + text[len(marker):]
            readme.write_text(text, encoding="utf-8")

    changelog = root / "CHANGELOG.md"
    if changelog.is_file():
        text = changelog.read_text(encoding="utf-8")
        if "## v0.0.45 - Secure Search Foundation" not in text:
            marker = "# Changelog\n\n"
            if not text.startswith(marker):
                raise RuntimeError("CHANGELOG structure changed; refusing blind insertion")
            text = marker + CHANGELOG_SECTION + text[len(marker):]
            changelog.write_text(text, encoding="utf-8")

    roadmap = root / "ROADMAP.md"
    if roadmap.is_file():
        text = roadmap.read_text(encoding="utf-8")
        if not text.startswith("## v0.0.45 candidate"):
            text = V45_ROADMAP + text
        if "## v0.0.46 planned - World TV Repair, Verified Broadcasts & Kaaba Live" not in text:
            insert_at = len(V45_ROADMAP) if text.startswith(V45_ROADMAP) else 0
            text = text[:insert_at] + V46_ROADMAP + text[insert_at:]
        roadmap.write_text(text, encoding="utf-8")



def snapshot_files(paths: list[Path]) -> dict[Path, tuple[bytes | None, int | None]]:
    snapshot: dict[Path, tuple[bytes | None, int | None]] = {}
    for path in paths:
        if path.is_file():
            snapshot[path] = (path.read_bytes(), path.stat().st_mode)
        else:
            snapshot[path] = (None, None)
    return snapshot


def restore_snapshot(snapshot: dict[Path, tuple[bytes | None, int | None]]) -> None:
    for path, (data, mode) in snapshot.items():
        if data is None:
            if path.exists():
                if path.is_dir():
                    shutil.rmtree(path)
                else:
                    path.unlink()
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        if mode is not None:
            path.chmod(mode)



def main() -> int:
    package_root = Path(__file__).resolve().parents[1]
    root = Path(sys.argv[1]).expanduser().resolve() if len(sys.argv) > 1 else Path.cwd().resolve()
    print("=== NOUGAT MEDIA SUITE v0.0.45 SECURE SEARCH APPLY R3 ===")
    print(f"Package: {package_root}")
    print(f"Project: {root}")

    if not (root / ".git").exists():
        return stop("target is not the Nougat Media Suite Git working tree")
    status = git(root, "status", "--porcelain")
    if status.returncode != 0:
        return stop("could not read Git working-tree status")
    if status.stdout.strip():
        print(status.stdout)
        return stop("working tree is not clean; v45 will not overwrite uncommitted work")

    cmake_text = (root / "CMakeLists.txt").read_text(encoding="utf-8") if (root / "CMakeLists.txt").is_file() else ""
    already_v45 = "VERSION 0.0.45" in cmake_text
    if not already_v45:
        for rel, expected in EXPECTED_BLOBS.items():
            if not (root / rel).is_file():
                return stop(f"missing v44 baseline file: {rel}")
            actual = git_blob(root, rel)
            if actual != expected:
                return stop(f"baseline mismatch for {rel}: expected Git blob {expected}, got {actual or 'UNKNOWN'}")
        print("PASS: exact committed v0.0.44 source baseline verified")

    payload_files = [
        "CMakeLists.txt",
        "src/nougat/nougat_bridge.hpp",
        "src/nougat/nougat_bridge.cpp",
        "src/search/secure_search.hpp",
        "src/search/secure_search.cpp",
        "src/privacy/privacy_policy.hpp",
        "src/privacy/privacy_policy.cpp",
        "src/privacy/privacy_receipt.hpp",
        "src/privacy/privacy_receipt.cpp",
        "src/privacy/privacy_broker_client.hpp",
        "src/privacy/privacy_broker_client.cpp",
        "src/crawler/crawler_access_manager.hpp",
        "src/crawler/crawler_access_manager.cpp",
        "components/nougat/nougat_common.py",
        "components/nougat/nougat_search_worker.py",
        "components/nougat/nougat_crawler_worker.py",
        "components/nougat/nougat_engine.py",
        "components/privacy_broker/Cargo.toml",
        "components/privacy_broker/rust-toolchain.toml",
        "components/privacy_broker/src/main.rs",
        "components/privacy_broker/src/protocol.rs",
        "components/privacy_broker/src/policy.rs",
        "components/privacy_broker/src/transport.rs",
        "components/privacy_broker/src/relay.rs",
        "components/privacy_broker/src/receipt.rs",
        "docs/security/NOUGAT_SECURE_SEARCH_ARCHITECTURE_v1.md",
        "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_45_SECURE_SEARCH_FOUNDATION_OWNER_HANDSHAKE.md",
        "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_45_SECURE_SEARCH_FOUNDATION_VALIDATION.md",
        "tools/test_nougat_secure_search_v45.py",
        "tools/build_v45_secure_search.py",
        "tools/apply_v45_secure_search.py",
    ]

    touched = [root / "src/main.cpp",
               root / "NougatMediaSuite.desktop",
               root / "com.elderredsoftworks.NougatMediaSuite.desktop",
               root / "README.md",
               root / "CHANGELOG.md",
               root / "ROADMAP.md"]
    touched.extend(root / rel for rel in payload_files)
    snapshot = snapshot_files(touched)

    try:
        if not already_v45:
            patch_main(root / "src/main.cpp")
        for rel in payload_files:
            src = package_root / rel
            dst = root / rel
            if not src.is_file():
                raise RuntimeError(f"package payload missing: {rel}")
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
        patch_desktop(root / "NougatMediaSuite.desktop")
        patch_desktop(root / "com.elderredsoftworks.NougatMediaSuite.desktop")
        patch_docs(root)

        for rel in [
            "components/nougat/nougat_engine.py",
            "components/nougat/nougat_search_worker.py",
            "components/nougat/nougat_crawler_worker.py",
            "tools/test_nougat_secure_search_v45.py",
            "tools/build_v45_secure_search.py",
            "tools/apply_v45_secure_search.py",
        ]:
            executable = root / rel
            executable.chmod(executable.stat().st_mode | 0o111)

        print("PASS: v0.0.45 changed files applied")
        test = root / "tools/test_nougat_secure_search_v45.py"
        result = subprocess.run([sys.executable, str(test), str(root)], cwd=root)
        if result.returncode != 0:
            raise RuntimeError("v0.0.45 pre-build Secure Search validation failed")
    except Exception as exc:
        print(f"STOP: {exc}")
        print("ROLLBACK: restoring the exact pre-v45 working-tree files...")
        restore_snapshot(snapshot)
        print("PASS: failed v45 apply was rolled back; committed v0.0.44 source restored")
        return 1

    print("PASS: v0.0.45 pre-build validation")
    print("NEXT: python3 tools/build_v45_secure_search.py")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
