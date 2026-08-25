#!/usr/bin/env python3
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

CANARY = "NOUGAT_PRIVACY_CANARY_9F72C81"

def fail(msg: str) -> None:
    print(f"FAIL: {msg}")
    raise SystemExit(1)

def ok(msg: str) -> None:
    print(f"PASS: {msg}")

def read(root: Path, rel: str) -> str:
    path = root / rel
    if not path.is_file():
        fail(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8")

def require(text: str, token: str, label: str) -> None:
    if token not in text:
        fail(f"{label}: missing {token!r}")

def forbid(text: str, token: str, label: str) -> None:
    if token in text:
        fail(f"{label}: forbidden token present: {token!r}")

def scan_files_for_canary(root: Path) -> list[str]:
    hits: list[str] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        try:
            data = path.read_bytes()
        except OSError:
            continue
        if CANARY.encode() in data:
            hits.append(str(path))
    return hits

def compile_secure_search_cpp(root: Path) -> None:
    compiler = shutil.which("g++")
    if not compiler:
        fail("g++ is required for Secure Search module compile validation")
    files = [
        "src/nougat/nougat_bridge.cpp",
        "src/search/secure_search.cpp",
        "src/privacy/privacy_policy.cpp",
        "src/privacy/privacy_receipt.cpp",
        "src/privacy/privacy_broker_client.cpp",
        "src/crawler/crawler_access_manager.cpp",
    ]
    with tempfile.TemporaryDirectory(prefix="nougat-v46-secure-search-obj-") as td:
        for rel in files:
            target = Path(td) / (Path(rel).name + ".o")
            result = subprocess.run(
                [
                    compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    "-c", rel, "-o", str(target),
                ],
                cwd=root, capture_output=True, text=True,
            )
            if result.returncode != 0:
                fail(f"C++ compile failed for {rel}:\n{result.stdout}{result.stderr}")
    ok("Secure Search C++ modules compile with warnings-as-errors")

def privacy_canary_test(root: Path) -> None:
    worker = root / "components/nougat/nougat_search_worker.py"
    with tempfile.TemporaryDirectory(prefix="nougat-v46-privacy-") as td:
        env = os.environ.copy()
        env["NOUGAT_HOME"] = td
        env["NOUGAT_TEST_HOLD_MS"] = "900"
        proc = subprocess.Popen(
            [sys.executable, str(worker), "search", "--limit", "5", "--offset", "0"],
            cwd=worker.parent,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=env,
        )
        if proc.stdin is None:
            fail("privacy canary worker stdin was not created")
        proc.stdin.write(CANARY + "\n")
        proc.stdin.close()
        time.sleep(0.15)

        cmdline_path = Path(f"/proc/{proc.pid}/cmdline")
        environ_path = Path(f"/proc/{proc.pid}/environ")
        cmdline = cmdline_path.read_bytes() if cmdline_path.exists() else b""
        environ = environ_path.read_bytes() if environ_path.exists() else b""

        if CANARY.encode() in cmdline:
            proc.kill()
            fail("plaintext search query leaked into process argv")
        if CANARY.encode() in environ:
            proc.kill()
            fail("plaintext search query leaked into process environment")

        stdout = proc.stdout.read() if proc.stdout else ""
        stderr = proc.stderr.read() if proc.stderr else ""
        rc = proc.wait(timeout=5)
        if rc != 0:
            fail(f"local search worker failed privacy canary test: {stderr[-1000:]}")
        if CANARY in stdout or CANARY in stderr:
            fail("plaintext search query was echoed to worker output")

        hits = scan_files_for_canary(Path(td))
        if hits:
            fail("plaintext search query persisted in Nougat data files: " + ", ".join(hits))
    ok("privacy canary absent from argv, environment, output, and persistent local files")

def main() -> None:
    root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()
    print(f"=== NOUGAT MEDIA SUITE v0.0.46 SECURE SEARCH RETENTION VALIDATION ===\nProject: {root}")

    cmake = read(root, "CMakeLists.txt")
    for token in [
        "project(NougatMediaSuite VERSION 0.0.46 LANGUAGES CXX)",
        "add_executable(Nougat_Media_Suite_v46",
        "src/search/secure_search.cpp",
        "src/privacy/privacy_broker_client.cpp",
        "src/crawler/crawler_access_manager.cpp",
        "target_compile_options(Nougat_Media_Suite_v46 PRIVATE -Wall -Wextra -Werror)",
    ]:
        require(cmake, token, "CMake v46")
    ok("CMake wires retained Secure Search modules into v46")

    main_cpp = read(root, "src/main.cpp")
    require(main_cpp, '#include "search/secure_search.hpp"', "main Secure Search wiring")
    require(
        main_cpp,
        "reddmedia::SecureSearchController::search(nougat, query, raw, include_peers, 100, offset)",
        "main Secure Search controller call",
    )
    forbid(main_cpp, "nougat.search(query, raw, include_peers, 100, offset)", "direct Search bridge call")
    require(main_cpp, 'nougatState->status = "Searching Local Index...";', "truthful Search stage")
    require(main_cpp, "Secure Search Complete", "Secure Search completion status")
    if main_cpp.count("PrivacyBrokerClient") != 0:
        fail("main.cpp contains Privacy Broker implementation instead of controller-only wiring")
    ok("main.cpp retains controller wiring only, not privacy implementation")

    bridge = read(root, "src/nougat/nougat_bridge.cpp")
    require(bridge, 'sibling_worker("nougat_search_worker.py")', "split Search worker")
    require(bridge, 'sibling_worker("nougat_crawler_worker.py")', "split crawler worker")
    require(bridge, 'query + "\\n"', "private stdin Search IPC")
    forbid(bridge, '{"search", query', "query in argv")
    ok("Search and crawler remain separate processes and Search query is not argv")

    search_worker = read(root, "components/nougat/nougat_search_worker.py")
    for token in ["urllib.request", "urllib.error", "socket", "subprocess", "requests", "http.client"]:
        forbid(search_worker, token, "local Search worker network isolation")
    require(search_worker, "sys.stdin.read()", "private stdin query")
    ok("local Search worker retains no direct network client path")

    engine = read(root, "components/nougat/nougat_engine.py")
    forbid(engine.lower(), "duckduckgo", "legacy live-discovery fallback")
    require(engine, 'ThreadingHTTPServer(("127.0.0.1", port)', "loopback admin bind")
    require(engine, "Secure remote search unavailable. Search not accepted.", "plaintext peer Search refusal")
    require(engine, "HTTPS peer addresses only", "plain HTTP peer prohibition")
    ok("silent live-discovery and plaintext peer Search paths remain disabled")

    crawler = read(root, "components/nougat/nougat_crawler_worker.py")
    require(crawler, "NougatSearchCrawler/", "truthful crawler identity")
    for state in [
        "ROBOTS RESTRICTED", "BOT POLICY BLOCKED", "RATE LIMITED", "AUTH REQUIRED",
        "FEED AVAILABLE", "PAYMENT REQUIRED", "TEMPORARILY UNAVAILABLE",
    ]:
        require(crawler, state, "crawler access classification")
    require(crawler, "Nougat did not spend or retry automatically", "pay-per-crawl fail-safe")
    ok("Crawler Access states and no-auto-payment rule remain present")

    receipt = read(root, "src/privacy/privacy_receipt.hpp") + read(root, "src/privacy/privacy_receipt.cpp")
    forbid(receipt, "std::string query", "Privacy Receipt query field")
    require(receipt, "direct_fallback = false", "Privacy Receipt direct-fallback state")
    require(receipt, "query_logged = false", "Privacy Receipt query-log state")
    ok("Privacy Receipt v1 still has no query field")

    policy = read(root, "src/privacy/privacy_policy.cpp")
    require(policy, "no-direct-fallback", "Privacy Law")
    require(policy, "no-query-in-argv", "Privacy Law")
    require(policy, "no-query-in-url", "Privacy Law")
    ok("Privacy Law invariants remain explicit")

    broker = read(root, "components/privacy_broker/src/protocol.rs")
    require(broker, "PrivacyBrokerProtocol/1", "broker protocol")
    require(broker, "remote-query-transport-not-implemented", "broker fail-closed status")
    ok("Privacy Broker protocol remains versioned and remote query transport remains fail-closed")

    privacy_canary_test(root)
    compile_secure_search_cpp(root)

    # Native executable identity/runtime belongs to tools/build_v46.py. This
    # validator intentionally tests the retained privacy contract only, so it
    # does not confuse v45 release identity with v46 release identity.
    print("=== v0.0.46 SECURE SEARCH RETENTION VALIDATION PASS ===")

if __name__ == "__main__":
    main()
