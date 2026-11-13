#!/usr/bin/env bash
# toplay.sh: after the inject, press through the warning screen, the main menu and "press any button" until the character exists
B=~/github-backups/silent-hill-2-remake-vr/dev-archive/recon/2026-10-29-buttons
sleep 12
powershell -NoProfile -ExecutionPolicy Bypass -File $B/sh2_front.ps1 >/dev/null 2>&1
python $B/sim_ctl.py 1,0.2,-0.2,-0.4,0,0,0,1 wait:0.3 1,0.2,-0.2,-0.4,0,0,0,0 >/dev/null
sleep 6
for i in $(seq 1 14); do
  p=$(curl -s -m 5 http://127.0.0.1:8899/api/player | python -c "import json,sys; d=json.load(sys.stdin); print((d.get('pawn') or {}).get('class'))" 2>/dev/null)
  [ "$p" != "None" ] && [ -n "$p" ] && break
  powershell -NoProfile -ExecutionPolicy Bypass -File $B/sh2_front.ps1 >/dev/null 2>&1
  curl -s -m 5 -X POST -H "Content-Type: application/json" -d '{"key":"enter","eventType":"tap"}' http://127.0.0.1:8899/api/input/key >/dev/null
  sleep 7
done
echo "pawn=$p after $i tries"
sleep 8
powershell -NoProfile -ExecutionPolicy Bypass -File $B/sh2_front.ps1 >/dev/null 2>&1
curl -s -m 5 -X POST -H "Content-Type: application/json" -d '{"key":"enter","eventType":"tap"}' http://127.0.0.1:8899/api/input/key >/dev/null
sleep 3
python $B/sim_ctl.py 1,0.12,-0.12,-0.35,0,0,1,0 wait:0.3 1,0.12,-0.12,-0.35,0,0,0,0 0,-0.12,-0.14,-0.35,0,0,0,0 >/dev/null
grep -i "warpscale\|sh2vr\] warp" "$APPDATA/UnrealVRMod/SHProto-Win64-Shipping/log.txt" | tail -5 | cut -c1-230
