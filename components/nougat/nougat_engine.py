#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import html
import json
import os
import re
import socket
import sqlite3
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from dataclasses import dataclass
from html.parser import HTMLParser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

VERSION = "0.0.19"
DEFAULT_PORT = 48731
USER_AGENT = f"ReddMedia-Nougat/{VERSION} decentralized research crawler"


def app_home() -> Path:
    override = os.environ.get("NOUGAT_HOME")
    p = Path(override).expanduser() if override else Path.home() / ".local" / "share" / "reddmedia" / "nougat"
    p.mkdir(parents=True, exist_ok=True)
    return p


def db_path() -> Path:
    return app_home() / "nougat.db"


def config_path() -> Path:
    return app_home() / "config.json"


SCHEMA = """
PRAGMA journal_mode=WAL;
CREATE TABLE IF NOT EXISTS pages (
    id INTEGER PRIMARY KEY,
    url TEXT NOT NULL UNIQUE,
    title TEXT NOT NULL DEFAULT '',
    body TEXT NOT NULL DEFAULT '',
    domain TEXT NOT NULL DEFAULT '',
    source_network TEXT NOT NULL DEFAULT 'CLEARNET',
    source_node TEXT NOT NULL DEFAULT 'LOCAL',
    crawled_at INTEGER NOT NULL,
    content_hash TEXT NOT NULL DEFAULT ''
);
CREATE VIRTUAL TABLE IF NOT EXISTS pages_fts USING fts5(
    title, body, url UNINDEXED, domain UNINDEXED,
    source_network UNINDEXED, source_node UNINDEXED,
    crawled_at UNINDEXED, content_hash UNINDEXED,
    content='pages', content_rowid='id',
    tokenize='unicode61 remove_diacritics 2'
);
CREATE TRIGGER IF NOT EXISTS pages_ai AFTER INSERT ON pages BEGIN
  INSERT INTO pages_fts(rowid,title,body,url,domain,source_network,source_node,crawled_at,content_hash)
  VALUES (new.id,new.title,new.body,new.url,new.domain,new.source_network,new.source_node,new.crawled_at,new.content_hash);
END;
CREATE TRIGGER IF NOT EXISTS pages_ad AFTER DELETE ON pages BEGIN
  INSERT INTO pages_fts(pages_fts,rowid,title,body,url,domain,source_network,source_node,crawled_at,content_hash)
  VALUES('delete',old.id,old.title,old.body,old.url,old.domain,old.source_network,old.source_node,old.crawled_at,old.content_hash);
END;
CREATE TRIGGER IF NOT EXISTS pages_au AFTER UPDATE ON pages BEGIN
  INSERT INTO pages_fts(pages_fts,rowid,title,body,url,domain,source_network,source_node,crawled_at,content_hash)
  VALUES('delete',old.id,old.title,old.body,old.url,old.domain,old.source_network,old.source_node,old.crawled_at,old.content_hash);
  INSERT INTO pages_fts(rowid,title,body,url,domain,source_network,source_node,crawled_at,content_hash)
  VALUES (new.id,new.title,new.body,new.url,new.domain,new.source_network,new.source_node,new.crawled_at,new.content_hash);
END;
"""


def connect_db() -> sqlite3.Connection:
    con = sqlite3.connect(db_path(), timeout=30, check_same_thread=False)
    con.row_factory = sqlite3.Row
    con.executescript(SCHEMA)
    return con


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
    cfg = load_config()
    if cfg.get("node_id"):
        return str(cfg["node_id"])
    raw = f"{socket.gethostname()}-{time.time_ns()}-{os.getpid()}".encode()
    nid = hashlib.sha256(raw).hexdigest()[:16]
    cfg["node_id"] = nid
    save_config(cfg)
    return nid


def hx(value: str) -> str:
    return value.encode("utf-8", "replace").hex()


class PageParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.in_title = False
        self.skip_depth = 0
        self.title_parts: list[str] = []
        self.text_parts: list[str] = []
        self.links: list[str] = []

    def handle_starttag(self, tag, attrs):
        tag = tag.lower()
        if tag == "title":
            self.in_title = True
        if tag in {"script", "style", "noscript", "svg"}:
            self.skip_depth += 1
        if tag == "a":
            href = dict(attrs).get("href")
            if href:
                self.links.append(href)

    def handle_endtag(self, tag):
        tag = tag.lower()
        if tag == "title":
            self.in_title = False
        if tag in {"script", "style", "noscript", "svg"} and self.skip_depth:
            self.skip_depth -= 1

    def handle_data(self, data):
        if self.skip_depth:
            return
        text = re.sub(r"\s+", " ", data).strip()
        if not text:
            return
        if self.in_title:
            self.title_parts.append(text)
        self.text_parts.append(text)

    @property
    def title(self) -> str:
        return " ".join(self.title_parts)[:500]

    @property
    def body(self) -> str:
        return "\n".join(self.text_parts)


@dataclass
class SearchResult:
    url: str
    title: str
    snippet: str
    domain: str
    source_network: str
    source_node: str
    crawled_at: int
    content_hash: str
    score: float = 0.0


def normalize_seed(seed: str) -> str:
    seed = seed.strip()
    if not seed:
        raise ValueError("Enter a domain or URL.")
    if "://" not in seed:
        # A crawler seed needs a real host. A bare search word is not silently promoted to https://word/.
        if "." not in seed and seed.lower() != "localhost":
            raise ValueError("Crawler seed must be a domain or URL, for example example.com or https://example.com/")
        seed = "https://" + seed
    out = normalize_url(seed)
    if not out:
        raise ValueError("Invalid crawler seed URL.")
    return out


def normalize_url(url: str) -> str | None:
    try:
        u = urllib.parse.urlsplit(url)
        if u.scheme not in {"http", "https"}:
            return None
        host = u.hostname or ""
        if not host:
            return None
        netloc = host.lower()
        if u.port:
            netloc += f":{u.port}"
        path = u.path or "/"
        return urllib.parse.urlunsplit((u.scheme.lower(), netloc, path, u.query, ""))
    except Exception:
        return None


def is_onion(url: str) -> bool:
    try:
        return (urllib.parse.urlsplit(url).hostname or "").lower().endswith(".onion")
    except Exception:
        return False


def fetch_clearnet(url: str, timeout: int = 15) -> tuple[bytes, str]:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT, "Accept": "text/html,application/xhtml+xml"})
    with urllib.request.urlopen(req, timeout=timeout) as response:
        return response.read(5_000_000), response.headers.get("Content-Type", "")


def fetch_tor(url: str, timeout: int = 25) -> tuple[bytes, str]:
    proc = subprocess.run([
        "curl", "-L", "--silent", "--show-error", "--max-time", str(timeout),
        "--socks5-hostname", "127.0.0.1:9050", "-A", USER_AGENT,
        "-D", "-", url,
    ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.decode("utf-8", "replace").strip() or "Tor fetch failed")
    raw = proc.stdout
    sep = raw.rfind(b"\r\n\r\n")
    if sep < 0:
        return raw, "text/html"
    headers = raw[:sep].decode("latin1", "replace")
    body = raw[sep + 4:]
    ctype = "text/html"
    for line in headers.splitlines():
        if line.lower().startswith("content-type:"):
            ctype = line.split(":", 1)[1].strip()
    return body, ctype


def index_document(url: str, title: str, body: str, network: str, source_node: str | None = None) -> None:
    domain = urllib.parse.urlsplit(url).hostname or ""
    digest = hashlib.sha256(body.encode("utf-8", "replace")).hexdigest()
    now = int(time.time())
    con = connect_db()
    with con:
        con.execute("""
        INSERT INTO pages(url,title,body,domain,source_network,source_node,crawled_at,content_hash)
        VALUES(?,?,?,?,?,?,?,?)
        ON CONFLICT(url) DO UPDATE SET
          title=excluded.title, body=excluded.body, domain=excluded.domain,
          source_network=excluded.source_network, source_node=excluded.source_node,
          crawled_at=excluded.crawled_at, content_hash=excluded.content_hash
        """, (url, title, body, domain, network, source_node or node_id(), now, digest))
    con.close()


def crawl(seed: str, max_pages: int = 25, same_domain: bool = True) -> dict:
    seed = normalize_seed(seed)
    seed_host = urllib.parse.urlsplit(seed).hostname
    todo = [seed]
    seen: set[str] = set()
    indexed = 0
    failures = 0
    while todo and indexed < max_pages:
        url = todo.pop(0)
        if url in seen:
            continue
        seen.add(url)
        print("LOG\t" + hx(f"Crawling {url}"), flush=True)
        try:
            data, ctype = fetch_tor(url) if is_onion(url) else fetch_clearnet(url)
            if "html" not in ctype.lower() and not data.lstrip().startswith(b"<"):
                print("LOG\t" + hx(f"Skipped non-HTML response: {url}"), flush=True)
                continue
            parser = PageParser()
            parser.feed(data.decode("utf-8", "replace"))
            title = parser.title or url
            body = parser.body
            if body.strip():
                index_document(url, title, body, "TOR" if is_onion(url) else "CLEARNET")
                indexed += 1
                print("LOG\t" + hx(f"Indexed {indexed}/{max_pages}: {title[:120]}"), flush=True)
            for href in parser.links:
                nxt = normalize_url(urllib.parse.urljoin(url, href))
                if not nxt or nxt in seen:
                    continue
                if same_domain and urllib.parse.urlsplit(nxt).hostname != seed_host:
                    continue
                todo.append(nxt)
        except Exception as exc:
            failures += 1
            print("LOG\t" + hx(f"Could not crawl {url}: {exc}"), flush=True)
    return {"indexed": indexed, "seen": len(seen), "failures": failures}


def make_match_query(query: str) -> str:
    query = query.strip()
    if not query:
        return ""
    phrases = re.findall(r'"([^"]+)"|([^\s]+)', query)
    tokens: list[str] = []
    for phrase, token in phrases:
        value = phrase or token
        value = re.sub(r"[^\w\-]+", " ", value, flags=re.UNICODE).strip()
        if value:
            tokens.append('"' + value.replace('"', '""') + '"')
    return " AND ".join(tokens)


def local_search(query: str, limit: int = 100, offset: int = 0, raw: bool = False) -> tuple[int, list[SearchResult]]:
    match = make_match_query(query)
    if not match:
        return 0, []
    con = connect_db()
    total = int(con.execute("SELECT count(*) FROM pages_fts WHERE pages_fts MATCH ?", (match,)).fetchone()[0])
    order = "rowid ASC" if raw else "bm25(pages_fts) ASC, rowid ASC"
    rows = con.execute(f"""
        SELECT url,title,domain,source_network,source_node,crawled_at,content_hash,
               snippet(pages_fts,1,'','', ' … ', 24) AS snippet,
               bm25(pages_fts) AS score
        FROM pages_fts WHERE pages_fts MATCH ?
        ORDER BY {order} LIMIT ? OFFSET ?
    """, (match, limit, offset)).fetchall()
    con.close()
    return total, [SearchResult(
        url=row["url"], title=row["title"] or row["url"], snippet=html.unescape(row["snippet"] or ""),
        domain=row["domain"], source_network=row["source_network"], source_node=row["source_node"],
        crawled_at=int(row["crawled_at"]), content_hash=row["content_hash"], score=float(row["score"] or 0.0)
    ) for row in rows]


def peer_search(peer: str, query: str, limit: int, offset: int, raw: bool, timeout: int = 4) -> dict:
    params = urllib.parse.urlencode({"q": query, "limit": limit, "offset": offset, "raw": "1" if raw else "0"})
    req = urllib.request.Request(peer.rstrip("/") + "/nougat/v1/search?" + params, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=timeout) as response:
        return json.loads(response.read(2_000_000).decode("utf-8"))


def federated_search(query: str, limit: int, offset: int, raw: bool, include_peers: bool) -> tuple[int, list[SearchResult], list[tuple[str, str]]]:
    local_total, local_results = local_search(query, limit, offset, raw)
    total = local_total
    results = list(local_results)
    statuses: list[tuple[str, str]] = []
    if include_peers:
        for peer in load_config().get("peers", []):
            try:
                data = peer_search(str(peer), query, limit, offset, raw)
                total += int(data.get("total", 0))
                for item in data.get("results", []):
                    results.append(SearchResult(**item))
                statuses.append((str(peer), "OK"))
            except Exception as exc:
                statuses.append((str(peer), f"OFFLINE: {exc}"))
    dedup: dict[str, SearchResult] = {}
    for result in results:
        if result.url not in dedup:
            dedup[result.url] = result
    results = list(dedup.values())
    if not raw:
        results.sort(key=lambda item: (item.score, item.url))
    return total, results[:limit], statuses


class NougatHandler(BaseHTTPRequestHandler):
    server_version = "ReddMediaNougatPeer/0.0.19"

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
            self._json({"name": "Nougat", "version": VERSION, "node_id": node_id(), "documents": count})
            return
        if parsed.path == "/nougat/v1/search":
            query = urllib.parse.parse_qs(parsed.query)
            q = query.get("q", [""])[0]
            limit = max(1, min(500, int(query.get("limit", [100])[0])))
            offset = max(0, int(query.get("offset", [0])[0]))
            raw = query.get("raw", ["0"])[0] == "1"
            total, results = local_search(q, limit, offset, raw)
            self._json({"node_id": node_id(), "total": total, "results": [item.__dict__ for item in results]})
            return
        self._json({"error": "not found"}, 404)


def serve_peer(port: int):
    ThreadingHTTPServer(("0.0.0.0", port), NougatHandler).serve_forever()


def normalize_peer(value: str) -> str:
    value = value.strip().rstrip("/")
    if not value:
        raise ValueError("Peer address is empty.")
    if not value.startswith(("http://", "https://")):
        value = "http://" + value
    parsed = urllib.parse.urlsplit(value)
    if not parsed.hostname:
        raise ValueError("Peer address is invalid.")
    return value


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="nougat_engine")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("version")
    sub.add_parser("node-id")
    p_search = sub.add_parser("search")
    p_search.add_argument("query")
    p_search.add_argument("--raw", action="store_true")
    p_search.add_argument("--peers", action="store_true")
    p_search.add_argument("--limit", type=int, default=100)
    p_search.add_argument("--offset", type=int, default=0)
    p_crawl = sub.add_parser("crawl")
    p_crawl.add_argument("url")
    p_crawl.add_argument("--max-pages", type=int, default=25)
    p_crawl.add_argument("--follow-external", action="store_true")
    sub.add_parser("list-peers")
    p_add = sub.add_parser("add-peer")
    p_add.add_argument("peer")
    p_remove = sub.add_parser("remove-peer")
    p_remove.add_argument("peer")
    p_serve = sub.add_parser("serve")
    p_serve.add_argument("--port", type=int, default=DEFAULT_PORT)
    sub.add_parser("add-test-docs")
    args = parser.parse_args(argv)

    if args.cmd == "version":
        print(f"Nougat engine v{VERSION}")
        return 0
    if args.cmd == "node-id":
        print(node_id())
        return 0
    if args.cmd == "search":
        total, rows, statuses = federated_search(args.query, max(1, min(args.limit, 500)), max(0, args.offset), args.raw, args.peers)
        print(f"META\t{total}")
        for result in rows:
            print("\t".join([
                "RESULT", hx(result.url), hx(result.title), hx(result.snippet), hx(result.domain),
                hx(result.source_network), hx(result.source_node), str(result.crawled_at), result.content_hash,
                repr(float(result.score)),
            ]))
        for peer, status in statuses:
            print("\t".join(["PEER", hx(peer), hx(status)]))
        return 0
    if args.cmd == "crawl":
        try:
            result = crawl(args.url, max(1, min(args.max_pages, 10000)), not args.follow_external)
            print(f"DONE\t{result['indexed']}\t{result['seen']}\t{result['failures']}", flush=True)
            return 0
        except Exception as exc:
            print("FAIL\t" + hx(str(exc)), flush=True)
            return 2
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
        print(f"Nougat node {node_id()} listening on port {args.port}", flush=True)
        serve_peer(args.port)
        return 0
    if args.cmd == "add-test-docs":
        docs = [
            ("https://nougat.test/candy", "Nougat Candy", "Nougat is a chewy candy filling and the best part of a candy bar."),
            ("https://nougat.test/search", "Decentralized Search", "Nougat decentralized search returns indexed knowledge without a SafeSearch layer."),
            ("https://nougat.test/cat", "Nugget the Cat", "Nugget is also called Nougat. This test document exists for search verification."),
        ]
        for url, title, body in docs:
            index_document(url, title, body, "CLEARNET", "TESTNODE")
        print(f"Added {len(docs)} test documents")
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
