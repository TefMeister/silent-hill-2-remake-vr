"""gun_in_hand.py <gun mesh address> <seconds> [every=1.0] -- reads, through the UEVR connector (uevr_mcp, port 8899),
the held gun's turn against the hand it is attached to (the mesh component's RelativeRotation: pitch, yaw, roll in
degrees) once every <every> seconds, and prints how far each has moved from the first reading. A gun that keeps its
place in the hand reads 0 0 0 throughout; the "curls up by itself" fault of 2026-11-19 shows as a number that grows.
The address: connector search for "<weapon actor name>.Mesh", class SkeletalMeshComponent. Read-only."""
import json, sys, time, urllib.request

addr, secs = sys.argv[1], float(sys.argv[2])
every = 1.0
for a in sys.argv[3:]:
    if a.startswith("every="): every = float(a[6:])
URL = "http://127.0.0.1:8899/api/explorer/field?address=%s&fieldName=RelativeRotation" % addr


def read():
    with urllib.request.urlopen(URL, timeout=4) as r:
        v = json.load(r)["value"]
    return v["x"], v["y"], v["z"]   # the connector's order: pitch, yaw, roll


def diff(a, b):
    d = (a - b + 180.0) % 360.0 - 180.0
    return d


first = read(); t0 = time.time(); worst = [0.0, 0.0, 0.0]
print("start: pitch %.1f yaw %.1f roll %.1f" % first)
while time.time() - t0 < secs:
    time.sleep(every)
    now = read()
    d = [diff(now[i], first[i]) for i in range(3)]
    worst = [max(worst[i], abs(d[i])) for i in range(3)]
    print("%5.1f s: moved pitch %+6.1f yaw %+6.1f roll %+6.1f" % (time.time() - t0, d[0], d[1], d[2]), flush=True)
print("largest move: pitch %.1f yaw %.1f roll %.1f deg" % tuple(worst))
