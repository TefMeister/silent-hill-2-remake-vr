#!/usr/bin/env bash
# Build and run the plugin's pure-number tests (no game, no UEVR): bash dev-archive/native/tests/run.sh
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="$HERE/../build/tests"
mkdir -p "$OUT"
INST="C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer"
VS="$("/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -property installationPath 2>/dev/null | tr -d '\r')"
fail=0
for t in "$HERE"/*_test.cpp; do
    name="$(basename "$t" .cpp)"
    bat="$OUT/$name.build.bat"
    {
        printf '@set "PATH=%s;%%PATH%%"\r\n' "$INST"
        printf '@call "%s\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\r\n' "$VS"
        printf '@cd /d "%s"\r\n' "$(cygpath -w "$OUT")"
        printf '@cl /nologo /std:c++20 /EHsc /O2 /W4 /Fe%s.exe "%s"\r\n' "$name" "$(cygpath -w "$t")"
    } > "$bat"
    cmd //c "$(cygpath -w "$bat")" >"$OUT/$name.build.log" 2>&1 || { echo "BUILD FAILED: $name"; cat "$OUT/$name.build.log"; fail=1; continue; }
    echo "== $name"
    "$OUT/$name.exe" || fail=1
done
exit $fail
