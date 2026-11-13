# run.py <label> [key=value ...] [stick=0.45] [motion=turn|none] : set UEVR settings, turn steadily, grab consecutive frames, measure
import json, os, sys, time, subprocess, urllib.request
SIM = os.path.join(os.environ["LOCALAPPDATA"], "OpenXR-Simulator")
def post(path, body):
    r = urllib.request.Request("http://127.0.0.1:8899" + path, json.dumps(body).encode(), {"Content-Type": "application/json"})
    return json.loads(urllib.request.urlopen(r, timeout=5).read())
label = sys.argv[1]; stick = 0.45; motion = "turn"
for a in sys.argv[2:]:
    k, v = a.split("=", 1)
    if k == "stick": stick = float(v)
    elif k == "motion": motion = v
    else: print("set", k, v, post("/api/vr/settings", {"key": k, "value": v}).get("success"))
time.sleep(1.0)
if motion == "turn": post("/api/input/gamepad", {"rightStick": {"x": stick, "y": 0}, "duration": 5})
time.sleep(2.0)
st = os.path.join(SIM, "flicker_status.json")
prev = json.load(open(st)).get("lastIncidentDirectory", "")
req = os.path.join(SIM, "flicker_capture_request.json")
open(req + ".tmp", "w").write(json.dumps({"requestedUnixMs": int(time.time() * 1000), "source": "mcp"})); os.replace(req + ".tmp", req)
d = None; t0 = time.time()
while time.time() - t0 < 8:
    try:
        cur = json.load(open(st)).get("lastIncidentDirectory", "")
        if cur and cur != prev and os.path.exists(os.path.join(cur, "capture_complete.json")): d = cur; break
    except Exception: pass
    time.sleep(0.1)
print("==", label, d and os.path.basename(d))
if d: subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "skin.py"), d])
