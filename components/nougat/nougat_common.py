#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import html
import os
import re
import sqlite3
import time
import urllib.parse
from dataclasses import dataclass
from pathlib import Path

VERSION = "0.0.45"


def app_home() -> Path:
    override = os.environ.get("NOUGAT_HOME")
    p = Path(override).expanduser() if override else Path.home() / ".local" / "share" / "reddmedia" / "nougat"
    p.mkdir(parents=True, exist_ok=True)
    return p


def db_path() -> Path:
    return app_home() / "nougat.db"


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
    con.execute("CREATE TABLE IF NOT EXISTS nougat_meta (key TEXT PRIMARY KEY, value TEXT NOT NULL DEFAULT '')")
    row = con.execute("SELECT value FROM nougat_meta WHERE key='fts_schema_revision'").fetchone()
    if row is None or str(row[0]) != "2":
        with con:
            con.execute("INSERT INTO pages_fts(pages_fts) VALUES('rebuild')")
            con.execute(
                "INSERT INTO nougat_meta(key,value) VALUES('fts_schema_revision','2') "
                "ON CONFLICT(key) DO UPDATE SET value=excluded.value"
            )
    return con


def hx(value: str) -> str:
    return value.encode("utf-8", "replace").hex()


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
    return " OR ".join(tokens)


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
    """, (match, max(1, min(limit, 500)), max(0, offset))).fetchall()
    con.close()
    return total, [SearchResult(
        url=row["url"], title=row["title"] or row["url"], snippet=html.unescape(row["snippet"] or ""),
        domain=row["domain"], source_network=row["source_network"], source_node=row["source_node"],
        crawled_at=int(row["crawled_at"]), content_hash=row["content_hash"], score=float(row["score"] or 0.0)
    ) for row in rows]


def index_document(url: str, title: str, body: str, network: str, source_node: str = "LOCAL-CRAWLER") -> None:
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
        """, (url, title, body, domain, network, source_node, now, digest))
    con.close()
