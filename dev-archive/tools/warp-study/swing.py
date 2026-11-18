"""swing.py <seconds> [hz=1.0] [amp_m=0.12] [turn_rad=0.35]: swing the OpenXR Simulator's RIGHT hand left and right in
front of the face (and turn it), for the hands-on-the-frame-warp tests. The simulator takes a new hand pose about 20
times a second, so the hand moves in small jumps: a harsh case for a smoothed gun, which is the point.
The left hand is parked low on the left first. Ends with the right hand back in the middle."""
import json, math, os, sys, time

CMD = os.path.join(os.environ["LOCALAPPDATA"], "OpenXR-Simulator", "controller_pose_command.json")
secs = float(sys.argv[1]); hz = 1.0; amp = 0.12; turn = 0.35
for a in sys.argv[2:]:
    k, v = a.split("=", 1)
    if k == "hz": hz = float(v)
    elif k == "amp_m": amp = float(v)
    elif k == "turn_rad": turn = float(v)

def send(hand, x, y, z, yaw=0.0, pitch=0.0):
    with open(CMD + ".tmp", "w") as f:
        json.dump({"hand": hand, "posX": x, "posY": y, "posZ": z, "yaw": yaw, "pitch": pitch, "trigger": 0.0, "buttonA": False}, f)
    os.replace(CMD + ".tmp", CMD)
    for _ in range(20):
        if not os.path.exists(CMD): return
        time.sleep(0.005)

send(0, -0.25, -0.35, -0.30)
t0 = time.time(); n = 0
while time.time() - t0 < secs:
    s = math.sin(2 * math.pi * hz * (time.time() - t0))
    send(1, 0.10 + amp * s, -0.14, -0.38, turn * s, 0.0)
    n += 1
send(1, 0.10, -0.14, -0.38)
print("swing: %d poses in %.1f s (%.0f a second)" % (n, secs, n / secs))
