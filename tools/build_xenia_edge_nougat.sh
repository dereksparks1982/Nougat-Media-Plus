#!/usr/bin/env bash
set -euo pipefail

PROJECT="${1:-$PWD}"
PROJECT="$(cd "$PROJECT" && pwd)"
RUNTIME="$PROJECT/components/games/runtime/xenia"
SRC="$RUNTIME/xenia-edge-nougat-src-7be830a"
PATCHER="$PROJECT/tools/patch_xenia_edge_for_nougat.py"
OUT="$RUNTIME/xenia_edge_nougat"
TAG="7be830a"
REPO="https://github.com/has207/xenia-edge.git"

echo "=== NOUGAT v0.0.53 CUSTOM XENIA EDGE BUILD ==="
echo

if [ ! -f "$PATCHER" ]; then
    echo "STOP: patch_xenia_edge_for_nougat.py is missing."
    exit 1
fi

need_packages=0
for cmd in git cmake ninja python3 clang-21 clang++-21; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        need_packages=1
    fi
done

if [ "$need_packages" -ne 0 ]; then
    echo "Installing Xenia Edge's documented Linux build dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential git cmake ninja-build \
        mesa-vulkan-drivers libc++-dev libc++abi-dev liblz4-dev libvulkan-dev \
        clang-21 llvm-21 \
        libgtk-3-dev libx11-xcb-dev libfontconfig1-dev libxtst-dev \
        libasound2-dev libpulse-dev libudev-dev libdbus-1-dev \
        libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev \
        libxi-dev libxss-dev libxkbcommon-dev libxfixes-dev
fi

mkdir -p "$RUNTIME"

if [ ! -d "$SRC/.git" ]; then
    echo
    echo "--- Downloading exact Xenia Edge source $TAG ---"
    git clone --branch "$TAG" --single-branch "$REPO" "$SRC"
else
    echo
    echo "--- Existing dedicated Xenia Edge source found ---"
    current="$(git -C "$SRC" rev-parse --short=7 HEAD 2>/dev/null || true)"
    if [ "$current" != "$TAG" ]; then
        echo "STOP: dedicated source directory is at $current, expected $TAG."
        echo "No files were deleted or reset."
        exit 1
    fi
fi

cd "$SRC"

echo
echo "--- Initializing Xenia Edge Linux dependencies ---"
git submodule update --init --depth=1 -j"$(getconf _NPROCESSORS_ONLN)" \
    $(grep -oP '(?<=path = )(?!third_party/DirectXShaderCompiler).+' .gitmodules)

./xenia-build.py slang
./xenia-build.py fetchdata

echo
echo "--- Applying Nougat source-level embedding patch ---"
python3 "$PATCHER" "$SRC"

echo
echo "--- Building native Xenia Edge for Nougat ---"
export CC=clang-21
export CXX=clang++-21
./xenia-build.py build --config=Release

BUILT="$SRC/build/bin/Linux/Release/xenia_edge"
if [ ! -x "$BUILT" ]; then
    echo "STOP: Xenia Edge build did not produce $BUILT"
    exit 1
fi

size="$(stat -c%s "$BUILT")"
if [ "$size" -le 100000 ]; then
    echo "STOP: built Xenia Edge binary is unexpectedly small: $size bytes"
    exit 1
fi

cp -f "$BUILT" "$OUT"
chmod +x "$OUT"
chmod +x "$RUNTIME/xenia_canary"

if ! strings "$OUT" | grep -q "NOUGAT_EMBED_MODE"; then
    echo "STOP: validation marker is missing from custom Xenia Edge binary."
    exit 1
fi

if ldd "$OUT" 2>/dev/null | grep -q "not found"; then
    echo "STOP: custom Xenia Edge has unresolved shared libraries:"
    ldd "$OUT" | grep "not found"
    exit 1
fi

echo
echo "=== CUSTOM XENIA EDGE BUILD READY ==="
echo "Binary: $OUT"
echo "Source: $TAG"
echo "Mode:   map -> Nougat reparent -> Vulkan/XCB surface"
