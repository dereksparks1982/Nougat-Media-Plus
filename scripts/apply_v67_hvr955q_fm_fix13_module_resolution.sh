#!/bin/bash

main() {
    PROJECT="/home/dereksparks1982/DKLab/Projects/Nougat Play Portal"
    KREL="$(uname -r)"
    UPDATE_ROOT="/lib/modules/$KREL/updates"
    UPDATE_DIR="$UPDATE_ROOT/nougat-v67-fm"
    ROOT_EXE="$PROJECT/Nougat_Play_Portal_v67"
    ICON="$PROJECT/assets/branding/nougat-play-portal-dock-N.png"
    DESKTOP_DST="$HOME/.local/share/applications/com.elderredsoftworks.NougatPlayPortal.desktop"
    TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
    ARCHIVE="/home/dereksparks1982/DKLab/Archives/Nougat Play Portal/v67-fm-fix13-module-backups-$TIMESTAMP"

    echo "============================================================"
    echo "Nougat Play Portal v0.0.67"
    echo "HVR-955Q FM FIX13 MODULE-RESOLUTION CONTINUATION"
    echo "Kernel: $KREL"
    echo "============================================================"
    echo "No kernel modules will be unloaded or reloaded."
    echo "No driver or app rebuild will run."
    echo "Git/GitHub untouched."

    [[ -d "$PROJECT" ]] || {
        echo "FAIL: project not found: $PROJECT"
        return 1
    }

    [[ -d "$UPDATE_DIR" ]] || {
        echo "FAIL: FIX12 driver directory is missing: $UPDATE_DIR"
        return 1
    }

    echo
    echo "=== VERIFY FIX12 MATCHED MODULES ARE PRESENT ==="
    local ko vm
    for ko in cx231xx.ko cx231xx-dvb.ko cx231xx-alsa.ko si2157.ko; do
        [[ -f "$UPDATE_DIR/$ko" ]] || {
            echo "FAIL: missing installed FIX12 module: $UPDATE_DIR/$ko"
            return 1
        }
        vm="$(modinfo -F vermagic "$UPDATE_DIR/$ko" 2>/dev/null)"
        echo "$ko: $vm"
        [[ "$vm" == "$KREL"* ]] || {
            echo "FAIL: $ko does not match the running kernel."
            return 1
        }
    done

    echo
    echo "=== MOVE OLD NOUGAT DRIVER BACKUPS OUT OF THE MODULE TREE ==="
    mkdir -p "$ARCHIVE" || return 1

    local old found=0
    while IFS= read -r -d '' old; do
        found=1
        echo "Archiving: $old"
        sudo mv "$old" "$ARCHIVE/" || {
            echo "FAIL: could not move old module backup out of $UPDATE_ROOT"
            return 1
        }
    done < <(
        find "$UPDATE_ROOT" -maxdepth 1 -mindepth 1 -type d \
            -name 'nougat-v67-fm.*' -print0 2>/dev/null
    )

    if [[ "$found" -eq 0 ]]; then
        echo "INFO: no sibling Nougat FM backup directories remained."
    else
        echo "PASS: old Nougat FM backups are outside /lib/modules."
    fi

    echo
    echo "=== REBUILD MODULE DEPENDENCY INDEX ==="
    sudo depmod -a "$KREL" || {
        echo "FAIL: depmod failed."
        return 1
    }

    echo
    echo "=== VERIFY ALL FOUR MODULE NAMES RESOLVE TO THE ACTIVE FIX12 DIRECTORY ==="
    local mod path
    for mod in cx231xx cx231xx_dvb cx231xx_alsa si2157; do
        path="$(modinfo -n "$mod" 2>/dev/null)"
        echo "$mod -> $path"
        case "$path" in
            "$UPDATE_DIR"/*) ;;
            *)
                echo "FAIL: $mod still does not resolve to $UPDATE_DIR"
                return 1
                ;;
        esac
    done

    echo
    echo "=== REGENERATE INITRAMFS WITH CORRECT MODULE RESOLUTION ==="
    if command -v update-initramfs >/dev/null 2>&1; then
        sudo update-initramfs -u -k "$KREL" || {
            echo "FAIL: initramfs regeneration failed."
            return 1
        }
    fi

    echo
    echo "=== VERIFY INSTALLED v67 APP + N ICON ==="
    [[ -x "$ROOT_EXE" ]] || {
        echo "FAIL: v67 root executable is missing: $ROOT_EXE"
        return 1
    }

    local VERSION
    VERSION="$("$ROOT_EXE" --version 2>&1)"
    [[ "$VERSION" == *"v0.0.67"* ]] || {
        echo "FAIL: installed app identity is wrong: $VERSION"
        return 1
    }

    strings "$ROOT_EXE" | grep -Fq "HVR-955Q FM receiving" || {
        echo "FAIL: installed v67 executable does not contain the native HVR FM path."
        return 1
    }

    [[ -f "$ICON" ]] || {
        echo "FAIL: approved Nougat N icon is missing: $ICON"
        return 1
    }

    [[ -f "$DESKTOP_DST" ]] || {
        echo "FAIL: Nougat desktop launcher is missing: $DESKTOP_DST"
        return 1
    }

    grep -Fq "Icon=$ICON" "$DESKTOP_DST" || {
        echo "FAIL: desktop launcher is not using the approved Nougat N icon."
        return 1
    }

    if command -v gio >/dev/null 2>&1; then
        gio set "$ROOT_EXE" metadata::custom-icon \
            "file:///home/dereksparks1982/DKLab/Projects/Nougat%20Media%20Plus/assets/branding/nougat-play-portal-dock-N.png" \
            >/dev/null 2>&1 || true
    fi

    echo
    echo "============================================================"
    echo "PASS: FIX13 MODULE RESOLUTION CONTINUATION COMPLETE"
    echo "Active driver: $UPDATE_DIR"
    echo "App:           $ROOT_EXE"
    echo "Identity:      $VERSION"
    echo "Old drivers:   archived outside /lib/modules"
    echo "Git/GitHub:    untouched"
    echo "A NORMAL REBOOT IS NOW REQUIRED for the patched FM stack to take over."
    echo "============================================================"
    return 0
}

main
