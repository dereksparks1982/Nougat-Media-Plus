#!/bin/bash

main() {
    PROJECT="$HOME/DKLab/Projects/Nougat Media Suite"
    # v67 HVR FM kernel build dependencies include libdw-dev
    KREL="$(uname -r)"
    KBASE="${KREL%%-*}"
    LOCALVERSION="${KREL#${KBASE}}"
    BUILDROOT="$HOME/DKLab/Build/Nougat_v67_hvr955q_fm_$KREL"
    KSRC="$BUILDROOT/linux-source-$KBASE"
    ARCHIVE="$HOME/DKLab/Archives/Nougat Media Suite/v67-hvr955q-fm-preinstall-$(date +%Y%m%d-%H%M%S)"
    UPDATE_DIR="/lib/modules/$KREL/updates/nougat-v67-fm"

    [[ -d "$PROJECT" ]] || {
        echo "FAIL: project not found: $PROJECT"
        return 1
    }

    [[ -f "$PROJECT/components/radio/kernel/hvr955q-fm/patch_kernel_source.py" ]] || {
        echo "FAIL: v67 FM driver package is not applied to the project."
        return 1
    }

    if [[ "$KREL" != 7.0.0-* ]]; then
        echo "FAIL: this candidate is locked to the 7.0.0 Ubuntu kernel family."
        echo "Current kernel: $KREL"
        return 1
    fi

    if ! lsusb | grep -q '2040:b123'; then
        echo "FAIL: Hauppauge WinTV-HVR-955Q 2040:b123 is not connected."
        return 1
    fi

    echo "=== STOP ONLY NOUGAT v67 IF IT IS RUNNING ==="
    for pid in $(pgrep -f 'Nougat_Media_Plus_v67' 2>/dev/null); do
        exe="$(readlink -f "/proc/$pid/exe" 2>/dev/null)"
        if [[ "$exe" == "$PROJECT/Nougat_Media_Plus_v67" ]]; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    echo
    echo "=== CHECK HVR TUNER OWNERSHIP ==="
    BUSY="$(fuser /dev/video0 /dev/vbi0 /dev/dvb/adapter0/* /dev/radio0 2>/dev/null || true)"
    if [[ -n "$BUSY" ]]; then
        echo "FAIL: the HVR tuner is owned by another process."
        echo "PIDs:$BUSY"
        echo "Close that process and rerun. Nothing has been replaced."
        return 1
    fi

    echo
    echo "=== ARCHIVE CURRENT DRIVER MODULES ==="
    mkdir -p "$ARCHIVE/modules" "$BUILDROOT" || return 1

    for mod in cx231xx cx231xx_dvb cx231xx_alsa si2157; do
        path="$(modinfo -n "$mod" 2>/dev/null || true)"
        if [[ -n "$path" && -f "$path" ]]; then
            mkdir -p "$ARCHIVE/modules/$(dirname "${path#/}")"
            cp -a "$path" "$ARCHIVE/modules/${path#/}" || return 1
        fi
    done

    echo
    echo "=== LOCATE UBUNTU KERNEL SOURCE ==="
    SOURCE_TAR=""
    for candidate in \
        "/usr/src/linux-source-$KBASE.tar.xz" \
        "/usr/src/linux-source-$KBASE.tar.bz2" \
        "/usr/src/linux-source-$KBASE.tar.gz"; do
        if [[ -f "$candidate" ]]; then
            SOURCE_TAR="$candidate"
            break
        fi
    done

    if [[ -z "$SOURCE_TAR" ]]; then
        echo "Installing linux-source-$KBASE..."
        sudo apt-get install -y "linux-source-$KBASE" || return 1

        for candidate in \
            "/usr/src/linux-source-$KBASE.tar.xz" \
            "/usr/src/linux-source-$KBASE.tar.bz2" \
            "/usr/src/linux-source-$KBASE.tar.gz"; do
            if [[ -f "$candidate" ]]; then
                SOURCE_TAR="$candidate"
                break
            fi
        done
    fi

    [[ -n "$SOURCE_TAR" ]] || {
        echo "FAIL: Ubuntu kernel source archive not found."
        return 1
    }

    echo "Kernel source: $SOURCE_TAR"

    if [[ ! -f "$KSRC/Makefile" ]]; then
        rm -rf "$KSRC"
        mkdir -p "$KSRC" || return 1
        tar -xf "$SOURCE_TAR" -C "$KSRC" --strip-components=1 || return 1
    fi

    echo
    echo "=== PATCH HVR-955Q FM DRIVER SOURCE ==="
    python3 \
        "$PROJECT/components/radio/kernel/hvr955q-fm/patch_kernel_source.py" \
        "$KSRC" || return 1

    echo
    echo "=== PREPARE EXACT RUNNING-KERNEL EXTERNAL MODULE BUILD ==="

    KBUILD="/lib/modules/$KREL/build"

    [[ -d "$KBUILD" && -f "$KBUILD/Makefile" ]] || {
        echo "FAIL: running-kernel headers are unavailable:"
        echo "$KBUILD"
        return 1
    }

    HEADER_RELEASE="$(
        cat "$KBUILD/include/config/kernel.release" 2>/dev/null
    )"

    if [[ -z "$HEADER_RELEASE" ]]; then
        HEADER_RELEASE="$(
            make -s -C "$KBUILD" kernelrelease
        )" || return 1
    fi

    echo "Running kernel: $KREL"
    echo "Header ABI:     $HEADER_RELEASE"

    if [[ "$HEADER_RELEASE" != "$KREL" ]]; then
        echo "FAIL: installed header ABI does not match running kernel."
        return 1
    fi

    EXTROOT="$BUILDROOT/external-modules"

    rm -rf "$EXTROOT"
    mkdir -p         "$EXTROOT/si2157"         "$EXTROOT/cx231xx" || return 1

    echo
    echo "=== PREPARE PATCHED SI2157 EXTERNAL MODULE ==="

    cp         "$KSRC/drivers/media/tuners/si2157.c"         "$EXTROOT/si2157/" || return 1

    cp         "$KSRC/drivers/media/tuners/si2157_priv.h"         "$EXTROOT/si2157/" || return 1

    cp \
        "$KSRC/drivers/media/tuners/si2157.h" \
        "$EXTROOT/si2157/" || return 1

    cat > "$EXTROOT/si2157/Makefile" <<EOF
obj-m += si2157.o
ccflags-y += -I$KSRC/drivers/media/dvb-frontends
EOF

    echo
    echo "=== PREPARE PATCHED CX231XX EXTERNAL MODULES ==="

    cp -a         "$KSRC/drivers/media/usb/cx231xx/."         "$EXTROOT/cx231xx/" || return 1

    cat > "$EXTROOT/cx231xx/Makefile" <<EOF
cx231xx-y += cx231xx-video.o
cx231xx-y += cx231xx-i2c.o
cx231xx-y += cx231xx-cards.o
cx231xx-y += cx231xx-core.o
cx231xx-y += cx231xx-avcore.o
cx231xx-y += cx231xx-417.o
cx231xx-y += cx231xx-pcb-cfg.o
cx231xx-y += cx231xx-vbi.o
cx231xx-\$(CONFIG_VIDEO_CX231XX_RC) += cx231xx-input.o

cx231xx-alsa-objs := cx231xx-audio.o

obj-m += cx231xx.o
obj-m += cx231xx-alsa.o
obj-m += cx231xx-dvb.o

ccflags-y += -I$KSRC/drivers/media/tuners
ccflags-y += -I$KSRC/drivers/media/dvb-frontends
EOF

    echo
    echo "=== BUILD PATCHED SI2157 AGAINST $KREL HEADERS ==="

    make         -C "$KBUILD"         M="$EXTROOT/si2157"         -j"$(nproc)"         modules || return 1

    echo
    echo "=== BUILD PATCHED CX231XX AGAINST $KREL HEADERS ==="

    make         -C "$KBUILD"         M="$EXTROOT/cx231xx"         -j"$(nproc)"         modules || return 1

    SI2157_KO="$EXTROOT/si2157/si2157.ko"
    CX231XX_KO="$EXTROOT/cx231xx/cx231xx.ko"
    DVB_KO="$EXTROOT/cx231xx/cx231xx-dvb.ko"
    ALSA_KO="$EXTROOT/cx231xx/cx231xx-alsa.ko"

    echo
    echo "=== VERIFY MODULE VERMAGIC ==="
    for ko in "$SI2157_KO" "$CX231XX_KO" "$DVB_KO" "$ALSA_KO"; do
        [[ -f "$ko" ]] || {
            echo "FAIL: expected module missing: $ko"
            return 1
        }

        VM="$(modinfo -F vermagic "$ko" 2>/dev/null || true)"
        echo "$(basename "$ko"): $VM"

        [[ "$VM" == "$KREL"* ]] || {
            echo "FAIL: module vermagic does not match $KREL."
            return 1
        }
    done

    echo
    echo "=== INSTALL NOUGAT-MANAGED DRIVER OVERRIDES ==="
    sudo rm -rf "$UPDATE_DIR" || return 1
    sudo mkdir -p "$UPDATE_DIR" || return 1
    sudo cp \
        "$SI2157_KO" \
        "$CX231XX_KO" \
        "$DVB_KO" \
        "$ALSA_KO" \
        "$UPDATE_DIR/" || return 1

    sudo depmod -a "$KREL" || return 1

    echo
    echo "=== RELOAD HVR DRIVER STACK ==="
    sudo modprobe -r cx231xx_dvb cx231xx_alsa 2>/dev/null || true
    sudo modprobe -r cx231xx 2>/dev/null || true
    sudo modprobe -r si2157 2>/dev/null || true

    sudo modprobe cx231xx || return 1
    sudo modprobe cx231xx_alsa || return 1
    sudo modprobe cx231xx_dvb || return 1

    sleep 3

    echo
    echo "=== REQUIRE REAL RADIO NODE ==="
    RADIO_NODE="$(ls /dev/radio* 2>/dev/null | head -n1)"

    if [[ -z "$RADIO_NODE" ]]; then
        echo "FAIL: patched driver loaded but no /dev/radio* node appeared."
        echo "Nougat will NOT be rebuilt and presented as a working FM candidate."
        return 1
    fi

    echo "Radio node: $RADIO_NODE"

    if command -v v4l2-ctl >/dev/null 2>&1; then
        v4l2-ctl -d "$RADIO_NODE" --all || return 1
    fi

    echo
    echo "=== APPLY NATIVE HVR RADIO BACKEND ==="
    python3 \
        "$PROJECT/scripts/patch_nougat_radio_backend.py" \
        "$PROJECT" || return 1

    echo
    echo "=== BUILD NOUGAT MEDIA PLUS v67 ==="
    cmake -S "$PROJECT" -B "$PROJECT/build" || return 1
    cmake --build "$PROJECT/build" \
        --target Nougat_Media_Plus_v67 \
        -j"$(nproc)" || return 1

    BUILT="$PROJECT/build/Nougat_Media_Plus_v67"
    ROOT_EXE="$PROJECT/Nougat_Media_Plus_v67"

    [[ -x "$BUILT" ]] || {
        echo "FAIL: v67 application executable was not produced."
        return 1
    }

    cp -a "$BUILT" "$ROOT_EXE" || return 1
    chmod +x "$ROOT_EXE" || return 1

    # Rejected/old root executables do not belong beside the active v67 build.
    rm -f "$PROJECT/Nougat_Media_Plus_v66"

    if command -v gio >/dev/null 2>&1; then
        gio set -t string \
            "$ROOT_EXE" \
            metadata::custom-icon-name \
            nougat-media-plus || true
    fi

    echo
    echo "=== VERIFY v67 IDENTITY ==="
    VERSION="$("$ROOT_EXE" --version 2>&1)" || return 1
    printf '%s\n' "$VERSION"

    grep -Fq 'v0.0.67' <<<"$VERSION" || {
        echo "FAIL: executable does not identify itself as v0.0.67."
        return 1
    }

    echo
    echo "=== VERIFY RADIO NODE SURVIVED APPLICATION BUILD ==="
    ls -l "$RADIO_NODE" || return 1

    echo
    echo "=== DRIVER LOG TAIL ==="
    sudo dmesg |
        grep -Ei 'cx231xx|cx25840|si2157|radio' |
        tail -n 100

    echo
    echo "============================================================"
    echo "PASS: v67 HVR-955Q FM DRIVER CANDIDATE BUILT AND APPLIED"
    echo "Radio node: $RADIO_NODE"
    echo "Executable: $ROOT_EXE"
    echo "Approved icon: nougat-media-plus"
    echo "Old v66 executable absent from project root."
    echo "Git/GitHub untouched."
    echo "Driver archive: $ARCHIVE"
    echo "============================================================"
}

main
