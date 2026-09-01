#!/usr/bin/env python3
from __future__ import annotations

import datetime as dt
import gzip
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from typing import Any

API_BASE = "https://iptv-org.github.io/api"
STREAMS_API = f"{API_BASE}/streams.json"
LOGOS_API = f"{API_BASE}/logos.json"
GUIDES_API = f"{API_BASE}/guides.json"

ALoula_CHANNELS = "https://aloula.faulio.com/api/v1/channels"
ALoula_PLAYER = "https://aloula.faulio.com/api/v1.1/channels/{channel}/player"

UA = "Mozilla/5.0 (X11; Linux x86_64) NougatMediaSuite/0.0.51"
CACHE_ROOT = Path.home() / ".cache" / "reddmedia" / "world_tv"
API_CACHE = CACHE_ROOT / "api"


def clean(value: Any) -> str:
    text = "" if value is None else str(value)
    return text.replace("\r", " ").replace("\n", " ").replace("\t", " ").strip()


def emit(**values: Any) -> None:
    for key, value in values.items():
        print(f"{key}={clean(value)}")


def request_bytes(url: str, headers: dict[str, str] | None = None,
                  timeout: float = 8.0, limit: int = 40 * 1024 * 1024) -> bytes:
    merged = {"User-Agent": UA, "Accept": "*/*"}
    if headers:
        merged.update(headers)
    req = urllib.request.Request(url, headers=merged, method="GET")
    with urllib.request.urlopen(req, timeout=timeout) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise RuntimeError("response exceeded safety limit")
    return data


def cached_json(url: str, name: str, ttl: int = 1800) -> Any:
    API_CACHE.mkdir(parents=True, exist_ok=True)
    path = API_CACHE / name
    now = time.time()
    if path.is_file() and now - path.stat().st_mtime <= ttl:
        try:
            return json.loads(path.read_text(encoding="utf-8"))
        except Exception:
            pass

    data = request_bytes(url, timeout=10.0, limit=48 * 1024 * 1024)
    obj = json.loads(data.decode("utf-8"))
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(obj, separators=(",", ":")), encoding="utf-8")
    temporary.replace(path)
    return obj


def parse_quality(value: Any) -> int:
    text = clean(value).lower()
    digits = "".join(ch for ch in text if ch.isdigit())
    if not digits:
        return 0
    try:
        return int(digits)
    except ValueError:
        return 0


def youtube_url(url: str) -> bool:
    lower = url.lower()
    return "youtube.com" in lower or "youtu.be" in lower


def blocked_label(value: Any) -> bool:
    lower = clean(value).lower()
    return "geo-block" in lower or lower == "blocked" or "region" in lower and "block" in lower


def aloula_channel_list(payload: Any) -> list[dict[str, Any]]:
    if isinstance(payload, list):
        return [x for x in payload if isinstance(x, dict)]
    if isinstance(payload, dict):
        for key in ("data", "channels", "items"):
            value = payload.get(key)
            if isinstance(value, list):
                return [x for x in value if isinstance(x, dict)]
    return []


def aloula_fetch_json(url: str) -> Any:
    headers = {
        "User-Agent": "Mozilla/5.0",
        "Origin": "https://www.aloula.sa",
        "Referer": "https://www.aloula.sa/",
        "Accept": "application/json,text/plain,*/*",
    }
    return json.loads(request_bytes(url, headers=headers, timeout=7.0,
                                    limit=2 * 1024 * 1024).decode("utf-8"))


def aloula_resolve_quran() -> tuple[str, str, str]:
    payload = aloula_fetch_json(ALoula_CHANNELS)
    channels = aloula_channel_list(payload)
    ranked: list[tuple[int, dict[str, Any]]] = []

    for channel in channels:
        if not channel.get("has_live", False):
            continue
        title = clean(channel.get("title", ""))
        slug = clean(channel.get("url", ""))
        joined = "".join(ch.lower() for ch in f"{title} {slug}" if ch.isalnum())
        score = 0
        if "القرآن" in title and "الكريم" in title:
            score = 200
        if any(token in joined for token in (
            "alkuranalkarim", "alquranalkarim", "alquranalkareem",
            "quranalkarim", "quranalkareem", "qurantv",
        )):
            score = max(score, 180)
        if any(token in joined for token in ("quran", "kuran", "koran")):
            score = max(score, 120)
        if score:
            ranked.append((score, channel))

    if not ranked:
        raise RuntimeError("official Aloula Quran channel was not found")

    ranked.sort(key=lambda item: item[0], reverse=True)
    channel = ranked[0][1]
    channel_id = channel.get("id")
    if not isinstance(channel_id, int):
        raise RuntimeError("official Aloula channel ID was invalid")

    direct = channel.get("streams")
    if isinstance(direct, dict):
        hls = direct.get("hls")
        if isinstance(hls, str) and hls.startswith(("https://", "http://")):
            return hls, "https://www.aloula.sa/", "Mozilla/5.0"

    player = aloula_fetch_json(ALoula_PLAYER.format(channel=channel_id))
    candidates = [player]
    if isinstance(player, dict):
        for key in ("data", "player"):
            nested = player.get(key)
            if isinstance(nested, dict):
                candidates.append(nested)
    for item in candidates:
        streams = item.get("streams") if isinstance(item, dict) else None
        if isinstance(streams, dict):
            hls = streams.get("hls")
            if isinstance(hls, str) and hls.startswith(("https://", "http://")):
                return hls, "https://www.aloula.sa/", "Mozilla/5.0"

    raise RuntimeError("official Aloula live HLS was unavailable")


# NOUGAT_V49_RUSSIA24_AUDIO_REPAIR: require a real audio stream for Russia-24.
LAST_PROBE_REASON = ""

def ffprobe_candidate(url: str, referrer: str, user_agent: str,
                      require_audio: bool = False) -> bool:
    global LAST_PROBE_REASON
    LAST_PROBE_REASON = ""
    ffprobe = shutil.which("ffprobe")
    if not ffprobe:
        LAST_PROBE_REASON = "ffprobe is not installed"
        return False
    probe_args = [ffprobe, "-v", "error", "-rw_timeout", "7000000"]
    if user_agent:
        probe_args += ["-user_agent", user_agent]
    headers = f"Referer: {referrer}\\r\\n" if referrer else ""
    if headers:
        probe_args += ["-headers", headers]
    probe_args += ["-show_entries", "stream=codec_type,width,height,codec_name", "-of", "json", url]
    try:
        result = subprocess.run(probe_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, timeout=8)
    except subprocess.TimeoutExpired:
        LAST_PROBE_REASON = "stream probe timed out"
        return False
    except OSError as exc:
        LAST_PROBE_REASON = f"stream probe could not start: {exc}"
        return False
    if result.returncode != 0:
        detail = clean((result.stderr or "").splitlines()[-1] if result.stderr else "")
        LAST_PROBE_REASON = "ffprobe rejected source" + (f": {detail}" if detail else "")
        return False
    try:
        payload = json.loads(result.stdout or "{}")
    except json.JSONDecodeError:
        LAST_PROBE_REASON = "ffprobe returned malformed stream metadata"
        return False
    has_video = any(isinstance(s,dict) and s.get("codec_type")=="video" and clean(s.get("codec_name")) for s in payload.get("streams",[]))
    has_audio = any(isinstance(s,dict) and s.get("codec_type")=="audio" and clean(s.get("codec_name")) for s in payload.get("streams",[]))
    if not has_video:
        LAST_PROBE_REASON = "source has no decodable video stream"
        return False
    if require_audio and not has_audio:
        LAST_PROBE_REASON = "source has video but no decodable audio stream"
        return False
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        LAST_PROBE_REASON = "verified by ffprobe; ffmpeg frame sample unavailable"
        return True
    frame_args=[ffmpeg,"-nostdin","-v","error","-rw_timeout","7000000"]
    if user_agent:
        frame_args += ["-user_agent",user_agent]
    if headers:
        frame_args += ["-headers",headers]
    frame_args += ["-i",url,"-t","3","-vf",
                   "fps=1,scale=32:18:force_original_aspect_ratio=decrease,pad=32:18:(ow-iw)/2:(oh-ih)/2,format=gray",
                   "-frames:v","3","-f","rawvideo","pipe:1"]
    try:
        sampled=subprocess.run(frame_args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
    except subprocess.TimeoutExpired:
        LAST_PROBE_REASON = "video frame sample timed out after stream metadata succeeded"
        return False
    except OSError as exc:
        LAST_PROBE_REASON = f"video frame sampler could not start: {exc}"
        return False
    frame_size=32*18
    data=sampled.stdout or b""
    if sampled.returncode != 0 or len(data) < frame_size:
        LAST_PROBE_REASON = "stream opened but no complete video frame arrived"
        return False
    frames=[data[o:o+frame_size] for o in range(0,min(len(data),frame_size*3),frame_size) if len(data[o:o+frame_size])==frame_size]
    for frame in frames:
        low=min(frame); high=max(frame); mean=sum(frame)/len(frame)
        if mean>=8.0 or high-low>=14:
            LAST_PROBE_REASON = "playable video verified"
            return True
    LAST_PROBE_REASON = "stream opened but sampled frames remained black/blank"
    return False

def resolve_mode(channel_id: str, feed_id: str, preferred_url: str,
                 resolver: str, max_height: int, exclude_url: str) -> int:
    candidates: list[dict[str, str]] = []

    def add(url: str, referrer: str = "", user_agent: str = "",
            label: str = "", preferred: bool = False) -> None:
        url = clean(url)
        if not url.startswith(("https://", "http://")) or youtube_url(url):
            return
        if any(item["url"] == url for item in candidates):
            return
        candidates.append({
            "url": url,
            "referrer": clean(referrer),
            "user_agent": clean(user_agent),
            "label": clean(label),
            "preferred": "1" if preferred else "0",
        })

    if resolver == "aloula-quran":
        try:
            url, referrer, user_agent = aloula_resolve_quran()
            add(url, referrer, user_agent, preferred=True)
        except Exception:
            pass

    if preferred_url:
        add(preferred_url, preferred=True)

    try:
        streams = cached_json(STREAMS_API, "streams.json", ttl=1800)
    except Exception:
        streams = []

    if isinstance(streams, list):
        exact_feed: list[dict[str, Any]] = []
        other_feed: list[dict[str, Any]] = []
        for item in streams:
            if not isinstance(item, dict) or item.get("channel") != channel_id:
                continue
            quality = parse_quality(item.get("quality"))
            if quality > 0 and max_height > 0 and quality > max_height:
                continue
            target = exact_feed if feed_id and clean(item.get("feed")) == feed_id else other_feed
            target.append(item)

        for item in exact_feed + other_feed:
            add(
                clean(item.get("url")),
                clean(item.get("referrer")),
                clean(item.get("user_agent")),
                clean(item.get("label")),
            )

    exclude_url = clean(exclude_url)
    candidates.sort(key=lambda item: (
        1 if exclude_url and item["url"] == exclude_url else 0,
        0 if item["preferred"] == "1" else 1,
        1 if blocked_label(item["label"]) else 0,
        0 if item["url"].startswith("https://") else 1,
    ))

    # Russia-24 was owner-tested with picture but no sound. Do not accept a
    # video-only candidate for that station; walk farther through its current
    # direct-source alternates until both video and a decodable audio stream exist.
    require_audio = channel_id == "Russia24.ru"
    max_checks = 8 if require_audio else 6
    deadline = time.monotonic() + (60.0 if require_audio else 48.0)
    checked = 0
    for item in candidates:
        if blocked_label(item["label"]):
            continue
        if time.monotonic() >= deadline or checked >= max_checks:
            break
        checked += 1
        if ffprobe_candidate(item["url"], item["referrer"], item["user_agent"],
                             require_audio=require_audio):
            emit(
                OK=1,
                URL=item["url"],
                REFERRER=item["referrer"],
                USER_AGENT=item["user_agent"] or UA,
            )
            return 0

    emit(OK=0, ERROR=f"No playable direct source passed verification after {checked} checks from {len(candidates)} candidates. Last probe: {LAST_PROBE_REASON or 'no candidate completed'}.")
    return 3


def safe_filename(value: str) -> str:
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in value)


def artwork_mode(channel_id: str, feed_id: str, output_bmp: str) -> int:
    output = Path(output_bmp)
    if output.is_file() and output.stat().st_size > 256:
        emit(OK=1)
        return 0

    try:
        logos = cached_json(LOGOS_API, "logos.json", ttl=12 * 3600)
    except Exception as exc:
        emit(OK=0, ERROR=f"logo catalog unavailable: {exc}")
        return 2

    choices: list[dict[str, Any]] = []
    if isinstance(logos, list):
        for item in logos:
            if not isinstance(item, dict) or item.get("channel") != channel_id:
                continue
            choices.append(item)

    if not choices:
        emit(OK=0, ERROR="no real station logo is registered")
        return 3

    def score(item: dict[str, Any]) -> tuple[int, int, int, int]:
        feed_score = 0 if feed_id and clean(item.get("feed")) == feed_id else 1
        active_score = 0 if item.get("in_use") is True else 1
        fmt = clean(item.get("format")).upper()
        format_score = 0 if fmt in ("PNG", "JPEG", "WEBP") else 1
        tags = item.get("tags") if isinstance(item.get("tags"), list) else []
        horizontal_score = 0 if "horizontal" in tags else 1
        return feed_score, active_score, format_score, horizontal_score

    choices.sort(key=score)
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        emit(OK=0, ERROR="ffmpeg is required to normalize real station artwork")
        return 4

    CACHE_ROOT.mkdir(parents=True, exist_ok=True)
    output.parent.mkdir(parents=True, exist_ok=True)

    for item in choices:
        url = clean(item.get("url"))
        if not url.startswith("https://"):
            continue
        try:
            data = request_bytes(url, timeout=8.0, limit=8 * 1024 * 1024)
        except Exception:
            continue
        if len(data) < 32:
            continue

        suffix = Path(urllib.parse.urlparse(url).path).suffix or ".img"
        with tempfile.TemporaryDirectory(prefix="nougat-world-tv-art-") as td:
            source = Path(td) / ("source" + suffix)
            target = Path(td) / "logo.ppm"
            source.write_bytes(data)

            cmd = [
                ffmpeg, "-nostdin", "-v", "error", "-y",
                "-i", str(source),
                "-vf",
                "scale=160:90:force_original_aspect_ratio=decrease,"
                "pad=160:90:(ow-iw)/2:(oh-ih)/2:color=white",
                "-frames:v", "1",
                "-f", "image2pipe", "-vcodec", "ppm",
                "pipe:1",
            ]
            try:
                result = subprocess.run(cmd, stdout=subprocess.PIPE,
                                        stderr=subprocess.DEVNULL, timeout=10)
            except (OSError, subprocess.TimeoutExpired):
                continue
            ppm = result.stdout or b""
            if result.returncode == 0 and ppm.startswith(b"P6") and len(ppm) > 256:
                temporary = output.with_suffix(output.suffix + ".tmp")
                temporary.write_bytes(ppm)
                temporary.replace(output)
                emit(OK=1)
                return 0

    emit(OK=0, ERROR="registered station artwork could not be decoded")
    return 5


def parse_xmltv_time(value: str) -> int:
    value = clean(value)
    if not value:
        return 0
    main, _, zone = value.partition(" ")
    main = main[:14]
    fmt = "%Y%m%d%H%M%S"
    try:
        parsed = dt.datetime.strptime(main, fmt)
    except ValueError:
        return 0
    if zone:
        zone = zone.strip()
        if len(zone) >= 5 and zone[0] in "+-" and zone[1:5].isdigit():
            sign = 1 if zone[0] == "+" else -1
            offset = dt.timedelta(hours=int(zone[1:3]), minutes=int(zone[3:5]))
            parsed = parsed.replace(tzinfo=dt.timezone(sign * offset))
            return int(parsed.timestamp())
    parsed = parsed.replace(tzinfo=dt.timezone.utc)
    return int(parsed.timestamp())


def guide_mode(channel_id: str, feed_id: str) -> int:
    try:
        guides = cached_json(GUIDES_API, "guides.json", ttl=6 * 3600)
    except Exception:
        emit(OK=0)
        return 2

    matches: list[dict[str, Any]] = []
    if isinstance(guides, list):
        for item in guides:
            if isinstance(item, dict) and item.get("channel") == channel_id:
                matches.append(item)

    if not matches:
        emit(OK=0)
        return 3

    matches.sort(key=lambda item: (
        0 if feed_id and clean(item.get("feed")) == feed_id else 1,
        0 if item.get("feed") in (None, "", feed_id) else 1,
    ))

    now = int(time.time())
    for match in matches:
        sources = match.get("sources")
        if not isinstance(sources, list):
            continue
        for source in sources[:3]:
            if not isinstance(source, dict):
                continue
            url = clean(source.get("url"))
            if not url.startswith(("https://", "http://")):
                continue
            try:
                data = request_bytes(url, timeout=10.0, limit=32 * 1024 * 1024)
                if data[:2] == b"\x1f\x8b":
                    data = gzip.decompress(data)
            except Exception:
                continue

            wanted = {channel_id, clean(match.get("site_id"))}
            current = None
            upcoming = None
            try:
                for _, element in ET.iterparse(io.BytesIO(data), events=("end",)):
                    if element.tag.rsplit("}", 1)[-1] != "programme":
                        element.clear()
                        continue
                    program_channel = clean(element.attrib.get("channel"))
                    if program_channel not in wanted:
                        element.clear()
                        continue
                    start = parse_xmltv_time(element.attrib.get("start", ""))
                    stop = parse_xmltv_time(element.attrib.get("stop", ""))
                    title = ""
                    for child in element:
                        if child.tag.rsplit("}", 1)[-1] == "title":
                            title = clean(child.text)
                            if title:
                                break
                    if start and stop and title:
                        entry = (start, stop, title)
                        if start <= now < stop:
                            current = entry
                        elif start > now and (upcoming is None or start < upcoming[0]):
                            upcoming = entry
                    element.clear()
            except ET.ParseError:
                continue

            if current or upcoming:
                emit(
                    OK=1,
                    CURRENT_TITLE=current[2] if current else "",
                    CURRENT_START=current[0] if current else 0,
                    CURRENT_END=current[1] if current else 0,
                    NEXT_TITLE=upcoming[2] if upcoming else "",
                    NEXT_START=upcoming[0] if upcoming else 0,
                    NEXT_END=upcoming[1] if upcoming else 0,
                    SOURCE=clean(source.get("host")) or urllib.parse.urlparse(url).netloc,
                )
                return 0

    emit(OK=0)
    return 4


def main() -> int:
    if len(sys.argv) < 2:
        emit(OK=0, ERROR="missing mode")
        return 64

    mode = sys.argv[1]
    if mode == "resolve" and len(sys.argv) == 8:
        try:
            max_height = max(1, min(1080, int(sys.argv[6])))
        except ValueError:
            max_height = 1080
        return resolve_mode(sys.argv[2], sys.argv[3], sys.argv[4],
                            sys.argv[5], max_height, sys.argv[7])

    if mode == "artwork" and len(sys.argv) == 5:
        return artwork_mode(sys.argv[2], sys.argv[3], sys.argv[4])

    if mode == "guide" and len(sys.argv) == 4:
        return guide_mode(sys.argv[2], sys.argv[3])

    emit(OK=0, ERROR="invalid World TV worker arguments")
    return 64


if __name__ == "__main__":
    raise SystemExit(main())
