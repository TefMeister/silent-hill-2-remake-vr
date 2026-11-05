#!/usr/bin/env bash
# ab.sh <label>: the average frame time of the next 3 heartbeats (15 s) after a 6 s settle, printed bare (ms).
L="$APPDATA/UnrealVRMod/SHProto-Win64-Shipping/log.txt"
sleep 6
start=$(grep -c "sh2vr\] alive" "$L")
while [ "$(grep -c "sh2vr\] alive" "$L")" -lt $((start + 3)) ]; do sleep 1; done
grep "sh2vr\] alive" "$L" | tail -n +$((start + 1)) | head -3 | grep -o "[0-9.]* ms avg" | awk '{s+=$1; n++} END {printf "%.2f\n", s/n}'
