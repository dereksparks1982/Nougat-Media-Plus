#!/bin/bash

cleanup_alias() {
    local alias_path="$1"
    if [[ -n "$alias_path" ]] && mountpoint -q "$alias_path" 2>/dev/null; then
        sudo umount "$alias_path" 2>/dev/null || true
    fi
    if [[ -n "$alias_path" ]]; then
        rmdir "$alias_path" 2>/dev/null || true
    fi
}

fetch_exact_ubuntu_source() {
    local source_pkg="$1"
    local source_ver="$2"
    local source_root="$3"
    local temp_sources="/tmp/nougat-v67-fix9-deb-src.sources"
    local installed_sources="/etc/apt/sources.list.d/nougat-v67-fix9-deb-src.sources"
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
            if (cd "$source_root" && apt-get source --only-source "$source_pkg=$source_ver"); then
                apt_ok=1
            fi
        fi
        sudo rm -f "$installed_sources" || true
    fi

    if [[ "$apt_ok" -eq 1 ]]; then
        return 0
    fi

    echo "Exact APT source unavailable; using exact Launchpad source archive."
    command -v curl >/dev/null 2>&1 || { echo "FAIL: curl is required."; return 1; }
    command -v dpkg-source >/dev/null 2>&1 || { echo "FAIL: dpkg-source is required."; return 1; }

    local version_no_epoch="${source_ver#*:}"
    local base="https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/${source_pkg}/${version_no_epoch}"
    local dsc="${source_pkg}_${version_no_epoch}.dsc"

    rm -rf "$source_root"
    mkdir -p "$source_root" || return 1
    curl -fL --retry 3 "$base/$dsc" -o "$source_root/$dsc" || return 1

    local files
    files="$(awk '
        /^Files:/ {inside=1; next}
        inside && /^[[:space:]]+[0-9a-fA-F]{32}[[:space:]]+[0-9]+[[:space:]]+/ {print $3; next}
        inside && !/^[[:space:]]/ {exit}
    ' "$source_root/$dsc")"
    [[ -n "$files" ]] || { echo "FAIL: exact source file list could not be parsed."; return 1; }

    local f
    while IFS= read -r f; do
        [[ -n "$f" ]] || continue
        curl -fL --retry 3 "$base/$f" -o "$source_root/$f" || return 1
    done <<< "$files"
    return 0
}

restore_app_source() {
    local archive="$1"
    local project="$2"
    local new_cpp="$3"
    local new_hpp="$4"
    [[ -d "$archive/app" ]] && cp -a "$archive/app/." "$project/" 2>/dev/null || true
    [[ "$new_cpp" == "1" ]] && rm -f "$project/src/radio/v4l2_radio_provider.cpp"
    [[ "$new_hpp" == "1" ]] && rm -f "$project/src/radio/v4l2_radio_provider.hpp"
}

main() {
    PROJECT="/home/dereksparks1982/DKLab/Projects/Nougat Play Portal"
    KREL="$(uname -r)"
    KBUILD="/lib/modules/$KREL/build"
    BUILDROOT="$PROJECT/build/v67-hvr955q-fm-$KREL"
    SRCROOT="$BUILDROOT/exact-ubuntu-source"
    CLEANROOT="$BUILDROOT/clean-exact-source"
    EXACTSRC="$CLEANROOT/linux-7.0.0"
    MATCHED="$BUILDROOT/fix9-matched-media-modules"
    BUILD_ALIAS="/tmp/nougat-v67-fix9-kbuild-$$"
    UPDATE_DIR="/lib/modules/$KREL/updates/nougat-v67-fm"
    APP_BUILD="$PROJECT/build/app-v67"
    KERNEL_PATCHER="$PROJECT/components/radio/kernel/hvr955q-fm/patch_kernel_source_fix9.py"
    APP_PATCHER="$PROJECT/scripts/patch_nougat_radio_backend_fix9.py"
    TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
    ARCHIVE="/home/dereksparks1982/DKLab/Archives/Nougat Play Portal/v67-fm-fix9-before-$TIMESTAMP"
    ROOT_EXE="$PROJECT/Nougat_Play_Portal_v67"
    ICON="$PROJECT/assets/branding/nougat-play-portal-dock-N.png"
    DESKTOP_SRC="$PROJECT/com.elderredsoftworks.NougatPlayPortal.desktop"
    DESKTOP_DST="$HOME/.local/share/applications/com.elderredsoftworks.NougatPlayPortal.desktop"

    [[ -d "$PROJECT" ]] || { echo "FAIL: project not found: $PROJECT"; return 1; }
    [[ -d "$KBUILD" && -f "$KBUILD/Makefile" ]] || { echo "FAIL: running-kernel headers missing: $KBUILD"; return 1; }
    [[ -f "$KERNEL_PATCHER" && -f "$APP_PATCHER" ]] || { echo "FAIL: FIX9 patchers missing."; return 1; }
    [[ -f "$ICON" ]] || { echo "FAIL: approved Nougat N dock icon missing: $ICON"; return 1; }

    mkdir -p "$BUILDROOT" "$ARCHIVE/app" "$PROJECT/logs" || return 1

    echo "============================================================"
    echo "Nougat Play Portal v0.0.67"
    echo "HVR-955Q FM RADIO END-TO-END DRIVER + APP + N DOCK ICON FIX9"
    echo "Kernel: $KREL"
    echo "============================================================"
    echo "No live HVR/media/audio kernel modules will be unloaded or reloaded."
    echo "Driver changes are for the next normal reboot only. Git/GitHub untouched."

    if lsusb 2>/dev/null | grep -q '2040:b123'; then
        echo "PASS: Hauppauge WinTV-HVR-955Q 2040:b123 detected."
    else
        echo "INFO: HVR-955Q not currently attached; build can continue."
    fi

    echo
    echo "=== IDENTIFY EXACT RUNNING-KERNEL SOURCE ==="
    local header_pkg source_pkg source_ver header_release version_no_epoch dsc
    header_pkg="linux-headers-$KREL"
    source_pkg="$(dpkg-query -W -f='${source:Package}' "$header_pkg" 2>/dev/null)"
    source_ver="$(dpkg-query -W -f='${source:Version}' "$header_pkg" 2>/dev/null)"
    header_release="$(cat "$KBUILD/include/config/kernel.release" 2>/dev/null)"
    [[ -n "$source_pkg" && -n "$source_ver" ]] || { echo "FAIL: running kernel source package unresolved."; return 1; }
    [[ "$header_release" == "$KREL" ]] || { echo "FAIL: kernel headers do not match uname."; return 1; }
    echo "Source package: $source_pkg"
    echo "Source version: $source_ver"
    echo "Header ABI:     $header_release"

    version_no_epoch="${source_ver#*:}"
    dsc="$SRCROOT/${source_pkg}_${version_no_epoch}.dsc"
    if [[ ! -f "$dsc" ]]; then
        fetch_exact_ubuntu_source "$source_pkg" "$source_ver" "$SRCROOT" || { echo "FAIL: exact Ubuntu source unavailable."; return 1; }
        dsc="$SRCROOT/${source_pkg}_${version_no_epoch}.dsc"
    fi
    [[ -f "$dsc" ]] || { echo "FAIL: exact Ubuntu DSC missing: $dsc"; return 1; }

    echo
    echo "=== EXTRACT FRESH CLEAN EXACT UBUNTU SOURCE ==="
    rm -rf "$CLEANROOT"
    mkdir -p "$CLEANROOT" || return 1
    dpkg-source -x "$dsc" "$EXACTSRC" || { echo "FAIL: exact Ubuntu source extraction failed."; return 1; }

    local changelog extracted_ver
    changelog="$EXACTSRC/debian/changelog"
    [[ -f "$changelog" ]] || changelog="$EXACTSRC/debian.master/changelog"
    [[ -f "$changelog" ]] || { echo "FAIL: source changelog missing."; return 1; }
    extracted_ver="$(dpkg-parsechangelog -l"$changelog" -S Version 2>/dev/null)"
    [[ "$extracted_ver" == "$source_ver" ]] || { echo "FAIL: source version mismatch: $extracted_ver"; return 1; }
    echo "PASS: exact source verified: $extracted_ver"

    echo
    echo "=== APPLY COMPLETE HVR-955Q FM KERNEL PATCH ==="
    python3 "$KERNEL_PATCHER" "$EXACTSRC" || return 1

    echo
    echo "=== PREPARE MATCHED FOUR-MODULE BUILD ==="
    rm -rf "$MATCHED"
    mkdir -p "$MATCHED" "$BUILD_ALIAS" || return 1
    sudo mount --bind "$BUILDROOT" "$BUILD_ALIAS" || { rmdir "$BUILD_ALIAS" 2>/dev/null || true; echo "FAIL: Kbuild alias failed."; return 1; }
    trap 'cleanup_alias "$BUILD_ALIAS"' EXIT

    local EXACT_ALIAS MATCHED_ALIAS
    EXACT_ALIAS="$BUILD_ALIAS/clean-exact-source/linux-7.0.0"
    MATCHED_ALIAS="$BUILD_ALIAS/fix9-matched-media-modules"
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
obj-m += cx231xx-dvb.o
obj-m += cx231xx-alsa.o
obj-m += si2157.o
ccflags-y += -I$EXACT_ALIAS/drivers/media/tuners
ccflags-y += -I$EXACT_ALIAS/drivers/media/dvb-frontends
EOF

    echo
    echo "=== BUILD MATCHED cx231xx + DVB + ALSA + Si2157 STACK ==="
    nice -n 10 make -C "$KBUILD" M="$MATCHED_ALIAS" -j2 modules || { echo "FAIL: matched FM kernel build failed."; return 1; }

    local CXKO DVBKO ALSAKO SIKO ko vm
    CXKO="$MATCHED/cx231xx.ko"
    DVBKO="$MATCHED/cx231xx-dvb.ko"
    ALSAKO="$MATCHED/cx231xx-alsa.ko"
    SIKO="$MATCHED/si2157.ko"
    echo
    echo "=== VERIFY ALL FOUR MODULES ==="
    for ko in "$CXKO" "$DVBKO" "$ALSAKO" "$SIKO"; do
        [[ -f "$ko" ]] || { echo "FAIL: missing built module: $ko"; return 1; }
        vm="$(modinfo -F vermagic "$ko" 2>/dev/null)"
        echo "$(basename "$ko"): $vm"
        [[ "$vm" == "$KREL"* ]] || { echo "FAIL: $(basename "$ko") ABI mismatch."; return 1; }
    done

    echo
    echo "=== SECURE BOOT CHECK ==="
    if command -v mokutil >/dev/null 2>&1 && mokutil --sb-state 2>/dev/null | grep -qi 'SecureBoot enabled'; then
        local sign_file mok_priv mok_der
        sign_file="$KBUILD/scripts/sign-file"
        mok_priv="/var/lib/shim-signed/mok/MOK.priv"
        mok_der="/var/lib/shim-signed/mok/MOK.der"
        [[ -x "$sign_file" && -f "$mok_priv" && -f "$mok_der" ]] || { echo "FAIL: Secure Boot key unavailable."; return 1; }
        for ko in "$CXKO" "$DVBKO" "$ALSAKO" "$SIKO"; do
            sudo "$sign_file" sha256 "$mok_priv" "$mok_der" "$ko" || return 1
        done
        echo "PASS: matched modules signed."
    else
        echo "PASS: Secure Boot signing not required."
    fi

    echo
    echo "=== BACK UP ONLY APP FILES FIX9 MAY CHANGE ==="
    local rel new_cpp=0 new_hpp=0
    for rel in "CMakeLists.txt" "src/main.cpp" "src/radio/radio_backend.cpp" "src/radio/radio_backend.hpp" "com.elderredsoftworks.NougatPlayPortal.desktop"; do
        if [[ -f "$PROJECT/$rel" ]]; then
            mkdir -p "$ARCHIVE/app/$(dirname "$rel")" || return 1
            cp -a "$PROJECT/$rel" "$ARCHIVE/app/$rel" || return 1
        fi
    done
    [[ -f "$PROJECT/src/radio/v4l2_radio_provider.cpp" ]] || new_cpp=1
    [[ -f "$PROJECT/src/radio/v4l2_radio_provider.hpp" ]] || new_hpp=1

    echo
    echo "=== INTEGRATE NATIVE HVR V4L2 FM APP BACKEND ==="
    python3 "$APP_PATCHER" "$PROJECT" || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; echo "FAIL: app patch failed; source restored."; return 1; }

    echo
    echo "=== BUILD APP IN DEDICATED build/app-v67 DIRECTORY ==="
    rm -rf "$APP_BUILD" || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; return 1; }
    mkdir -p "$APP_BUILD" || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; return 1; }
    cmake -S "$PROJECT" -B "$APP_BUILD" || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; echo "FAIL: CMake configure failed; source restored."; return 1; }
    nice -n 10 cmake --build "$APP_BUILD" --target Nougat_Play_Portal_v67 --parallel 2 || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; echo "FAIL: app build failed; source restored."; return 1; }

    local BUILT VERSION
    BUILT="$APP_BUILD/Nougat_Play_Portal_v67"
    [[ -x "$BUILT" ]] || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; echo "FAIL: v67 executable not produced."; return 1; }
    VERSION="$("$BUILT" --version 2>&1)"
    [[ "$VERSION" == *"v0.0.67"* ]] || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; echo "FAIL: wrong app identity: $VERSION"; return 1; }
    strings "$BUILT" | grep -Fq "HVR-955Q FM receiving" || { restore_app_source "$ARCHIVE" "$PROJECT" "$new_cpp" "$new_hpp"; echo "FAIL: native HVR FM path missing from executable."; return 1; }
    echo "PASS: v67 app candidate contains native HVR FM receive path."

    echo
    echo "=== FINAL INSTALL: DRIVER + ROOT APP + APPROVED N DOCK ICON ==="
    local pid exe
    for pid in $(pgrep -f 'Nougat_Play_Portal_v67' 2>/dev/null); do
        exe="$(readlink -f "/proc/$pid/exe" 2>/dev/null)"
        [[ "$exe" == "$ROOT_EXE" ]] && kill "$pid" 2>/dev/null || true
    done

    local ROOT_BACKUP="$ARCHIVE/Nougat_Play_Portal_v67.before"
    local DESKTOP_BACKUP="$ARCHIVE/com.elderredsoftworks.NougatPlayPortal.desktop.before"
    [[ -f "$ROOT_EXE" ]] && cp -a "$ROOT_EXE" "$ROOT_BACKUP"
    [[ -f "$DESKTOP_DST" ]] && cp -a "$DESKTOP_DST" "$DESKTOP_BACKUP"

    local NEW_UPDATE="${UPDATE_DIR}.fix9-new-$$"
    local OLD_UPDATE="${UPDATE_DIR}.fix9-old-$$"
    sudo rm -rf "$NEW_UPDATE" "$OLD_UPDATE" || return 1
    sudo mkdir -p "$NEW_UPDATE" || return 1
    sudo cp "$CXKO" "$DVBKO" "$ALSAKO" "$SIKO" "$NEW_UPDATE/" || return 1
    if [[ -d "$UPDATE_DIR" ]]; then sudo mv "$UPDATE_DIR" "$OLD_UPDATE" || return 1; fi
    sudo mv "$NEW_UPDATE" "$UPDATE_DIR" || { [[ -d "$OLD_UPDATE" ]] && sudo mv "$OLD_UPDATE" "$UPDATE_DIR"; return 1; }

    if ! sudo depmod -a "$KREL"; then
        sudo rm -rf "$UPDATE_DIR"
        [[ -d "$OLD_UPDATE" ]] && sudo mv "$OLD_UPDATE" "$UPDATE_DIR"
        sudo depmod -a "$KREL" || true
        echo "FAIL: depmod failed; previous override restored."
        return 1
    fi
    if command -v update-initramfs >/dev/null 2>&1; then
        if ! sudo update-initramfs -u -k "$KREL"; then
            sudo rm -rf "$UPDATE_DIR"
            [[ -d "$OLD_UPDATE" ]] && sudo mv "$OLD_UPDATE" "$UPDATE_DIR"
            sudo depmod -a "$KREL" || true
            sudo update-initramfs -u -k "$KREL" || true
            echo "FAIL: initramfs update failed; previous override restored."
            return 1
        fi
    fi

    cp -a "$BUILT" "$ROOT_EXE" || return 1
    chmod +x "$ROOT_EXE" || return 1
    mkdir -p "$HOME/.local/share/applications" || return 1
    cp -a "$DESKTOP_SRC" "$DESKTOP_DST" || return 1

    python3 - "$DESKTOP_DST" "$PROJECT" <<'PY'
from pathlib import Path
import re
import sys
desktop = Path(sys.argv[1])
project = Path(sys.argv[2])
text = desktop.read_text()
values = {
    "Exec": f'Exec="{project}/Nougat_Play_Portal_v67"',
    "Icon": f"Icon={project}/assets/branding/nougat-play-portal-dock-N.png",
    "StartupWMClass": "StartupWMClass=NougatPlayPortal",
    "X-GNOME-Application-ID": "X-GNOME-Application-ID=com.elderredsoftworks.NougatPlayPortal",
    "X-GNOME-WMClass": "X-GNOME-WMClass=NougatPlayPortal",
}
for key, line in values.items():
    if re.search(rf"^{re.escape(key)}=", text, flags=re.M):
        text = re.sub(rf"^{re.escape(key)}=.*$", line, text, flags=re.M)
    else:
        if not text.endswith("\n"):
            text += "\n"
        text += line + "\n"
desktop.write_text(text)
PY
    [[ $? -eq 0 ]] || return 1
    chmod 0644 "$DESKTOP_DST" || return 1
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
    fi
    if command -v gio >/dev/null 2>&1; then
        gio set "$ROOT_EXE" metadata::custom-icon "file:///home/dereksparks1982/DKLab/Projects/Nougat%20Media%20Plus/assets/branding/nougat-play-portal-dock-N.png" >/dev/null 2>&1 || true
    fi

    echo
    echo "=== VERIFY NEXT-BOOT MODULE RESOLUTION ==="
    local mod path
    for mod in cx231xx cx231xx_dvb cx231xx_alsa si2157; do
        path="$(modinfo -n "$mod" 2>/dev/null)"
        echo "$mod -> $path"
        case "$path" in "$UPDATE_DIR"/*) ;; *) echo "FAIL: $mod is not resolving to FIX9."; return 1 ;; esac
    done
    grep -Fq "Icon=$ICON" "$DESKTOP_DST" || { echo "FAIL: dock launcher does not point to approved N icon."; return 1; }
    VERSION="$("$ROOT_EXE" --version 2>&1)"
    [[ "$VERSION" == *"v0.0.67"* ]] || { echo "FAIL: installed app identity wrong."; return 1; }
    sudo rm -rf "$OLD_UPDATE" 2>/dev/null || true

    echo
    echo "============================================================"
    echo "PASS: FIX9 END-TO-END CANDIDATE INSTALLED"
    echo "Radio driver: HVR-955Q radio node + Si2157 FM + cx231xx FM DIF/audio"
    echo "App backend:  native V4L2 LISTEN / SCAN / RECORD"
    echo "Dock icon:    approved Nougat N"
    echo "Executable:   $ROOT_EXE"
    echo "Identity:     $VERSION"
    echo "Git/GitHub:   untouched"
    echo "A NORMAL REBOOT IS REQUIRED before the new matched driver can take over."
    echo "============================================================"
    return 0
}

main
