#!/usr/bin/env python3
from __future__ import annotations
import argparse, datetime as dt, gzip, json, os, shlex, tempfile, urllib.parse, urllib.request, zlib
import xml.etree.ElementTree as ET
from pathlib import Path


def fetch(url: str, timeout: float = 12.0) -> bytes:
    req=urllib.request.Request(url,headers={"Accept-Encoding":"gzip","User-Agent":"Nougat-Media-Suite/0.0.58"})
    with urllib.request.urlopen(req,timeout=timeout) as response:
        data=response.read()
        if response.headers.get("Content-Encoding","").lower()=="gzip": data=gzip.decompress(data)
        return data


def norm(value: str) -> str:
    return "".join(ch.lower() for ch in value if ch.isalnum())


def load_channels(path: Path):
    rows=[]
    if not path.is_file(): return rows
    for line in path.read_text(errors="ignore").splitlines():
        try: parts=shlex.split(line,posix=True)
        except ValueError: continue
        if len(parts)>=2: rows.append((parts[0],parts[1]))
    return rows


def quote(value: str) -> str:
    return '"'+value.replace('\\','\\\\').replace('"','\\"').replace('\n',' ').replace('\r',' ')+'"'


def parse_stamp(value: str) -> int:
    value=value.strip()
    for fmt in ("%Y%m%d%H%M%S %z","%Y%m%d%H%M %z","%Y%m%d%H%M%S","%Y%m%d%H%M"):
        try:
            parsed=dt.datetime.strptime(value,fmt)
            if parsed.tzinfo is None: parsed=parsed.replace(tzinfo=dt.timezone.utc)
            return int(parsed.timestamp())
        except ValueError: pass
    return 0


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--device-address",required=True)
    ap.add_argument("--channels",type=Path,required=True)
    ap.add_argument("--guide",type=Path,required=True)
    args=ap.parse_args()
    discover=json.loads(fetch(f"http://{args.device_address}/discover.json",5.0).decode("utf-8"))
    auth=str(discover.get("DeviceAuth","")).strip()
    if not auth: raise SystemExit("HDHomeRun did not provide DeviceAuth.")
    url="https://api.hdhomerun.com/api/xmltv?"+urllib.parse.urlencode({"DeviceAuth":auth})
    xml=fetch(url,30.0)
    root=ET.fromstring(xml)
    known=load_channels(args.channels)
    by_id={cid:cid for cid,_ in known}
    by_name={norm(name):cid for cid,name in known if name}
    xml_map={}
    for channel in root.findall("channel"):
        xml_id=channel.get("id","")
        names=[(x.text or "").strip() for x in channel.findall("display-name")]
        mapped=by_id.get(xml_id)
        if not mapped:
            for value in [xml_id,*names]:
                key=norm(value)
                if key in by_name: mapped=by_name[key]; break
                first=value.split()[0] if value.split() else ""
                if first in by_id: mapped=first; break
        if mapped: xml_map[xml_id]=mapped
    now=int(dt.datetime.now(tz=dt.timezone.utc).timestamp())
    events=[]
    for program in root.findall("programme"):
        channel=xml_map.get(program.get("channel","") or "")
        if not channel: continue
        start=parse_stamp(program.get("start","") or ""); stop=parse_stamp(program.get("stop","") or "")
        if start<=0 or stop<=start or stop<now-6*3600: continue
        title=(program.findtext("title") or "").strip() or "Program"
        desc=(program.findtext("desc") or "").strip()
        event_id=zlib.crc32(f"{channel}|{start}|{title}".encode("utf-8"))
        events.append((channel,title,desc,start,stop-start,event_id))
    if not events: raise SystemExit("HDHomeRun XMLTV returned no listings matching stored channels.")
    events.sort(key=lambda row:(row[0],row[3],row[5]))
    args.guide.parent.mkdir(parents=True,exist_ok=True)
    fd,tmp=tempfile.mkstemp(prefix=args.guide.name+".",suffix=".tmp",dir=args.guide.parent)
    try:
        with os.fdopen(fd,"w",encoding="utf-8") as out:
            for channel,title,desc,start,duration,event_id in events:
                out.write(f"{quote(channel)}\t{quote(title)}\t{quote(desc)}\t{start}\t{duration}\t{event_id}\n")
            out.flush(); os.fsync(out.fileno())
        os.chmod(tmp,0o600); os.replace(tmp,args.guide)
    finally:
        if os.path.exists(tmp): os.unlink(tmp)
    print(f"HDHomeRun XMLTV guide refreshed: {len(events)} events")
    return 0

if __name__=="__main__": raise SystemExit(main())
