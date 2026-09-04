#!/usr/bin/env bash
set -u
PROJECT="${1:-$HOME/DKLab/Projects/Nougat Media Suite}"
VENDOR="${2:-$PROJECT/components/drone/vendor}"
mkdir -p "$VENDOR"
if ! command -v git >/dev/null 2>&1; then
    echo "WARN: git is not installed; Drone source stack cannot be fetched."
    exit 2
fi
failures=0
fetch_tagged_repo() {
    name="$1"; url="$2"; ref="$3"; recursive="$4"; dest="$VENDOR/$name"
    if [ -d "$dest/.git" ]; then
        echo "PRESENT: $name ($(git -C "$dest" rev-parse --short HEAD 2>/dev/null || echo unknown))"
        return 0
    fi
    tmp="$VENDOR/.${name}.fetch.$$"; rm -rf "$tmp"
    echo; echo "FETCH: $name  ref=$ref"
    if ! git clone --quiet --depth 1 --branch "$ref" "$url" "$tmp"; then
        echo "WARN: clone failed for $name"; rm -rf "$tmp"; failures=$((failures+1)); return 0
    fi
    if [ "$recursive" = yes ]; then
        if ! git -C "$tmp" submodule update --init --recursive --depth 1; then
            echo "WARN: one or more $name submodules did not finish; retaining the top-level checkout."
            failures=$((failures+1))
        fi
    fi
    mv "$tmp" "$dest"
    echo "READY: $name ($(git -C "$dest" rev-parse --short HEAD))"
}
fetch_commit_repo() {
    name="$1"; url="$2"; commit="$3"; dest="$VENDOR/$name"
    if [ -d "$dest/.git" ]; then
        echo "PRESENT: $name ($(git -C "$dest" rev-parse --short HEAD 2>/dev/null || echo unknown))"
        return 0
    fi
    tmp="$VENDOR/.${name}.fetch.$$"; rm -rf "$tmp"; mkdir -p "$tmp"
    git -C "$tmp" init -q; git -C "$tmp" remote add origin "$url"
    echo; echo "FETCH: $name  commit=$commit"
    if ! git -C "$tmp" fetch --quiet --depth 1 origin "$commit"; then
        echo "WARN: fetch failed for $name"; rm -rf "$tmp"; failures=$((failures+1)); return 0
    fi
    git -C "$tmp" checkout -q --detach FETCH_HEAD
    actual="$(git -C "$tmp" rev-parse HEAD)"
    if [ "$actual" != "$commit" ]; then
        echo "WARN: $name pin mismatch: expected $commit got $actual"; rm -rf "$tmp"; failures=$((failures+1)); return 0
    fi
    mv "$tmp" "$dest"
    echo "READY: $name ($(git -C "$dest" rev-parse --short HEAD))"
}
echo "=== NOUGAT DRONE OPEN-SOURCE STACK FETCH ==="
echo "Destination: $VENDOR"
echo "This downloads source/runtime foundations only. It does not arm or control an aircraft."
fetch_tagged_repo "MAVSDK" "https://github.com/mavlink/MAVSDK.git" "v3.17.4" yes
fetch_commit_repo "mavlink" "https://github.com/mavlink/mavlink.git" "aae24cc6e7b2a1a6d5ad3a58ef0799f90703a59e"
fetch_tagged_repo "PX4-Autopilot" "https://github.com/PX4/PX4-Autopilot.git" "v1.17.0" yes
fetch_tagged_repo "ardupilot" "https://github.com/ArduPilot/ardupilot.git" "Copter-4.7.0" yes
echo; echo "--- Host media / simulation helpers ---"
for tool in ffmpeg gst-launch-1.0 cmake ninja python3; do
    if command -v "$tool" >/dev/null 2>&1; then echo "READY: $tool -> $(command -v "$tool")"; else echo "MISSING: $tool"; fi
done
echo
if [ "$failures" -eq 0 ]; then echo "PASS: all pinned Drone source foundations are present."; exit 0; fi
echo "WARN: Drone source fetch completed with $failures warning(s)."
echo "The v60 Studio Drone scaffold still works; run this script again to retry missing sources."
exit 1
