#!/usr/bin/env python3
"""Static GitHub Pages navigation and Watch contracts; no browser or remote media.

Intended for source review. Actual phone layout, streaming, and Pages
deployment still require separate end-to-end verification.
"""
from html.parser import HTMLParser
from pathlib import Path
import json
import unittest

ROOT = Path(__file__).resolve().parents[2]

def source(path):
    return (ROOT / path).read_text(encoding="utf-8")

class Tags(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.scripts = []
        self.links = []
        self.views = set()
        self.buttons = set()
    def handle_starttag(self, tag, attrs):
        fields = dict(attrs)
        if fields.get("id"):
            self.ids.add(fields["id"])
        if fields.get("data-view"):
            self.views.add(fields["data-view"])
        if fields.get("src") and tag == "script":
            self.scripts.append(fields["src"])
        if fields.get("href") and tag == "a":
            self.links.append(fields["href"])
        if tag == "button" and fields.get("id"):
            self.buttons.add(fields["id"])

class PagesContracts(unittest.TestCase):
    def test_watch_page_and_controls(self):
        doc = Tags()
        doc.feed(source("watch/index.html"))
        required = {
            "nougatLegalGate", "nougatLegalConsent", "nougatLegalAgree",
            "nougatLegalReject", "nougatLegalRejectWarning",
            "serverForm", "serverUrl", "connectionStatus", "retry",
            "searchInput", "library", "libraryCount", "libraryNotice",
            "video", "compatibility", "back10", "forward10",
            "closePlayer", "playbackStatus", "watchPanel"
        }
        self.assertFalse(required - doc.ids, required - doc.ids)
        self.assertIn("../legal-notice.js", doc.scripts)
        self.assertIn("../config.js", doc.scripts)
        self.assertIn("./watch.js", doc.scripts)
        self.assertIn("../#library", doc.links)
    def test_phone_player_matches_host_api_and_legal_gate(self):
        js = source("watch/watch.js")
        for endpoint in ("/nougat/v1/health", "/nougat/v1/catalog",
                         "/nougat/v1/media", "/nougat/v1/transcode",
                         "/nougat/v1/poster", "/nougat/v1/artwork"):
            self.assertIn(endpoint, js)
        for rule in ("https:", "'same-origin' : 'omit'", "AbortController",
                     "nougat:legal-accepted", "NougatLegalAccepted",
                     "search.addEventListener", "compatButton.addEventListener",
                     "localStorage", "resumeKey()", "selected = null",
                     "data-filter", "startStream(true, offset)"):
            self.assertIn(rule, js)
        self.assertNotIn("innerHTML =", js)
        self.assertIn("prefers-reduced-motion", source("watch/watch.css"))
        manifest = json.loads(source("watch/manifest.webmanifest"))
        self.assertEqual(manifest["start_url"], "./")
        self.assertEqual(manifest["display"], "standalone")
    def test_home_links_and_versioned_precache(self):
        doc = Tags()
        doc.feed(source("index.html"))
        self.assertIn("./watch/", doc.links)
        for view in ("home", "player", "library", "discover", "livetv",
                     "worldtv", "radio", "search", "stream", "studio",
                     "games", "network", "system"):
            self.assertIn(view, doc.views)
        sw = source("sw.js")
        for asset in ("./watch/index.html", "./watch/watch.css",
                      "./watch/watch.js", "./watch/manifest.webmanifest"):
            self.assertIn(asset, sw)
    def test_rail_and_module_navigation(self):
        css = source("styles.css")
        for rule in ("overflow-y: auto", "flex: 0 0 42px", "flex-basis: 39px",
                     "isolation: isolate", ".nougat-home-watch-row",
                     "flex-wrap:wrap"):
            self.assertIn(rule, css)
        bridge = source("modules-bridge.js")
        self.assertIn("function activate(view)", bridge)
        self.assertIn("showModuleFailure(view, error)", bridge)
        self.assertIn("clearTimeout(timeout)", bridge)
        self.assertIn("controller.abort()", bridge)
        self.assertNotIn("await ensureCatalog(false);", bridge)
        self.assertIn("if (window.NougatLegalAccepted === true) initializeModules()", bridge)
        self.assertIn("nougat:legal-accepted", bridge)
        self.assertIn("watch/", bridge)
    def test_readme_is_product_overview(self):
        readme = source("README.md")
        self.assertIn("https://dereksparks1982.github.io/Nougat-Media-Plus/watch/", readme)
        self.assertIn("https://dereksparks1982.github.io/Nougat-Media-Plus/", readme)
        self.assertIn("## What Nougat includes", readme)
        self.assertIn("CHANGELOG.md", readme)
        self.assertNotIn("## Unreleased", readme)
        self.assertNotIn("source candidate):", readme)
        self.assertLess(len(readme.splitlines()), 100)

if __name__ == "__main__":
    unittest.main(verbosity=2)
