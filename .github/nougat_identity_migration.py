from pathlib import Path
import subprocess
import re

root = Path('.')
workflow = Path('.github/workflows/nougat-play-portal-identity-migration.yml')
helper = Path('.github/nougat_identity_migration.py')
old_plus = 'Nougat' + ' Media' + ' Plus'
old_suite = 'Nougat' + ' Media' + ' Suite'
new_brand = 'Nougat Play Portal'

replacements = [
    (old_plus.upper(), new_brand.upper()),
    (old_suite.upper(), new_brand.upper()),
    (old_plus, new_brand),
    (old_suite, new_brand),
    ('Nougat_' + 'Media_' + 'Plus', 'Nougat_Play_Portal'),
    ('Nougat_' + 'Media_' + 'Suite', 'Nougat_Play_Portal'),
    ('NOUGAT_' + 'MEDIA_' + 'PLUS', 'NOUGAT_PLAY_PORTAL'),
    ('NOUGAT_' + 'MEDIA_' + 'SUITE', 'NOUGAT_PLAY_PORTAL'),
    ('nougat_' + 'media_' + 'plus', 'nougat_play_portal'),
    ('nougat_' + 'media_' + 'suite', 'nougat_play_portal'),
    ('nougat-' + 'media-' + 'plus', 'nougat-play-portal'),
    ('nougat-' + 'media-' + 'suite', 'nougat-play-portal'),
    ('Nougat-' + 'Media-' + 'Plus', 'Nougat-Play-Portal'),
    ('Nougat-' + 'Media-' + 'Suite', 'Nougat-Play-Portal'),
    ('Nougat' + 'Media' + 'Plus', 'NougatPlayPortal'),
    ('Nougat' + 'Media' + 'Suite', 'NougatPlayPortal'),
    ('nougat' + 'Media' + 'Plus', 'nougatPlayPortal'),
    ('nougat' + 'Media' + 'Suite', 'nougatPlayPortal'),
]

def tracked():
    return [p for p in subprocess.check_output(['git','ls-files','-z']).decode().split('\0') if p]

# Rewrite every tracked UTF-8 text file. Binary bytes are preserved.
for name in tracked():
    p = Path(name)
    if p in {workflow, helper} or not p.is_file():
        continue
    raw = p.read_bytes()
    if b'\x00' in raw:
        continue
    try:
        text = raw.decode('utf-8')
    except UnicodeDecodeError:
        continue
    changed = text
    for old, new in replacements:
        changed = changed.replace(old, new)
    if changed != text:
        p.write_text(changed, encoding='utf-8', newline='')

# Rename tracked paths, including binary branding assets, without changing their bytes.
for name in sorted([p for p in tracked() if p not in {str(workflow), str(helper)}], key=lambda s:(s.count('/'),len(s)), reverse=True):
    new_name = name
    for old, new in replacements:
        new_name = new_name.replace(old, new)
    if new_name != name:
        Path(new_name).parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['git','mv','-f','--',name,new_name], check=True)

# Rewrite the maintained README identity and v69 UI direction.
readme = Path('README.md')
s = readme.read_text(encoding='utf-8')
if not s.startswith('# Nougat Play Portal\n'):
    raise SystemExit('FAIL: README heading did not migrate')
first_section = s.find('\n## ')
if first_section < 0:
    raise SystemExit('FAIL: README first section boundary missing')
opening = '''# Nougat Play Portal

**Powered by NougatOS**

**Nougat Play Portal** is a native Linux, game-first entertainment platform developed by **Elderred Softworks LLC**. It brings games and emulation, local and network media, television, radio, streaming, social and multiplayer foundations, production tools, privacy-focused search, P2P media, security analysis, diagnostics, and connected hardware together inside one integrated console-style environment. **NougatOS** is the underlying platform identity that ties those systems together.

The **v0.0.69** development line is the identity and interface transition to Nougat Play Portal. The approved visual authority is `ExampleImages/Nougat_Play_Portal_v69_UI_Sheet.png`: a dark chocolate/black console shell with cream, copper, and amber focus accents; game-first cinematic presentation; top-level **Home, Games, Library, Network, Media, System** navigation; a **Continue Playing** shelf; friends and activity surfaces; and a compact controller-friendly Quick Menu. The previous tactical/military presentation is superseded for current v0.0.69 work.
'''
s = opening + s[first_section:]
s = re.sub(r'Beginning with \*\*v0\.0\.65\*\*, the Player is moving into the approved black/deep-green military HUD family.*?immediate retriggerable UI-click feedback\.\n\n', 'Beginning with **v0.0.69**, the Player presentation is being integrated into the approved Nougat Play Portal console interface while retaining chapter navigation, episode controls, seeking, volume, information overlays, Fullscreen, Settings, and immediate retriggerable UI-click feedback.\n\n', s, count=1, flags=re.S)
start = s.find('## **Nougat Play Portal Interface**')
end = s.find('## **Platform & Repository**', start if start >= 0 else 0)
if start >= 0 and end > start:
    current_ui = '''## **Nougat Play Portal & NougatOS Interface**

**Nougat Play Portal** is the current user-facing product identity and **NougatOS** is the underlying platform identity. The v0.0.69 development line replaces the prior tactical/military shell with the owner-approved game-first console interface shown in `ExampleImages/Nougat_Play_Portal_v69_UI_Sheet.png`.

The active hierarchy is **Home, Games, Library, Network, Media, System**. Home centers the selected game in a large cinematic hero with direct Play and Game Details actions and a persistent **Continue Playing** shelf. Games provides systems, categories, favorites, recently added, multiplayer, and Nougat 3D organization. Network is the social and multiplayer layer for friends, parties, server browsing, Looking for Game, messages, leaderboards, and events. Media remains a full part of the platform while moving behind the game-first hierarchy rather than competing with it for the primary shell.

The visual language uses a dark chocolate/black shell with warm cream, copper, and amber focus treatment. A controller-friendly Quick Menu provides Home, Sound, Brightness, Wi-Fi, Bluetooth, Controller, and Power surfaces. Game artwork supplies the dominant scene color and selected titles expand into the surrounding visual field, giving the Portal identity its game-to-world transition.

The approved **N** remains the required application icon identity. The v0.0.69 source migration must carry the Nougat Play Portal and NougatOS names through live UI strings, executable targets, desktop metadata, packaging, server-facing branding, and current documentation without a mixed current identity.

'''
    s = s[:start] + current_ui + s[end:]
marker = '## v0.0.68 - Accepted Checkpoint'
v69 = '''## v0.0.69 - Nougat Play Portal / NougatOS Identity Transition (In Development)

- Begins the exhaustive current-facing identity transition to **Nougat Play Portal**, powered by **NougatOS**.
- Establishes the approved v69 console-style, game-first UI sheet as the current visual authority and supersedes the prior tactical/military shell for new interface work.
- Migrates current tracked source identifiers, filenames, documentation, build targets, and branding paths away from superseded product names while preserving Git history and the accepted v0.0.68 tag as historical records.
- This is an **in-development** transition. It does not claim that a new v0.0.69 executable has passed owner-machine gameplay, media, hardware, or acceptance testing.

'''
if marker in s and '## v0.0.69 - Nougat Play Portal / NougatOS Identity Transition (In Development)' not in s:
    s = s.replace(marker, v69 + marker, 1)
readme.write_text(s, encoding='utf-8', newline='')

# Make the Company Bible name rule explicit without deleting its existing laws.
bible = Path('COMPANY_BIBLE.md')
b = bible.read_text(encoding='utf-8')
heading = '## 7. Nougat Play Portal identity law'
if heading not in b:
    raise SystemExit('FAIL: Company Bible identity section did not migrate')
rule = '''The current user-facing product identity is **Nougat Play Portal** and the underlying platform identity is **NougatOS**. Beginning with the v0.0.69 development line, superseded product names are forbidden in current tracked filenames, live source identifiers, UI strings, executable/build targets, desktop metadata, packaging, server-facing branding, and current documentation. Git history and accepted historical tags remain historical records and are not rewritten to pretend they were created under a later identity. A v0.0.69 candidate may not ship with a mixed current identity.

'''
if rule.strip() not in b:
    b = b.replace(heading + '\n\n', heading + '\n\n' + rule, 1)
bible.write_text(b, encoding='utf-8', newline='')

# Advance current build configuration and source-facing versioned program references to v69.
cmake = Path('CMakeLists.txt')
c = cmake.read_text(encoding='utf-8')
c = c.replace('project(NougatPlayPortal VERSION 0.0.68', 'project(NougatPlayPortal VERSION 0.0.69', 1)
c = c.replace('Nougat_Play_Portal_v68', 'Nougat_Play_Portal_v69')
cmake.write_text(c, encoding='utf-8', newline='')
current_exts = {'.c','.cc','.cpp','.cxx','.h','.hh','.hpp','.sh','.py','.desktop','.service','.json','.toml','.ini','.conf','.cmake','.yml','.yaml'}
for p in root.rglob('*'):
    if not p.is_file() or p in {workflow, helper} or '.git' in p.parts:
        continue
    if p.suffix.lower() not in current_exts and p.name != 'CMakeLists.txt':
        continue
    raw = p.read_bytes()
    if b'\x00' in raw:
        continue
    try:
        text = raw.decode('utf-8')
    except UnicodeDecodeError:
        continue
    changed = text.replace('Nougat_Play_Portal_v68', 'Nougat_Play_Portal_v69').replace('Nougat Play Portal v0.0.68', 'Nougat Play Portal v0.0.69')
    if changed != text:
        p.write_text(changed, encoding='utf-8', newline='')

# Hard gate: no superseded product identity may remain in current tracked text or paths.
forbidden = [x[0] for x in replacements]
bad_paths, bad_text = [], []
for name in tracked():
    if name in {str(workflow), str(helper)}:
        continue
    if any(token in name for token in forbidden):
        bad_paths.append(name)
    p = Path(name)
    if not p.is_file():
        continue
    raw = p.read_bytes()
    if b'\x00' in raw:
        continue
    try:
        text = raw.decode('utf-8')
    except UnicodeDecodeError:
        continue
    hits = [token for token in forbidden if token in text]
    if hits:
        bad_text.append((name, hits[:3]))
if bad_paths or bad_text:
    print('FAIL: superseded identity remains')
    for x in bad_paths[:50]: print('PATH', x)
    for x in bad_text[:100]: print('TEXT', x)
    raise SystemExit(1)
if '**Powered by NougatOS**' not in readme.read_text(encoding='utf-8'):
    raise SystemExit('FAIL: README NougatOS line missing')
if 'ExampleImages/Nougat_Play_Portal_v69_UI_Sheet.png' not in readme.read_text(encoding='utf-8'):
    raise SystemExit('FAIL: README approved v69 UI sheet reference missing')
print('PASS: tracked current identity migrated to Nougat Play Portal / NougatOS')
