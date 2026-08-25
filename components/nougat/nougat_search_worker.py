#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import sys
import time

from nougat_common import VERSION, hx, local_search


def read_private_query() -> str:
    # Search text arrives only through the anonymous stdin pipe created by the
    # C++ bridge. Never echo it, log it, place it in argv, or serialize it.
    return sys.stdin.read().rstrip("\r\n")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="nougat_search_worker")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("version")
    p_search = sub.add_parser("search")
    p_search.add_argument("--raw", action="store_true")
    p_search.add_argument("--limit", type=int, default=100)
    p_search.add_argument("--offset", type=int, default=0)
    args = parser.parse_args(argv)

    if args.cmd == "version":
        print(f"Nougat local search worker v{VERSION}")
        return 0

    query = read_private_query()
    hold_ms = int(os.environ.get("NOUGAT_TEST_HOLD_MS", "0") or "0")
    if hold_ms > 0:
        time.sleep(min(hold_ms, 5000) / 1000.0)
    if not query.strip():
        print("META\t0")
        return 0

    total, rows = local_search(
        query,
        max(1, min(args.limit, 500)),
        max(0, args.offset),
        args.raw,
    )
    print(f"META\t{total}")
    for result in rows:
        print("\t".join([
            "RESULT", hx(result.url), hx(result.title), hx(result.snippet), hx(result.domain),
            hx(result.source_network), hx(result.source_node), str(result.crawled_at),
            result.content_hash, repr(float(result.score)),
        ]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
