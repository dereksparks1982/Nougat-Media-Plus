#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/nougat-media-plus/ps3"
CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
PROFILE="$CONFIG_HOME/nougat-media-plus/ps3-graphics.conf"
mkdir -p "$CACHE" "$HERE"
find_real(){ local p; if [[ -n "${NOUGAT_RPCS3_REAL:-}" && -x "${NOUGAT_RPCS3_REAL}" ]]; then printf '%s\n' "$NOUGAT_RPCS3_REAL"; return; fi; for p in "$HERE/RPCS3.AppImage" "$HOME/Applications/rpcs3.AppImage" /usr/local/bin/rpcs3 /usr/bin/rpcs3; do [[ -x "$p" && "$p" != "$HERE/rpcs3" ]] && { printf '%s\n' "$p"; return; }; done; return 1; }
install_official(){ command -v curl >/dev/null; command -v python3 >/dev/null; local meta="$CACHE/rpcs3-latest.json" tmp="$HERE/.RPCS3.AppImage.download" values url digest got; curl -fsSL --retry 2 --connect-timeout 15 https://api.github.com/repos/RPCS3/rpcs3-binaries-linux/releases/latest -o "$meta"; values="$(python3 - "$meta" <<'PY'
import json,sys
with open(sys.argv[1],encoding='utf-8') as f:d=json.load(f)
a=next((x for x in d.get('assets',[]) if x.get('name','').endswith('_linux64.AppImage')),None)
if a is None: raise SystemExit(2)
print(a['browser_download_url']); print((a.get('digest') or '').removeprefix('sha256:'))
PY
)"; url="$(printf '%s\n' "$values"|sed -n '1p')"; digest="$(printf '%s\n' "$values"|sed -n '2p')"; [[ -n "$url" && -n "$digest" ]]; curl -fL --retry 2 --connect-timeout 15 "$url" -o "$tmp"; got="$(sha256sum "$tmp"|awk '{print $1}')"; [[ "$got" == "$digest" ]] || { rm -f "$tmp"; return 1; }; chmod 0755 "$tmp"; mv -f "$tmp" "$HERE/RPCS3.AppImage"; }
real="$(find_real || true)"; if [[ -z "$real" ]]; then install_official; real="$(find_real)"; fi
if [[ "${1:-}" == "--nougat-ensure-runtime" ]]; then printf '%s\n' "$real"; exit 0; fi
export QT_QPA_PLATFORM=xcb SDL_VIDEODRIVER=x11
global=""; for p in "$CONFIG_HOME/rpcs3/config.yml" "$HOME/.config/rpcs3/config.yml"; do [[ -f "$p" ]] && { global="$p"; break; }; done
override="$CACHE/rpcs3-nougat-v66.yml"
if [[ -f "$PROFILE" && -n "$global" ]]; then
python3 - "$PROFILE" "$global" "$override" <<'PY'
from pathlib import Path
import sys
prof,src,dst=map(Path,sys.argv[1:4]); p={}
for raw in prof.read_text(encoding='utf-8').splitlines():
    if '=' in raw and not raw.lstrip().startswith('#'): k,v=raw.split('=',1); p[k]=v
r={'Resolution Scale':p.get('render_scale','150'),'Anisotropic Filter Override':p.get('anisotropic','16'),'Frame limit':p.get('frame_limit','Auto'),'MSAA':p.get('msaa','Auto'),'Output Scaling Mode':p.get('output_scaling','Bilinear'),'VSync Mode':'Full' if p.get('vsync','0') in ('1','true','yes','on') else 'Disabled','Use GPU texture scaling':'true' if p.get('gpu_texture_scaling','1') in ('1','true','yes','on') else 'false'}
lines=src.read_text(encoding='utf-8').splitlines(); out=[]; inv=False; seen={k:False for k in r}
def ind(s): return len(s)-len(s.lstrip(' '))
for line in lines:
    if line=='Video:': inv=True; out.append(line); continue
    if inv and line and ind(line)==0 and line.endswith(':'):
        for k,v in r.items():
            if not seen[k]: out.append(f'  {k}: {v}')
        inv=False
    if inv and ':' in line and ind(line)>0:
        k=line.strip().split(':',1)[0]
        if k in r: out.append(' '*ind(line)+k+': '+r[k]); seen[k]=True; continue
    out.append(line)
if inv:
    for k,v in r.items():
        if not seen[k]: out.append(f'  {k}: {v}')
dst.parent.mkdir(parents=True,exist_ok=True); dst.write_text('\n'.join(out)+'\n',encoding='utf-8')
PY
if grep -Eq '^neural_enabled=(1|true|yes|on)$' "$PROFILE"; then layer="${NOUGAT_PS3_NEURAL_LAYER:-}"; if [[ -n "$layer" && -e "$layer" ]] || [[ -e "$HOME/.local/share/nougat-media-plus/ps3-neural/libnougat_ps3_neural.so" ]] || [[ -e "$HOME/.local/share/nougat-media-plus/ps3-neural/VkLayer_nougat_ps3_neural.json" ]]; then export NOUGAT_PS3_NEURAL=1 NOUGAT_PS3_NEURAL_PROFILE="$PROFILE"; fi; fi
exec "$real" --no-gui --config "$override" "$@"
fi
exec "$real" --no-gui "$@"
