#!/usr/bin/env bash
# Pack the Silent Hill 2 VR mod into ONE private zip: our launcher, the UEVR profile (with sh2vr.dll), the pak,
# PureDark's AFW build of UEVR (unchanged, checked by its sha256), the licences, README.txt and FILES.txt.
#   bash dev-archive/launcher/tools/pack.sh <version>          e.g. 0.1.0
# Env: AFW_ZIP (PureDark's download), OUT_ROOT (where the folder and the zip go).
# Releases stay PRIVATE (no public releases since 2026-10-03): the zip is for named people only.
set -euo pipefail
VER="${1:?usage: pack.sh <version, e.g. 0.1.0>}"
REPO="$(cd "$(dirname "$0")/../../.." && pwd)"
LAUNCHER="$REPO/dev-archive/launcher"
NATIVE="$REPO/dev-archive/native"
MODP="$REPO/mod/UnrealVRMod/SHProto-Win64-Shipping"
AFW_ZIP="${AFW_ZIP:-$USERPROFILE/Downloads/UEVR-nightly_AFW_v1.0-beta.6.zip}"
AFW_SHA="c5eb5d3f6c356d20d817849c0cf5cc95356ed03aa99100b6237dbb3081dbbe2c"   # UEVR-nightly_AFW_v1.0-beta.6.zip
OUT_ROOT="${OUT_ROOT:-/d/SH2 builds/releases}"
STAGE="$OUT_ROOT/Silent Hill 2 VR $VER"
ZIP="$OUT_ROOT/Silent-Hill-2-VR-$VER.zip"
# Never shipped: dev tools, files UEVR writes at run time, other people's add-ons we have not been allowed to share.
NEVER='uevr_mcp\.dll|/cache/|log\.txt$|imgui\.ini$|uobjecthook|breadcrumb|crash\.dmp|ammo_diff|silenttraverse|version\.dll|UEVRInjector'

[ -e "$STAGE" ] || [ -e "$ZIP" ] && { echo "already packed: $STAGE (pick a new version)"; exit 1; }
[ -f "$AFW_ZIP" ] || { echo "PureDark's AFW zip not found: $AFW_ZIP (set AFW_ZIP)"; exit 1; }
[ "$(sha256sum < "$AFW_ZIP" | cut -c1-64)" = "$AFW_SHA" ] || { echo "the AFW zip is not the tested beta.6 download"; exit 1; }

echo "== build sh2vr.dll"
bash "$NATIVE/tools/build.sh" | tail -4
echo "== build SH2VR.exe"
[ -f "$LAUNCHER/build/CMakeCache.txt" ] || cmake -S "$LAUNCHER" -B "$LAUNCHER/build" -G "Visual Studio 17 2022" -A x64 >/dev/null
rm -f "$LAUNCHER/build/Release/SH2VR.exe"
cmake --build "$LAUNCHER/build" --config Release 2>&1 | grep -E "warning|error" || true
[ -f "$LAUNCHER/build/Release/SH2VR.exe" ] || { echo "LAUNCHER BUILD FAILED"; exit 1; }

echo "== stage"
mkdir -p "$STAGE/package/profile" "$STAGE/package/paks" "$STAGE/uevr" "$STAGE/licences"
cp "$LAUNCHER/build/Release/SH2VR.exe" "$STAGE/"
cp "$REPO/mod/release/Uninstall.bat" "$STAGE/"
sed "s/{VERSION}/$VER/" "$REPO/mod/release/README.txt" > "$STAGE/README.txt"
cp -r "$MODP/." "$STAGE/package/profile/"
cp "$NATIVE/build/Release/sh2vr.dll" "$STAGE/package/profile/plugins/"
cp "$REPO/mod/Paks/~mods/"*.pak "$STAGE/package/paks/"
cp "$REPO/mod/licences/"* "$STAGE/licences/"
python -I -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" "$AFW_ZIP" "$STAGE/uevr"
rm -f "$STAGE/uevr/UEVRInjector.exe" "$STAGE/uevr/UEVRInjector.pdb" "$STAGE/uevr/UEVRInjector.dll.config"

# The modules the C++ plugin has taken over, the same file build.sh --deploy writes.
eval "$(grep -m1 '^PORTED=' "$NATIVE/tools/build.sh")"
{
    echo "-- written by pack.sh: the modules the C++ plugin has taken over (their Lua stands down)"
    echo "SH2VR_PORTED = {"
    for m in "${PORTED[@]}"; do echo "    $m = true,"; done
    echo "}"
} > "$STAGE/package/profile/scripts/sh2vr_ported.lua"

echo "== check"
BAD=$(cd "$STAGE" && find . -type f | grep -E "$NEVER" || true)
[ -z "$BAD" ] || { echo "REFUSED, files that must never ship:"; echo "$BAD"; exit 1; }
for f in SH2VR.exe uevr/UEVRBackend.dll uevr/openxr_loader.dll package/profile/plugins/sh2vr.dll package/profile/plugins/sh2r.dll package/profile/scripts/main.lua; do
    [ -f "$STAGE/$f" ] || { echo "MISSING: $f"; exit 1; }
done
(cd "$STAGE" && { echo "Silent Hill 2 VR $VER: every file in this package, with its sha256."; echo;
  find . -type f ! -name FILES.txt | sed 's#^\./##' | sort | while IFS= read -r f; do printf '%s  %s\n' "$(sha256sum < "$f" | cut -c1-64)" "$f"; done; } > FILES.txt)

echo "== zip"
python -I - "$STAGE" "$ZIP" <<'EOF'
import os, sys, zipfile
stage, out = sys.argv[1], sys.argv[2]
top = os.path.basename(stage)
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for root, _, files in os.walk(stage):
        for f in sorted(files):
            p = os.path.join(root, f)
            z.write(p, os.path.join(top, os.path.relpath(p, stage)))
EOF
echo "files: $(($(wc -l < "$STAGE/FILES.txt") - 2))   zip: $(du -h "$ZIP" | cut -f1)   sha256: $(sha256sum < "$ZIP" | cut -c1-16)"
echo "packed: $ZIP"
