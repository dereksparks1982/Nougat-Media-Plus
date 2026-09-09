#!/bin/bash

main() {
    PROJECT="/home/dereksparks1982/DKLab/Projects/Nougat Play Portal"
    KREL="$(uname -r)"
    UPDATE_DIR="/lib/modules/$KREL/updates/nougat-v67-fm"
    APP_PATCHER="$PROJECT/scripts/patch_nougat_radio_backend_v2.py"
    APP_BUILD="$PROJECT/build/app-v67"
    TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
    ARCHIVE="/home/dereksparks1982/DKLab/Archives/Nougat Play Portal/v67-fm-cmake-fix-$TIMESTAMP"

    [[ -d "$PROJECT" ]] || {
        echo "FAIL: Nougat project not found: $PROJECT"
        return 1
    }

    [[ -f "$APP_PATCHER" ]] || {
        echo "FAIL: v67 app patcher is missing: $APP_PATCHER"
        return 1
    }

    echo "============================================================"
    echo "Nougat Play Portal v0.0.67 FM CMake repair"
    echo "Kernel: $KREL"
    echo "============================================================"

    echo
    echo "=== VERIFY THE ALREADY-INSTALLED REBOOT-SAFE DRIVER ==="

    local cxko siko
    cxko="$UPDATE_DIR/cx231xx.ko"
    siko="$UPDATE_DIR/si2157.ko"

    [[ -f "$cxko" && -f "$siko" ]] || {
        echo "FAIL: the reboot-safe v67 FM driver files are not installed for this kernel."
        echo "Expected:"
        echo "  $cxko"
        echo "  $siko"
        echo "No kernel modules were changed."
        return 1
    }

    [[ "$(modinfo -F vermagic "$cxko" 2>/dev/null | awk '{print $1}')" == "$KREL" ]] || {
        echo "FAIL: cx231xx.ko does not match the running kernel ABI."
        return 1
    }

    [[ "$(modinfo -F vermagic "$siko" 2>/dev/null | awk '{print $1}')" == "$KREL" ]] || {
        echo "FAIL: si2157.ko does not match the running kernel ABI."
        return 1
    }

    echo "PASS: cx231xx + si2157 reboot-safe modules are already installed on disk."
    echo "PASS: this repair will NOT rebuild, unload, reload, or touch the running HVR driver stack."

    echo
    echo "=== STOP ONLY THE NOUGAT v67 ROOT EXECUTABLE ==="

    local pid exe
    for pid in $(pgrep -f 'Nougat_Play_Portal_v67' 2>/dev/null); do
        exe="$(readlink -f "/proc/$pid/exe" 2>/dev/null)"
        if [[ "$exe" == "$PROJECT/Nougat_Play_Portal_v67" ]]; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    mkdir -p "$ARCHIVE/app" || return 1

    echo
    echo "=== BACK UP APP FILES ==="

    local rel
    for rel in \
        "CMakeLists.txt" \
        "src/main.cpp" \
        "src/radio/radio_backend.cpp" \
        "src/radio/radio_backend.hpp" \
        "com.elderredsoftworks.NougatPlayPortal.desktop"; do
        if [[ -f "$PROJECT/$rel" ]]; then
            mkdir -p "$ARCHIVE/app/$(dirname "$rel")" || return 1
            cp -a "$PROJECT/$rel" "$ARCHIVE/app/$rel" || return 1
        fi
    done

    echo
    echo "=== INTEGRATE NATIVE HVR FM PROVIDER ==="

    python3 "$APP_PATCHER" "$PROJECT" || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" 2>/dev/null || true
        echo "FAIL: application backend patch failed; app source restored."
        return 1
    }

    echo
    echo "=== CREATE CLEAN, PATH-SAFE CMAKE BUILD DIRECTORY ==="
    echo "Application build: $APP_BUILD"

    # Never reuse PROJECT/build/CMakeCache.txt. That cache belongs to the old
    # Nougat Play Portal path. Only this dedicated application build directory
    # is removed, leaving the v67 kernel build/source material untouched.
    rm -rf "$APP_BUILD" || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" 2>/dev/null || true
        echo "FAIL: could not clear the dedicated v67 app build directory."
        return 1
    }
    mkdir -p "$APP_BUILD" || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" 2>/dev/null || true
        return 1
    }

    echo
    echo "=== CONFIGURE NOUGAT PLAY PORTAL v0.0.67 ==="

    cmake -S "$PROJECT" -B "$APP_BUILD" || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" 2>/dev/null || true
        echo "FAIL: CMake configure failed; app source restored."
        return 1
    }

    echo
    echo "=== BUILD NOUGAT PLAY PORTAL v0.0.67 ==="

    nice -n 10 cmake --build "$APP_BUILD" \
        --target Nougat_Play_Portal_v67 \
        --parallel 2 || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" 2>/dev/null || true
        echo "FAIL: v67 application build failed; app source restored."
        return 1
    }

    local BUILT ROOT_EXE VERSION
    BUILT="$APP_BUILD/Nougat_Play_Portal_v67"
    ROOT_EXE="$PROJECT/Nougat_Play_Portal_v67"

    [[ -x "$BUILT" ]] || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" 2>/dev/null || true
        echo "FAIL: expected v67 executable was not produced: $BUILT"
        return 1
    }

    cp -a "$BUILT" "$ROOT_EXE" || return 1
    chmod +x "$ROOT_EXE" || return 1

    VERSION="$("$ROOT_EXE" --version 2>&1)"
    [[ "$VERSION" == *"v0.0.67"* ]] || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" 2>/dev/null || true
        echo "FAIL: built executable has the wrong identity:"
        echo "$VERSION"
        return 1
    }

    rm -f "$PROJECT/Nougat_Play_Portal_v66"

    if command -v gio >/dev/null 2>&1; then
        gio set \
            "$ROOT_EXE" \
            metadata::custom-icon \
            "file:///home/dereksparks1982/DKLab/Projects/Nougat%20Media%20Plus/assets/branding/nougat-play-portal-dock-N.png" \
            >/dev/null 2>&1 || true
        gio set -t string \
            "$ROOT_EXE" \
            metadata::custom-icon-name \
            nougat-play-portal \
            >/dev/null 2>&1 || true
    fi

    echo
    echo "============================================================"
    echo "PASS: v67 application build completed"
    echo "Executable: $ROOT_EXE"
    echo "Identity:   $VERSION"
    echo "Modules:    already installed for next reboot"
    echo "Backup:     $ARCHIVE"
    echo "Git/GitHub: untouched"
    echo "============================================================"
    echo
    echo "The running HVR/media stack was not touched."
    echo "Restart the computer normally when you are ready to load the v67 FM modules."
    return 0
}

main
