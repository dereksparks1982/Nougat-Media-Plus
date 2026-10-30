#!/usr/bin/env python3
"""Static browser wiring checks. Does not use GitHub Actions or live servers."""
from html.parser import HTMLParser
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
def read(path):
    return (ROOT / path).read_text(encoding='utf-8')
class Markup(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids=set()
        self.scripts=[]
        self.styles=[]
        self.tabs=[]
    def handle_starttag(self, tag, attrs):
        a=dict(attrs)
        if 'id' in a:self.ids.add(a['id'])
        if tag=='script' and 'src' in a:self.scripts.append(a['src'])
        if tag=='link' and a.get('rel')=='stylesheet':self.styles.append(a.get('href'))
        if 'data-view' in a:self.tabs.append(a['data-view'])

class PlayerContracts(unittest.TestCase):
    def test_thirteen_tabs_have_shell_and_bridge(self):
        html=Markup();html.feed(read('index.html'))
        self.assertEqual(set(html.tabs),set(
            'home player library discover livetv worldtv radio search stream studio games network system'.split()))
        self.assertIn('./modules-bridge.js',html.scripts)
        self.assertIn('./web-capabilities.js',html.scripts)
        self.assertIn('./web-host-panel.js',html.scripts)
        self.assertIn('./web-api.js',html.scripts)
        self.assertIn('./web-host-panel.css',html.styles)
        self.assertIn('webHostOpen',html.ids)
        self.assertIn('webHostDialog',html.ids)
        self.assertIn('webHostTest',html.ids)
        self.assertIn('webHostSave',html.ids)
        self.assertIn('webHostDefault',html.ids)
    def test_cross_page_host_configuration(self):
        root=read('index.html')
        watch=read('watch/index.html')
        self.assertIn('./web-host.js',root)
        self.assertIn('../web-host.js',watch)
        source=read('web-host.js')
        for token in ('nougat-web-host-v1','nougat-watch-server-v1',
                      'getBase','getDefault','setServer','reset','nougat:host-changed',
                      "parsed.protocol !== 'https:'",'parsed.password'):
            self.assertIn(token,source)
        player=read('app.js')
        self.assertIn('window.NougatWebHost.getBase()',player)
        self.assertIn('hostRevision',player)
        self.assertIn('window.addEventListener(\'nougat:host-changed\'',player)
        self.assertIn('window.NougatWebPlayer = Object.freeze',player)
        bridge=read('modules-bridge.js')
        self.assertIn('window.NougatWebPlayer?.playById',bridge)
        self.assertIn('window.NougatWebHost.getBase()',bridge)
        self.assertIn('catalogRevision',bridge)
        self.assertIn('nougat:host-changed',bridge)
        self.assertIn('sharedHost.setServer(base)',read('watch/watch.js'))
    def test_bounded_native_service_calls(self):
        helper=read('web-api.js')
        for token in ('AbortController','setTimeout','redirect:',
                      "'same-origin' : 'omit'",'clearTimeout(timer)',
                      'signal: controller.signal'):
            self.assertIn(token,helper)
        for name in ('modules-live-tv.js','modules-radio.js','modules-system.js',
                     'modules-network-satellite.js','modules-system-tuner.js',
                     'modules-world-tv-guide.js'):
            self.assertIn('window.NougatWebAPI.request(',read(name),name)
    def test_no_known_dead_buttons(self):
        base=read('modules.js')
        for token in ("bind('crawlDomain'", "bind('systemDiag'",
                      "'networkConnections'", "'networkDevices'",
                      "'networkSecurity'", "'networkDiagnostics'",
                      "'networkLogs'", "networkSections.forEach(([id,handler])=>bind(id,handler))",
                      "run.addEventListener('click',searchArchive)"):
            self.assertIn(token,base)
        self.assertIn('crawlerSameDomain=!crawlerSameDomain',base)
        self.assertIn('controller.abort()',base)
        self.assertIn('This tab could not start:',read('modules-bridge.js'))
        capability=read('web-capabilities.js')
        self.assertIn('Configure media server',capability)
        self.assertIn('Browser capabilities & host requirements',capability)
        self.assertIn('Native',capability.replace('native','Native'))
    def test_service_worker_cache_and_product_readme(self):
        sw=read('sw.js')
        for item in ('web-host.js','web-api.js','web-host-panel.js',
                     'web-host-panel.css','web-capabilities.js','web-capabilities.css'):
            self.assertIn(item,sw)
        readme=read('README.md')
        self.assertIn('https://dereksparks1982.github.io/Nougat-Media-Plus/watch/',readme)
        self.assertIn('https://dereksparks1982.github.io/Nougat-Media-Plus/',readme)
        self.assertLess(len(readme.splitlines()),110)
        self.assertNotIn('## Unreleased:',readme)

if __name__=='__main__':
    unittest.main(verbosity=2)
