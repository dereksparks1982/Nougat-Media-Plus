#!/usr/bin/env python3
"""Offline contract audit for the Nougat v0.0.69 web reliability phase."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT/path).read_text(encoding="utf-8")

class WebReliability(unittest.TestCase):
    def test_shared_resume(self):
        helper=read("web-resume.js")
        for name in ("nougat-web-resume-v3:", "nougat-web-resume-v2",
                     "nougat-watch-position-v1:", "NougatWebResume",
                     "LIMIT = 200", "nougat:resume-changed", "getDefault()"):
            self.assertIn(name,helper)
        self.assertIn("./web-resume.js",read("index.html"))
        self.assertIn("../web-resume.js",read("watch/index.html"))
        self.assertIn("NougatWebResume.read",read("app.js"))
        self.assertIn("NougatWebResume.set",read("watch/watch.js"))
        self.assertIn('data-filter="continue"',read("watch/index.html"))
        self.assertIn("mode==='continue'",read("watch/watch.js"))
    def test_single_catalog_source(self):
        for path in ("player-enhancements.js", "player-media-session.js",
                     "modules-media-card-actions.js"):
            src=read(path)
            self.assertIn("NougatWebPlayer?.getCatalog",src)
            self.assertNotIn("fetch(",src)
            self.assertIn("nougat:catalog-updated",src)
        self.assertIn("nougatMediaId",read("app.js"))
        self.assertIn("nougat:catalog-updated",read("app.js"))
    def test_health_monitor(self):
        source=read("connection-monitor.js")
        self.assertIn("nougat:health-state",source)
        self.assertIn("COOLDOWN=15000",source)
        self.assertNotIn("fetch(",source)
        self.assertNotIn("cfg.baseUrl",source)
        self.assertIn("nougat:health-state",read("app.js"))
    def test_catalog_privacy_and_server_partition(self):
        source=read("modules-catalog-cache.js")
        for needle in ("nougat-web-catalog-cache-v2:",
                       "encodeURIComponent(url.origin + url.pathname)",
                       "window.NougatLegalAccepted !== true",
                       "init.signal?.aborted", "X-Nougat-Catalog-Cache",
                       "__nougatOfflineCatalog", "MAX_AGE=24*60*60*1000"):
            self.assertIn(needle,source)
        self.assertNotIn("refreshInBackground",source)
        self.assertIn("OFFLINE LIBRARY CACHE",read("app.js"))
    def test_sw_static_only(self):
        source=read("sw.js")
        for needle in ("ALLOWED","CORE","Promise.allSettled",
                       "url.pathname.includes('/nougat/v1/')", "url.search",
                       "headers.get('range')", "request.cache ===",
                       "cache.match(key)", "web-resume.js",
                       "watch/index.html", "optional asset"):
            self.assertIn(needle,source)
        self.assertNotIn("cache.addAll(SHELL)",source)
    def test_host_modules(self):
        for path in ("modules-radio-cellular.js", "modules-stream.js",
                     "modules-system-tmdb.js", "modules-media-card-actions.js"):
            self.assertIn("NougatWebAPI.request(",read(path))
        self.assertIn("matchesKey()",read("modules-media-card-actions.js"))
    def test_fixture_presence_and_readme(self):
        for name in ("test_pages_catalog_cache.js","test_pages_service_worker.js",
                     "test_web_resume_sync.js", "test_pages_web_host.js"):
            self.assertTrue((ROOT/"tools/tests"/name).is_file(),name)
        self.assertIn("v0.0.69",read("README.md"))
        self.assertLess(len(read("README.md").splitlines()),115)

if __name__ == "__main__":
    unittest.main(verbosity=2)
