#!/usr/bin/env python3
"""Behavior validation for the ReddMedia v0.0.19 integrated Nougat engine."""
from __future__ import annotations

import os
import pathlib
import socket
import subprocess
import sys
import tempfile
import time

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
ENGINE = ROOT / "components/nougat/nougat_engine.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def run(home: pathlib.Path, *args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["NOUGAT_HOME"] = str(home)
    return subprocess.run(
        [sys.executable, str(ENGINE), *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
        check=check,
        timeout=20,
    )


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return int(sock.getsockname()[1])


def wait_port(port: int, timeout: float = 5.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.05)
    raise SystemExit(f"Nougat peer did not start on port {port}")


def main() -> int:
    require(ENGINE.is_file(), "integrated Nougat engine is missing")
    subprocess.run([sys.executable, "-m", "py_compile", str(ENGINE)], check=True)

    with tempfile.TemporaryDirectory(prefix="reddmedia-nougat-v19.") as raw:
        root = pathlib.Path(raw)
        local = root / "local"
        peer = root / "peer"
        empty = root / "empty"

        require("v0.0.19" in run(local, "version").stdout, "wrong Nougat engine version")
        run(local, "add-test-docs")

        ranked = run(local, "search", "Nougat").stdout
        raw_search = run(local, "search", "Nougat", "--raw").stdout
        require("META\t" in ranked and "RESULT\t" in ranked, "ranked local search returned no result")
        require("META\t" in raw_search and "RESULT\t" in raw_search, "RAW local search returned no result")

        invalid = run(local, "crawl", "google", "--max-pages", "1", check=False)
        require(invalid.returncode == 2, "bare crawler word should be rejected")
        parts = invalid.stdout.strip().split("\t", 1)
        decoded_invalid = bytes.fromhex(parts[1]).decode("utf-8", "replace") if len(parts) == 2 and parts[0] == "FAIL" else invalid.stdout
        require("Crawler seed must be a domain or URL" in decoded_invalid, "crawler rejection is not clear")

        # Peer A owns the only document. Client B must see it only while A is online.
        run(peer, "add-test-docs")
        port = free_port()
        env = os.environ.copy()
        env["NOUGAT_HOME"] = str(peer)
        server = subprocess.Popen(
            [sys.executable, str(ENGINE), "serve", "--port", str(port)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            env=env,
        )
        try:
            wait_port(port)
            address = f"http://127.0.0.1:{port}"
            run(empty, "add-peer", address)
            online = run(empty, "search", "decentralized", "--raw", "--peers").stdout
            require("RESULT\t" in online, "remote peer result missing while peer is online")
            require("PEER\t" in online, "peer status missing while peer is online")
        finally:
            server.terminate()
            try:
                server.wait(timeout=3)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait(timeout=3)

        offline = run(empty, "search", "decentralized", "--raw", "--peers").stdout
        require("RESULT\t" not in offline, "remote-only result remained after peer went offline")
        require("PEER\t" in offline, "offline peer status missing")

        config = empty / "config.json"
        require(config.is_file(), "Nougat peer configuration was not persisted")
        require((empty / "nougat.db").is_file(), "Nougat SQLite index was not persisted")

    print(
        "nougat-engine=pass ranked=pass raw=pass crawler-validation=pass "
        "peer-online=pass peer-offline=pass persistence=pass"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
