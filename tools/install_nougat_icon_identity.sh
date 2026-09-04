#!/usr/bin/env bash

PROJECT="${1:-$HOME/DKLab/Projects/Nougat Media Suite}"
EXE="${2:-$PROJECT/Nougat_Media_Suite_v60}"
MASTER="$PROJECT/assets/branding/nougat-media-suite-v51-master-N.png"
ICONS="$PROJECT/assets/icons"
THEME="$HOME/.local/share/icons/hicolor"
STABLE="$HOME/.local/share/icons/nougat-media-suite/Nougat-N.png"

if [ ! -f "$MASTER" ]; then
    echo "STOP: approved Nougat N is missing."
    return 1 2>/dev/null || false
fi

mkdir -p "$(dirname "$STABLE")"
cp -a "$MASTER" "$STABLE"

for source in "$ICONS"/nougat-media-suite-v51-*.png; do
    [ -f "$source" ] || continue
    name="$(basename "$source" .png)"
    size="${name##*-}"

    case "$size" in
        *[!0-9]*|'') continue ;;
    esac

    mkdir -p "$THEME/${size}x${size}/apps"
    cp -a "$source" "$THEME/${size}x${size}/apps/nougat-media-suite.png"
done

mkdir -p "$HOME/.local/share/applications"
cp -a \
    "$PROJECT/com.elderredsoftworks.NougatMediaSuite.desktop" \
    "$HOME/.local/share/applications/com.elderredsoftworks.NougatMediaSuite.desktop"

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
    gio set "$EXE" metadata::custom-icon-name nougat-media-suite >/dev/null 2>&1
fi

echo "PASS: approved Nougat N identity installed."
