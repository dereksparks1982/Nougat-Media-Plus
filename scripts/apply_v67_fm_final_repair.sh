#!/bin/bash

AUDIO_STACK_ACTIVE_UNITS=()

cleanup_build_alias() {
    local alias_path="$1"

    if [[ -n "$alias_path" ]] && mountpoint -q "$alias_path" 2>/dev/null; then
        sudo umount "$alias_path" 2>/dev/null || true
    fi

    if [[ -n "$alias_path" ]]; then
        rmdir "$alias_path" 2>/dev/null || true
    fi
}

capture_driver_log() {
    local path="$1"
    {
        echo "=== DATE ==="
        date
        echo
        echo "=== KERNEL ==="
        uname -a
        echo
        echo "=== MODULE PATHS ==="
        for mod in cx231xx cx231xx_alsa cx231xx_dvb si2157; do
            printf '%-16s ' "$mod"
            modinfo -n "$mod" 2>&1 || true
        done
        echo
        echo "=== LOADED MODULES ==="
        lsmod | grep -E 'cx231xx|cx25840|si2157|lgdt3306a' || true
        echo
        echo "=== DEVICE NODES ==="
        ls -l /dev/radio* /dev/video* /dev/vbi* 2>/dev/null || true
        find /dev/dvb -maxdepth 3 \( -type c -o -type l \) 2>/dev/null | sort || true
        echo
        echo "=== KERNEL MEDIA LOG ==="
        sudo dmesg | grep -Ei 'cx231xx|cx25840|si2157|lgdt3306|hauppauge|radio|module' | tail -n 220
    } > "$path" 2>&1
}

fetch_exact_ubuntu_source() {
    local source_pkg="$1"
    local source_ver="$2"
    local source_root="$3"
    local temp_sources="/tmp/nougat-v67-deb-src.sources"
    local installed_sources="/etc/apt/sources.list.d/nougat-v67-deb-src.sources"
    local ubuntu_sources="/etc/apt/sources.list.d/ubuntu.sources"
    local apt_ok=0

    rm -rf "$source_root"
    mkdir -p "$source_root" || return 1

    echo "=== FETCH EXACT UBUNTU SOURCE $source_pkg=$source_ver ==="

    if [[ -f "$ubuntu_sources" ]]; then
        python3 - "$ubuntu_sources" "$temp_sources" <<'PY'
from pathlib import Path
import sys

src = Path(sys.argv[1]).read_text()
out = []

for line in src.splitlines():
    if line.startswith("Types:"):
        words = line.split(":", 1)[1].split()
        if "deb" in words or "deb-src" in words:
            line = "Types: deb-src"
    out.append(line)

Path(sys.argv[2]).write_text("\n".join(out) + "\n")
PY
        [[ $? -eq 0 ]] || return 1

        sudo install -m 0644 "$temp_sources" "$installed_sources" || return 1
        rm -f "$temp_sources"

        if sudo apt-get update; then
            if (
                cd "$source_root" &&
                apt-get source --only-source "$source_pkg=$source_ver"
            ); then
                apt_ok=1
            fi
        fi

        sudo rm -f "$installed_sources" || true
    fi

    if [[ "$apt_ok" -eq 1 ]]; then
        return 0
    fi

    echo
    echo "APT source index did not provide the exact installed source."
    echo "Using the exact Launchpad source archive instead."

    command -v curl >/dev/null 2>&1 || {
        echo "FAIL: curl is required for the Launchpad source fallback."
        return 1
    }

    command -v dpkg-source >/dev/null 2>&1 || {
        echo "FAIL: dpkg-source is required for exact Ubuntu source extraction."
        return 1
    }

    local version_no_epoch="${source_ver#*:}"
    local base="https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/${source_pkg}/${version_no_epoch}"
    local dsc="${source_pkg}_${version_no_epoch}.dsc"

    rm -rf "$source_root"
    mkdir -p "$source_root" || return 1

    curl -fL --retry 3 \
        "$base/$dsc" \
        -o "$source_root/$dsc" || {
        echo "FAIL: exact Launchpad DSC could not be downloaded:"
        echo "$base/$dsc"
        return 1
    }

    local files
    files="$(
        awk '
            /^Files:/ {inside=1; next}
            inside && /^[[:space:]]+[0-9a-fA-F]{32}[[:space:]]+[0-9]+[[:space:]]+/ {
                print $3
                next
            }
            inside && !/^[[:space:]]/ {exit}
        ' "$source_root/$dsc"
    )"

    [[ -n "$files" ]] || {
        echo "FAIL: could not parse source files from $dsc"
        return 1
    }

    local f
    while IFS= read -r f; do
        [[ -n "$f" ]] || continue
        curl -fL --retry 3 \
            "$base/$f" \
            -o "$source_root/$f" || return 1
    done <<< "$files"

    (
        cd "$source_root" &&
        dpkg-source -x "$dsc"
    ) || return 1

    return 0
}

main() {
    PROJECT="$HOME/DKLab/Projects/Nougat Media Plus"
    KREL="$(uname -r)"
    KBUILD="/lib/modules/$KREL/build"

    BUILDROOT="$PROJECT/build/v67-hvr955q-fm-$KREL"
    SRCROOT="$BUILDROOT/exact-ubuntu-source"
    EXTROOT="$BUILDROOT/external-modules"

    # Linux Kbuild rejects paths containing spaces. All build files remain
    # physically under PROJECT/build; this is only a temporary path alias.
    BUILD_ALIAS="/tmp/nougat-v67-kbuild-$$"
    NOUGAT_BUILD_ALIAS="$BUILD_ALIAS"
    UPDATE_DIR="/lib/modules/$KREL/updates/nougat-v67-fm"
    TIMESTAMP="$(date +%Y%m%d-%H%M%S)"

    ARCHIVE="$HOME/DKLab/Archives/Nougat Media Plus/v67-fm-final-before-$TIMESTAMP"
    LOGDIR="$PROJECT/logs"
    DRIVER_LOG="$LOGDIR/v67-fm-driver-$TIMESTAMP.log"

    KERNEL_PATCHER="$PROJECT/components/radio/kernel/hvr955q-fm/patch_kernel_source_v3.py"
    APP_PATCHER="$PROJECT/scripts/patch_nougat_radio_backend_v2.py"

    [[ -d "$PROJECT" ]] || {
        echo "FAIL: Nougat project not found: $PROJECT"
        return 1
    }

    [[ -x "$KERNEL_PATCHER" || -f "$KERNEL_PATCHER" ]] || {
        echo "FAIL: final kernel patcher is missing."
        return 1
    }

    [[ -x "$APP_PATCHER" || -f "$APP_PATCHER" ]] || {
        echo "FAIL: final Nougat radio patcher is missing."
        return 1
    }

    [[ -d "$KBUILD" && -f "$KBUILD/Makefile" ]] || {
        echo "FAIL: headers for the running kernel are missing:"
        echo "$KBUILD"
        return 1
    }

    if lsusb | grep -q '2040:b123'; then
        echo "HVR-955Q detected. It will remain attached to the current stock driver until reboot."
    else
        echo "HVR-955Q is not currently connected. That is fine in reboot-safe build mode."
    fi

    mkdir -p "$BUILDROOT" "$ARCHIVE/app" "$LOGDIR" || return 1

    mkdir -p "$BUILD_ALIAS" || return 1
    sudo mount --bind "$BUILDROOT" "$BUILD_ALIAS" || {
        echo "FAIL: could not create the temporary Kbuild path alias."
        rmdir "$BUILD_ALIAS" 2>/dev/null || true
        return 1
    }
    trap 'cleanup_build_alias "$NOUGAT_BUILD_ALIAS"' EXIT

    echo "Build storage:  $BUILDROOT"
    echo "Kbuild alias:   $BUILD_ALIAS (temporary bind view only)"

    echo "============================================================"
    echo "Nougat Media Plus v0.0.67 HVR-955Q FM reboot-safe repair"
    echo "Kernel: $KREL"
    echo "============================================================"

    echo
    echo "=== STOP ONLY THE NOUGAT v67 ROOT EXECUTABLE ==="

    local pid exe
    for pid in $(pgrep -f 'Nougat_Media_Plus_v67' 2>/dev/null); do
        exe="$(readlink -f "/proc/$pid/exe" 2>/dev/null)"
        if [[ "$exe" == "$PROJECT/Nougat_Media_Plus_v67" ]]; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    sleep 1

    echo
    echo "=== REBOOT-SAFE MODE ==="
    echo "No running media, USB, audio, DVB, or cx231xx modules will be unloaded or reloaded."
    echo "The candidate will only be built and installed on disk for the next reboot."
    echo
    echo "=== IDENTIFY EXACT SOURCE PACKAGE FOR RUNNING KERNEL ==="

    local header_pkg source_pkg source_ver header_release
    header_pkg="linux-headers-$KREL"

    source_pkg="$(
        dpkg-query -W -f='${source:Package}' "$header_pkg" 2>/dev/null
    )"
    source_ver="$(
        dpkg-query -W -f='${source:Version}' "$header_pkg" 2>/dev/null
    )"

    [[ -n "$source_pkg" && -n "$source_ver" ]] || {
        echo "FAIL: could not resolve the installed kernel's source package."
        return 1
    }

    header_release="$(
        cat "$KBUILD/include/config/kernel.release" 2>/dev/null
    )"

    [[ "$header_release" == "$KREL" ]] || {
        echo "FAIL: running header ABI does not match uname."
        echo "uname:  $KREL"
        echo "header: $header_release"
        return 1
    }

    echo "Source package: $source_pkg"
    echo "Source version: $source_ver"
    echo "Header ABI:     $header_release"

    echo
    echo "=== PREPARE CLEAN EXACT UBUNTU SOURCE FROM LOCAL ARCHIVE ==="

    local version_no_epoch dsc clean_source_root EXACTSRC
    version_no_epoch="${source_ver#*:}"
    dsc="$SRCROOT/${source_pkg}_${version_no_epoch}.dsc"
    clean_source_root="$BUILDROOT/clean-exact-source"

    if [[ ! -f "$dsc" ]]; then
        if ! fetch_exact_ubuntu_source "$source_pkg" "$source_ver" "$SRCROOT"; then
            echo "FAIL: exact Ubuntu kernel source could not be obtained."
            return 1
        fi
        dsc="$SRCROOT/${source_pkg}_${version_no_epoch}.dsc"
    fi

    [[ -f "$dsc" ]] || {
        echo "FAIL: exact Ubuntu DSC is missing: $dsc"
        return 1
    }

    rm -rf "$clean_source_root"
    mkdir -p "$clean_source_root" || return 1

    dpkg-source -x "$dsc" "$clean_source_root/linux-7.0.0" || {
        echo "FAIL: clean exact Ubuntu source extraction failed."
        return 1
    }

    EXACTSRC="$clean_source_root/linux-7.0.0"

    [[ -f "$EXACTSRC/Makefile" ]] || {
        echo "FAIL: clean exact Ubuntu source tree was not produced."
        return 1
    }

    local changelog extracted_ver
    changelog="$EXACTSRC/debian/changelog"
    if [[ ! -f "$changelog" && -f "$EXACTSRC/debian.master/changelog" ]]; then
        changelog="$EXACTSRC/debian.master/changelog"
    fi

    [[ -f "$changelog" ]] || {
        echo "FAIL: exact Ubuntu source changelog was not found."
        return 1
    }

    extracted_ver="$(
        dpkg-parsechangelog \
            -l"$changelog" \
            -S Version \
            2>/dev/null
    )"

    [[ "$extracted_ver" == "$source_ver" ]] || {
        echo "FAIL: clean source version is not the running kernel source."
        echo "Expected: $source_ver"
        echo "Found:    $extracted_ver"
        return 1
    }

    echo "PASS: clean exact Ubuntu source verified: $extracted_ver"

    local EXACTSRC_REL EXACTSRC_BUILD EXTROOT_BUILD
    EXACTSRC_REL="${EXACTSRC#"$BUILDROOT"/}"

    [[ "$EXACTSRC_REL" != "$EXACTSRC" ]] || {
        echo "FAIL: exact source was extracted outside the Nougat project build folder."
        return 1
    }

    EXACTSRC_BUILD="$BUILD_ALIAS/$EXACTSRC_REL"
    EXTROOT_BUILD="$BUILD_ALIAS/external-modules"

    echo "Exact source:   $EXACTSRC"
    echo "Build location: $BUILDROOT"

    echo
    echo "=== APPLY FM PATCH TO EXACT UBUNTU SOURCE ==="

    python3 "$KERNEL_PATCHER" "$EXACTSRC" || return 1

    echo
    echo "=== BUILD ONLY THE TWO FM IMPLEMENTATION MODULES ==="

    rm -rf "$EXTROOT"
    mkdir -p "$EXTROOT/si2157" "$EXTROOT/cx231xx" || return 1

    cp "$EXACTSRC/drivers/media/tuners/si2157.c" "$EXTROOT/si2157/" || return 1
    cp "$EXACTSRC/drivers/media/tuners/si2157.h" "$EXTROOT/si2157/" || return 1
    cp "$EXACTSRC/drivers/media/tuners/si2157_priv.h" "$EXTROOT/si2157/" || return 1

    cat > "$EXTROOT/si2157/Makefile" <<EOF
obj-m += si2157.o
ccflags-y += -I$EXACTSRC_BUILD/drivers/media/tuners
ccflags-y += -I$EXACTSRC_BUILD/drivers/media/dvb-frontends
EOF

    cp -a "$EXACTSRC/drivers/media/usb/cx231xx/." "$EXTROOT/cx231xx/" || return 1

    cat > "$EXTROOT/cx231xx/Makefile" <<EOF
cx231xx-y += cx231xx-video.o cx231xx-i2c.o cx231xx-cards.o cx231xx-core.o
cx231xx-y += cx231xx-avcore.o cx231xx-417.o cx231xx-pcb-cfg.o cx231xx-vbi.o
cx231xx-\$(CONFIG_VIDEO_CX231XX_RC) += cx231xx-input.o

obj-m += cx231xx.o

ccflags-y += -I$EXACTSRC_BUILD/drivers/media/tuners
ccflags-y += -I$EXACTSRC_BUILD/drivers/media/dvb-frontends
EOF

    nice -n 10 make -C "$KBUILD" M="$EXTROOT_BUILD/si2157" -j2 modules || return 1
    nice -n 10 make -C "$KBUILD" M="$EXTROOT_BUILD/cx231xx" -j2 modules || return 1

    SI2157_KO="$EXTROOT/si2157/si2157.ko"
    CX231XX_KO="$EXTROOT/cx231xx/cx231xx.ko"

    echo
    echo "=== VERIFY MODULE ABI BEFORE INSTALLATION ==="

    local ko vm
    for ko in "$SI2157_KO" "$CX231XX_KO"; do
        [[ -f "$ko" ]] || {
            echo "FAIL: missing built module: $ko"
            return 1
        }

        vm="$(modinfo -F vermagic "$ko" 2>/dev/null)"
        echo "$(basename "$ko"): $vm"

        [[ "$vm" == "$KREL"* ]] || {
            echo "FAIL: module ABI does not match the running kernel."
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
            echo "FAIL: Secure Boot is enabled but an enrolled Ubuntu MOK signing key was not found."
            echo "No custom driver was installed."
            return 1
        }

        for ko in "$SI2157_KO" "$CX231XX_KO"; do
            sudo "$sign_file" sha256 "$mok_priv" "$mok_der" "$ko" || return 1
        done

        echo "PASS: custom modules signed for Secure Boot."
    else
        echo "PASS: Secure Boot module signing not required."
    fi

    echo
    echo "=== INSTALL CANDIDATE ON DISK ONLY ==="

    sudo rm -rf "$UPDATE_DIR" || return 1
    sudo mkdir -p "$UPDATE_DIR" || return 1
    sudo cp "$SI2157_KO" "$CX231XX_KO" "$UPDATE_DIR/" || return 1
    sudo depmod -a "$KREL" || return 1

    echo "PASS: patched cx231xx and si2157 modules installed for the next boot."
    echo "PASS: the currently running stock HVR stack has not been touched."

    echo
    echo "=== BACK UP APP FILES BEFORE NATIVE BACKEND INTEGRATION ==="

    for rel in \
        "CMakeLists.txt" \
        "src/main.cpp" \
        "src/radio/radio_backend.cpp" \
        "src/radio/radio_backend.hpp" \
        "com.elderredsoftworks.NougatMediaPlus.desktop"; do
        if [[ -f "$PROJECT/$rel" ]]; then
            mkdir -p "$ARCHIVE/app/$(dirname "$rel")"
            cp -a "$PROJECT/$rel" "$ARCHIVE/app/$rel" || return 1
        fi
    done

    echo
    echo "=== INTEGRATE NATIVE HVR FM PROVIDER INTO NOUGAT ==="

    python3 "$APP_PATCHER" "$PROJECT" || {
        echo "Restoring application source backup."
        cp -a "$ARCHIVE/app/." "$PROJECT/" || true
        echo "FAIL: application backend patch failed. Running kernel driver was never touched."
        return 1
    }

    echo
    echo "=== BUILD NOUGAT MEDIA PLUS v0.0.67 ==="

    if ! cmake -S "$PROJECT" -B "$PROJECT/build"; then
        cp -a "$ARCHIVE/app/." "$PROJECT/" || true
        echo "FAIL: CMake configure failed; app source restored. Running kernel driver was never touched."
        return 1
    fi

    if ! nice -n 10 cmake --build "$PROJECT/build" \
        --target Nougat_Media_Plus_v67 \
        -j2; then
        cp -a "$ARCHIVE/app/." "$PROJECT/" || true
        echo "FAIL: v67 application build failed; app source restored. Running kernel driver was never touched."
        return 1
    fi

    local BUILT ROOT_EXE VERSION
    BUILT="$PROJECT/build/Nougat_Media_Plus_v67"
    ROOT_EXE="$PROJECT/Nougat_Media_Plus_v67"

    [[ -x "$BUILT" ]] || {
        cp -a "$ARCHIVE/app/." "$PROJECT/" || true
        echo "FAIL: expected v67 executable was not produced. Running kernel driver was never touched."
        return 1
    }

    cp -a "$BUILT" "$ROOT_EXE" || return 1
    chmod +x "$ROOT_EXE" || return 1

    VERSION="$("$ROOT_EXE" --version 2>&1)"
    if ! grep -Fq 'v0.0.67' <<< "$VERSION"; then
        cp -a "$ARCHIVE/app/." "$PROJECT/" || true
        echo "FAIL: built executable has the wrong identity:"
        echo "$VERSION"
        return 1
    fi

    rm -f "$PROJECT/Nougat_Media_Plus_v66"

    if command -v gio >/dev/null 2>&1; then
        gio set \
            "$ROOT_EXE" \
            metadata::custom-icon \
            "file:///home/dereksparks1982/DKLab/Projects/Nougat%20Media%20Plus/assets/branding/nougat-media-plus-dock-N.png" \
            || true
        gio set -t string \
            "$ROOT_EXE" \
            metadata::custom-icon-name \
            nougat-media-plus \
            || true
    fi

    echo
    echo "============================================================"
    echo "PASS: v67 HVR-955Q FM reboot-safe build completed"
    echo "Executable: $ROOT_EXE"
    echo "Identity:   $VERSION"
    echo "Modules:    $UPDATE_DIR"
    echo "Backup:     $ARCHIVE"
    echo "Git/GitHub: untouched"
    echo "============================================================"
    echo
    echo "IMPORTANT: the running kernel was deliberately left untouched."
    echo "Restart the computer normally when you are ready."
    echo "On the next boot, Linux can load the patched cx231xx + si2157 modules fresh,"
    echo "without live-unloading the audio/USB/media stack."
    return 0
}

main
