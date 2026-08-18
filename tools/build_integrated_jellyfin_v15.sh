#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
server_deb="$project_root/components/jellyfin/packages/jellyfin-server_10.11.11+ubu2604_amd64.deb"
web_deb="$project_root/components/jellyfin/packages/jellyfin-web_10.11.11+ubu2604_all.deb"
server_source="$project_root/components/jellyfin/source/jellyfin-10.11.11.zip"
web_source="$project_root/components/jellyfin/source/jellyfin-web-10.11.11.zip"
build_root="$(mktemp -d "${TMPDIR:-/tmp}/reddmedia-v15-jellyfin.XXXXXX")"
stage_root="$build_root/runtime"

cleanup() {
    case "$build_root" in
        "${TMPDIR:-/tmp}"/reddmedia-v15-jellyfin.*) rm -rf -- "$build_root" ;;
    esac
}
trap cleanup EXIT

echo "PHASE START: Stable Jellyfin package preflight"
for path in "$server_deb" "$web_deb" "$server_source" "$web_source"; do
    test -f "$path"
done
command -v dpkg-deb >/dev/null
command -v sha256sum >/dev/null
command -v unzip >/dev/null
case "$(uname -m)" in
    x86_64) ;;
    *) echo "ERROR: Bundled Jellyfin runtime requires an x86-64 Ubuntu system." >&2; exit 1 ;;
esac
test "$(dpkg-deb -f "$server_deb" Package)" = "jellyfin-server"
test "$(dpkg-deb -f "$server_deb" Version)" = "10.11.11+ubu2604"
test "$(dpkg-deb -f "$server_deb" Architecture)" = "amd64"
test "$(dpkg-deb -f "$web_deb" Package)" = "jellyfin-web"
test "$(dpkg-deb -f "$web_deb" Version)" = "10.11.11+ubu2604"
test "$(dpkg-deb -f "$web_deb" Architecture)" = "all"
test "$(sha256sum "$server_deb" | cut -d' ' -f1)" = "a956e58f9e6a95315dc09ca8111a8d29672b36584e5365035dc1bbac9bdd8aea"
test "$(sha256sum "$web_deb" | cut -d' ' -f1)" = "5b276a8b3142550d3c98dab391bca71fd489d7a51f032515c15440974a0af16f"
test "$(sha256sum "$server_source" | cut -d' ' -f1)" = "a07f2fcab3465f8dbae54a0d43cd0389bd77ef1df385dc49adf3ae70a233d3c2"
test "$(sha256sum "$web_source" | cut -d' ' -f1)" = "7e1cf362996b8cd49b894428da3d5dec3bf6e121d0b6b7df24b8410000f04a4b"
unzip -tq "$server_source" >/dev/null
unzip -tq "$web_source" >/dev/null
echo "PHASE PASS: Stable Jellyfin package preflight"

echo "PHASE START: Extract stable Jellyfin runtime"
mkdir -p "$build_root/server-package" "$build_root/web-package" "$stage_root/jellyfin" "$stage_root/web"
dpkg-deb -x "$server_deb" "$build_root/server-package"
dpkg-deb -x "$web_deb" "$build_root/web-package"
cp -a "$build_root/server-package/usr/lib/jellyfin/bin/." "$stage_root/jellyfin/"
cp -a "$build_root/web-package/usr/share/jellyfin/web/." "$stage_root/web/"
test -x "$stage_root/jellyfin/jellyfin"
test -f "$stage_root/web/index.html"
server_version="$("$stage_root/jellyfin/jellyfin" --version 2>&1 || true)"
test "$server_version" = "Jellyfin.Server 10.11.11.0"
echo "PHASE PASS: Extract stable Jellyfin runtime"

echo "PHASE START: Install integrated Jellyfin runtime"
runtime_parent="$project_root/components/jellyfin"
new_runtime="$runtime_parent/runtime.new"
old_runtime="$runtime_parent/runtime.old"
mkdir -p "$runtime_parent"
if [[ -e "$new_runtime" || -e "$old_runtime" ]]; then
    echo "ERROR: Stale runtime staging directory exists under $runtime_parent" >&2
    exit 1
fi
cp -a "$stage_root" "$new_runtime"
if [[ -d "$runtime_parent/runtime" ]]; then mv "$runtime_parent/runtime" "$old_runtime"; fi
mv "$new_runtime" "$runtime_parent/runtime"
if [[ -d "$old_runtime" ]]; then rm -rf -- "$old_runtime"; fi
test -x "$runtime_parent/runtime/jellyfin/jellyfin"
test -f "$runtime_parent/runtime/web/index.html"
echo "PHASE PASS: Install integrated Jellyfin runtime"

echo "FINAL PASS: Stable Jellyfin 10.11.11 server and web runtime are installed."
