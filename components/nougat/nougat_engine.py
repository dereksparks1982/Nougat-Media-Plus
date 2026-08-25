#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import secrets
import sys
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from nougat_common import VERSION, app_home, connect_db, hx

DEFAULT_PORT = 48731


def config_path() -> Path:
    return app_home() / "config.json"


def load_config() -> dict:
    p = config_path()
    if p.exists():
        try:
            data = json.loads(p.read_text(encoding="utf-8"))
            if isinstance(data, dict):
                return data
        except Exception:
            pass
    return {"peers": [], "port": DEFAULT_PORT, "max_pages": 25}


def save_config(cfg: dict) -> None:
    config_path().write_text(json.dumps(cfg, indent=2, sort_keys=True), encoding="utf-8")
    os.chmod(config_path(), 0o600)


def node_id() -> str:
    # Node identity belongs to the administrative peer surface only. It is
    # never emitted by the local search worker and is never attached to a query.
    cfg = load_config()
    if cfg.get("node_id"):
        return str(cfg["node_id"])
    raw = f"{secrets.token_hex(32)}-{time.time_ns()}".encode("utf-8")
    nid = hashlib.sha256(raw).hexdigest()[:16]
    cfg["node_id"] = nid
    save_config(cfg)
    return nid


def normalize_peer(value: str) -> str:
    value = value.strip().rstrip("/")
    if not value:
        raise ValueError("Peer address is empty.")
    if not value.startswith("https://"):
        raise ValueError("Nougat v0.0.45 accepts HTTPS peer addresses only. Plain HTTP is prohibited.")
    parsed = urllib.parse.urlsplit(value)
    if not parsed.hostname:
        raise ValueError("Peer address is invalid.")
    return value


class NougatAdminHandler(BaseHTTPRequestHandler):
    server_version = "NougatAdmin/0.0.45"

    def log_message(self, fmt, *args):
        return

    def _json(self, obj, status=200):
        payload = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        parsed = urllib.parse.urlsplit(self.path)
        if parsed.path == "/nougat/v1/status":
            con = connect_db()
            count = int(con.execute("SELECT count(*) FROM pages").fetchone()[0])
            con.close()
            self._json({
                "name": "Nougat",
                "version": VERSION,
                "node_id": node_id(),
                "documents": count,
                "search_transport": "local-only-fail-closed",
            })
            return
        if parsed.path == "/nougat/v1/search":
            # Never accept plaintext search terms through a URL. The old peer
            # query endpoint is intentionally disabled until Privacy Broker
            # transport can provide unlinkability and query privacy.
            self._json({"error": "Secure remote search unavailable. Search not accepted."}, 426)
            return
        self._json({"error": "not found"}, 404)


def serve_admin(port: int) -> None:
    # Loopback only. v0.0.45 does not expose a plaintext remote peer-search server.
    ThreadingHTTPServer(("127.0.0.1", port), NougatAdminHandler).serve_forever()


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="nougat_engine")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("version")
    sub.add_parser("node-id")
    sub.add_parser("list-peers")
    p_add = sub.add_parser("add-peer")
    p_add.add_argument("peer")
    p_remove = sub.add_parser("remove-peer")
    p_remove.add_argument("peer")
    p_serve = sub.add_parser("serve")
    p_serve.add_argument("--port", type=int, default=DEFAULT_PORT)
    p_search = sub.add_parser("search")
    p_search.add_argument("legacy_query", nargs="?")
    p_crawl = sub.add_parser("crawl")
    p_crawl.add_argument("legacy_seed", nargs="?")
    args = parser.parse_args(argv)

    if args.cmd == "version":
        print(f"Nougat engine v{VERSION}")
        return 0
    if args.cmd == "node-id":
        print(node_id())
        return 0
    if args.cmd == "search":
        print("Legacy search command disabled. Use nougat_search_worker.py with query on stdin.", file=sys.stderr)
        return 3
    if args.cmd == "crawl":
        print("Legacy crawler command disabled. Use nougat_crawler_worker.py with seed on stdin.", file=sys.stderr)
        return 3
    if args.cmd == "list-peers":
        for peer in load_config().get("peers", []):
            print("PEER\t" + hx(str(peer)))
        return 0
    if args.cmd == "add-peer":
        peer = normalize_peer(args.peer)
        cfg = load_config()
        peers = [str(value) for value in cfg.get("peers", [])]
        if peer not in peers:
            peers.append(peer)
        cfg["peers"] = peers
        save_config(cfg)
        print(peer)
        return 0
    if args.cmd == "remove-peer":
        peer = normalize_peer(args.peer)
        cfg = load_config()
        cfg["peers"] = [str(value) for value in cfg.get("peers", []) if str(value).rstrip("/") != peer]
        save_config(cfg)
        return 0
    if args.cmd == "serve":
        cfg = load_config()
        cfg["port"] = int(args.port)
        save_config(cfg)
        print(f"Nougat local admin node {node_id()} listening on loopback port {args.port}", flush=True)
        serve_admin(args.port)
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
