#!/usr/bin/env bash

PROJECT="${1:-$HOME/DKLab/Projects/Nougat Play Portal}"
EXE="${2:-$PROJECT/Nougat_Play_Portal_v60}"
MASTER="$PROJECT/assets/branding/nougat-play-portal-v51-master-N.png"
ICONS="$PROJECT/assets/icons"
THEME="$HOME/.local/share/icons/hicolor"
STABLE="$HOME/.local/share/icons/nougat-play-portal/Nougat-N.png"

if [ ! -f "$MASTER" ]; then
    echo "STOP: approved Nougat N is missing."
    return 1 2>/dev/null || false
fi

mkdir -p "$(dirname "$STABLE")"
cp -a "$MASTER" "$STABLE"

for source in "$ICONS"/nougat-play-portal-v51-*.png; do
    [ -f "$source" ] || continue
    name="$(basename "$source" .png)"
    size="${name##*-}"

    case "$size" in
        *[!0-9]*|'') continue ;;
    esac

    mkdir -p "$THEME/${size}x${size}/apps"
    cp -a "$source" "$THEME/${size}x${size}/apps/nougat-play-portal.png"
done

mkdir -p "$HOME/.local/share/applications"
cp -a \
    "$PROJECT/com.elderredsoftworks.NougatPlayPortal.desktop" \
    "$HOME/.local/share/applications/com.elderredsoftworks.NougatPlayPortal.desktop"

command -v gtk-update-icon-cache >/dev/null 2>&1 &&
    gtk-update-icon-cache -f -t "$THEME" >/dev/null 2>&1

command -v update-desktop-database >/dev/null 2>&1 &&
    update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1

if command -v gio >/dev/null 2>&1 && [ -e "$EXE" ]; then
    URI="$(python3 - "$STABLE" <<'PY'
from pathlib import Path
import sys
print(Path(sys.argv[1]).resolve().as_uri())
PY
)"
    gio set "$EXE" metadata::custom-icon "$URI"
    gio set "$EXE" metadata::custom-icon-name nougat-play-portal >/dev/null 2>&1
fi

echo "PASS: approved Nougat N identity installed."
