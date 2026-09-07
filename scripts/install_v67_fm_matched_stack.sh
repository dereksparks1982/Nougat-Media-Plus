#!/bin/bash

cleanup_alias() {
    local alias_path="$1"
    if [[ -n "$alias_path" ]] && mountpoint -q "$alias_path" 2>/dev/null; then
        sudo umount "$alias_path" 2>/dev/null || true
    fi
    rmdir "$alias_path" 2>/dev/null || true
}

main() {
    PROJECT="/home/dereksparks1982/DKLab/Projects/Nougat Media Plus"
    KREL="$(uname -r)"
    KBUILD="/lib/modules/$KREL/build"
    BUILDROOT="$PROJECT/build/v67-hvr955q-fm-$KREL"
    EXACTSRC="$BUILDROOT/clean-exact-source/linux-7.0.0"
    SRCROOT="$BUILDROOT/exact-ubuntu-source"
    MATCHED="$BUILDROOT/external-modules-matched"
    ALIAS="/tmp/nougat-v67-fm-matched-$$"
    UPDATE_DIR="/lib/modules/$KREL/updates/nougat-v67-fm"
    PATCHER="$PROJECT/components/radio/kernel/hvr955q-fm/patch_kernel_source_v4.py"
    LOG="$PROJECT/logs/v67-fm-fix8-$(date +%Y%m%d-%H%M%S).log"

    [[ -d "$PROJECT" ]] || {
        echo "FAIL: project not found: $PROJECT"
        return 1
    }
    [[ -d "$KBUILD" && -f "$KBUILD/Makefile" ]] || {
        echo "FAIL: running-kernel headers are missing: $KBUILD"
        return 1
    }
    [[ -f "$PATCHER" ]] || {
        echo "FAIL: matched-stack kernel patcher is missing."
        return 1
    }

    mkdir -p "$PROJECT/logs" "$BUILDROOT" || return 1

    echo "============================================================"
    echo "Nougat Media Plus v0.0.67 HVR-955Q FM MATCHED STACK FIX8"
    echo "Kernel: $KREL"
    echo "============================================================"
    echo
    echo "No live HVR/audio/media modules will be unloaded."
    echo "CMake will not run."
    echo

    if [[ ! -f "$EXACTSRC/Makefile" ]]; then
        local source_pkg source_ver version_no_epoch dsc changelog extracted_ver
        source_pkg="$(dpkg-query -W -f='${source:Package}' "linux-headers-$KREL" 2>/dev/null)"
        source_ver="$(dpkg-query -W -f='${source:Version}' "linux-headers-$KREL" 2>/dev/null)"
        version_no_epoch="${source_ver#*:}"
        dsc="$SRCROOT/${source_pkg}_${version_no_epoch}.dsc"

        [[ -f "$dsc" ]] || {
            echo "FAIL: exact Ubuntu source archive from the earlier v67 build is missing:"
            echo "$dsc"
            return 1
        }

        echo "=== RESTORE CLEAN EXACT UBUNTU SOURCE ==="
        rm -rf "$BUILDROOT/clean-exact-source"
        mkdir -p "$BUILDROOT/clean-exact-source" || return 1

        if ! dpkg-source -x "$dsc" "$EXACTSRC" >"$LOG" 2>&1; then
            echo "FAIL: exact Ubuntu source extraction failed."
            echo "Log: $LOG"
            return 1
        fi

        changelog="$EXACTSRC/debian/changelog"
        [[ -f "$changelog" ]] || changelog="$EXACTSRC/debian.master/changelog"
        extracted_ver="$(dpkg-parsechangelog -l"$changelog" -S Version 2>/dev/null)"
        [[ "$extracted_ver" == "$source_ver" ]] || {
            echo "FAIL: extracted source version mismatch."
            echo "Expected: $source_ver"
            echo "Found:    $extracted_ver"
            return 1
        }
        echo "PASS: exact Ubuntu source restored: $extracted_ver"
    else
        echo "PASS: existing exact Ubuntu v67 source tree found."
    fi

    echo
    echo "=== APPLY ACTUAL FM DRIVER PATCH ==="
    python3 "$PATCHER" "$EXACTSRC" || return 1

    rm -rf "$MATCHED"
    mkdir -p "$MATCHED" "$ALIAS" || return 1

    sudo mount --bind "$BUILDROOT" "$ALIAS" || {
        rmdir "$ALIAS" 2>/dev/null || true
        echo "FAIL: could not create temporary no-space Kbuild alias."
        return 1
    }
    trap 'cleanup_alias "$ALIAS"' EXIT

    EXACT_ALIAS="$ALIAS/clean-exact-source/linux-7.0.0"
    MATCHED_ALIAS="$ALIAS/external-modules-matched"

    cp -a "$EXACTSRC/drivers/media/usb/cx231xx/." "$MATCHED/" || return 1
    cp "$EXACTSRC/drivers/media/tuners/si2157.c" "$MATCHED/" || return 1
    cp "$EXACTSRC/drivers/media/tuners/si2157.h" "$MATCHED/" || return 1
    cp "$EXACTSRC/drivers/media/tuners/si2157_priv.h" "$MATCHED/" || return 1

    cat > "$MATCHED/Makefile" <<EOF
cx231xx-y += cx231xx-video.o cx231xx-i2c.o cx231xx-cards.o cx231xx-core.o
cx231xx-y += cx231xx-avcore.o cx231xx-417.o cx231xx-pcb-cfg.o cx231xx-vbi.o
cx231xx-\$(CONFIG_VIDEO_CX231XX_RC) += cx231xx-input.o
cx231xx-alsa-objs := cx231xx-audio.o

obj-m += cx231xx.o
obj-m += cx231xx-alsa.o
obj-m += cx231xx-dvb.o
obj-m += si2157.o

ccflags-y += -I$EXACT_ALIAS/drivers/media/tuners
ccflags-y += -I$EXACT_ALIAS/drivers/media/dvb-frontends
EOF

    echo
    echo "=== BUILD MATCHED FOUR-MODULE MEDIA STACK ==="
    nice -n 10 make -C "$KBUILD" M="$MATCHED_ALIAS" -j2 modules || return 1

    local cx dvb alsa si ko vm
    cx="$MATCHED/cx231xx.ko"
    dvb="$MATCHED/cx231xx-dvb.ko"
    alsa="$MATCHED/cx231xx-alsa.ko"
    si="$MATCHED/si2157.ko"

    echo
    echo "=== VERIFY MODULE ABI ==="
    for ko in "$cx" "$dvb" "$alsa" "$si"; do
        [[ -f "$ko" ]] || {
            echo "FAIL: missing built module: $ko"
            return 1
        }
        vm="$(modinfo -F vermagic "$ko" 2>/dev/null)"
        echo "$(basename "$ko"): $vm"
        [[ "$vm" == "$KREL"* ]] || {
            echo "FAIL: $(basename "$ko") does not match $KREL"
            return 1
        }
    done

    echo
    echo "=== SECURE BOOT CHECK ==="
    if command -v mokutil >/dev/null 2>&1 &&
       mokutil --sb-state 2>/dev/null | grep -qi 'SecureBoot enabled'; then
        local sign_file mok_priv mok_der
        sign_file="$KBUILD/scripts/sign-file"
        mok_priv="/var/lib/shim-signed/mok/MOK.priv"
        mok_der="/var/lib/shim-signed/mok/MOK.der"

        [[ -x "$sign_file" && -f "$mok_priv" && -f "$mok_der" ]] || {
            echo "FAIL: Secure Boot is enabled but the enrolled Ubuntu MOK key was not found."
            return 1
        }

        for ko in "$cx" "$dvb" "$alsa" "$si"; do
            sudo "$sign_file" sha256 "$mok_priv" "$mok_der" "$ko" || return 1
        done
        echo "PASS: all four matched modules signed."
    else
        echo "PASS: Secure Boot module signing not required."
    fi

    echo
    echo "=== INSTALL MATCHED FM STACK FOR NEXT BOOT ==="
    sudo rm -rf "$UPDATE_DIR" || return 1
    sudo mkdir -p "$UPDATE_DIR" || return 1
    sudo cp "$cx" "$dvb" "$alsa" "$si" "$UPDATE_DIR/" || return 1
    sudo depmod -a "$KREL" || return 1

    echo
    echo "=== VERIFY NEXT-BOOT MODULE RESOLUTION ==="
    local path
    for mod in cx231xx cx231xx_dvb cx231xx_alsa si2157; do
        path="$(modinfo -n "$mod" 2>/dev/null)"
        echo "$mod -> $path"
        case "$path" in
            "$UPDATE_DIR"/*) ;;
            *)
                echo "FAIL: $mod is not resolving to the Nougat matched stack."
                return 1
                ;;
        esac
    done

    echo
    echo "=== UPDATE NEXT-BOOT IMAGE ==="
    if command -v update-initramfs >/dev/null 2>&1; then
        sudo update-initramfs -u -k "$KREL" || return 1
        echo "PASS: initramfs updated for $KREL."
    else
        echo "PASS: /lib/modules override installed; no update-initramfs command is present."
    fi

    ROOT_EXE="$PROJECT/Nougat_Media_Plus_v67"
    if [[ -x "$ROOT_EXE" ]]; then
        if strings "$ROOT_EXE" | grep -Fq "HVR-955Q FM receiving"; then
            echo "PASS: v67 executable contains the native HVR FM receive path."
        else
            echo "WARNING: v67 executable does not contain the expected HVR FM marker."
        fi
    fi

    echo
    echo "============================================================"
    echo "PASS: MATCHED FM STACK INSTALLED"
    echo "cx231xx + cx231xx-dvb + cx231xx-alsa + si2157 are one build set."
    echo "No live module unload. No CMake. Git/GitHub untouched."
    echo
    echo "A NORMAL REBOOT is required before this driver can take over."
    echo "============================================================"
    return 0
}

main
