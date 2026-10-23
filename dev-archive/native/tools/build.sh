#!/usr/bin/env bash
# Build sh2vr.dll (Release x64), check its two UEVR entry points, and optionally install it into the UEVR profile.
#   bash dev-archive/native/tools/build.sh             build only
#   bash dev-archive/native/tools/build.sh --deploy    build, back up what it replaces, install
#   bash dev-archive/native/tools/build.sh --undo DIR  put a --deploy backup back
# PORTED lists the modules the C++ has taken over. The Lua files stay (other Lua modules require them); --deploy
# writes scripts/sh2vr_ported.lua with SH2VR_PORTED.<module> = true and the Lua module stands down when it sees it.
# PATCHED lists our edited copies of pack files (mod/UnrealVRMod/...) that --deploy copies into the profile.
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
PROFILE="$APPDATA/UnrealVRMod/SHProto-Win64-Shipping"
MODPROFILE="$HERE/../../mod/UnrealVRMod/SHProto-Win64-Shipping"
BACKUPS="/d/SH2 builds"
BUILD="$HERE/build"
PORTED=(hands ik)
PATCHED=(scripts/libs/hands.lua scripts/libs/ik.lua scripts/sh2vr_bridge.lua scripts/main.lua)

undo() {
    local dir="$1"
    [ -d "$dir/files" ] || { echo "not a deploy backup: $dir"; exit 1; }
    [ -f "$dir/ADDED.txt" ] && while IFS= read -r rel; do [ -n "$rel" ] && rm -f "$PROFILE/$rel" && echo "removed $rel"; done < "$dir/ADDED.txt"
    (cd "$dir/files" && find . -type f | while IFS= read -r f; do mkdir -p "$PROFILE/$(dirname "$f")"; cp -p "$f" "$PROFILE/$f"; echo "restored ${f#./}"; done)
    echo "undo done from $dir"
}
if [ "${1:-}" = "--undo" ]; then undo "${2:?usage: build.sh --undo <backup folder>}"; exit 0; fi

[ -f "$BUILD/CMakeCache.txt" ] || cmake -S "$HERE" -B "$BUILD" -G "Visual Studio 17 2022" -A x64 >/dev/null
DLL="$BUILD/Release/sh2vr.dll"
rm -f "$DLL"   # a failed build must never leave an old DLL behind
cmake --build "$BUILD" --config Release 2>&1 | tee "$BUILD/last-build.log" | grep -E "warning|error|Build succeeded|FAILED" || true
[ -f "$DLL" ] || { echo "BUILD FAILED"; exit 1; }
ls -l "$DLL"; sha256sum "$DLL"
for e in uevr_plugin_required_version uevr_plugin_initialize; do
    grep -q "$e" "$DLL" && echo "export $e: present" || { echo "export $e: MISSING"; exit 1; }
done
[ "${1:-}" = "--deploy" ] || exit 0

[ -d "$PROFILE/plugins" ] || { echo "UEVR profile not found at $PROFILE"; exit 1; }
BK="$BACKUPS/_backup-$(date +%Y-%m-%d-%H%M)-before-sh2vr-deploy"
mkdir -p "$BK/files/plugins"
: > "$BK/ADDED.txt"
if [ -f "$PROFILE/plugins/sh2vr.dll" ]; then cp -p "$PROFILE/plugins/sh2vr.dll" "$BK/files/plugins/"; else echo "plugins/sh2vr.dll" >> "$BK/ADDED.txt"; fi
tasklist 2>/dev/null | grep -qi "SHProto-Win64-Shipping" && { echo "the game is running: close it first, the plugin file is locked"; exit 1; }
cp "$DLL" "$PROFILE/plugins/sh2vr.dll" && cmp -s "$DLL" "$PROFILE/plugins/sh2vr.dll" && echo "installed plugins/sh2vr.dll" || { echo "INSTALL FAILED: the copy does not match"; exit 1; }
for f in "${PATCHED[@]}"; do
    if cmp -s "$MODPROFILE/$f" "$PROFILE/$f"; then echo "same    $f"; continue; fi
    mkdir -p "$BK/files/$(dirname "$f")"; [ -f "$PROFILE/$f" ] && cp -p "$PROFILE/$f" "$BK/files/$f"
    cp "$MODPROFILE/$f" "$PROFILE/$f" && echo "patched $f"
done
FLAG="scripts/sh2vr_ported.lua"
if [ -f "$PROFILE/$FLAG" ]; then mkdir -p "$BK/files/scripts"; cp -p "$PROFILE/$FLAG" "$BK/files/$FLAG"; else echo "$FLAG" >> "$BK/ADDED.txt"; fi
{
    echo "-- written by sh2vr's build.sh --deploy: the modules the C++ plugin has taken over (their Lua stands down)"
    echo "SH2VR_PORTED = {"
    for m in "${PORTED[@]}"; do echo "    $m = true,"; done
    echo "}"
} > "$PROFILE/$FLAG"
echo "ported: ${PORTED[*]:-none} (written to $FLAG)"
echo "deployed; backup in: $BK"
echo "undo:  bash dev-archive/native/tools/build.sh --undo \"$BK\""
