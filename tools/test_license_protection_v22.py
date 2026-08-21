#!/usr/bin/env python3
from __future__ import annotations
import pathlib, sys

ROOT=pathlib.Path(sys.argv[1] if len(sys.argv)>1 else '.').resolve()

def need(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

def text(rel: str) -> str:
    p=ROOT/rel
    need(p.is_file(), f'missing required licensing file: {rel}')
    return p.read_text(encoding='utf-8')

license_text=text('LICENSE')
copyright_text=text('COPYRIGHT.md')
contrib=text('CONTRIBUTING.md')
third=text('THIRD_PARTY_NOTICES.md')
policy=text('docs/LICENSING_POLICY.md')
pr=text('.github/PULL_REQUEST_TEMPLATE.md')
bible=text('COMPANY_BIBLE.md')
readme=text('README.md')

for body,name in ((license_text,'LICENSE'),(copyright_text,'COPYRIGHT.md'),(contrib,'CONTRIBUTING.md'),(policy,'LICENSING_POLICY.md')):
    need('Elderred Softworks LLC' in body, f'{name}: exact rights-holder identity missing')
    need('Elderedd Softworks' not in body, f'{name}: old misspelled rights-holder identity remains')

need('PolyForm Noncommercial License 1.0.0' in license_text, 'controlling PolyForm license missing')
need('https://polyformproject.org/licenses/noncommercial/1.0.0/' in license_text, 'controlling PolyForm URL missing')
need('Commercial use of the Original Materials requires separate written permission' in license_text, 'commercial-use boundary missing')
need("does not restrict the copyright holder's use of its own work" in license_text, 'owner commercial-rights reservation missing')
need('does **not** replace, narrow, expand, or relicense third-party' in license_text, 'third-party exclusion missing from root license')

for marker in (
    'perpetual, worldwide, non-exclusive, irrevocable, royalty-free license',
    'sublicense, and relicense',
    'including commercial licensing and distribution',
    'patent license',
):
    need(marker in contrib, f'contributor inbound-right marker missing: {marker}')

for marker in ('Jellyfin','FFmpeg','VLC / libVLC','libtorrent-rasterbar','yt-dlp','llama.cpp','Nomic Embed Text v1.5','TMDb'):
    need(marker in third, f'third-party inventory missing: {marker}')
need('does not relicense any component listed below' in third, 'third-party non-relicensing boundary missing')
need('some packaged standalone binaries can contain third-party code under additional licenses' in third, 'yt-dlp packaged-artifact nuance missing')

for marker in ('license boundary','No accidental relicensing','Owner commercial rights','Outside contributions','Release gate'):
    need(marker.lower() in policy.lower(), f'licensing policy section missing: {marker}')

need('I have the right to submit every contribution' in pr, 'PR provenance confirmation missing')
need('I have read and agree to `CONTRIBUTING.md`' in pr, 'PR contributor-terms confirmation missing')
need('## 6A. Licensing and contribution law' in bible, 'Company Bible licensing law missing')
need('No build may change `LICENSE`, `COPYRIGHT.md`, `CONTRIBUTING.md`, `THIRD_PARTY_NOTICES.md`, or `docs/LICENSING_POLICY.md` without explicit owner approval.' in bible, 'Company Bible owner approval gate missing')
need('## Licensing and third-party software' in readme, 'README license summary missing')

for rel in (
    'licenses/LIBTORRENT_BSD_LICENSE.txt',
    'licenses/ai/LLAMA_CPP_MIT_LICENSE.txt',
    'licenses/ai/NOMIC_EMBED_TEXT_APACHE_2_LICENSE.txt',
    'licenses/jellyfin/JELLYFIN_SERVER_GPL_LICENSE.txt',
    'licenses/jellyfin/JELLYFIN_WEB_GPL_LICENSE.txt',
):
    need((ROOT/rel).is_file(), f'preserved upstream license copy missing: {rel}')

print('license-v22=pass owner-rights=pass polyform=pass third-party-boundary=pass contributor-inbound=pass pr-gate=pass upstream-notices=pass')
