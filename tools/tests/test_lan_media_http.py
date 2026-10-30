#!/usr/bin/env python3
"""Opt-in integration checks for a running Nougat LAN server.

This file does not start services, download media, or contact the Internet.
It deliberately SKIPS unless NOUGAT_LAN_TEST_BASE is explicitly supplied.
An optional NOUGAT_LAN_TEST_MEDIA_ID enables direct-media byte-range tests.
Use with a throwaway test library and a trusted local host, not production data.
"""
from __future__ import annotations

import json
import os
import unittest
import urllib.error
import urllib.parse
import urllib.request

BASE = os.environ.get("NOUGAT_LAN_TEST_BASE", "").rstrip("/")
MEDIA_ID = os.environ.get("NOUGAT_LAN_TEST_MEDIA_ID", "")
# Never allow proxy environment variables to tunnel local HTTP requests.
CLIENT = urllib.request.build_opener(urllib.request.ProxyHandler({}))


def request(path: str, *, method: str = "GET", headers: dict | None = None):
    req = urllib.request.Request(BASE + path, method=method, headers=headers or {})
    try:
        with CLIENT.open(req, timeout=12) as response:
            return response.status, response.headers, response.read()
    except urllib.error.HTTPError as exc:
        return exc.code, exc.headers, exc.read()


@unittest.skipUnless(BASE, "Explicitly opt in with NOUGAT_LAN_TEST_BASE")
class LanHttpContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        parsed = urllib.parse.urlsplit(BASE)
        if parsed.scheme != "http" or not parsed.hostname:
            raise AssertionError("LAN smoke tests require an explicit HTTP base URL")
        # Avoid accidental WAN requests by keeping the default suite local-only.
        import ipaddress
        try:
            addr = ipaddress.ip_address(parsed.hostname)
        except ValueError as exc:
            raise AssertionError("Use a numeric loopback/private-LAN address") from exc
        if not (addr.is_private or addr.is_loopback or addr.is_link_local):
            raise AssertionError("WAN targets are forbidden")

    def test_health_has_lan_only_flag(self):
        code, _, raw = request("/nougat/v1/health")
        self.assertEqual(code, 200)
        payload = json.loads(raw)
        self.assertTrue(payload["ok"])
        self.assertTrue(payload["lanOnly"])

    def test_every_required_shell_asset(self):
        for path, content_type in {
            "/": "text/html",
            "/index.html": "text/html",
            "/app.js": "text/javascript",
            "/config.js": "text/javascript",
            "/legal-notice.js": "text/javascript",
            "/legal-notice.css": "text/css",
            "/styles.css": "text/css",
            "/LEGAL_NOTICE.md": "text/markdown",
            "/sw.js": "text/javascript",
            "/manifest.webmanifest": "application/manifest+json",
            "/nougat-icon.png": "image/png",
        }.items():
            with self.subTest(path=path):
                code, headers, raw = request(path)
                self.assertEqual(code, 200)
                self.assertIn(content_type, headers.get("Content-Type", ""))
                self.assertTrue(raw)

    def test_head_is_bodyless_with_matching_length(self):
        for path in ("/index.html", "/config.js", "/legal-notice.js"):
            with self.subTest(path=path):
                code, headers, body = request(path, method="HEAD")
                self.assertEqual(code, 200)
                self.assertEqual(body, b"")
                self.assertGreater(int(headers["Content-Length"]), 0)

    def test_unknown_and_traversal_routes(self):
        for path in ("/no-such-asset", "/%2e%2e/%2e%2e/etc/passwd"):
            code, _, _ = request(path)
            self.assertEqual(code, 404)

    def test_local_config_uses_same_origin(self):
        code, _, body = request("/config.js")
        self.assertEqual(code, 200)
        self.assertIn(b"baseUrl:''", body)
        self.assertNotIn(b"sslip.io", body)

    def test_catalog_status_is_truthful(self):
        code, _, raw = request("/nougat/v1/catalog")
        if code == 503:
            self.assertFalse(json.loads(raw)["ok"])
            return
        self.assertEqual(code, 200)
        payload = json.loads(raw)
        self.assertEqual(payload["count"], len(payload["items"]))
        self.assertIn("available", payload)
        self.assertTrue(all("ready" in item for item in payload["items"]))

    @unittest.skipUnless(MEDIA_ID, "Supply only a known, non-sensitive test-media ID")
    def test_direct_media_byte_ranges(self):
        path = "/nougat/v1/media?id=" + urllib.parse.quote(MEDIA_ID, safe="")
        code, hdr, raw = request(path, headers={"Range": "bytes=0-0"})
        self.assertEqual(code, 206)
        self.assertEqual(len(raw), 1)
        self.assertIn("bytes 0-0/", hdr.get("Content-Range", ""))
        code, hdr, raw = request(path, method="HEAD", headers={"Range": "bytes=0-0"})
        self.assertEqual(code, 206)
        self.assertEqual(raw, b"")
        self.assertEqual(hdr.get("Content-Length"), "1")
        for invalid in ("bytes=5junk-", "bytes=-0", "bytes=5-4", "bytes=0-1,3-4"):
            with self.subTest(range=invalid):
                code, _, _ = request(path, headers={"Range": invalid})
                self.assertEqual(code, 416)


if __name__ == "__main__":
    unittest.main(verbosity=2)
