#!/bin/bash
BASE="$(cd "$(dirname "$0")/.." && pwd)"
python3 "$BASE/installer/nougat_v50_installer.py" --source-root "$BASE"
