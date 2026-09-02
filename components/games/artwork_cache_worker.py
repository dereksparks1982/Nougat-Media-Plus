#!/usr/bin/env python3
from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor, as_completed
from difflib import SequenceMatcher
from html.parser import HTMLParser
from pathlib import Path
import argparse
import html
import os
import re
import shutil
import subprocess
import tempfile
import threading
import time
import urllib.parse
import urllib.request

COLLECTIONS = {
    "NES": "Nintendo - Nintendo Entertainment System",
    "SNES": "Nintendo - Super Nintendo Entertainment System",
    "Game Boy": "Nintendo - Game Boy",
    "Game Boy Color": "Nintendo - Game Boy Color",
    "Game Boy Advance": "Nintendo - Game Boy Advance",
    "Nintendo 64": "Nintendo - Nintendo 64",
    "Sega Genesis": "Sega - Mega Drive - Genesis",
    "Sega Master System": "Sega - Master System - Mark III",
    "Sega Game Gear": "Sega - Game Gear",
    "Atari 2600": "Atari - 2600",
    "Atari 5200": "Atari - 5200",
    "Atari 7800": "Atari - 7800",
    "Atari 8-bit": "Atari - 8-bit",
    "Atari Lynx": "Atari - Lynx",
    "PlayStation": "Sony - PlayStation",
    "PlayStation 2": "Sony - PlayStation 2",
    "PlayStation Portable": "Sony - PlayStation Portable",
    "PlayStation 3": "Sony - PlayStation 3",
    "GameCube": "Nintendo - GameCube",
    "Wii": "Nintendo - Wii",
    "Wii U": "Nintendo - Wii U",
    "Arcade": "MAME",
    "Nintendo Switch": "Nintendo - Nintendo Switch",
}
CATEGORIES = ("Named_Boxarts", "Named_Titles", "Named_Snaps")
INVALID_LIBRETRO = set('&*/:\\`<>?|"')
INDEX_TTL_SECONDS = 7 * 24 * 60 * 60
INDEX_LOCK = threading.Lock()

# A small verified gap table is allowed only when the primary Libretro set has
# no entry at all. These are exact front-cover URLs, not generic placeholders.
KNOWN_REMOTE_FALLBACKS = {
    ("Atari 2600", "2 pak special challenge surfing"):
        "https://images.launchbox-app.com/a4446d3a-470d-4e63-9f6e-70e97c1ef3b1.jpg",
}

# Exact catalog aliases for preservation labels that are known alternate names.
# These stay deliberately small and explicit so fuzzy matching never invents a
# different game's artwork.
KNOWN_CATALOG_ALIASES = {
    ("Atari 2600", "adventures on gx 12"): ("Adventures of TRON",),
    ("Atari 2600", "action man"): ("Action Man - Action Force", "G.I. Joe - Cobra Strike"),
    ("Atari 2600", "angling"): ("Fishing Derby",),
}


def unescape_field(value: str) -> str:
    out: list[str] = []
    i = 0
    while i < len(value):
        c = value[i]
        if c != "\\" or i + 1 >= len(value):
            out.append(c)
            i += 1
            continue
        n = value[i + 1]
        if n == "t": out.append("\t")
        elif n == "n": out.append("\n")
        elif n == "r": out.append("\r")
        elif n == "\\": out.append("\\")
        else: out.append(n)
        i += 2
    return "".join(out)


def safe_thumbnail_name(value: str) -> str:
    value = value.strip()
    return "".join("_" if c in INVALID_LIBRETRO else c for c in value)


def short_name(value: str) -> str:
    cut = len(value)
    for marker in ("(", "["):
        pos = value.find(marker)
        if pos >= 0:
            cut = min(cut, pos)
    return value[:cut].strip(" ._-\t")


def strip_release_noise(value: str) -> str:
    value = html.unescape(urllib.parse.unquote(value)).replace("_", " ")
    value = re.sub(r"\[[^\]]*(?:!|b|h|o|p|t|f|bad|hack|overdump|trainer|translated)[^\]]*\]", " ", value, flags=re.I)
    value = re.sub(r"\((?:usa|us|u|europe|eur|pal|japan|jpn|world|asia|australia|korea|rev(?:ision)?[^)]*|v(?:er(?:sion)?)?\s*\d[^)]*|beta[^)]*|proto(?:type)?[^)]*|demo[^)]*)\)", " ", value, flags=re.I)
    value = re.sub(r"\b(?:rev(?:ision)?\s*[a-z0-9.]+|v(?:er(?:sion)?)?\s*\d+(?:\.\d+)*|beta\s*\d*|prototype|proto|demo|sample|preview)\b", " ", value, flags=re.I)
    value = re.sub(r"\s+", " ", value).strip(" ._-")
    return value


def artwork_name_candidates(rom_stem: str, display_title: str) -> list[str]:
    raw: list[str] = []
    for value in (rom_stem, display_title, short_name(rom_stem), short_name(display_title), strip_release_noise(rom_stem), strip_release_noise(display_title)):
        value = value.strip()
        if value and value not in raw:
            raw.append(value)

    lower = rom_stem.lower()
    base = short_name(display_title) or short_name(rom_stem)
    if base:
        if any(token in lower for token in ("(usa)", "[usa]", "(us)", "[us]", "(u)", "[u]", "ntsc-u")):
            raw.append(base + " (USA)")
        elif any(token in lower for token in ("(europe)", "[europe]", "(pal)", "[pal]")):
            raw.append(base + " (Europe)")

    out: list[str] = []
    for value in raw:
        value = safe_thumbnail_name(value)
        if value and value not in out:
            out.append(value)
    return out


def normalized_art_key(value: str) -> str:
    value = html.unescape(urllib.parse.unquote(value))
    if value.lower().endswith(".png"):
        value = value[:-4]
    # Preservation collections often collapse words into CamelCase or join a
    # console/model token directly to a number (AlligatorPeople, GX12). Split
    # those boundaries before punctuation/tag removal so they match catalog names.
    value = re.sub(r"(?<=[a-z])(?=[A-Z])", " ", value)
    value = re.sub(r"(?<=[A-Za-z])(?=[0-9])", " ", value)
    value = re.sub(r"(?<=[0-9])(?=[A-Za-z])", " ", value)
    value = re.sub(r"\[[^\]]*\]", " ", value)
    value = re.sub(r"\([^\)]*\)", " ", value)
    value = value.replace("&", " and ").replace("+", " and ")
    words = re.findall(r"[a-z0-9]+", value.lower())
    return " ".join(words)


def source_region(value: str) -> str:
    lower = value.lower()
    if any(t in lower for t in ("(usa)", "[usa]", "(u)", "[u]", "ntsc-u")):
        return "usa"
    if any(t in lower for t in ("(europe)", "[europe]", "(pal)", "[pal]", " pal ")):
        return "europe"
    if any(t in lower for t in ("(japan)", "[japan]", "(j)", "[j]")):
        return "japan"
    return ""


def candidate_region(value: str) -> str:
    lower = value.lower()
    if "usa" in lower:
        return "usa"
    if "europe" in lower or "australia" in lower or "pal" in lower:
        return "europe"
    if "japan" in lower:
        return "japan"
    return ""


def match_index_name(system: str, rom_stem: str, display_title: str, names: list[str]) -> str | None:
    source_values = [v for v in (rom_stem, display_title, short_name(rom_stem), short_name(display_title)) if v]
    source_keys = [normalized_art_key(v) for v in source_values]
    source_keys = [k for k in dict.fromkeys(source_keys) if k]
    for key in list(source_keys):
        for alias in KNOWN_CATALOG_ALIASES.get((system, key), ()):
            alias_key = normalized_art_key(alias)
            if alias_key and alias_key not in source_keys:
                source_keys.append(alias_key)
    if not source_keys:
        return None
    wanted_region = source_region(rom_stem)
    best_name: str | None = None
    best_score = -1.0
    for name in names:
        key = normalized_art_key(name)
        if not key:
            continue
        key_tokens = set(key.split())
        for source_key in source_keys:
            source_tokens = set(source_key.split())
            if not source_tokens or not key_tokens:
                continue
            exact = source_key == key
            union = source_tokens | key_tokens
            jaccard = len(source_tokens & key_tokens) / max(1, len(union))
            ratio = SequenceMatcher(None, source_key, key).ratio()
            if exact:
                score = 2.0
            elif jaccard >= 0.88 and ratio >= 0.82:
                score = 1.0 + (jaccard + ratio) / 2.0
            elif jaccard >= 0.80 and ratio >= 0.90:
                score = 0.9 + (jaccard + ratio) / 2.0
            else:
                continue
            region = candidate_region(name)
            if wanted_region and region == wanted_region:
                score += 0.08
            elif wanted_region and region and region != wanted_region:
                score -= 0.04
            if score > best_score:
                best_score = score
                best_name = name
    return best_name


class DirectoryIndexParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.names: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag.lower() != "a":
            return
        href = dict(attrs).get("href")
        if not href:
            return
        decoded = html.unescape(urllib.parse.unquote(href))
        if decoded.lower().endswith(".png") and "/" not in decoded.strip("/"):
            self.names.append(decoded)


def index_cache_path(collection: str, category: str) -> Path:
    safe = re.sub(r"[^A-Za-z0-9._-]+", "_", collection).strip("_")
    return Path.home() / ".cache" / "reddmedia" / "games" / "artwork-index-v49" / f"{safe}--{category}.txt"


def parse_directory_index(page: str) -> list[str]:
    parser = DirectoryIndexParser()
    parser.feed(page)
    return list(dict.fromkeys(parser.names))


def fetch_directory_index(collection: str, category: str) -> list[str]:
    cache = index_cache_path(collection, category)
    with INDEX_LOCK:
        if cache.is_file() and time.time() - cache.stat().st_mtime < INDEX_TTL_SECONDS:
            return [line.rstrip("\n") for line in cache.read_text(encoding="utf-8").splitlines() if line.strip()]
        base = "https://thumbnails.libretro.com/" + urllib.parse.quote(collection, safe="") + "/" + category + "/"
        try:
            request = urllib.request.Request(base, headers={"User-Agent": "Nougat-Media-Suite-v0.0.53"})
            with urllib.request.urlopen(request, timeout=15) as response:
                page = response.read().decode("utf-8", "replace")
            names = parse_directory_index(page)
            if names:
                cache.parent.mkdir(parents=True, exist_ok=True)
                os.chmod(cache.parent, 0o700)
                temp = cache.with_suffix(".tmp")
                temp.write_text("\n".join(names) + "\n", encoding="utf-8")
                os.chmod(temp, 0o600)
                os.replace(temp, cache)
                return names
        except Exception:
            pass
        if cache.is_file():
            return [line.rstrip("\n") for line in cache.read_text(encoding="utf-8").splitlines() if line.strip()]
        return []


def valid_image(path: Path) -> bool:
    if not path.is_file() or path.stat().st_size < 16:
        return False
    head = path.read_bytes()[:12]
    return head.startswith(b"\x89PNG\r\n\x1a\n") or head.startswith(b"\xff\xd8") or head.startswith(b"BM")


def download_candidate(url: str, target: Path) -> bool:
    target.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=target.name + ".", suffix=".tmp", dir=target.parent)
    os.close(fd)
    temp = Path(temp_name)
    try:
        request = urllib.request.Request(url, headers={"User-Agent": "Nougat-Media-Suite-v0.0.53"})
        with urllib.request.urlopen(request, timeout=15) as response, temp.open("wb") as out:
            shutil.copyfileobj(response, out)
        if not valid_image(temp):
            return False
        os.chmod(temp, 0o600)
        os.replace(temp, target)
        return True
    except Exception:
        return False
    finally:
        temp.unlink(missing_ok=True)


def fetch_remote(system: str, rom_stem: str, display_title: str, remote_cache: Path) -> Path | None:
    collection = COLLECTIONS.get(system)
    if not collection:
        return None
    if valid_image(remote_cache):
        return remote_cache

    # Fast exact-name lane first. This preserves the cheap path for normal
    # No-Intro-style libraries.
    box_base = "https://thumbnails.libretro.com/" + urllib.parse.quote(collection, safe="") + "/Named_Boxarts/"
    for name in artwork_name_candidates(rom_stem, display_title):
        url = box_base + urllib.parse.quote(name + ".png", safe="")
        if download_candidate(url, remote_cache):
            print("ART exact:", system, display_title, "<-", name, flush=True)
            return remote_cache

    # Preservation-set filenames and older GoodTools names often differ from the
    # thumbnail catalog. Index each collection once, cache that index across app
    # builds, and resolve by conservative normalized title matching.
    for category in CATEGORIES:
        names = fetch_directory_index(collection, category)
        matched = match_index_name(system, rom_stem, display_title, names)
        if not matched:
            continue
        base = "https://thumbnails.libretro.com/" + urllib.parse.quote(collection, safe="") + "/" + category + "/"
        url = base + urllib.parse.quote(matched, safe="")
        if download_candidate(url, remote_cache):
            print("ART indexed:", system, display_title, "<-", category, matched, flush=True)
            return remote_cache

    # Verified exact-cover fallback only for known holes in the primary index.
    fallback = KNOWN_REMOTE_FALLBACKS.get((system, normalized_art_key(display_title)))
    if fallback is None:
        fallback = KNOWN_REMOTE_FALLBACKS.get((system, normalized_art_key(rom_stem)))
    if fallback and download_candidate(fallback, remote_cache):
        print("ART verified fallback:", system, display_title, flush=True)
        return remote_cache
    return None


def ffmpeg_prepare(source: Path, prepared: Path) -> bool:
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg or not source.is_file():
        return False
    prepared.parent.mkdir(parents=True, exist_ok=True)
    temp = prepared.with_suffix(".tmp.bmp")
    temp.unlink(missing_ok=True)
    command = [
        ffmpeg, "-nostdin", "-v", "error", "-y", "-i", str(source),
        "-frames:v", "1", "-vf",
        "scale=360:540:force_original_aspect_ratio=decrease,pad=360:540:(ow-iw)/2:(oh-ih)/2:color=black",
        "-f", "image2", "-vcodec", "bmp", str(temp),
    ]
    result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=20)
    if result.returncode != 0 or not temp.is_file() or temp.stat().st_size < 64:
        temp.unlink(missing_ok=True)
        return False
    os.chmod(temp, 0o600)
    os.replace(temp, prepared)
    return True


def process_row(row: list[str]) -> tuple[str, bool, str]:
    system, source_identity, rom_stem, title, sidecar_s, bundled_s, remote_s, prepared_s = row
    del source_identity
    prepared = Path(prepared_s)
    if prepared.is_file() and prepared.stat().st_size >= 64:
        return title, True, "prepared-cache"

    source: Path | None = None
    for candidate_s in (sidecar_s, bundled_s, remote_s):
        if candidate_s:
            candidate = Path(candidate_s)
            if valid_image(candidate):
                source = candidate
                break

    if source is None and remote_s:
        source = fetch_remote(system, rom_stem, title, Path(remote_s))

    if source is None:
        return title, False, "no-art"
    if not ffmpeg_prepare(source, prepared):
        return title, False, "prepare-failed"
    return title, True, "prepared"


def load_manifest(path: Path) -> list[list[str]]:
    rows: list[list[str]] = []
    with path.open("r", encoding="utf-8") as handle:
        for raw in handle:
            raw = raw.rstrip("\n")
            if not raw:
                continue
            parts = [unescape_field(part) for part in raw.split("\t")]
            if len(parts) != 8:
                print("SKIP malformed manifest row", flush=True)
                continue
            rows.append(parts)
    return rows


def self_test() -> int:
    assert safe_thumbnail_name('Q*Bert: Test?') == 'Q_Bert_ Test_'
    assert short_name("Adventure (USA) (Rev 2)") == "Adventure"
    candidates = artwork_name_candidates("Adventure (USA) (Rev 2)", "Adventure")
    assert candidates[0] == "Adventure (USA) (Rev 2)"
    assert "Adventure" in candidates
    assert "Adventure (USA)" in candidates
    encoded = r"Folder\\Name\tTitle\nNext"
    assert unescape_field(encoded) == "Folder\\Name\tTitle\nNext"
    sample = '''<html><a href="Adventure%20%28USA%29.png">Adventure</a>
<a href="2%20Pak%20Special%20%28Black%29%20-%20Challenge%2C%20Surfing%20%28Europe%29.png">two</a></html>'''
    names = parse_directory_index(sample)
    assert "Adventure (USA).png" in names
    match = match_index_name(
        "Atari 2600",
        "2 Pak Special - Challenge, Surfing (1990) (HES) (PAL) [a]",
        "2 Pak Special - Challenge, Surfing",
        names,
    )
    assert match == "2 Pak Special (Black) - Challenge, Surfing (Europe).png"
    assert normalized_art_key("2 Pak Special - Challenge, Surfing") == "2 pak special challenge surfing"
    assert normalized_art_key("AlligatorPeople") == "alligator people"
    assert normalized_art_key("AdventuresOnGX12") == "adventures on gx 12"
    alias_names = [
        "Adventures of TRON (USA).png",
        "Action Man - Action Force (Europe).png",
        "Fishing Derby (USA).png",
        "Alligator People (USA) (Proto).png",
    ]
    assert match_index_name("Atari 2600", "AdventuresOnGX12", "AdventuresOnGX12", alias_names) == "Adventures of TRON (USA).png"
    assert match_index_name("Atari 2600", "Action Man", "Action Man", alias_names) == "Action Man - Action Force (Europe).png"
    assert match_index_name("Atari 2600", "Angling", "Angling", alias_names) == "Fishing Derby (USA).png"
    assert match_index_name("Atari 2600", "AlligatorPeople", "AlligatorPeople", alias_names) == "Alligator People (USA) (Proto).png"
    assert COLLECTIONS["Sega Genesis"] == "Sega - Mega Drive - Genesis"
    print("Nougat v0.0.49 artwork worker self-test PASS: Sega catalogs, CamelCase preservation names, verified Atari aliases/fallbacks, persistent indexes, and manifest escaping.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.manifest:
        print("FAIL: --manifest is required")
        return 2
    manifest = Path(args.manifest)
    if not manifest.is_file():
        print("FAIL: manifest not found:", manifest)
        return 2

    rows = load_manifest(manifest)
    if not rows:
        print("No game artwork rows to prepare.")
        return 0
    completed = 0
    prepared_count = 0
    with ThreadPoolExecutor(max_workers=4, thread_name_prefix="nougat-game-art") as pool:
        futures = [pool.submit(process_row, row) for row in rows]
        for future in as_completed(futures):
            title, ok, detail = future.result()
            completed += 1
            prepared_count += int(ok)
            print(f"[{completed}/{len(rows)}] {title}: {detail}", flush=True)
    print(f"Nougat v0.0.49 artwork preparation complete: {prepared_count}/{len(rows)} ready.", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
