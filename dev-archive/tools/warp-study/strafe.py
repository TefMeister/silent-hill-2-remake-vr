"""strafe.py <label> [key=value ...] [stick=0.6] [motion=strafe|turn|walk|none] [secs=5]  (2026-11-22)
DOES THE WORLD MOVE ON THE MADE-UP FRAMES? Sets UEVR settings, moves James steadily by the stick (sideways by default),
asks the pretend headset for its run of consecutive frames and prints, per eye, how far the whole picture moved from
each frame to the next (phase correlation, the picture's upper two thirds so the hands count little).
With the frame warp each eye is really drawn on every other frame. A world that moves on every frame reads
"s s s s"; a world the made-up frames hold still reads "2s 0 2s 0". The last line is that ratio: the mean step of the
smaller-moving frames over the mean step of the larger-moving ones (1 = every frame moves, 0 = every other one)."""
import glob, json, os, re, sys, time, urllib.request
import numpy as np, cv2

SIM = os.path.join(os.environ["LOCALAPPDATA"], "OpenXR-Simulator")


def post(path, body):
    r = urllib.request.Request("http://127.0.0.1:8899" + path, json.dumps(body).encode(), {"Content-Type": "application/json"})
    return json.loads(urllib.request.urlopen(r, timeout=5).read())


def shift(p, q):
    p = p.astype(np.float32); q = q.astype(np.float32)
    w = np.outer(np.hanning(p.shape[0]), np.hanning(p.shape[1])).astype(np.float32)
    (dx, dy), resp = cv2.phaseCorrelate(q * w, p * w)
    return dx, dy, resp


def capture():
    st = os.path.join(SIM, "flicker_status.json")
    prev = json.load(open(st)).get("lastIncidentDirectory", "")
    req = os.path.join(SIM, "flicker_capture_request.json")
    open(req + ".tmp", "w").write(json.dumps({"requestedUnixMs": int(time.time() * 1000), "source": "mcp"}))
    os.replace(req + ".tmp", req)
    t0 = time.time()
    while time.time() - t0 < 8:
        try:
            cur = json.load(open(st)).get("lastIncidentDirectory", "")
            if cur and cur != prev and os.path.exists(os.path.join(cur, "capture_complete.json")):
                return cur
        except Exception:
            pass
        time.sleep(0.1)
    return None


def measure(d):
    fr = sorted({int(re.search(r"frame(\d+)_color_L", f).group(1)) for f in glob.glob(os.path.join(d, "frame*_color_L.bmp"))})
    out = {}
    for e in "LR":
        g = [cv2.cvtColor(cv2.imread(os.path.join(d, "frame%d_color_%s.bmp" % (n, e))), cv2.COLOR_BGR2GRAY) for n in fr]
        h = g[0].shape[0] * 2 // 3
        s = [shift(g[k - 1][:h], g[k][:h]) for k in range(1, len(g))]
        dx = np.array([v[0] for v in s]); dy = np.array([v[1] for v in s])
        mag = np.hypot(dx, dy)
        a, b = mag[0::2].mean(), mag[1::2].mean()
        out[e] = (dx, mag, min(a, b) / max(a, b, 1e-6), g[0].shape[1])
        print("%s x-steps (capture %d px wide): %s" % (e, g[0].shape[1], " ".join("%+.1f" % v for v in dx)))
        print("%s every-other-frame ratio %.2f (small frames %.2f px, large frames %.2f px)" % (e, out[e][2], min(a, b), max(a, b)))
    return out


if __name__ == "__main__":
    label = sys.argv[1]; stick = 0.6; motion = "strafe"; secs = 5.0
    for a in sys.argv[2:]:
        k, v = a.split("=", 1)
        if k == "stick": stick = float(v)
        elif k == "motion": motion = v
        elif k == "secs": secs = float(v)
        else: print("set", k, v, post("/api/vr/settings", {"key": k, "value": v}).get("success"))
    time.sleep(1.0)
    body = {"strafe": {"leftStick": {"x": stick, "y": 0}}, "walk": {"leftStick": {"x": 0, "y": stick}},
            "turn": {"rightStick": {"x": stick, "y": 0}}}.get(motion)
    if body:
        body["duration"] = secs
        post("/api/input/gamepad", body)
    time.sleep(min(2.0, secs * 0.5))
    d = capture()
    print("==", label, motion, d and os.path.basename(d))
    if d:
        measure(d)
