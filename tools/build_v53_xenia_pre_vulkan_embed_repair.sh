#!/usr/bin/env bash
set -e

PROJECT="${1:-$PWD}"
RUNTIME="$PROJECT/components/games/runtime/xenia"
PATCHER="$PROJECT/tools/apply_v53_xenia_pre_vulkan_embed_repair.py"
SHIM_SRC="$RUNTIME/nougat_xenia_embed_preload.c"
SHIM_SO="$RUNTIME/libnougat_xenia_embed.so"
TARGET="$PROJECT/build-v53/Nougat_Media_Suite_v53"
ROOT_EXE="$PROJECT/Nougat_Media_Suite_v53"

cd "$PROJECT"

python3 "$PATCHER" "$PROJECT"

cc -shared -fPIC -O2 -Wall -Wextra \
   -o "$SHIM_SO" "$SHIM_SRC" \
   -ldl -lX11 -pthread

test -s "$SHIM_SO"
chmod +x "$RUNTIME/xenia_canary"

cmake --build build-v53 --target Nougat_Media_Suite_v53 -j4

test -x "$TARGET"
cp -f "$TARGET" "$ROOT_EXE"
chmod +x "$ROOT_EXE"

echo
echo "=== NOUGAT v0.0.53 XENIA EDGE PRE-VULKAN EMBED REPAIR BUILT ==="
echo "Runtime: Xenia Edge"
echo "Embed:   before first X11 map / before Vulkan swapchain"
echo "Binary:  $ROOT_EXE"
