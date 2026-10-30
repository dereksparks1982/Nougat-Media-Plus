#!/usr/bin/env python3
"""Source-only contract audit for Nougat's LAN web player.

Uses only Python's standard library. Checks source wiring, not streaming,
native runtime behavior, network access, pairing, or actual acceptance.
Run manually from the internal checkout when validation is authorized.
"""
from __future__ import annotations

import re
import unittest
from html.parser import HTMLParser
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WEB = ROOT / "components" / "web_player"
SERVER = (ROOT / "src" / "lan" / "lan_media_service.cpp").read_text(encoding="utf-8")


class Assets(HTMLParser):
    def __init__(self):
        super().__init__()
        self.local_urls: set[str] = set()
        self.ids: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]):
        props = dict(attrs)
        if props.get("id"):
            self.ids.append(props["id"] or "")
        if tag == "script" and props.get("src"):
            self.local_urls.add(props["src"] or "")
        if tag == "link" and props.get("href"):
            self.local_urls.add(props["href"] or "")


class LocalWebContract(unittest.TestCase):
    def test_required_static_files_present_and_explicitly_served(self):
        parser = Assets()
        parser.feed((WEB / "index.html").read_text(encoding="utf-8"))
        for url in sorted(parser.local_urls):
            if not url.startswith("./"):
                continue
            name = url[2:]
            self.assertNotIn("/", name, f"No arbitrary subpath should be used: {url}")
            self.assertIn(f'path == "/{name}"', SERVER, f"Local HTTP service must serve {url}")
            if name == "nougat-icon.png":
                self.assertTrue((ROOT / "assets/branding/nougat-media-plus-master-N.png").is_file())
                self.assertIn('asset == "nougat-icon.png"', SERVER)
            else:
                self.assertTrue((WEB / name).is_file(), f"Required local player asset missing: {name}")

    def test_legal_controls_exist_in_both_web_views(self):
        required = {
            "nougatLegalGate", "nougatLegalConsent", "nougatLegalAgree",
            "nougatLegalReject", "nougatLegalRejectWarning",
        }
        for page in (ROOT / "index.html", WEB / "index.html"):
            parser = Assets()
            parser.feed(page.read_text(encoding="utf-8"))
            for widget in required:
                self.assertEqual(parser.ids.count(widget), 1, f"{page}: required ID {widget}")
            self.assertIn('src="./legal-notice.js"', page.read_text(encoding="utf-8"))

    def test_notice_version_matches_native_and_browser(self):
        native = (ROOT / "src/lan/../legal_notice.cpp").resolve().read_text(encoding="utf-8")
        scripts = [(ROOT / "legal-notice.js").read_text(encoding="utf-8"),
                   (WEB / "legal-notice.js").read_text(encoding="utf-8")]
        expected = "2026-10-30-v2"
        self.assertIn(expected, native)
        for js in scripts:
            self.assertIn(expected, js)
            self.assertIn("window.NougatLegalAccepted = true", js)
            self.assertIn("nougat:legal-accepted", js)
            self.assertIn("refusedOnce", js)

    def test_catalog_loads_only_after_accepted_notice(self):
        for js_path in (ROOT / "app.js", WEB / "app.js"):
            js = js_path.read_text(encoding="utf-8")
            self.assertIn("window.NougatLegalAccepted === true", js)
            self.assertIn("nougat:legal-accepted", js)
            self.assertIn("AbortController", js)
            self.assertIn("catalogRequest", js)
            self.assertIn("online", js)

    def test_static_routes_do_not_expose_disk_paths(self):
        self.assertIn("Explicit allowlist", SERVER)
        self.assertIn('asset = "legal-notice.js"', SERVER)
        self.assertIn('asset = "LEGAL_NOTICE.md"', SERVER)
        self.assertNotIn('asset = percent_decode(path)', SERVER)

    def test_byte_range_and_transcoder_guards_present(self):
        self.assertIn("std::all_of(text.begin()", SERVER)
        self.assertIn("fstat(file, &info)", SERVER)
        self.assertIn("first_amount <= 0", SERVER)
        self.assertIn('"Range Not Satisfiable"', SERVER)

    def test_service_worker_precache_contains_required_notice(self):
        for file in (ROOT / "sw.js", WEB / "sw.js"):
            content = file.read_text(encoding="utf-8")
            for item in ("./legal-notice.css", "./legal-notice.js", "./LEGAL_NOTICE.md"):
                self.assertIn(item, content)
            self.assertIn("-lan-20261030", content)

    def test_lan_catalog_contract_is_id_based(self):
        source = (ROOT / "src" / "lan" / "lan_viewer_service.cpp").read_text(encoding="utf-8")
        header = (ROOT / "src" / "lan" / "lan_viewer_service.hpp").read_text(encoding="utf-8")
        self.assertIn('text.find("\\\"items\\\"")', source)
        self.assertIn('json_string_field(object, "id")', source)
        self.assertIn("/nougat/v1/media?id=", source)
        self.assertIn("std::string id;", header)
        self.assertNotIn("/nougat/v1/media?path=", source)


if __name__ == "__main__":
    unittest.main(verbosity=2)
