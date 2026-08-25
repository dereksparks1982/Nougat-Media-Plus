#!/usr/bin/env python3
from __future__ import annotations

import argparse
import html
import re
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request
from html.parser import HTMLParser

from nougat_common import VERSION, hx, index_document

USER_AGENT = (
    f"NougatSearchCrawler/{VERSION} "
    "(+https://github.com/dereksparks1982/Nougat-Media-Suite; purpose=search-index)"
)


class PageParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.in_title = False
        self.skip_depth = 0
        self.title_parts: list[str] = []
        self.text_parts: list[str] = []
        self.links: list[str] = []
        self.feeds: list[str] = []

    def handle_starttag(self, tag, attrs):
        tag = tag.lower()
        data = dict(attrs)
        if tag == "title":
            self.in_title = True
        if tag in {"script", "style", "noscript", "svg"}:
            self.skip_depth += 1
        if tag == "a" and data.get("href"):
            self.links.append(str(data["href"]))
        if tag == "link":
            rel = str(data.get("rel", "")).lower()
            typ = str(data.get("type", "")).lower()
            href = data.get("href")
            if href and "alternate" in rel and typ in {"application/rss+xml", "application/atom+xml"}:
                self.feeds.append(str(href))

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


def normalize_url(url: str) -> str | None:
    try:
        u = urllib.parse.urlsplit(url)
        if u.scheme not in {"http", "https"} or not u.hostname:
            return None
        netloc = u.hostname.lower()
        if u.port:
            netloc += f":{u.port}"
        return urllib.parse.urlunsplit((u.scheme.lower(), netloc, u.path or "/", u.query, ""))
    except Exception:
        return None


def normalize_seed(seed: str) -> str:
    seed = seed.strip()
    if not seed:
        raise ValueError("Enter a domain or URL.")
    if "://" not in seed:
        if "." not in seed and seed.lower() != "localhost":
            raise ValueError("Crawler seed must be a domain or URL.")
        seed = "https://" + seed
    result = normalize_url(seed)
    if not result:
        raise ValueError("Invalid crawler seed URL.")
    return result


def is_onion(url: str) -> bool:
    try:
        return (urllib.parse.urlsplit(url).hostname or "").lower().endswith(".onion")
    except Exception:
        return False


def classify_status(status: int) -> str:
    if 200 <= status < 400:
        return "ALLOWED"
    if status == 401:
        return "AUTH REQUIRED"
    if status == 402:
        return "PAYMENT REQUIRED"
    if status == 403:
        return "BOT POLICY BLOCKED"
    if status == 429:
        return "RATE LIMITED"
    if status >= 500:
        return "TEMPORARILY UNAVAILABLE"
    return "TEMPORARILY UNAVAILABLE"


def emit_access(url: str, state: str) -> None:
    print("\t".join(["ACCESS", hx(url), hx(state)]), flush=True)


def fetch_clearnet(url: str, timeout: int = 15) -> tuple[bytes, str, int]:
    req = urllib.request.Request(url, headers={
        "User-Agent": USER_AGENT,
        "Accept": "text/html,application/xhtml+xml,application/rss+xml,application/atom+xml",
    })
    try:
        with urllib.request.urlopen(req, timeout=timeout) as response:
            status = int(getattr(response, "status", 200) or 200)
            return response.read(5_000_000), response.headers.get("Content-Type", ""), status
    except urllib.error.HTTPError as exc:
        return exc.read(256_000), exc.headers.get("Content-Type", "") if exc.headers else "", int(exc.code)


def fetch_tor(url: str, timeout: int = 25) -> tuple[bytes, str, int]:
    proc = subprocess.run([
        "curl", "-L", "--silent", "--show-error", "--max-time", str(timeout),
        "--socks5-hostname", "127.0.0.1:9050", "-A", USER_AGENT,
        "-w", "\nNOUGAT_HTTP_STATUS:%{http_code}\nNOUGAT_CONTENT_TYPE:%{content_type}\n", url,
    ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.decode("utf-8", "replace").strip() or "Tor fetch failed")
    raw = proc.stdout
    marker = raw.rfind(b"\nNOUGAT_HTTP_STATUS:")
    if marker < 0:
        return raw, "text/html", 200
    body = raw[:marker]
    trailer = raw[marker:].decode("utf-8", "replace")
    status_match = re.search(r"NOUGAT_HTTP_STATUS:(\d+)", trailer)
    type_match = re.search(r"NOUGAT_CONTENT_TYPE:([^\r\n]*)", trailer)
    status = int(status_match.group(1)) if status_match else 200
    ctype = type_match.group(1).strip() if type_match else "text/html"
    return body, ctype, status


def fetch_url(url: str) -> tuple[bytes, str, int]:
    return fetch_tor(url) if is_onion(url) else fetch_clearnet(url)


def robots_allowed(url: str) -> tuple[bool, str]:
    parsed = urllib.parse.urlsplit(url)
    robots_url = urllib.parse.urlunsplit((parsed.scheme, parsed.netloc, "/robots.txt", "", ""))
    try:
        data, _ctype, status = fetch_url(robots_url)
        if status == 404:
            return True, "ALLOWED"
        state = classify_status(status)
        if state != "ALLOWED":
            return False, state
        text = data.decode("utf-8", "replace")
        groups: list[tuple[list[str], list[str]]] = []
        agents: list[str] = []
        disallows: list[str] = []
        for raw in text.splitlines():
            line = raw.split("#", 1)[0].strip()
            if not line or ":" not in line:
                continue
            key, value = [part.strip() for part in line.split(":", 1)]
            key = key.lower()
            if key == "user-agent":
                if agents and disallows:
                    groups.append((agents, disallows))
                    agents, disallows = [], []
                agents.append(value.lower())
            elif key == "disallow" and agents:
                disallows.append(value)
        if agents:
            groups.append((agents, disallows))
        ua = "nougatsearchcrawler"
        path = parsed.path or "/"
        for group_agents, group_disallows in groups:
            if "*" not in group_agents and not any(agent in ua for agent in group_agents):
                continue
            for prefix in group_disallows:
                if prefix and path.startswith(prefix):
                    return False, "ROBOTS RESTRICTED"
        return True, "ALLOWED"
    except Exception:
        # A robots transport failure is treated as unavailable rather than as
        # permission to crawl. Secure/courteous behavior fails closed.
        return False, "TEMPORARILY UNAVAILABLE"


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
        allowed, robots_state = robots_allowed(url)
        emit_access(url, robots_state)
        if not allowed:
            failures += 1
            continue
        try:
            data, ctype, status = fetch_url(url)
            state = classify_status(status)
            emit_access(url, state)
            if state != "ALLOWED":
                failures += 1
                if state == "PAYMENT REQUIRED":
                    print("LOG\t" + hx("Payment required; Nougat did not spend or retry automatically."), flush=True)
                continue
            if "html" not in ctype.lower() and not data.lstrip().startswith(b"<"):
                print("LOG\t" + hx(f"Skipped non-HTML response: {url}"), flush=True)
                continue
            parser = PageParser()
            parser.feed(data.decode("utf-8", "replace"))
            if parser.feeds:
                emit_access(url, "FEED AVAILABLE")
            title = html.unescape(parser.title or url)
            body = parser.body
            if body.strip():
                index_document(url, title, body, "TOR" if is_onion(url) else "CLEARNET", "NOUGAT-SEARCH-CRAWLER")
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
            emit_access(url, "TEMPORARILY UNAVAILABLE")
            print("LOG\t" + hx(f"Could not crawl {url}: {exc}"), flush=True)
    return {"indexed": indexed, "seen": len(seen), "failures": failures}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="nougat_crawler_worker")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("version")
    p_crawl = sub.add_parser("crawl")
    p_crawl.add_argument("--max-pages", type=int, default=25)
    p_crawl.add_argument("--follow-external", action="store_true")
    args = parser.parse_args(argv)

    if args.cmd == "version":
        print(f"Nougat crawler worker v{VERSION}")
        return 0

    seed = sys.stdin.read().rstrip("\r\n")
    try:
        result = crawl(seed, max(1, min(args.max_pages, 10000)), not args.follow_external)
        print(f"DONE\t{result['indexed']}\t{result['seen']}\t{result['failures']}", flush=True)
        return 0
    except Exception as exc:
        print("FAIL\t" + hx(str(exc)), flush=True)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
